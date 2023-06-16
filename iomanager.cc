#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "log.h"
#include "iomanager.h"
#include "macro.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

// 根据事件类型返回成员(读成员或写成员)
IOManager::FdContext::EventContext& IOManager::FdContext::getContext(Event event) {
    switch(event) {
        case READ:
            return read;
        case WRITE:
            return write;
        default:
            SYLAR_ASSERT2(false, "getContext");
    }
}

void IOManager::FdContext::resetContext(EventContext& ctx) {
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

// 触发指定事件并删除事件
void IOManager::FdContext::triggerEvent(Event event) {
    SYLAR_ASSERT(events & event);
    events = (Event)(events & ~event);
    EventContext& ctx = getContext(event);
    if (ctx.cb) {
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        ctx.scheduler->schedule(&ctx.fiber);
    }
    ctx.scheduler = nullptr;
    return ;
}

IOManager::IOManager(size_t threads, bool use_caller, const std::string& name) 
    : Scheduler(threads, use_caller, name){
    // 创建scheduler
    // 创建epoll
    m_epfd = epoll_create(5000);
    SYLAR_ASSERT(m_epfd > 0);

    // 创建管道
    int rt = pipe(m_tickleFds);
    SYLAR_ASSERT(!rt);

    // 创建epoll事件
    epoll_event event;
    memset(&event, 0, sizeof(epoll_event)); // 初始化
    event.events = EPOLLIN | EPOLLET;       // 读 | 边缘触发
    event.data.fd = m_tickleFds[0];         // 绑定管道读端

    // 管道读端设置为非阻塞
    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    SYLAR_ASSERT(!rt);
    // 添加事件
    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    SYLAR_ASSERT(!rt);

    //m_fdContexts.resize(64);
    contextResize(32);
    //启动调度器
    start();
}
IOManager::~IOManager() {
    stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for (size_t i = 0; i < m_fdContexts.size(); i++) {
        if (m_fdContexts[i]) {
            delete m_fdContexts[i];
        }
    }
}

void IOManager::contextResize(size_t size) {
    m_fdContexts.resize(size);
    
    for (size_t i = 0; i < m_fdContexts.size(); i++) {
        if (!m_fdContexts[i]) {
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    } 
}


int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
    FdContext* fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    // 判断fd是否比fd队列小
    if ((int)m_fdContexts.size() > fd) {
        // 使其指向该位置
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    } else {    // 扩容
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        contextResize(fd * 1.5);
        fd_ctx = m_fdContexts[fd];
    }

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    // 判断是否已存在该事件且事件类型相同
    if (fd_ctx->events & event) {
        SYLAR_LOG_ERROR(g_logger) << "addEvent assert fd = " << fd
                << " event = " << event << " fd_ctx.event = " << fd_ctx->events;
        SYLAR_ASSERT(!(fd_ctx->events & event));
    }

    // 如果为空则为添加事件， 否则为修改事件
    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    epevent.events = EPOLLET | fd_ctx->events | event;  // 边缘触发 | 原有的事件 | 需要添加/修改的事件
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << op
                << ", " << fd << ", " << epevent.events << "): " << rt 
                << " (" << errno << ") (" << strerror(errno) << ")";
        return -1;
    }

    ++m_pendingEventCount;
    fd_ctx->events = (Event)(fd_ctx->events | event);   // 队列中相应fd加入该事件
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event); // 取出对应成员
    SYLAR_ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);
    
    // 为该fd的读成员或写成员设置调度器
    event_ctx.scheduler = Scheduler::GetThis();
    // 设置回调函数或者fiber
    if (cb) {
        event_ctx.cb.swap(cb);
    } else {
        event_ctx.fiber = Fiber::GetThis();
        SYLAR_ASSERT(event_ctx.fiber->getState() == Fiber::EXEC);
    }

    return 0;
}

// 删除fd上的某个事件
bool IOManager::delEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    // fd要在队列范围以内
    if ((int)m_fdContexts.size() <= fd) {
        return false;
    }
    // 指向队列中的fd
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    // 待删除的事件类型不存在指定类型
    if (!(fd_ctx->events & event)) {
        return false;
    }
    // 消除该事件类型
    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << op
                << ", " << fd << ", " << epevent.events << "): " << rt 
                << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    --m_pendingEventCount;
    // 修改队列中相应的fd
    fd_ctx->events = new_events;
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);    // 置空
    return true;
}

