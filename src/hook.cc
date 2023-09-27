#include "hook.h"
#include <dlfcn.h>
#include "iomanager.h"
#include "fiber.h"
#include "log.h"
#include "fd_manager.h"
#include "config.h"

co_lib::Logger::ptr g_logger = LOG_NAME("system");

namespace co_lib {

static co_lib::ConfigVar<int>::ptr g_tcp_connect_timeout = 
    co_lib::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX)    \
    XX(sleep)       \
    XX(usleep)      \
    XX(nanosleep)   \
    XX(socket)      \
    XX(connect)     \
    XX(accept)      \
    XX(read)        \
    XX(readv)       \
    XX(recv)        \
    XX(recvfrom)    \
    XX(recvmsg)     \
    XX(write)       \
    XX(writev)      \
    XX(send)        \
    XX(sendto)      \
    XX(sendmsg)     \
    XX(close)       \
    XX(fcntl)       \
    XX(ioctl)       \
    XX(getsockopt)  \
    XX(setsockopt)
   
// 为name_f设置指向到库函数
void hook_init() {
    static bool is_inited = false;
    if (is_inited) {
        return ;
    }
    /*
        HOOK_FUN(XX) {
            XX(sleep) {
                // 宏 RTLD_NEXT 的含义是告诉链接器，将 sleep 这个符号的定义解析到后面
                // 的可执行目标文件或者共享库中，而不要解析为本模块中的定义.
                // dlsym返回的是void*
                sleep_f = (sleep_fun)dlsym(RTLD_NEXT, "sleep");
            }
            ...
        }
    */
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX);
#undef XX
}
static uint64_t s_connect_timeout = -1;
struct _HookIniter {
    _HookIniter() {
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();

        g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value){
            LOG_INFO(g_logger) << "tcp connect timeout changed from "
                << old_value << " to " << new_value;
            s_connect_timeout = new_value;
        });
    }
};

static _HookIniter s_hook_initer;

bool is_hook_enable() {
    return t_hook_enable;
}

void set_hook_enable(bool flag) {
    t_hook_enable = flag;
}

}





struct timer_info {
    int cancelled = 0;
};

// 用于传一个要hook的函数、函数名、 io event里的事件以及fdmanager里超时的类型
template<typename OriginFun, typename ... Args>
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name,
                uint32_t event, int timeout_so, Args&&... args) {
    // 如果不用hook
    if (!co_lib::t_hook_enable) {
        return fun(fd, std::forward<Args>(args)...);
    }

    //LOG_DEBUG(g_logger) << "do_ioc<" << hook_fun_name << ">";

    // 判断文件是否存在
    co_lib::FdCtx::ptr ctx = co_lib::FdMgr::GetInstance()->get(fd);
    if (!ctx) { // 不存在该fd
        return fun(fd, std::forward<Args>(args)...);
    }
    // fd是否关闭了
    if (ctx->isClosed()) {
        errno = EBADF;
        return -1;
    }
    // 如果不是socket或者是用户设置的非阻塞
    if(!ctx->isSocket() || ctx->getUserNonblock()) {
        return fun(fd, std::forward<Args>(args)...);
    }
    // 取出超时时间
    uint64_t to = ctx->getTimeout(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info);

retry:
    // 执行函数
    ssize_t n = fun(fd, std::forward<Args>(args)...);
    // 如果被中断的话，继续执行
    while (n == -1 && errno == EINTR) {
        n = fun(fd, std::forward<Args>(args)...);
    }
    // 缓冲区满了
    if (n == -1 && errno == EAGAIN) {
        co_lib::IOManager* iom = co_lib::IOManager::GetThis();
        co_lib::Timer::ptr timer;
        std::weak_ptr<timer_info> winfo(tinfo);
        // 超时时间不等于-1
        if (to != (uint64_t)-1) {
            // 添加条件超时定时器
            timer = iom->addConditionTimer(to, [winfo, fd, iom, event]() {
                auto t = winfo.lock();
                if (!t || t->cancelled) {   // 设了错误就返回
                    return ;
                }
                // 没有则设置并取消该事件
                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(fd, (co_lib::IOManager::Event)(event));
            }, winfo);
        }
        int c = 0;
        uint64_t now = 0;
        // 添加事件(传的cb为空，即以当前协程为唤醒对象)
        int rt = iom->addEvent(fd, (co_lib::IOManager::Event)(event));
        if (rt) {
            LOG_ERROR(g_logger) << hook_fun_name << "addEvent("
                << fd << ", " << event << ")";
            if (timer) {
                timer->cancel();
            }
            return -1;
        } else {    // 添加成功就让出协程
            co_lib::Fiber::YieldToHold();
            if (timer) {
                timer->cancel();
            }
            if (tinfo->cancelled) {
                errno = tinfo->cancelled;
                return -1;
            }

            goto retry;
        }
    }

    return n;
}




