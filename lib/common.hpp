#ifndef COMMON_HPP
#define COMMON_HPP

////////////////////////////////////////////////////////////////////////////////
/// STRING UTILS                                                             ///
////////////////////////////////////////////////////////////////////////////////

#include <string.h> //strncpy, strlen, strtok
#include <stdlib.h> //malloc

static inline char * dup(const char * src, int len) {
    char * ret = (char *) malloc(len + 1);
    strncpy(ret, src, len);
    ret[len] = '\0';
    return ret;
}

static inline char * dup(const char * src, const char * end) {
    return dup(src, end - src);
}

static inline char * dup(const char * src) {
    if (!src) return nullptr;
    return dup(src, strlen(src));
}

static inline bool is_empty(const char * str) {
    return !str || str[0] == '\0';
}

////////////////////////////////////////////////////////////////////////////////
/// MISC                                                                     ///
////////////////////////////////////////////////////////////////////////////////

#include "list.hpp"

static inline List<char *> split_lines_in_place(char * mutableSrc) {
    List<char *> lines = {};
    char * line = strtok(mutableSrc, "\r\n");
    while (line) {
        lines.add(line);
        line = strtok(nullptr, "\r\n");
    }
    return lines;
}

#include <ctype.h> //tolower

static inline int case_insensitive_ascii_compare(const char * a, const char * b) {
    for (int i = 0; a[i] || b[i]; ++i) {
        if (tolower(a[i]) != tolower(b[i])) {
            return tolower(a[i]) - tolower(b[i]);
        }
    }
    return 0;
}

#include <stdarg.h> //va_list etc.
#include <stdio.h> //vsnprintf

//allocates a buffer large enough to fit resulting string, and `sprintf`s to it
__attribute__((format(printf, 2, 3)))
static inline char * dsprintf(char * buf, const char * fmt, ...) {
    size_t len = buf? strlen(buf) : 0;
    va_list args1, args2;
    va_start(args1, fmt);
    va_copy(args2, args1);
    buf = (char *) realloc(buf, len + vsnprintf(nullptr, 0, fmt, args1) + 1);
    vsprintf(buf + len, fmt, args2);
    va_end(args1);
    va_end(args2);
    return buf;
}

// #include <math.h>

// static inline float dbToVolume(float db) { return powf(1.0f, 0.05f * db); }
// static inline float volumeToDb(float volume) { return 20.0f * log10f(volume); }

#define ARR_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#endif