// 删除fd上的某个事件， 但先触发该事件
bool IOManager::cancelEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if ((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (!(fd_ctx->events & event)) {
        return false;
    }

    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << op
                << ", " << fd << ", " << epevent.events << "): " << rt 
                << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    
    return true;
}

// 删除该fd， 删除前触发对应的事件
bool IOManager::cancelAll(int fd) {
    RWMutexType::ReadLock lock(m_mutex);
    if ((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (!fd_ctx->events) {
        return false;
    }

    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << op
                << ", " << fd << ", " << epevent.events << "): " << rt 
                << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    if (fd_ctx->events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }
    if (fd_ctx->events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }
    SYLAR_ASSERT(fd_ctx->events == 0);
    return true;
}

IOManager* IOManager::GetThis() {
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

void IOManager::tickle() {
    if (hasIdleThreads()) {
        return ;
    }
    int rt = write(m_tickleFds[1], "T", 1);
    SYLAR_ASSERT(rt == 1);
}

bool IOManager::stopping(uint64_t& timeout) {
    // 查看定时器是否为空(或返回还有多久发生)
    timeout = getNextTimer();
    return timeout == ~0ull
        && m_pendingEventCount == 0
        && Scheduler::stopping();
}

bool IOManager::stopping() {
    uint64_t timeout = 0;
    return stopping(timeout);
}

void IOManager::idle() {
    // 创建epoll事件组
    epoll_event* events = new epoll_event[64]();
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr) {
        delete[] ptr;
    });

    while (true) {
        uint64_t next_timeout;
        if (stopping(next_timeout)) {
            SYLAR_LOG_INFO(g_logger) << "name = " << getName() << " idle stopping exit";
            break;
        }

        int rt = 0;
        do {
            // 最大超时时间
            static const int MAX_TIMEOUT = 3000;
            // 如果定时器不为空
            if (next_timeout != ~0ull) {
                // 取两者的最小值
                next_timeout = (int)next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
            } else {
                // 否则取最大超时时间
                next_timeout = MAX_TIMEOUT;
            }
            // 监听
            rt = epoll_wait(m_epfd, events, 64, int(next_timeout));
            // 是否发生中断
            if (rt < 0 && errno == EINTR) {

            } else {
                break;
            }
        } while (true);

        std::vector<std::function<void()>> cbs;
        // 取出超时定时器的回调函数
        listExpiredCb(cbs);
        if (!cbs.empty()) {
            // 把它们做成协程
            schedule(cbs.begin(), cbs.end());
            // for (auto& i : cbs) {
            //     schedule(i);
            // }
            cbs.clear();
        }

        for (int i = 0; i < rt; i++) {
            epoll_event& event = events[i];
            // 判断是否为管道读事件
            if (event.data.fd == m_tickleFds[0]) {
                uint8_t dummy;
                while (read(m_tickleFds[0], &dummy, 1) == 1);
                continue;
            }

            FdContext* fd_ctx = (FdContext*)event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            if (event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= EPOLLIN | EPOLLOUT;
            }
            int real_events = NONE;
            if (event.events & EPOLLIN) {
                real_events |= READ;
            }
            if (event.events & EPOLLOUT) {
                real_events |= WRITE;
            }

            // 发生的不是期望的事件？
            if ((fd_ctx->events & real_events) == NONE) {
                continue;
            }
            // “已读”
            int left_events = (fd_ctx->events & ~real_events);
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;

            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if (rt2) {
                SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << op
                    << ", " << fd_ctx->fd << ", " << event.events << "): " << rt2 
                    << " (" << errno << ") (" << strerror(errno) << ")";
                continue;
            }
            // 触发上面相应的事件
            if (real_events & READ) {
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if (real_events & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }
        // 切回到run协程
        Fiber::ptr cur = Fiber::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();

        raw_ptr->swapOut();
    }
} 

void IOManager::onTimerInsertedAtFront() {
    tickle();
}

}