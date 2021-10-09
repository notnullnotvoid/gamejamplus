#include "level.hpp"
#include "platform.hpp"
#include "trace.hpp"
#include "deep.hpp"
#include <algorithm>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SERIALIZATION                                                                                                    ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//for explanation, see https://yave.handmade.network/blog/p/2723-how_media_molecule_does_serialization
//we basically do exactly what that articles says, but we use C++ templates to make the code shorter

enum Version {
    SV_INITIAL,

    // ^^^^^ add versions here ^^^^^

    SV_LATEST_PLUS_ONE,
    SV_LATEST = SV_LATEST_PLUS_ONE - 1,
};

#define SV_ADD(_version, _field) do { if (ver >= _version) { go(&x->_field); } } while (0)
#define SV_REMOVE(added, removed, T) do { if (ver >= added && ver < removed) { T t; go(&t); } } while (0)

struct Serializer {
    //universal state
    int32_t ver;
    bool writing;

    //reader state
    char * head;
    char * end;

    //writer state
    List<char> writer;

    inline void go(void * mem, size_t bytes) {
        if (writing) {
            writer.add((char *) mem, bytes);
        } else {
            assert(head + bytes <= end);
            memcpy(mem, head, bytes);
            head += bytes;
        }
    }

    template <typename TYPE>
    inline void go(TYPE * x) {
        go(x, sizeof(x));
    }

    inline void go(char ** x) {
        int32_t len = writing && *x? strlen(*x) : 0;
        go(&len);
        if (!writing) *x = (char *) malloc(len + 1);
        go(*x, len);
        if (!writing) (*x)[len] = '\0';
    }

    template<typename TYPE> void go(List<TYPE> * x) {
        if (!writing) assert(!x->data && !x->len && !x->max);
        int32_t count = x->len; // shall be 0 in reading-mode
        go(&count);
        for (int32_t i = 0; i < count; ++i) {
            if (!writing) x->add({}); //PERF: pre-allocate the array instead of adding individually
            go(&(*x)[i]);
        }
    }

    void start_read(void * start, size_t bytes) {
        writing = false;
        head = (char *) start;
        end = head + bytes;
        go(&ver);
        assert(ver >= 0 && ver <= SV_LATEST);
    }

    void start_write() {
        writing = true;
        writer.init(1024 * 1024 - 1);
        ver = SV_LATEST;
        go(&ver);
    }

    void AddTile(int id, int x, int y)
    {
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DEEP BOILERPLATE                                                                                                 ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
