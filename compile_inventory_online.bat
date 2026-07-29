@echo off
setlocal
cd /d "%~dp0"

set "COMPILER=%~dp0qawno\pawncc.exe"
set "INCLUDE=%~dp0qawno\include"
set "SRC=%~dp0gamemodes\inventory_test_online.pwn"
set "OUT=%~dp0gamemodes\inventory_test_online.amx"

if not exist "%COMPILER%" (
    echo [ERROR] Compiler not found: %COMPILER%
    pause
    exit /b 1
)

echo Compiling inventory_test_online.pwn ...
"%COMPILER%" "%SRC%" -i"%INCLUDE%" -o"%OUT%"
set "ERR=%ERRORLEVEL%"

echo.
if %ERR% neq 0 (
    echo [FAILED] Compile error code %ERR%
) else (
    echo [OK] Built: gamemodes\inventory_test_online.amx
)

pause
exit /b %ERR%
