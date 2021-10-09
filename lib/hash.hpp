#ifndef DUPE_HASH_HPP
#define DUPE_HASH_HPP

#include "alloc.hpp"
#include "list.hpp"

static inline uint hash(uint key) {
    //from https://burtleburtle.net/bob/hash/integer.html
    key += ~(key << 15);
    key ^=  (key >> 10);
    key +=  (key <<  3);
    key ^=  (key >>  6);
    key += ~(key << 11);
    key ^=  (key >> 16);
    return key;
}

//a trivial one-to-one hash table (map) with a fixed entry count using linear probing
template <typename KEY, typename VAL, VAL sentinel>
struct HashMap {
    List<Pair<KEY, VAL>> data;
    uint modMask;

    void init(int entries) {
        //round up to a power of 2
        uint log = 32 - __builtin_clz(entries * 2 + 1);
        uint tableSize = 1 << log;
        modMask = tableSize - 1;

        data.init(tableSize);
        data.len = data.max;
        for (Pair<KEY, VAL> & pair : data) pair.second = sentinel;
    }

    void finalize() { data.finalize(); }

    void add(KEY key, VAL val) {
        uint idx = hash(key) & modMask;
        while (data[idx].second != sentinel && data[idx].first != key) {
            idx = (idx + 1) & modMask;
        }
        data[idx] = { key, val };
    }

    VAL & get(KEY key) {
        uint idx = hash(key) & modMask;
        while (data[idx].second != sentinel && data[idx].first != key) {
            idx = (idx + 1) & modMask;
        }
        return data[idx].second;
    }
};

template <typename KEY, typename VAL>
struct MultiMap {
    List<Pair<KEY, List<VAL>>> data;
    uint modMask;

    void init(uint entries) {
        //round up to a power of 2
        uint log = 32 - __builtin_clz(entries * 2 + 1);
        uint tableSize = 1 << log;
        modMask = tableSize - 1;

        data.init(tableSize);
        data.len = data.max;
        // memset(data.data, 0, data.len * sizeof(*data.data));
        for (Pair<KEY, List<VAL>> & pair : data) pair.second = {};
    }

    void finalize() {
        for (Pair<KEY, List<VAL>> pair : data) pair.second.finalize();
        data.finalize();
    }

    void add(KEY key, VAL val) {
        uint idx = hash(key) & modMask;
        while (data[idx].second.len && data[idx].first != key) {
            idx = (idx + 1) & modMask;
        }
        data[idx].first = key;
        data[idx].second.add(val);
    }

    void add(KEY key, VAL val, ArenaListAlloc & alloc) {
        uint idx = hash(key) & modMask;
        while (data[idx].second.len && data[idx].first != key) {
            idx = (idx + 1) & modMask;
        }
        data[idx].first = key;
        data[idx].second.add(val, alloc);
    }

    List<VAL> & get(KEY key) {
        uint idx = hash(key) & modMask;
        while (data[idx].second.len && data[idx].first != key) {
            idx = (idx + 1) & modMask;
        }
        return data[idx].second;
    }
};

#endif // DUPE_HASH_HPP
