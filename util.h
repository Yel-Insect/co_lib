#ifndef __UTIL_H__
#define __UTIL_H__

#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>

namespace sylar {
pid_t GetThreadId();

uint32_t GetFiberId();
}

#endif