extern "C" {
    /*
        HOOK_FUN(XX) {
            XX(sleep) {
                sleep_fun sleep_f = nullptr;
            }
            ...
        }
    */
// 定义函数指针
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX

/*
    sleep系列
    是否需要hook
    是则指定携程与调度器，添加定时器，让出执行时间
    否则调用系统函数
*/
unsigned int sleep(unsigned int seconds) {
    if (!co_lib::t_hook_enable) {
        return sleep_f(seconds);
    }

    co_lib::Fiber::ptr fiber = co_lib::Fiber::GetThis();
    co_lib::IOManager* iom = co_lib::IOManager::GetThis();
    iom->addTimer(seconds * 1000, std::bind((void(co_lib::Scheduler::*)
            (co_lib::Fiber::ptr, int thread))&co_lib::IOManager::schedule
            , iom, fiber, -1));
    co_lib::Fiber::YieldToHold();
    return 0;
}

int usleep(useconds_t usec) {
    if (!co_lib::t_hook_enable) {
        return usleep_f(usec);
    }

    co_lib::Fiber::ptr fiber = co_lib::Fiber::GetThis();
    co_lib::IOManager* iom = co_lib::IOManager::GetThis();
    iom->addTimer(usec / 1000, std::bind((void(co_lib::Scheduler::*)
            (co_lib::Fiber::ptr, int thread))&co_lib::IOManager::schedule
            , iom, fiber, -1));
    
    co_lib::Fiber::YieldToHold();
    return 0;
}

int nanosleep(const struct timespec* reg, struct timespec* rem) {
    if (!co_lib::t_hook_enable) {
        return nanosleep_f(reg, rem);
    }

    int timeout_ms = reg->tv_sec * 1000 + reg->tv_nsec / 1000 / 1000;
    co_lib::Fiber::ptr fiber = co_lib::Fiber::GetThis();
    co_lib::IOManager* iom = co_lib::IOManager::GetThis();
    iom->addTimer(timeout_ms / 1000, std::bind((void(co_lib::Scheduler::*)
            (co_lib::Fiber::ptr, int thread))&co_lib::IOManager::schedule
            , iom, fiber, -1));
    
    co_lib::Fiber::YieldToHold();
    return 0;
}

// socket系列(同上吧)
int socket(int domain, int type, int protocol) {
    if (!co_lib::t_hook_enable) {
        return socket_f(domain, type, protocol);
    }
    int fd = socket_f(domain, type, protocol);
    if (fd == -1) {
        return fd;
    }
    co_lib::FdMgr::GetInstance()->get(fd, true);
    return fd;
}

int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms) {
    // 是否hook
    if (!co_lib::t_hook_enable) {
        return connect_f(fd, addr, addrlen);
    }
    co_lib::FdCtx::ptr ctx = co_lib::FdMgr::GetInstance()->get(fd);
    if (!ctx || ctx->isClosed()) {
        errno = EBADF;  // fd已经失效了
        return -1;
    }
    // 不是socket
    if (!ctx->isSocket()) {     
        return connect_f(fd, addr, addrlen);
    }
    // 用户设置的非阻塞
    if (ctx->getUserNonblock()) {
        return connect_f(fd, addr, addrlen);
    }

    int n = connect_f(fd, addr, addrlen);
    if (n == 0) {   // 连接成功
        return 0;
    } else if (n != -1 || errno != EINPROGRESS) {   // 不为-1或不为非阻塞？
        return n;
    }
    // 取得iom
    co_lib::IOManager* iom = co_lib::IOManager::GetThis();
    co_lib::Timer::ptr timer;
    /*
        timer_info用于检测该任务协程是否执行完成
        如果协程执行完成则虚指针指向的winfo会失效，无法提升从而判断
        已在规定时间内连接成功
    */
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info> winfo(tinfo);
    // 超时时间不为-1
    if (timeout_ms != (uint64_t)-1) {
        // 添加条件定时器(连接超时时间)
        timer = iom->addConditionTimer(timeout_ms, [winfo, fd, iom]() {
            auto t = winfo.lock();  // 检查该协程是否还在hold
            if (!t || t->cancelled) {   // 是否提升成功以及定时器是否被取消
                return ;
            }
            t->cancelled = ETIMEDOUT;   // 连接超时
            iom->cancelEvent(fd, co_lib::IOManager::WRITE);  // 取消该fd上的写事件
        }, winfo);
    }
    // 往epoll上添加该fd的写事件
    int rt = iom->addEvent(fd, co_lib::IOManager::WRITE);
    if (rt == 0) {
        // 让出协程
        co_lib::Fiber::YieldToHold();
        // 取消定时器
        if (timer) {
            timer->cancel();
        }
        // 查看是否已经超时
        if (tinfo->cancelled) {
            errno = tinfo->cancelled;
            return -1;
        }
    } else {
        if (timer) {
            timer->cancel();
        }
        LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
    }

    int error = 0;
    socklen_t len = sizeof(int);
    if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {
        return -1;
    }
    if (!error) {
        return 0;
    } else {
        errno = error;
        return -1;
    }


}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return connect_with_timeout(sockfd, addr, addrlen, co_lib::s_connect_timeout);
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    int fd = do_io(sockfd, accept_f, "accept", co_lib::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
    if (fd >= 0) {
        co_lib::FdMgr::GetInstance()->get(fd, true);
    }
    return fd;
}

