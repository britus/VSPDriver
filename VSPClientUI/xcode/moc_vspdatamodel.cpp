/****************************************************************************
** Meta object code from reading C++ file 'vspdatamodel.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../model/vspdatamodel.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'vspdatamodel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_VSPDataModel_t {
    QByteArrayData data[4];
    char stringdata0[41];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_VSPDataModel_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_VSPDataModel_t qt_meta_stringdata_VSPDataModel = {
    {
QT_MOC_LITERAL(0, 0, 12), // "VSPDataModel"
QT_MOC_LITERAL(1, 13, 9), // "TDataType"
QT_MOC_LITERAL(2, 23, 8), // "PortItem"
QT_MOC_LITERAL(3, 32, 8) // "PortLink"

    },
    "VSPDataModel\0TDataType\0PortItem\0"
    "PortLink"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_VSPDataModel[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       1,   14, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // enums: name, alias, flags, count, data
       1,    1, 0x0,    2,   19,

 // enum data: key, value
       2, uint(VSPDataModel::PortItem),
       3, uint(VSPDataModel::PortLink),

       0        // eod
};

void VSPDataModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject VSPDataModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QAbstractTableModel::staticMetaObject>(),
    qt_meta_stringdata_VSPDataModel.data,
    qt_meta_data_VSPDataModel,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *VSPDataModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *VSPDataModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_VSPDataModel.stringdata0))
        return static_cast<void*>(this);
    return QAbstractTableModel::qt_metacast(_clname);
}

int VSPDataModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QAbstractTableModel::qt_metacall(_c, _id, _a);
    return _id;
}
struct qt_meta_stringdata_VSPPortListModel_t {
    QByteArrayData data[1];
    char stringdata0[17];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_VSPPortListModel_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_VSPPortListModel_t qt_meta_stringdata_VSPPortListModel = {
    {
QT_MOC_LITERAL(0, 0, 16) // "VSPPortListModel"

    },
    "VSPPortListModel"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_VSPPortListModel[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void VSPPortListModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject VSPPortListModel::staticMetaObject = { {
    QMetaObject::SuperData::link<VSPDataModel::staticMetaObject>(),
    qt_meta_stringdata_VSPPortListModel.data,
    qt_meta_data_VSPPortListModel,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *VSPPortListModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *VSPPortListModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_VSPPortListModel.stringdata0))
        return static_cast<void*>(this);
    return VSPDataModel::qt_metacast(_clname);
}

int VSPPortListModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = VSPDataModel::qt_metacall(_c, _id, _a);
    return _id;
}
struct qt_meta_stringdata_VSPLinkListModel_t {
    QByteArrayData data[1];
    char stringdata0[17];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_VSPLinkListModel_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_VSPLinkListModel_t qt_meta_stringdata_VSPLinkListModel = {
    {
QT_MOC_LITERAL(0, 0, 16) // "VSPLinkListModel"

    },
    "VSPLinkListModel"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_VSPLinkListModel[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void VSPLinkListModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject VSPLinkListModel::staticMetaObject = { {
    QMetaObject::SuperData::link<VSPDataModel::staticMetaObject>(),
    qt_meta_stringdata_VSPLinkListModel.data,
    qt_meta_data_VSPLinkListModel,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *VSPLinkListModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *VSPLinkListModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_VSPLinkListModel.stringdata0))
        return static_cast<void*>(this);
    return VSPDataModel::qt_metacast(_clname);
}

int VSPLinkListModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = VSPDataModel::qt_metacall(_c, _id, _a);
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
