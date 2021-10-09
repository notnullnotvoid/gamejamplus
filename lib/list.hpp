#ifndef LIST_HPP
#define LIST_HPP

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

//a simple array list implementation
//NOTE: this struct zero-initializes to a valid state!
//      List<T> list = {}; //this is valid
template <typename TYPE>
struct List {
    TYPE * data;
    uint32_t len;
    uint32_t max;

    inline void init(uint32_t reserve = 1024 / sizeof(TYPE) + 1) {
        assert(reserve > 0);
        data = (TYPE *) malloc(reserve * sizeof(TYPE));
        max = reserve;
        len = 0;
    }

    //TODO: create unsafe_add() method (that doesn't do the resize check) and use it where relevant

    inline void add(TYPE t) {
        if (len == max) {
            max = max * 2 + 1;
            data = (TYPE *) realloc(data, max * sizeof(TYPE));
        }

        data[len] = t;
        ++len;
    }

    template <typename ALLOC>
    inline void add(TYPE t, ALLOC & alloc) {
        if (len == max) {
            max = max * 2 + 1;
            data = (TYPE *) alloc.realloc(data, len * sizeof(TYPE), max * sizeof(TYPE), alignof(TYPE));
        }

        data[len] = t;
        ++len;
    }

    inline void add(TYPE * t, int num) {
        if (len + num > max) {
            while (len + num > max) {
                max = max * 2 + 1;
            }
            data = (TYPE *) realloc(data, max * sizeof(TYPE));
        }
        memcpy(&data[len], t, num * sizeof(TYPE));
        len += num;
    }

    inline void remove(uint32_t index) {
        assert(index < len);
        --len;
        data[index] = data[len];
    }

    //ordered insertion at index
    inline void insert(uint32_t index, TYPE t) {
        assert(index <= len);
        if (index > len) index = len;
        add({}); // will realloc if needed
        for (uint32_t i = len - 1; i >= index + 1; --i) {
            data[i] = data[i - 1];
        }
        data[index] = t;
    }

    //removes elements in range [first, last)
    inline void remove(uint32_t first, uint32_t lastPlusOne) {
        assert(first < lastPlusOne);
        assert(lastPlusOne <= len);
        uint32_t range = lastPlusOne - first;
        len -= range;
        for (uint32_t i = first; i < len; ++i) {
            data[i] = data[i + range];
        }
    }

    inline TYPE pop() {
        assert(len > 0);
        return data[--len];
    }

    inline void shrink_to_fit() {
        data = (TYPE *) realloc(data, len * sizeof(TYPE));
    }

    inline List<TYPE> clone() {
        List<TYPE> ret = { (TYPE *) malloc(len * sizeof(TYPE)), len, len };
        memcpy(ret.data, data, len * sizeof(TYPE));
        return ret;
    }

    inline void finalize() {
        free(data);
        *this = {};
    }

    inline TYPE & operator[](uint32_t index) {
    	assert(index < len);
        return data[index];
    }

    //no need to define an iterator class, because range-for will work with raw pointers
    inline TYPE * begin() { return { data }; }
    inline TYPE * end() { return { data + len }; }
};

//convenience function for same-line declaration+initialization
template<typename TYPE>
static inline List<TYPE> create_list(uint32_t reserve = 1024 / sizeof(TYPE) + 1) {
    List<TYPE> list;
    list.init(reserve);
    return list;
}

#endif
