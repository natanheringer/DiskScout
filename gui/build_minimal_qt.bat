@echo off
echo Building DiskScout GUI with Minimal Qt Setup...

REM Check for MSYS2 Qt
set QT_DIR=C:\msys64\mingw64
if exist "%QT_DIR%\bin\qmake.exe" (
    echo Found MSYS2 Qt at %QT_DIR%
    set PATH=%QT_DIR%\bin;%PATH%
    goto :build
)

REM Check for custom Qt build
set QT_DIR=C:\qt5
if exist "%QT_DIR%\qtbase\bin\qmake.exe" (
    echo Found custom Qt at %QT_DIR%
    set PATH=%QT_DIR%\qtbase\bin;%PATH%
    goto :build
)

REM Try to find Qt in common locations
for %%i in (C:\Qt\5.12.12\mingw_64 C:\Qt\5.15.2\mingw_64 C:\Qt\6.5.0\mingw_64) do (
    if exist "%%i\bin\qmake.exe" (
        echo Found Qt at %%i
        set PATH=%%i\bin;%PATH%
        goto :build
    )
)

echo Qt not found! Please install Qt or build from source.
echo.
echo Options:
echo 1. Download Qt from https://www.qt.io/download-open-source
echo 2. Continue building from source in C:\qt5
echo 3. Use simple Windows GUI instead
echo.
pause
exit /b 1

:build
echo Using Qt at: %QT_DIR%

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

REM Create minimal Qt project file
echo Creating minimal Qt project...
(
echo QT += core widgets
echo CONFIG += c++17
echo TARGET = diskscout_gui
echo TEMPLATE = app
echo.
echo SOURCES += ^
echo     ../main.cpp ^
echo     ../mainwindow.cpp ^
echo     ../scanner_wrapper.cpp
echo.
echo HEADERS += ^
echo     ../mainwindow.h ^
echo     ../scanner_wrapper.h
echo.
echo INCLUDEPATH += ../../
echo INCLUDEPATH += ../../src
echo.
echo LIBS += -L../../ -ldiskscout
echo.
echo QMAKE_CXXFLAGS += -O3 -Wall
echo.
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
