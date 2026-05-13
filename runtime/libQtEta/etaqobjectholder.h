#ifndef Eta_QOBJECT_HOLDER_H
#define Eta_QOBJECT_HOLDER_H

#include <cassert>
#include <QObject>
#include <QPointer>

#include <etaobj.h>
#include <gcpin.h>

// Since Qt methods will return QObjects and not the wrappers,
// we keep a table that lets us do reverse lookup.
Etaobj* etaqGetWrapper(QObject* object);
void   etaqPutWrapper(QObject* object, Etaobj* wrapper);
void   etaqForgetWrapper(QObject* object);

// This is a helper to manage pinning/unpinning/deletion of QObjects
// wrapped by Eta objects. The basic invariant is that QObjects
// with parents are managed by C++ and are pinned; those without
// parents are managed by the Eta GC.
//
// Because of pinning, this should be included by value in the
// wrapper object
template<typename T> class EtaQObjectHolder
{
private:
    QPointer<T> handle; // the use of QPointer here isn't needed for
                        // operation, but is rather a precaution
                        // to make crashes more predictable in case of
                        // mistakes

    GCPin pin;
public:
    EtaQObjectHolder(): pin(this)
    {}

    T* get() {
        return handle.data();
    }

    bool isNull() const {
        return handle.isNull();
    }

    void setObject(T* object, Etaobj* wrapper) {
        assert (handle.isNull());
        handle = object;
        if (object->parent())
            pin.pin();
        etaqPutWrapper(object, wrapper);
    }

    void parentChange() {
        if (handle->parent())
            pin.pin();
        else
            pin.unpin();
    }

    ~EtaQObjectHolder() {
        // if we are not pinned, we own the object, so we should delete it.
        // (and if we're pinned, we should not be here!)
        assert (!pin.isPinned());

        etaqForgetWrapper(handle.data());
        delete handle.data();
    }
};

#endif
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
