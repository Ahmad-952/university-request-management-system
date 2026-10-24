@echo off
echo Building University Request Management System...
echo.

REM Check if Visual Studio is available
where cl.exe >nul 2>&1
if %errorlevel% neq 0 (
    echo Setting up MSVC environment...
    call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
)

echo Compiling server with SQLite...
cl /nologo /EHsc /W3 /std:c++17 /Fe:RequestServer.exe server.cpp sqlite3.c /link ws2_32.lib shell32.lib

if %errorlevel% equ 0 (
    echo.
    echo ========================================
    echo Build successful!
    echo ========================================
    echo.
    echo Run: RequestServer.exe
    echo URL: http://localhost:8080
    echo.
) else (
    echo.
    echo ========================================
    echo Build failed!
    echo ========================================
    echo.
    pause
    exit /b 1
)

pause
