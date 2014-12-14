#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include <stdexcept>

namespace dirty {

typedef void* fptr_t;
typedef fptr_t* vtable_t;
typedef vtable_t* anchor_t;

template <typename T>
vtable_t obj_get_vtable(T* obj) {
    return *reinterpret_cast<anchor_t>(obj);
}

vtable_t vtable_copy(vtable_t orig, size_t len) {
    vtable_t copy = static_cast<vtable_t>(malloc(len));
    memcpy(copy, orig, len * sizeof(fptr_t));
    return copy;
}

template <typename T>
vtable_t obj_replace_vtable(T* obj, vtable_t vtable) {
    vtable_t old = obj_get_vtable(obj);

    anchor_t anchor = reinterpret_cast<anchor_t>(obj);
    *anchor = vtable;

    return old;
}

template <typename F>
uintptr_t vtable_calc_idx(F ptr) {
    void* offset = *reinterpret_cast<void**>(&ptr);
    return reinterpret_cast<uintptr_t>(offset) / sizeof(void*);
}

fptr_t vtable_replace_entry(vtable_t vtable, uintptr_t idx, fptr_t fkt) {
    fptr_t old = vtable[idx];
    vtable[idx] = fkt;

    return old;
}

template <typename T, typename F>
fptr_t vtable_patch(F ptr, fptr_t fkt, T* obj) {
    vtable_t vtable = obj_get_vtable(obj);

    int pagesize = getpagesize();
    void* page = reinterpret_cast<void*>(
        reinterpret_cast<long>(vtable) & ~(pagesize - 1)
    );

    // make the page with the vtable writable
    if (mprotect(page, static_cast<size_t>(pagesize), PROT_WRITE | PROT_READ | PROT_EXEC)) {
        throw std::runtime_error("Unable to use mprotect!");
    }

    return vtable_replace_entry(vtable, vtable_calc_idx(ptr), fkt);
}

template <typename T, typename F>
fptr_t vtable_patch(F ptr, fptr_t fkt) {
    // create instance to get vtable later
    T obj;

    return vtable_patch<T, F>(ptr, fkt, &obj);
}

template <typename T, typename M>
M& obj_member(T* t, void* ptr) {
    return reinterpret_cast<M&>(reinterpret_cast<void**>(t)[reinterpret_cast<uintptr_t>(ptr) / sizeof(void*)]);
}


namespace member {

template<typename Tag, int n>
struct result {
    /* export it ... */
    typedef typename Tag::type type;
    static type ptr;
};

template<typename Tag, int n>
typename result<Tag, n>::type result<Tag, n>::ptr;

template<typename Tag, typename Tag::type p, int n>
struct rob : result<Tag, n> {
    /* fill it ... */
    struct filler {
    filler() { result<Tag, n>::ptr = p; }
    };
    static filler filler_obj;
};

template<typename Tag, typename Tag::type p, int n>
typename rob<Tag, p, n>::filler rob<Tag, p, n>::filler_obj;

template <typename M>
struct Af { typedef M type; };

template <typename M, int n, typename T>
auto& get(T* t) {
    return (*t).*result<Af<M>, n>::ptr;
}

}
}

