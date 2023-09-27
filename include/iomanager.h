#ifndef __IOMANAGER_H__
#define __IOMANAGER_H__

#include "scheduler.h"
#include "timer.h"
//#include <string>

namespace co_lib {

class IOManager : public Scheduler, public TimerManager {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;

    enum Event {
        NONE    = 0x0,
        READ    = 0x1,      // EPOLLIN
        WRITE   = 0x4       // EPOLLOUT
    };
private:
    // 描述符上下文，封装读写事件上下文、锁、句柄以及事件类型
    struct FdContext {
        typedef Mutex MutexType;
        // 事件上下文， 封装调度器、协程、回调函数
        struct EventContext {
            Scheduler* scheduler = nullptr; // 事件执行的scheduler
            Fiber::ptr fiber;               // 事件的协程
            std::function<void()> cb;       // 事件的回调函数
        };

        EventContext& getContext(Event event);
        void resetContext(EventContext& ctx);
        void triggerEvent(Event event);

        EventContext read;      // 读事件
        EventContext write;     // 写事件
        int fd = 0;             // 事件关联的句柄
        Event events = NONE;    // 已经注册的事件
        MutexType mutex;
    };

    
public:
    IOManager(size_t thread = 1, bool use_caller = true, const std::string& name = "");
    ~IOManager();

    // 0 success, -1 error
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    bool delEvent(int fd, Event event);
    bool cancelEvent(int fd, Event event);

    bool cancelAll(int fd);

    static IOManager* GetThis();

protected:
    void tickle() override;     // 有协程需要执行时会被触发
    bool stopping() override;   // 调度模块是否需要终止
    void idle() override;       // epoll_wait
    void onTimerInsertedAtFront() override;

    void contextResize(size_t size);
    bool stopping(uint64_t& timeout);
private:
    int m_epfd = 0;
    int m_tickleFds[2];

    std::atomic<size_t> m_pendingEventCount = {0};
    RWMutexType m_mutex;
    std::vector<FdContext*> m_fdContexts;
};

}

#endif