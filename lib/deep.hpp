#ifndef DEEP_HPP
#define DEEP_HPP

//in a better language we'd use metaprogramming for this
//(and some of it, like operator== for POD structs, would be provided by the language)
//but this is C++ so we have to write the boilerplate by hand
//this header exists to facilitate writing that boilerplate
//no I will not use RAII for this, it'd be just as much boilerplate and it'd be worse

#include "list.hpp"
#include "common.hpp" //for is_empty()

template <typename TYPE>
static inline bool deep_equals(TYPE a, TYPE b) {
    return a == b;
}

//NOTE: this must be char*, NOT const char*, in order to be selected and that skeeves me the fuck out, I hate C++
static inline bool deep_equals(char * a, char * b) {
    return (is_empty(a) && is_empty(b)) || !strcmp(a, b);
}

template <typename TYPE>
static inline bool deep_equals(List<TYPE> a, List<TYPE> b) {
    if (a.len != b.len) return false;
    for (int i = 0; i < a.len; ++i) if (!deep_equals(a[i], b[i])) return false;
    return true;
}

template <typename TYPE>
static inline TYPE deep_clone(TYPE t) {
    return t;
}

//NOTE: this must be char*, NOT const char*, in order to be selected and that skeeves me the fuck out, I hate C++
static inline char * deep_clone(char * s) {
    return dup(s);
}

template <typename TYPE>
static inline List<TYPE> deep_clone(List<TYPE> l) {
    l = l.clone();
    for (TYPE & t : l) t = deep_clone(t);
    return l;
}

template <typename TYPE>
static inline void deep_finalize(TYPE & t) {}

template <typename TYPE>
static inline void deep_finalize(TYPE * t) {
    free(t); //TODO: very suspicious...
}

template <typename TYPE>
static inline void deep_finalize(List<TYPE> & l) {
    for (TYPE & t : l) deep_finalize(t);
    l.finalize();
}

//helper macros
#define DEEP_EQUALS(x) if (!deep_equals(a.x, b.x)) return false;
#define DEEP_CLONE(x) out.x = deep_clone(in.x);

#endif//DEEP_HPP
