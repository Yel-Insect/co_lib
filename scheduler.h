#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <memory>
#include <list>
#include "thread.h"
#include "fiber.h"

namespace sylar {

class Scheduler {
public: 
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    virtual ~Scheduler();

    const std::string& getName() const { return m_name; }

    static Scheduler* GetThis();
    static Fiber* GetMainFiber();

    void start();
    void stop();
    
    // 传入的参数要不是fiber要不是callback函数
    template<class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread);
        }
        if (need_tickle) {
            tickle();
        }
    }

    template<class InputIterator>
    void schedule(InputIterator begin, InputIterator end) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while (begin != end) {
                need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
                ++begin;
            }
        }
        if (need_tickle) {
            tickle();
        }
    }

protected:
    virtual void tickle();
    virtual void idle();
    virtual bool stopping();
    
    void run();
    void setThis();

    bool hasIdleThreads() const { return m_idleThreadCount > 0; }
private:
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread) {
        // 如果协程队列为空，则通知
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if (ft.fiber || ft.cb) {
            m_fibers.push_back(ft);
        }

        return need_tickle;
    }

    struct FiberAndThread {
        Fiber::ptr fiber;
        std::function<void()> cb;
        int thread;

        FiberAndThread(Fiber::ptr f, int thr) 
            : fiber(f), thread(thr) { 
        }

        // 传递智能指针的指针
        // 智能指针对象本身在堆区的情况(应该吧？)
        FiberAndThread(Fiber::ptr* f, int thr) 
            : thread(thr) {
            fiber.swap(*f);
        }

        FiberAndThread(std::function<void()> f, int thr) 
            : cb(f), thread(thr){
        }

        FiberAndThread(std::function<void()>* f, int thr) 
            : thread(thr) {
            cb.swap(*f);
        }

        // stl调用默认构造函数
        FiberAndThread() 
            : thread(-1) {
        }

        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };
private:
    MutexType m_mutex;
    std::vector<Thread::ptr> m_threads;     // 线程池
    std::list<FiberAndThread> m_fibers;     // 即将要执行或者计划执行的协程
    std::string m_name;
    Fiber::ptr m_rootFiber;

protected:
    std::vector<int> m_threadIds;
    size_t m_threadCount = 0;
    std::atomic<size_t> m_activeThreadCount = {0};  // 活跃线程数量
    std::atomic<size_t> m_idleThreadCount = {0};    // 空闲线程数量
    bool m_stopping = true;
    bool m_autoStop = false;                        // 是否主动停止
    int m_rootThread = 0;                       // 主线程id(use_caller的id)
};



}

#endif