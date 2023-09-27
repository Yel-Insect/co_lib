#include <atomic>
#include "fiber.h"
#include "macro.h"
#include "config.h"
#include "scheduler.h"
#include "log.h"

namespace co_lib {

static std::atomic<uint64_t> s_fiber_id {0};
static std::atomic<uint64_t> s_fiber_count {0};

// 当前协程
static thread_local Fiber* t_fiber = nullptr;
static thread_local Fiber::ptr t_threadFiber = nullptr;

static ConfigVar<uint32_t>::ptr g_fiber_stack_size = 
    Config::Lookup<uint32_t>("fiber.stack_size", 1024 * 1024, "fiber stack size");

static co_lib::Logger::ptr g_logger = LOG_NAME("system");

class MallocStackAllocator {
public:
    static void* Alloc(size_t size) {
        return malloc(size);
    }

    static void Dealloc(void* vp, size_t size) {
        return free(vp);
    }
};

using StackAllocator = MallocStackAllocator;

uint64_t Fiber::GetFiberId() {
    if (t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

Fiber::Fiber() {
    m_state = EXEC;
    SetThis(this);

    if (getcontext(&m_ctx)) {
        CO_LIB_ASSERT2(false, "getcontext");
    }

    ++s_fiber_count;

    // LOG_DEBUG(g_logger) << "Fiber::Fiber";
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller) 
    : m_id(++s_fiber_id), m_cb(cb) {
    ++s_fiber_count;    // 增加协程数量
    // 如果大于0, 则按用户设置栈大小， 否则按照默认设置
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();
    // 分配一个栈空间
    m_stack = StackAllocator::Alloc(m_stacksize);
    // 初始化上下文
    if (getcontext(&m_ctx)) {
        CO_LIB_ASSERT2(false, "getcontext");
    } 
    // 初始化上下文状态
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    // 设置上下文
    // 如果不为use_caller则与MainFunc绑定
    // 否则则与CallerMainFUnc绑定(需要区分主线程与工作线程当中的协程切换)
    if (!use_caller) {
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
    } else {
        makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
    }
    /*
        这里为什么需要区分
        在工作线程里主协程与run协程相同，与工作协程相互切换，因此只需要交换t_fiber
        而在主线程里主协程与run协程是分开的，需要区分，否则就会与自己交换
    */

    // LOG_DEBUG(g_logger) << "Fiber::Fiber id = " << m_id;
}

Fiber::~Fiber() {
    --s_fiber_count;
    if (m_stack) {
        CO_LIB_ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT);
        StackAllocator::Dealloc(m_stack, m_stacksize);
    } else {
        CO_LIB_ASSERT(!m_cb);
        CO_LIB_ASSERT(m_state == EXEC);

        Fiber* cur = t_fiber;
        if (cur == this) {
            SetThis(nullptr);
        }
    }

    // LOG_DEBUG(g_logger) << "Fiber::~Fiber id = " << m_id;
}

void Fiber::reset(std::function<void()> cb) {
    CO_LIB_ASSERT(m_stack);
    CO_LIB_ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT);
    m_cb = cb;
    if (getcontext(&m_ctx)) {
        CO_LIB_ASSERT2(false, "getcontext");
    }

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = INIT;
}

// 与工作线程的主协程互换
void Fiber::call() {
    SetThis(this);
    m_state = EXEC;
    if (swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
         CO_LIB_ASSERT2(false, "swapcontext");
    }
}

// 同上
void Fiber::back() {
    SetThis(t_threadFiber.get());
    if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
        CO_LIB_ASSERT2(false, "swapcontext");
    }
}

// 与工作线程的主协程互换
void Fiber::swapIn() {
    SetThis(this);
    CO_LIB_ASSERT(m_state != EXEC);
    m_state = EXEC;
    if (swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
        CO_LIB_ASSERT2(false, "swapcontext");
    }
}

// 同上
void Fiber::swapOut() {
    // if (t_fiber != Scheduler::GetMainFiber()) { 
        SetThis(Scheduler::GetMainFiber());
        if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
            CO_LIB_ASSERT2(false, "swapcontext");
        }
    // } else {
    //     SetThis(t_threadFiber.get());
    //     if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
    //         CO_LIB_ASSERT2(false, "swapcontext");
    //     }
    // }
    
}

void Fiber::SetThis(Fiber* f) {
    t_fiber = f;
}

Fiber::ptr Fiber::GetThis() {
    if (t_fiber) {
        return t_fiber->shared_from_this();
    }
    Fiber::ptr main_fiber(new Fiber);
    CO_LIB_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
}


void Fiber::YieldToHold() {
    Fiber::ptr cur = GetThis();
    CO_LIB_ASSERT(cur->m_state == EXEC);
    // cur->m_state = HOLD;
    cur->swapOut();
}

uint64_t Fiber::TotalFibers() {
    return s_fiber_count;
}

void Fiber::MainFunc() {
    // 当协程结束时栈还存在，因此该智能指针不会销毁
    Fiber::ptr cur = GetThis();     // 当前获取协程指针
    CO_LIB_ASSERT(cur);
    try {
        cur->m_cb();    // 调用回调函数
        cur->m_cb = nullptr;    // 置空
        cur->m_state = TERM;    // 设为结束状态
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        LOG_ERROR(g_logger) << "Fiber Except" << ex.what()
        << " fiber_id = " << cur->getId()
        << std::endl << co_lib::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        LOG_ERROR(g_logger) << "Fiber Except"
        << " fiber_id = " << cur->getId()
        << std::endl << co_lib::BacktraceToString();
    }

    // 拿出裸指针
    auto raw_ptr = cur.get();
    // 将计数-1
    cur.reset();
    // 切换到主协程
    raw_ptr->swapOut(); // 区别

    CO_LIB_ASSERT2(false, "never reach" + std::to_string(raw_ptr->getId()))
}

void Fiber::CallerMainFunc() {
    // 当协程结束时栈还存在，因此该智能指针不会销毁
    Fiber::ptr cur = GetThis();
    CO_LIB_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        LOG_ERROR(g_logger) << "Fiber Except" << ex.what()
        << " fiber_id = " << cur->getId()
        << std::endl << co_lib::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        LOG_ERROR(g_logger) << "Fiber Except"
        << " fiber_id = " << cur->getId()
        << std::endl << co_lib::BacktraceToString();
    }

    // 拿出裸指针
    auto raw_ptr = cur.get();
    // 将计数-1
    cur.reset();
    // 切换到主协程
    raw_ptr->back();    // 区别

    CO_LIB_ASSERT2(false, "never reach" + std::to_string(raw_ptr->getId()))
}

}