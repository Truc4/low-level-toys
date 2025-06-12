@echo off
setlocal

gcc -g -O0 src/main.c -o main.exe -lgdi32 -luser32

if %errorlevel% neq 0 (
    echo Build failed.
    exit /b %errorlevel%
)

main.exe
