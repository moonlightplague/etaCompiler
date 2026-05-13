#ifndef Eta_OBJ_H
#define Eta_OBJ_H

#include <cstring>
#include <new>
#include <type_traits>
#include <stdint.h>

#include "libeta.h"

struct Etavtable {
};

struct Etaobj {
    Etaobj(): initialized(true)
    {}

    Etavtable* vptr;

    // This field is set to false when we are merely allocated
    // via GC, such as when an Eta subclass allocates our memory;
    // and true when we've actually called a constructor.
    bool initialized;
};

template<typename T> T* gcNew() {
    T* t = reinterpret_cast<T*>(_eta_alloc(sizeof(T)));
    new (t) T;
    registerFinalizer(t, T::invokeDtor);
    return t;
}

// Like gcNew, but doesn't deal with constructors or destructors
template<typename T> T* gcCalloc() {
    return reinterpret_cast<T*>(_eta_alloc(sizeof(T)));
}

inline int toEta(int v) {
    return v;
}

inline bool toEta(bool v) {
    return v;
}

inline int fromEta(int v) {
    return v;
}

inline bool fromEta(bool v) {
    return v;
}

inline void* fromEta(void* v) {
    return v;
}

enum ClassOp {
    CallCtor, // also responsible for calling createImpl
    CallDtor
};

struct DummyPeer
{};

template<typename T> T* copy_vtable(const T* vtable) {
    static_assert(std::is_trivial<T>::value, "vtable type must be trivial");
    T* const copy = gcCalloc<T>();
    std::memcpy(copy, vtable, sizeof(T));
    return copy;
}

void downcastError [[noreturn]] ();
void* lookupEtaFunction(etastring name, const char* typeEncoding);

#endif

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on; hl c++;
