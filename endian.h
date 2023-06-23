#ifndef __ENDIAN_H__
#define __ENDIAN_H__

#include <byteswap.h>
#include <type_traits>
#include <stdint.h>

#define SYLAR_LITTER_ENDIAN 1   // 小端
#define SYLAR_BIG_ENDIAN 2      // 大端

namespace sylar {
// 大端到小端 或 小端到大端
// 64位
template<class T>   // enable_if是满足条件时类型是有效的
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
byteswap(T value) {
    return (T)bswap_64((uint64_t)value);
}

// 32位
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
byteswap(T value) {
    return (T)bswap_32((uint32_t)value);
}

// 16位
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
byteswap(T value) {
    return (T)bswap_16((uint16_t)value);
}

#if BYTE_ORDER == BIG_ENDIAN
#define SYLAR_BYTE_ORDER SYLAR_BIG_ENDIAN
#else
#define SYLAR_BYTE_ORDER SYLAR_LITTLE_ENDIAN
#endif

// 大端
#if SYLAR_BYTE_ORDER == SYLAR_BIG_ENDIAN
template<class T>
T byteswapOnLittleEndian(T t) {
    return t;
}

template<class T>
T byteswapOnBigEndian(T t) {
    return byteswap(t);
}

#else
// 小端
template<class T>
T byteswapOnLittleEndian(T t) {
    return byteswap(t);
}

template<class T>
T byteswapOnBigEndian(T t) {
    return t;
}
#endif
}

#endif