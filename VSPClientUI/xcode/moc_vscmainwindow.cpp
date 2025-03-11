/****************************************************************************
** Meta object code from reading C++ file 'vscmainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../ui/vscmainwindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'vscmainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_VSCMainWindow_t {
    QByteArrayData data[34];
    char stringdata0[506];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_VSCMainWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_VSCMainWindow_t qt_meta_stringdata_VSCMainWindow = {
    {
QT_MOC_LITERAL(0, 0, 13), // "VSCMainWindow"
QT_MOC_LITERAL(1, 14, 15), // "onActionExecute"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 29), // "VSPClient::TVSPControlCommand"
QT_MOC_LITERAL(4, 61, 7), // "command"
QT_MOC_LITERAL(5, 69, 4), // "data"
QT_MOC_LITERAL(6, 74, 21), // "updateOverlayGeometry"
QT_MOC_LITERAL(7, 96, 15), // "onCommitSession"
QT_MOC_LITERAL(8, 112, 16), // "QSessionManager&"
QT_MOC_LITERAL(9, 129, 13), // "onSaveSession"
QT_MOC_LITERAL(10, 143, 9), // "onAppQuit"
QT_MOC_LITERAL(11, 153, 17), // "onClientConnected"
QT_MOC_LITERAL(12, 171, 20), // "onClientDisconnected"
QT_MOC_LITERAL(13, 192, 13), // "onClientError"
QT_MOC_LITERAL(14, 206, 26), // "VSPClient::TVSPSystemError"
QT_MOC_LITERAL(15, 233, 5), // "error"
QT_MOC_LITERAL(16, 239, 7), // "message"
QT_MOC_LITERAL(17, 247, 17), // "onUpdateStatusLog"
QT_MOC_LITERAL(18, 265, 15), // "onUpdateButtons"
QT_MOC_LITERAL(19, 281, 7), // "enabled"
QT_MOC_LITERAL(20, 289, 15), // "onCommandResult"
QT_MOC_LITERAL(21, 305, 17), // "VSPPortListModel*"
QT_MOC_LITERAL(22, 323, 9), // "portModel"
QT_MOC_LITERAL(23, 333, 17), // "VSPLinkListModel*"
QT_MOC_LITERAL(24, 351, 9), // "linkModel"
QT_MOC_LITERAL(25, 361, 10), // "onComplete"
QT_MOC_LITERAL(26, 372, 20), // "onSetupFailWithError"
QT_MOC_LITERAL(27, 393, 4), // "code"
QT_MOC_LITERAL(28, 398, 11), // "const char*"
QT_MOC_LITERAL(29, 410, 23), // "onSetupFinishWithResult"
QT_MOC_LITERAL(30, 434, 24), // "onSetupNeedsUserApproval"
QT_MOC_LITERAL(31, 459, 12), // "onSelectPage"
QT_MOC_LITERAL(32, 472, 15), // "onActionInstall"
QT_MOC_LITERAL(33, 488, 17) // "onActionUninstall"

    },
    "VSCMainWindow\0onActionExecute\0\0"
    "VSPClient::TVSPControlCommand\0command\0"
    "data\0updateOverlayGeometry\0onCommitSession\0"
    "QSessionManager&\0onSaveSession\0onAppQuit\0"
    "onClientConnected\0onClientDisconnected\0"
    "onClientError\0VSPClient::TVSPSystemError\0"
    "error\0message\0onUpdateStatusLog\0"
    "onUpdateButtons\0enabled\0onCommandResult\0"
    "VSPPortListModel*\0portModel\0"
    "VSPLinkListModel*\0linkModel\0onComplete\0"
    "onSetupFailWithError\0code\0const char*\0"
    "onSetupFinishWithResult\0"
    "onSetupNeedsUserApproval\0onSelectPage\0"
    "onActionInstall\0onActionUninstall"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_VSCMainWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      19,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    2,  109,    2, 0x0a /* Public */,
       6,    0,  114,    2, 0x09 /* Protected */,
       7,    1,  115,    2, 0x08 /* Private */,
       9,    1,  118,    2, 0x08 /* Private */,
      10,    0,  121,    2, 0x08 /* Private */,
      11,    0,  122,    2, 0x08 /* Private */,
      12,    0,  123,    2, 0x08 /* Private */,
      13,    2,  124,    2, 0x08 /* Private */,
      17,    1,  129,    2, 0x08 /* Private */,
      18,    1,  132,    2, 0x08 /* Private */,
      18,    0,  135,    2, 0x28 /* Private | MethodCloned */,
      20,    3,  136,    2, 0x08 /* Private */,
      25,    0,  143,    2, 0x08 /* Private */,
      26,    2,  144,    2, 0x08 /* Private */,
      29,    2,  149,    2, 0x08 /* Private */,
      30,    0,  154,    2, 0x08 /* Private */,
      31,    0,  155,    2, 0x08 /* Private */,
      32,    0,  156,    2, 0x08 /* Private */,
      33,    0,  157,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3, QMetaType::QVariant,    4,    5,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 8,    2,
    QMetaType::Void, 0x80000000 | 8,    2,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 14, QMetaType::QString,   15,   16,
    QMetaType::Void, QMetaType::QByteArray,   16,
    QMetaType::Void, QMetaType::Bool,   19,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 21, 0x80000000 | 23,    4,   22,   24,
    QMetaType::Void,
    QMetaType::Void, QMetaType::ULongLong, 0x80000000 | 28,   27,   16,
    QMetaType::Void, QMetaType::ULongLong, 0x80000000 | 28,   27,   16,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void VSCMainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<VSCMainWindow *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->onActionExecute((*reinterpret_cast< const VSPClient::TVSPControlCommand(*)>(_a[1])),(*reinterpret_cast< const QVariant(*)>(_a[2]))); break;
        case 1: _t->updateOverlayGeometry(); break;
        case 2: _t->onCommitSession((*reinterpret_cast< QSessionManager(*)>(_a[1]))); break;
        case 3: _t->onSaveSession((*reinterpret_cast< QSessionManager(*)>(_a[1]))); break;
        case 4: _t->onAppQuit(); break;
        case 5: _t->onClientConnected(); break;
        case 6: _t->onClientDisconnected(); break;
        case 7: _t->onClientError((*reinterpret_cast< const VSPClient::TVSPSystemError(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 8: _t->onUpdateStatusLog((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 9: _t->onUpdateButtons((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 10: _t->onUpdateButtons(); break;
        case 11: _t->onCommandResult((*reinterpret_cast< VSPClient::TVSPControlCommand(*)>(_a[1])),(*reinterpret_cast< VSPPortListModel*(*)>(_a[2])),(*reinterpret_cast< VSPLinkListModel*(*)>(_a[3]))); break;
        case 12: _t->onComplete(); break;
        case 13: _t->onSetupFailWithError((*reinterpret_cast< quint64(*)>(_a[1])),(*reinterpret_cast< const char*(*)>(_a[2]))); break;
        case 14: _t->onSetupFinishWithResult((*reinterpret_cast< quint64(*)>(_a[1])),(*reinterpret_cast< const char*(*)>(_a[2]))); break;
        case 15: _t->onSetupNeedsUserApproval(); break;
        case 16: _t->onSelectPage(); break;
        case 17: _t->onActionInstall(); break;
        case 18: _t->onActionUninstall(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 11:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 2:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< VSPLinkListModel* >(); break;
            case 1:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< VSPPortListModel* >(); break;
            }
            break;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject VSCMainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_VSCMainWindow.data,
    qt_meta_data_VSCMainWindow,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *VSCMainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *VSCMainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_VSCMainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int VSCMainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 19)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 19;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 19)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 19;
    }
    return _id;
}
struct qt_meta_stringdata_PopupMenu_t {
    QByteArrayData data[1];
    char stringdata0[10];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_PopupMenu_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_PopupMenu_t qt_meta_stringdata_PopupMenu = {
    {
QT_MOC_LITERAL(0, 0, 9) // "PopupMenu"

    },
    "PopupMenu"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_PopupMenu[] = {

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

void PopupMenu::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject PopupMenu::staticMetaObject = { {
    QMetaObject::SuperData::link<QMenu::staticMetaObject>(),
    qt_meta_stringdata_PopupMenu.data,
    qt_meta_data_PopupMenu,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *PopupMenu::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PopupMenu::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_PopupMenu.stringdata0))
        return static_cast<void*>(this);
    return QMenu::qt_metacast(_clname);
}

int PopupMenu::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMenu::qt_metacall(_c, _id, _a);
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
