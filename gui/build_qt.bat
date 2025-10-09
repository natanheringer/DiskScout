@echo off
echo Building DiskScout GUI with Qt...

REM Check for Qt installation
set QT_DIR=C:\Qt\6.5.0\mingw_64
if not exist "%QT_DIR%" (
    echo Qt not found at %QT_DIR%
    echo Please install Qt 6.5 with MinGW support
    echo Download from: https://www.qt.io/download-open-source
    pause
    exit /b 1
)

REM Add Qt to PATH
set PATH=%QT_DIR%\bin;%PATH%

REM Check if qmake is available
qmake --version
if %errorlevel% neq 0 (
    echo qmake not found. Please check Qt installation.
    pause
    exit /b 1
)

REM Create build directory
if not exist build mkdir build
cd build

REM Create Qt project file
echo Creating Qt project file...
(
echo # DiskScout GUI Project
echo QT += core widgets
echo CONFIG += c++17
echo TARGET = diskscout_gui
echo TEMPLATE = app
echo.
echo # Source files
echo SOURCES += ^
echo     ../main.cpp ^
echo     ../mainwindow.cpp ^
echo     ../scanner_wrapper.cpp ^
echo     models/filesystemmodel.cpp ^
echo     widgets/sunburstwidget.cpp ^
echo     widgets/treemapwidget.cpp
echo.
echo # Header files
echo HEADERS += ^
echo     ../mainwindow.h ^
echo     ../scanner_wrapper.h ^
echo     models/filesystemmodel.h ^
echo     widgets/sunburstwidget.h ^
echo     widgets/treemapwidget.h
echo.
echo # Include directories
echo INCLUDEPATH += ../../
echo INCLUDEPATH += ../../src
echo.
echo # Link with C backend
echo LIBS += -L../../ -ldiskscout
echo.
echo # Compiler flags
echo QMAKE_CXXFLAGS += -O3 -Wall
echo.
echo # Windows specific
echo win32 {
echo     LIBS += -lpthread
echo     DEFINES += WIN32_LEAN_AND_MEAN
echo }
) > diskscout_gui.pro

REM Generate Makefile
echo Generating Makefile...
qmake diskscout_gui.pro
if %errorlevel% neq 0 (
    echo Failed to generate Makefile
    pause
    exit /b 1
)

REM Build the project
echo Building project...
mingw32-make -j4
if %errorlevel% neq 0 (
    echo Build failed
    pause
    exit /b 1
)

echo.
echo Build successful!
echo Executable: build/diskscout_gui.exe
echo.
echo To run: cd build && diskscout_gui.exe
pause
