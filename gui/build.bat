@echo off
echo Building DiskScout GUI...

REM Check if Qt is available
where qmake >nul 2>nul
if %errorlevel% neq 0 (
    echo Qt not found in PATH. Please install Qt or add it to PATH.
    echo You can download Qt from: https://www.qt.io/download
    pause
    exit /b 1
)

REM Create build directory
if not exist build mkdir build
cd build

REM Generate Makefile
echo Generating Makefile...
qmake ../CMakeLists.txt
if %errorlevel% neq 0 (
    echo Failed to generate Makefile
    pause
    exit /b 1
)

REM Build the project
echo Building project...
make -j4
if %errorlevel% neq 0 (
    echo Build failed
    pause
    exit /b 1
)

echo Build successful!
echo Executable: build/diskscout_gui.exe
pause
