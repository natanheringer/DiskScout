QT += core widgets

TARGET = DiskScoutGUI
TEMPLATE = app

# C++ source files
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    scanner_wrapper.cpp \
    models/filesystemmodel.cpp \
    widgets/sunburstwidget.cpp \
    widgets/treemapwidget.cpp

# Header files
HEADERS += \
    mainwindow.h \
    scanner_wrapper.h \
    models/filesystemmodel.h \
    models/sortproxymodel.h \
    widgets/sunburstwidget.h \
    widgets/treemapwidget.h

# C backend source files
SOURCES += \
    ../src/scanner.c \
    ../src/cache.c \
    backend_interface.c

# Assembly object file
OBJECTS += ../disk_assembler.o

# Include directories
INCLUDEPATH += ../src

# Link libraries
LIBS += -lpthread

# Windows specific
win32 {
    LIBS += -luser32 -lkernel32
}
