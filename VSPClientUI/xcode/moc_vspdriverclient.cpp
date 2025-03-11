/****************************************************************************
** Meta object code from reading C++ file 'vspdriverclient.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../driver/vspdriverclient.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'vspdriverclient.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_VSPDriverClient_t {
    QByteArrayData data[24];
    char stringdata0[321];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_VSPDriverClient_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_VSPDriverClient_t qt_meta_stringdata_VSPDriverClient = {
    {
QT_MOC_LITERAL(0, 0, 15), // "VSPDriverClient"
QT_MOC_LITERAL(1, 16, 16), // "didFailWithError"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 4), // "code"
QT_MOC_LITERAL(4, 39, 11), // "const char*"
QT_MOC_LITERAL(5, 51, 7), // "message"
QT_MOC_LITERAL(6, 59, 19), // "didFinishWithResult"
QT_MOC_LITERAL(7, 79, 17), // "needsUserApproval"
QT_MOC_LITERAL(8, 97, 9), // "connected"
QT_MOC_LITERAL(9, 107, 12), // "disconnected"
QT_MOC_LITERAL(10, 120, 13), // "commandResult"
QT_MOC_LITERAL(11, 134, 29), // "VSPClient::TVSPControlCommand"
QT_MOC_LITERAL(12, 164, 7), // "command"
QT_MOC_LITERAL(13, 172, 17), // "VSPPortListModel*"
QT_MOC_LITERAL(14, 190, 9), // "portModel"
QT_MOC_LITERAL(15, 200, 17), // "VSPLinkListModel*"
QT_MOC_LITERAL(16, 218, 9), // "linkModel"
QT_MOC_LITERAL(17, 228, 12), // "errorOccured"
QT_MOC_LITERAL(18, 241, 26), // "VSPClient::TVSPSystemError"
QT_MOC_LITERAL(19, 268, 5), // "error"
QT_MOC_LITERAL(20, 274, 15), // "updateStatusLog"
QT_MOC_LITERAL(21, 290, 13), // "updateButtons"
QT_MOC_LITERAL(22, 304, 7), // "enabled"
QT_MOC_LITERAL(23, 312, 8) // "complete"

    },
    "VSPDriverClient\0didFailWithError\0\0"
    "code\0const char*\0message\0didFinishWithResult\0"
    "needsUserApproval\0connected\0disconnected\0"
    "commandResult\0VSPClient::TVSPControlCommand\0"
    "command\0VSPPortListModel*\0portModel\0"
    "VSPLinkListModel*\0linkModel\0errorOccured\0"
    "VSPClient::TVSPSystemError\0error\0"
    "updateStatusLog\0updateButtons\0enabled\0"
    "complete"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_VSPDriverClient[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      11,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   69,    2, 0x06 /* Public */,
       6,    2,   74,    2, 0x06 /* Public */,
       7,    0,   79,    2, 0x06 /* Public */,
       8,    0,   80,    2, 0x06 /* Public */,
       9,    0,   81,    2, 0x06 /* Public */,
      10,    3,   82,    2, 0x06 /* Public */,
      17,    2,   89,    2, 0x06 /* Public */,
      20,    1,   94,    2, 0x06 /* Public */,
      21,    1,   97,    2, 0x06 /* Public */,
      21,    0,  100,    2, 0x26 /* Public | MethodCloned */,
      23,    0,  101,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::ULongLong, 0x80000000 | 4,    3,    5,
    QMetaType::Void, QMetaType::ULongLong, 0x80000000 | 4,    3,    5,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 11, 0x80000000 | 13, 0x80000000 | 15,   12,   14,   16,
    QMetaType::Void, 0x80000000 | 18, QMetaType::QString,   19,    5,
    QMetaType::Void, QMetaType::QByteArray,    5,
    QMetaType::Void, QMetaType::Bool,   22,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void VSPDriverClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<VSPDriverClient *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->didFailWithError((*reinterpret_cast< quint64(*)>(_a[1])),(*reinterpret_cast< const char*(*)>(_a[2]))); break;
        case 1: _t->didFinishWithResult((*reinterpret_cast< quint64(*)>(_a[1])),(*reinterpret_cast< const char*(*)>(_a[2]))); break;
        case 2: _t->needsUserApproval(); break;
        case 3: _t->connected(); break;
        case 4: _t->disconnected(); break;
        case 5: _t->commandResult((*reinterpret_cast< VSPClient::TVSPControlCommand(*)>(_a[1])),(*reinterpret_cast< VSPPortListModel*(*)>(_a[2])),(*reinterpret_cast< VSPLinkListModel*(*)>(_a[3]))); break;
        case 6: _t->errorOccured((*reinterpret_cast< const VSPClient::TVSPSystemError(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 7: _t->updateStatusLog((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 8: _t->updateButtons((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 9: _t->updateButtons(); break;
        case 10: _t->complete(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 5:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 2:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< VSPLinkListModel* >(); break;
            case 1:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< VSPPortListModel* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (VSPDriverClient::*)(quint64 , const char * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VSPDriverClient::didFailWithError)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (VSPDriverClient::*)(quint64 , const char * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VSPDriverClient::didFinishWithResult)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (VSPDriverClient::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VSPDriverClient::needsUserApproval)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (VSPDriverClient::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VSPDriverClient::connected)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (VSPDriverClient::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VSPDriverClient::disconnected)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (VSPDriverClient::*)(VSPClient::TVSPControlCommand , VSPPortListModel * , VSPLinkListModel * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VSPDriverClient::commandResult)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (VSPDriverClient::*)(const VSPClient::TVSPSystemError & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VSPDriverClient::errorOccured)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (VSPDriverClient::*)(const QByteArray & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VSPDriverClient::updateStatusLog)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (VSPDriverClient::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VSPDriverClient::updateButtons)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (VSPDriverClient::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VSPDriverClient::complete)) {
                *result = 10;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject VSPDriverClient::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_VSPDriverClient.data,
    qt_meta_data_VSPDriverClient,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *VSPDriverClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *VSPDriverClient::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_VSPDriverClient.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "VSPController"))
        return static_cast< VSPController*>(this);
    if (!strcmp(_clname, "VSPDriverSetup"))
        return static_cast< VSPDriverSetup*>(this);
    return QObject::qt_metacast(_clname);
}

int VSPDriverClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void VSPDriverClient::didFailWithError(quint64 _t1, const char * _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void VSPDriverClient::didFinishWithResult(quint64 _t1, const char * _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void VSPDriverClient::needsUserApproval()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void VSPDriverClient::connected()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void VSPDriverClient::disconnected()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void VSPDriverClient::commandResult(VSPClient::TVSPControlCommand _t1, VSPPortListModel * _t2, VSPLinkListModel * _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void VSPDriverClient::errorOccured(const VSPClient::TVSPSystemError & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void VSPDriverClient::updateStatusLog(const QByteArray & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void VSPDriverClient::updateButtons(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 10
void VSPDriverClient::complete()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
