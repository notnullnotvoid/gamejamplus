#ifndef TYPES_HPP
#define TYPES_HPP

//these exist so we can easily switch non-essential console messages on/off
// #if CONFIG_RELEASE
// #define print_error(...)
// #define print_log(...)
// #else
#define print_error printf
// #define print_log(fmt, ...) printf("[%s] " fmt, __func__,##__VA_ARGS__)
#define print_log printf
// #endif

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef unsigned int uint;

template <typename T1, typename T2>
struct Pair {
    T1 first;
    T2 second;
};

#endif //TYPES_HPP
