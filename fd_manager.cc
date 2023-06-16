#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "fd_manager.h"
#include "hook.h"

namespace sylar {

FdCtx::FdCtx(int fd) 
    : m_isInit(false), m_isSocket(false)
    , m_sysNonblock(false), m_userNonblock(false), m_isClosed(false)
    , m_fd(fd), m_recvTimeout(-1), m_sendTimeout(-1) {
    init();
}

FdCtx::~FdCtx() {
}

bool FdCtx::init() {
    if (m_isInit) {
        return true;
    }
    m_recvTimeout = -1;
    m_sendTimeout = -1;

    struct stat fd_stat;
    // 将文件状态复制到stat中
    if (-1 == fstat(m_fd, &fd_stat)) {
        m_isInit = false;
        m_isSocket = false;
    } else {
        m_isInit = true;
        m_isSocket = S_ISSOCK(fd_stat.st_mode); // 判断是否为socket
    }

    if (m_isSocket) {   // 如果是socket
        int flags = fcntl_f(m_fd, F_GETFL, 0);  // 取得文件描述符的状态标志
        if (!(flags & O_NONBLOCK)) {    // 如果为阻塞
            fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK); //设为非阻塞
        }
        m_sysNonblock = true;
    } else {
        m_sysNonblock = false;
    }

    m_userNonblock = false;
    m_isClosed = false;
    return m_isInit;
}

void FdCtx::setTimeout(int type, uint64_t v) {
    if (type == SO_RCVTIMEO) {
        m_recvTimeout = v;
    } else {
        m_sendTimeout = v;
    }
}

uint64_t FdCtx::getTimeout(int type) {
    if (type == SO_RCVTIMEO) {
        return m_recvTimeout;
    } else {
        return m_sendTimeout;
    }
}

FdManager::FdManager() {
    m_datas.resize(64);
}

FdCtx::ptr FdManager::get(int fd, bool auto_create) {
    RWMutexType::ReadLock lock(m_mutex);
    // 超过队列大小
    if ((int)m_datas.size() <= fd) {
        if (auto_create == false) { //如果不需要自动创建则直接返回
            return nullptr;
        }
    } else {    // 如果有数据且不需要自动创建
        if (m_datas[fd] || !auto_create) {
            return m_datas[fd];
        }
    }
    lock.unlock();
    // 创建并放入
    RWMutexType::WriteLock lock2(m_mutex);
    FdCtx::ptr ctx(new FdCtx(fd));
    if(fd >= (int)m_datas.size()) {
        m_datas.resize(fd * 1.5);
    }
    m_datas[fd] = ctx;
    return ctx;
    
}

void FdManager::del(int fd) {
    RWMutexType::WriteLock lock(m_mutex);
    if ((int)m_datas.size() <= fd) {
        return ;
    }
    m_datas[fd].reset();
}

}