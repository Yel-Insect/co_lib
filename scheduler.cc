#include "scheduler.h"
#include "log.h"
#include "macro.h"
#include "hook.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static thread_local Scheduler* t_scheduler = nullptr;
static thread_local Fiber* t_fiber = nullptr;

// 创建调度器
Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name) 
    : m_name(name) {
    SYLAR_ASSERT(threads > 0);
    // 是否为use_caller
    // use_caller似乎是让老板一起打工😂
    if (use_caller) {
        sylar::Fiber::GetThis();        // 新建一个主协程(管理外部主线程的代码)
        --threads;                      // 线程数-1
        // 判断线程是否已存在的调度器
        SYLAR_ASSERT(GetThis() == nullptr);
        t_scheduler = this;     // 另线程的调度器为自己
        // 创建一个(另一个？我觉得run协程可能比较合适)主协程，将其与调度器的run方法绑定
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        sylar::Thread::SetName(m_name);     // 设置线程名称

        t_fiber = m_rootFiber.get();
        m_rootThread = sylar::GetThreadId();    // 设当前线程id为主线程id
        m_threadIds.push_back(m_rootThread);
    } else {
        m_rootThread = -1;
    }
    m_threadCount = threads;
}

Scheduler::~Scheduler() {
    SYLAR_ASSERT(m_stopping);
    if (GetThis() == this) {
        t_scheduler = nullptr;
    }
}

Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}

Fiber* Scheduler::GetMainFiber() {
    return t_fiber;
}

void Scheduler::start() {
    MutexType::Lock lock(m_mutex);
    if (!m_stopping) {
        return ;
    }
    m_stopping = false;
    SYLAR_ASSERT(m_threads.empty());

    m_threads.resize(m_threadCount);
    for (size_t i = 0; i < m_threadCount; i++) {
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), 
                            m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
    lock.unlock();

    // if (m_rootFiber) {
    //     m_rootFiber->call();
    //     SYLAR_LOG_INFO(g_logger) << "call out " << m_rootFiber->getState();
    // }
}

void Scheduler::stop() {
    m_autoStop = true;
    // 如果只用了创建scheduler的线程，它的run协程为TERM或者INIT则stop
    if (m_rootFiber && m_threadCount == 0 && (m_rootFiber->getState() == Fiber::TERM
        || m_rootFiber->getState() == Fiber::INIT)) {
        SYLAR_LOG_INFO(g_logger) << this << " stopped";
        m_stopping = true;

        if (stopping()) {
            return ;
        }
    }

    //bool exit_on_this_fiber = false;
    if (m_rootThread != -1) {
        SYLAR_ASSERT(GetThis() == this);
    } else {
        SYLAR_ASSERT(GetThis() != this);
    }

    m_stopping = true;
    for (size_t i = 0; i < m_threadCount; i++) {
        tickle();
    }

    if (m_rootFiber) {
        tickle();
    }
    if (m_rootFiber){ 
        // while (!stopping()) {
        //     if (m_rootFiber->getState() == Fiber::TERM
        //         || m_rootFiber->getState() == Fiber::EXCEPT) {
        //         m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        //         SYLAR_LOG_INFO(g_logger) << " root fiber is term, reset";
        //         t_fiber = m_rootFiber.get();
        //     }
        //     m_rootFiber->call();
        // }
        if (!stopping()) {
            m_rootFiber->call();
        }
    }

    // 回收线程
    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }

    for (auto& i : thrs) {
        i->join();
    }
}

void Scheduler::setThis() {
    t_scheduler = this;
}

// 线程调度主函数
void Scheduler::run() {
    SYLAR_LOG_INFO(g_logger) << "run";
    set_hook_enable(true);
    setThis();      // 把当前线程的schedule设置为自己
    if (sylar::GetThreadId() != m_rootThread) {     // 如果线程id不为主线程id
        t_fiber = Fiber::GetThis().get();           // 为工作线程设置fiber
    }

    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    // SYLAR_LOG_INFO(g_logger) << "idle created" ;
    Fiber::ptr cb_fiber;

    FiberAndThread ft;
    while (true) {
        ft.reset();
        bool tickle_me = false;
        bool is_active = false;
        {
            // 从消息队列中取出任务
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            while (it != m_fibers.end()) {
                // 如果任务设置的线程id不等于此线程id
                if (it->thread != -1 && it->thread != sylar::GetThreadId()) {
                    it++;
                    // 通知其他线程
                    tickle_me = true;
                    continue;
                }

                // 检查fiber或者cb是否为空
                SYLAR_ASSERT(it->fiber || it->cb);
                // 如果fiber不为空且为执行状态则跳过
                if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    it++;
                    continue;
                }

                ft = *it;
                m_fibers.erase(it);
                ++m_activeThreadCount;
                is_active = true;
                break;
            }
        }
        // 通知其他线程处理id为自己的任务
        if (tickle_me) {
            tickle();
        }
        // fibe且状态不为TERM或EXCEPT则执行
        if (ft.fiber && (ft.fiber->getState() != Fiber::TERM
                    || ft.fiber->getState() != Fiber::EXCEPT)) {
            
            ft.fiber->swapIn();
            --m_activeThreadCount;

            // 如果状态为ready则继续加入消息队列里(说明它YiledtoReady了)
            if (ft.fiber->getState() == Fiber::READY) {
                schedule(ft.fiber);
            } else if (ft.fiber->getState() != Fiber::TERM  // 如果状态不为TERM或EXCEPT则设为HOLD
                && ft.fiber->getState() != Fiber::EXCEPT) {
                ft.fiber->m_state = Fiber::HOLD;
            }
            ft.reset();     // 重置
        } else if(ft.cb) {  // 如果回调
            if (cb_fiber) {     // 如果cb_fiber创建好了直接reset
                cb_fiber->reset(ft.cb);
            } else {            // 否则new一个新的fiber
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();     // 重置
            //++m_activeThreadCount;
            cb_fiber->swapIn();
            --m_activeThreadCount;

            // 同上
            if (cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);
                cb_fiber.reset();
            } else if (cb_fiber->getState() == Fiber::EXCEPT
                    || cb_fiber->getState() == Fiber::TERM) {
                cb_fiber->reset(nullptr);
            } else {
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        } else {
            if (is_active) {
                --m_activeThreadCount;
                continue;
            }
            if (idle_fiber->getState() == Fiber::TERM) {
                SYLAR_LOG_INFO(g_logger) << "idle fiber term";
                // tickle();
                break ;
                //continue;
            }

            //++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;
            if (idle_fiber->getState() != Fiber::TERM
                && idle_fiber->getState() != Fiber::EXCEPT) {
                idle_fiber->m_state = Fiber::HOLD;
            }
            
        }
    }
}

void Scheduler::tickle() {
    SYLAR_LOG_INFO(g_logger) << "tickle";
}

bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping && m_fibers.empty() 
            && m_activeThreadCount == 0;
}

void Scheduler::idle() {
    SYLAR_LOG_INFO(g_logger) << "idle";
    while (!stopping()) {
        sylar::Fiber::YieldToHold();
    }
}

}