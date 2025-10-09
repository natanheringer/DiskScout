@echo off
echo Testing C Backend Integration...

REM Compile test
g++ -I../src -o test_backend.exe test_backend.cpp ../src/cache.c ../src/scanner.c ../disk_assembler.o -lpthread

if %errorlevel% neq 0 (
    echo Compilation failed
    pause
    exit /b 1
)

echo Compilation successful!
echo Running test...

REM Run test
test_backend.exe

echo.
echo Test completed!
pause
