#include "timer.h"
#include "util.h"

namespace sylar {

bool Timer::Comparator::operator()(const Timer::ptr& lhs, 
                                    const Timer::ptr& rhs) const {
    if (!lhs && !rhs) {
        return false;
    }                
    if (!lhs) {
        return true;
    }               
    if (!rhs) {
        return false;
    }
    if (lhs->m_next < rhs->m_next) {
        return true;
    }
    if (lhs->m_next > rhs->m_next) {
        return false;
    }

    return lhs.get() < rhs.get();
}

Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring
        , TimerManager* manager) 
    : m_recurring(recurring), m_ms(ms), m_cb(cb), m_manager(manager) {
    m_next = sylar::GetCurrentMS() + m_ms;
}

Timer::Timer(uint64_t next) 
    : m_next(next){
}

bool Timer::cancel() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    // 是否有回调函数
    if (m_cb) {
        m_cb = nullptr;         // 置空
        auto it = m_manager->m_timers.find(shared_from_this());     // 查找定时器
        m_manager->m_timers.erase(it);      // 删除
        return true;
    }
    return false;
}

// 重新计时
bool Timer::refresh() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if (!m_cb) {
        return false;
    }
    auto it = m_manager->m_timers.find(shared_from_this());
    if (it == m_manager->m_timers.end()) {
        return false;
    }
    // 移除定时器
    m_manager->m_timers.erase(it);
    // 当前时间 + 预设时间
    m_next = sylar::GetCurrentMS() + m_ms;
    // 插入定时器
    m_manager->m_timers.insert(shared_from_this());
    return true;
}

// 重设定时器(预设时间，是否从现在开始)
bool Timer::reset(uint64_t ms, bool from_now) {
    // 预设时间相同且不重新设定开始时间
    if (ms == m_ms && !from_now) {
        return true;
    }
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if (!m_cb) {
        return false;
    }
    // 找
    auto it = m_manager->m_timers.find(shared_from_this());
    if (it == m_manager->m_timers.end()) {
        return false;
    }
    // 删除
    m_manager->m_timers.erase(it);
    uint64_t start = 0;
    if (from_now) {
        // 取现在的时间
        start = sylar::GetCurrentMS();
    } else {
        // 取得曾经的开始时间
        start = m_next - m_ms;
    }
    m_ms = ms;
    m_next = start + m_ms;
    m_manager->addTimer(shared_from_this(), lock);
    return true;
}

TimerManager::TimerManager() {
    m_previousTime = sylar::GetCurrentMS();
}

TimerManager::~TimerManager() {}

// 添加定时器
Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb
                        , bool recurring) {
    Timer::ptr timer(new Timer(ms, cb, recurring, this));
    RWMutexType::WriteLock lock(m_mutex);
    addTimer(timer, lock);
    return timer;
}

static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
    std::shared_ptr<void> tmp = weak_cond.lock();
    if (tmp) {
        cb();
    }
}

Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb
                            , std::weak_ptr<void> weak_cond
                            , bool recurring) {
    return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
}

// 查询下一个定时器还有多久触发
uint64_t TimerManager::getNextTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    m_tickled = false;
    // 定时器队列为空
    if (m_timers.empty()) {
        return ~0ull;
    }
    // 取得下一个定时器
    const Timer::ptr& next = *m_timers.begin();
    // 获取当前时间
    uint64_t now_ms = sylar::GetCurrentMS();
    // 现在时间是否大于等于定时器时间
    if (now_ms >= next->m_next) {
        return 0;
    } else {    // 小于则返回还需要多长时间
        return next->m_next - now_ms;
    }
}

void TimerManager::listExpiredCb(std::vector<std::function<void()>>& cbs) {
    // 获取当前时间
    uint64_t now_ms = sylar::GetCurrentMS();
    std::vector<Timer::ptr> expired;
    {
        // 查看队列是否为空
        RWMutexType::ReadLock lock(m_mutex);
        if (m_timers.empty()) {
            return ;
        }
    }
    RWMutexType::WriteLock lock(m_mutex);
    // 此处为个人修改
    if (m_timers.empty()) {
        return ;
    }
    bool rollover = detectClockRollover(now_ms);
    // 如果没有调系统时间且还没到下一个定时器的执行时间
    if (!rollover && ((*m_timers.begin())->m_next > now_ms)) {
        return ;
    }

    Timer::ptr now_timer(new Timer(now_ms));
    // 全部执行或者只执行到点的定时器
    auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
    // 压点的定时器可能有多个
    while (it != m_timers.end() && (*it)->m_next == now_ms) {
        it++;
    }
    // 取出超时的定时器
    expired.insert(expired.begin(), m_timers.begin(), it);
    // 擦除队列上对应的定时器
    m_timers.erase(m_timers.begin(), it);
    // 对vector进行内存分配
    cbs.reserve(expired.size());
    // 将超时定时器的回调函数取出
    for (auto& timer : expired) {
        cbs.push_back(timer->m_cb);
        // 如果该定时器为循环定时器， 则重新插入该定时器
        if (timer->m_recurring) {
            timer->m_next = now_ms + timer->m_ms;
            m_timers.insert(timer);
        } else {    // 否则将回调函数置空
            timer->m_cb = nullptr;
        }
    }
}

// 添加定时器
void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock& lock) {
    // 插入定时器
    auto it = m_timers.insert(val).first;
    // 查看定时器是否被插在开头且是否已通知
    bool at_front = (it == m_timers.begin()) && !m_tickled;
    if (at_front) {
        m_tickled = true;
    }
    lock.unlock();
    // 通知
    if (at_front) {
        onTimerInsertedAtFront();
    }
}

// 检测服务器是否调过本系统的时间
bool TimerManager::detectClockRollover(uint64_t now_ms) {
    bool rollover = false;
    // 如果当前时间小于上次执行时间且超过一小时
    if (now_ms < m_previousTime && 
        now_ms < (m_previousTime - 60 * 60 * 1000)) {
        rollover = true;
    }
    // 上次执行时间设为现在时间
    m_previousTime = now_ms;
    return rollover;
}

// 查询队列是否有定时器
bool TimerManager::hasTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    return !m_timers.empty();
}


}