// read系列
ssize_t read(int fd, void *buf, size_t count) {
    return do_io(fd, read_f, "read", co_lib::IOManager::READ, SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, readv_f, "readv", co_lib::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    return do_io(sockfd, recv_f, "recv", co_lib::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    return do_io(sockfd, recvfrom_f, "recvfrom", co_lib::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    return do_io(sockfd, recvmsg_f, "recvmsg", co_lib::IOManager::READ, SO_RCVTIMEO, msg, flags);
}

// write系列
ssize_t write(int fd, const void *buf, size_t count) {
    return do_io(fd, write_f, "write", co_lib::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, writev_f, "writev", co_lib::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    return do_io(sockfd, send_f, "send", co_lib::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags);
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
    return do_io(sockfd, sendto_f, "sendto", co_lib::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags, dest_addr, addrlen);
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags) {
    return do_io(sockfd, sendmsg_f, "sendmsg", co_lib::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}

int close(int fd) {
    if (!co_lib::t_hook_enable) {
        return close_f(fd);
    }

    co_lib::FdCtx::ptr ctx = co_lib::FdMgr::GetInstance()->get(fd);
    if (ctx) {
        auto iom = co_lib::IOManager::GetThis();
        if (iom) {
            iom->cancelAll(fd);
        }
        co_lib::FdMgr::GetInstance()->del(fd);
    }
    return close_f(fd);
}

int fcntl(int fd, int cmd, ...) {
    // if (!co_lib::t_hook_enable) {
    //     return fcntl_f(fd, int cmd)
    // }

    va_list va;
    va_start(va, cmd);
    switch(cmd) {
        case F_SETFL:   // 给fd设置属性
            {
                // 获取下一个为int类型的参数
                int arg = va_arg(va, int);
                va_end(va);
                co_lib::FdCtx::ptr ctx = co_lib::FdMgr::GetInstance()->get(fd);
                if (!ctx || ctx->isClosed() || !ctx->isSocket()) {
                    return fcntl_f(fd, cmd, arg);
                }
                // 看用户有没有设非阻塞
                ctx->setUserNonblock(arg & O_NONBLOCK);
                // 如果系统为非阻塞的话则设上(以系统为主)
                if (ctx->getSysNonblock()) {
                    arg |= O_NONBLOCK;
                } else {
                    arg &= ~O_NONBLOCK;
                }
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETFL:   // 取得fd的属性
            {
                va_end(va);
                int arg = fcntl_f(fd, cmd);
                co_lib::FdCtx::ptr ctx = co_lib::FdMgr::GetInstance()->get(fd);
                if (!ctx || ctx->isClosed() || !ctx->isSocket()) {
                    return arg;
                } 
                if (ctx->getUserNonblock()) {
                    return arg | O_NONBLOCK;
                } else {
                    return arg & ~O_NONBLOCK;
                }
            }
            break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
        case F_SETPIPE_SZ: 
            {
                int arg = va_arg(va, int);
                va_end(va);
                return fcntl_f(fd, cmd);
            }
            break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
        case F_GETPIPE_SZ:
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }
            break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
            {
                struct flock* arg = va_arg(va, struct flock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETOWN_EX:
        case F_SETOWN_EX:
            {
                struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        default:
            va_end(va);
            return fcntl_f(fd, cmd);
    }
}

int ioctl(int d, unsigned long int request, ...) {
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);

    if (FIONBIO == request) {
        bool user_nonblock = !!*(int*)arg;
        co_lib::FdCtx::ptr ctx = co_lib::FdMgr::GetInstance()->get(d);
        if (!ctx || ctx->isClosed() || !ctx->isSocket()) {
            return ioctl_f(d, request, arg);
        }
        ctx->setUserNonblock(user_nonblock);
    }
    return ioctl_f(d, request, arg);
}


int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}


int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    if (!co_lib::t_hook_enable) {
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }

    // 如果想要在套接字级别上设置选项，就必须把level设置为 SOL_SOCKET
    if (level == SOL_SOCKET) {
        if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
            co_lib::FdCtx::ptr ctx = co_lib::FdMgr::GetInstance()->get(sockfd);
            if (ctx) {
                const timeval* tv = (const timeval*)optval;
                ctx->setTimeout(optname, tv->tv_sec * 1000 + tv->tv_usec / 1000);
            }
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}


}

