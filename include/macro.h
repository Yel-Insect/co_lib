#ifndef __MACRO_H__
#define __MACRO_H__

#include <string.h>
#include <assert.h>
#include "util.h"

#if defined __GNUC__ || defined __llvm__
#   define CO_LIB_LICKLY(x)      __builtin_expect(!!(x), 1)
#   define CO_LIB_UNLICKLY(x)    __builtin_expect(!!(x), 0)
#else
#   define CO_LIB_LICKY(x)       (x)
#   define CO_LIB_UNLICKLY(x)    (x)
#endif

#define CO_LIB_ASSERT(x) \
    if (CO_LIB_UNLICKLY(!(x))) {     \
        LOG_ERROR(LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << co_lib::BacktraceToString(100, 2, "   "); \
        assert(x);  \
    }

#define CO_LIB_ASSERT2(x, w) \
    if (CO_LIB_UNLICKLY(!(x))) { \
        LOG_ERROR(LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << co_lib::BacktraceToString(100, 2, "   "); \
        assert(x); \
    }

#endif