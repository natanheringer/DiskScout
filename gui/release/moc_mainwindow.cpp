/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.17)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../mainwindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.17. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MainWindow_t {
    QByteArrayData data[18];
    char stringdata0[245];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MainWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
QT_MOC_LITERAL(0, 0, 10), // "MainWindow"
QT_MOC_LITERAL(1, 11, 13), // "onScanClicked"
QT_MOC_LITERAL(2, 25, 0), // ""
QT_MOC_LITERAL(3, 26, 15), // "onBrowseClicked"
QT_MOC_LITERAL(4, 42, 13), // "onPathChanged"
QT_MOC_LITERAL(5, 56, 17), // "onViewModeChanged"
QT_MOC_LITERAL(6, 74, 15), // "onScanCompleted"
QT_MOC_LITERAL(7, 90, 14), // "onScanProgress"
QT_MOC_LITERAL(8, 105, 8), // "progress"
QT_MOC_LITERAL(9, 114, 11), // "onScanError"
QT_MOC_LITERAL(10, 126, 5), // "error"
QT_MOC_LITERAL(11, 132, 22), // "onTreeSelectionChanged"
QT_MOC_LITERAL(12, 155, 22), // "onContextMenuRequested"
QT_MOC_LITERAL(13, 178, 3), // "pos"
QT_MOC_LITERAL(14, 182, 16), // "onOpenInExplorer"
QT_MOC_LITERAL(15, 199, 12), // "onDeleteFile"
QT_MOC_LITERAL(16, 212, 16), // "onShowProperties"
QT_MOC_LITERAL(17, 229, 15) // "onShowHelpGuide"

    },
    "MainWindow\0onScanClicked\0\0onBrowseClicked\0"
    "onPathChanged\0onViewModeChanged\0"
    "onScanCompleted\0onScanProgress\0progress\0"
    "onScanError\0error\0onTreeSelectionChanged\0"
    "onContextMenuRequested\0pos\0onOpenInExplorer\0"
    "onDeleteFile\0onShowProperties\0"
    "onShowHelpGuide"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      13,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   79,    2, 0x08 /* Private */,
       3,    0,   80,    2, 0x08 /* Private */,
       4,    0,   81,    2, 0x08 /* Private */,
       5,    0,   82,    2, 0x08 /* Private */,
       6,    0,   83,    2, 0x08 /* Private */,
       7,    1,   84,    2, 0x08 /* Private */,
       9,    1,   87,    2, 0x08 /* Private */,
      11,    0,   90,    2, 0x08 /* Private */,
      12,    1,   91,    2, 0x08 /* Private */,
      14,    0,   94,    2, 0x08 /* Private */,
      15,    0,   95,    2, 0x08 /* Private */,
      16,    0,   96,    2, 0x08 /* Private */,
      17,    0,   97,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    8,
    QMetaType::Void, QMetaType::QString,   10,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QPoint,   13,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onScanClicked(); break;
        case 1: _t->onBrowseClicked(); break;
        case 2: _t->onPathChanged(); break;
        case 3: _t->onViewModeChanged(); break;
        case 4: _t->onScanCompleted(); break;
        case 5: _t->onScanProgress((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->onScanError((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->onTreeSelectionChanged(); break;
        case 8: _t->onContextMenuRequested((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 9: _t->onOpenInExplorer(); break;
        case 10: _t->onDeleteFile(); break;
        case 11: _t->onShowProperties(); break;
        case 12: _t->onShowHelpGuide(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_MainWindow.data,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 13;
    }
    return _id;
}
struct qt_meta_stringdata_PercentageBarDelegate_t {
    QByteArrayData data[1];
    char stringdata0[22];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_PercentageBarDelegate_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_PercentageBarDelegate_t qt_meta_stringdata_PercentageBarDelegate = {
    {
QT_MOC_LITERAL(0, 0, 21) // "PercentageBarDelegate"

    },
    "PercentageBarDelegate"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_PercentageBarDelegate[] = {

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

void PercentageBarDelegate::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

QT_INIT_METAOBJECT const QMetaObject PercentageBarDelegate::staticMetaObject = { {
    QMetaObject::SuperData::link<QStyledItemDelegate::staticMetaObject>(),
    qt_meta_stringdata_PercentageBarDelegate.data,
    qt_meta_data_PercentageBarDelegate,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *PercentageBarDelegate::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PercentageBarDelegate::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_PercentageBarDelegate.stringdata0))
        return static_cast<void*>(this);
    return QStyledItemDelegate::qt_metacast(_clname);
}

int PercentageBarDelegate::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QStyledItemDelegate::qt_metacall(_c, _id, _a);
    return _id;
}
struct qt_meta_stringdata_ScanThread_t {
    QByteArrayData data[7];
    char stringdata0[67];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ScanThread_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ScanThread_t qt_meta_stringdata_ScanThread = {
    {
QT_MOC_LITERAL(0, 0, 10), // "ScanThread"
QT_MOC_LITERAL(1, 11, 13), // "scanCompleted"
QT_MOC_LITERAL(2, 25, 0), // ""
QT_MOC_LITERAL(3, 26, 9), // "scanError"
QT_MOC_LITERAL(4, 36, 5), // "error"
QT_MOC_LITERAL(5, 42, 15), // "progressUpdated"
QT_MOC_LITERAL(6, 58, 8) // "progress"

    },
    "ScanThread\0scanCompleted\0\0scanError\0"
    "error\0progressUpdated\0progress"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ScanThread[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   29,    2, 0x06 /* Public */,
       3,    1,   30,    2, 0x06 /* Public */,
       5,    1,   33,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::Int,    6,

       0        // eod
};

void ScanThread::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ScanThread *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->scanCompleted(); break;
        case 1: _t->scanError((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->progressUpdated((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ScanThread::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScanThread::scanCompleted)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ScanThread::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScanThread::scanError)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ScanThread::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScanThread::progressUpdated)) {
                *result = 2;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ScanThread::staticMetaObject = { {
    QMetaObject::SuperData::link<QThread::staticMetaObject>(),
    qt_meta_stringdata_ScanThread.data,
    qt_meta_data_ScanThread,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ScanThread::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ScanThread::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ScanThread.stringdata0))
        return static_cast<void*>(this);
    return QThread::qt_metacast(_clname);
}

int ScanThread::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void ScanThread::scanCompleted()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ScanThread::scanError(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void ScanThread::progressUpdated(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
