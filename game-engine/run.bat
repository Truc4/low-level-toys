@echo off
setlocal

gcc -g -O0 -Wall -Wextra src/main.c src/win32_layer.c -o main.exe -lgdi32 -luser32

if %errorlevel% neq 0 (
    echo Build failed.
    exit /b %errorlevel%
)

main.exe

