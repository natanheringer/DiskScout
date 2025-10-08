@echo off
echo Clearing DiskScout cache...
if exist "%APPDATA%\DiskScout" (
    rmdir /s /q "%APPDATA%\DiskScout"
    echo Cache cleared successfully!
) else (
    echo No cache found.
)
echo Done.
pause
