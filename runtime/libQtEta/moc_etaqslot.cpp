/****************************************************************************
** Meta object code from reading C++ file 'etaqslot.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "etaqslot.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'etaqslot.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_EtaQPushButtonSigs_t {
    QByteArrayData data[3];
    char stringdata0[32];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EtaQPushButtonSigs_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EtaQPushButtonSigs_t qt_meta_stringdata_EtaQPushButtonSigs = {
    {
QT_MOC_LITERAL(0, 0, 18), // "EtaQPushButtonSigs"
QT_MOC_LITERAL(1, 19, 11), // "slotClicked"
QT_MOC_LITERAL(2, 31, 0) // ""

    },
    "EtaQPushButtonSigs\0slotClicked\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EtaQPushButtonSigs[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   19,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool,    2,

       0        // eod
};

void EtaQPushButtonSigs::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<EtaQPushButtonSigs *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->slotClicked((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject EtaQPushButtonSigs::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_EtaQPushButtonSigs.data,
    qt_meta_data_EtaQPushButtonSigs,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *EtaQPushButtonSigs::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EtaQPushButtonSigs::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_EtaQPushButtonSigs.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int EtaQPushButtonSigs::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 1;
    }
    return _id;
}
struct qt_meta_stringdata_EtaQTimerSigs_t {
    QByteArrayData data[3];
    char stringdata0[27];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EtaQTimerSigs_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EtaQTimerSigs_t qt_meta_stringdata_EtaQTimerSigs = {
    {
QT_MOC_LITERAL(0, 0, 13), // "EtaQTimerSigs"
QT_MOC_LITERAL(1, 14, 11), // "slotTimeout"
QT_MOC_LITERAL(2, 26, 0) // ""

    },
    "EtaQTimerSigs\0slotTimeout\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EtaQTimerSigs[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   19,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void EtaQTimerSigs::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<EtaQTimerSigs *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->slotTimeout(); break;
        default: ;
        }
    }
    (void)_a;
}

QT_INIT_METAOBJECT const QMetaObject EtaQTimerSigs::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_EtaQTimerSigs.data,
    qt_meta_data_EtaQTimerSigs,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *EtaQTimerSigs::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EtaQTimerSigs::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_EtaQTimerSigs.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int EtaQTimerSigs::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 1;
    }
    return _id;
}
struct qt_meta_stringdata_EtaQApplicationSigs_t {
    QByteArrayData data[3];
    char stringdata0[37];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EtaQApplicationSigs_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EtaQApplicationSigs_t qt_meta_stringdata_EtaQApplicationSigs = {
    {
QT_MOC_LITERAL(0, 0, 19), // "EtaQApplicationSigs"
QT_MOC_LITERAL(1, 20, 15), // "slotAboutToQuit"
QT_MOC_LITERAL(2, 36, 0) // ""

    },
    "EtaQApplicationSigs\0slotAboutToQuit\0"
    ""
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EtaQApplicationSigs[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   19,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void EtaQApplicationSigs::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<EtaQApplicationSigs *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->slotAboutToQuit(); break;
        default: ;
        }
    }
    (void)_a;
}

QT_INIT_METAOBJECT const QMetaObject EtaQApplicationSigs::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_EtaQApplicationSigs.data,
    qt_meta_data_EtaQApplicationSigs,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *EtaQApplicationSigs::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EtaQApplicationSigs::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_EtaQApplicationSigs.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int EtaQApplicationSigs::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 1;
    }
    return _id;
}
struct qt_meta_stringdata_EtaQActionSigs_t {
    QByteArrayData data[3];
    char stringdata0[30];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EtaQActionSigs_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EtaQActionSigs_t qt_meta_stringdata_EtaQActionSigs = {
    {
QT_MOC_LITERAL(0, 0, 14), // "EtaQActionSigs"
QT_MOC_LITERAL(1, 15, 13), // "slotTriggered"
QT_MOC_LITERAL(2, 29, 0) // ""

    },
    "EtaQActionSigs\0slotTriggered\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EtaQActionSigs[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   19,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void EtaQActionSigs::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<EtaQActionSigs *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->slotTriggered(); break;
        default: ;
        }
    }
    (void)_a;
}

QT_INIT_METAOBJECT const QMetaObject EtaQActionSigs::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_EtaQActionSigs.data,
    qt_meta_data_EtaQActionSigs,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *EtaQActionSigs::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EtaQActionSigs::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_EtaQActionSigs.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int EtaQActionSigs::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 1;
    }
    return _id;
}
struct qt_meta_stringdata_EtaQCheckBoxSigs_t {
    QByteArrayData data[3];
    char stringdata0[30];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EtaQCheckBoxSigs_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EtaQCheckBoxSigs_t qt_meta_stringdata_EtaQCheckBoxSigs = {
    {
QT_MOC_LITERAL(0, 0, 16), // "EtaQCheckBoxSigs"
QT_MOC_LITERAL(1, 17, 11), // "slotToggled"
QT_MOC_LITERAL(2, 29, 0) // ""

    },
    "EtaQCheckBoxSigs\0slotToggled\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EtaQCheckBoxSigs[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   19,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool,    2,

       0        // eod
};

void EtaQCheckBoxSigs::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<EtaQCheckBoxSigs *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->slotToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject EtaQCheckBoxSigs::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_EtaQCheckBoxSigs.data,
    qt_meta_data_EtaQCheckBoxSigs,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *EtaQCheckBoxSigs::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EtaQCheckBoxSigs::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_EtaQCheckBoxSigs.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int EtaQCheckBoxSigs::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 1;
    }
    return _id;
}
struct qt_meta_stringdata_EtaQRadioButtonSigs_t {
    QByteArrayData data[3];
    char stringdata0[33];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EtaQRadioButtonSigs_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EtaQRadioButtonSigs_t qt_meta_stringdata_EtaQRadioButtonSigs = {
    {
QT_MOC_LITERAL(0, 0, 19), // "EtaQRadioButtonSigs"
QT_MOC_LITERAL(1, 20, 11), // "slotToggled"
QT_MOC_LITERAL(2, 32, 0) // ""

    },
    "EtaQRadioButtonSigs\0slotToggled\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EtaQRadioButtonSigs[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   19,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool,    2,

       0        // eod
};

void EtaQRadioButtonSigs::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<EtaQRadioButtonSigs *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->slotToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject EtaQRadioButtonSigs::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_EtaQRadioButtonSigs.data,
    qt_meta_data_EtaQRadioButtonSigs,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *EtaQRadioButtonSigs::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EtaQRadioButtonSigs::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_EtaQRadioButtonSigs.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int EtaQRadioButtonSigs::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 1;
    }
    return _id;
}
struct qt_meta_stringdata_EtaQAbstractSliderSigs_t {
    QByteArrayData data[3];
    char stringdata0[40];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EtaQAbstractSliderSigs_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EtaQAbstractSliderSigs_t qt_meta_stringdata_EtaQAbstractSliderSigs = {
    {
QT_MOC_LITERAL(0, 0, 22), // "EtaQAbstractSliderSigs"
QT_MOC_LITERAL(1, 23, 15), // "slotSliderMoved"
QT_MOC_LITERAL(2, 39, 0) // ""

    },
    "EtaQAbstractSliderSigs\0slotSliderMoved\0"
    ""
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EtaQAbstractSliderSigs[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   19,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::Int,    2,

       0        // eod
};

void EtaQAbstractSliderSigs::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<EtaQAbstractSliderSigs *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->slotSliderMoved((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject EtaQAbstractSliderSigs::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_EtaQAbstractSliderSigs.data,
    qt_meta_data_EtaQAbstractSliderSigs,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *EtaQAbstractSliderSigs::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EtaQAbstractSliderSigs::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_EtaQAbstractSliderSigs.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int EtaQAbstractSliderSigs::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 1;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
