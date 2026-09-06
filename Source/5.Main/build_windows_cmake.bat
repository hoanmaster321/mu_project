@echo off
@chcp 65001 >nul
title BUILD MU CLIENT WINDOWS (CMAKE)
color 0A
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "BUILD_DIR=%SCRIPT_DIR%build-windows"
set "CONFIG=Release"

echo ================================================================================
echo               BUILD MU CLIENT (WINDOWS DESKTOP) BANG CMAKE
echo ================================================================================
echo  Thu muc source : %SCRIPT_DIR%
echo  Thu muc build  : %BUILD_DIR%
echo  Cau hinh       : %CONFIG% (Win32)
echo ================================================================================
echo.

if not exist "%BUILD_DIR%" (
    echo [*] Dang tao va cau hinh project CMake...
    cmake -B "%BUILD_DIR%" -S "%SCRIPT_DIR%" -A Win32
    if errorlevel 1 (
        color 0C
        echo [X] Cau hinh CMake THAT BAI!
        pause
        exit /b 1
    )
)

echo [*] Dang tien hanh bien dich Client...
cmake --build "%BUILD_DIR%" --config %CONFIG% -j %NUMBER_OF_PROCESSORS%

if errorlevel 1 (
    color 0C
    echo [X] Bien dich Client THAT BAI!
    pause
    exit /b 1
)

echo.
echo ================================================================================
echo [V] BIEN DICH THANH CONG!
echo Output: G:\mu_project\ClientBuild_192.168.99.200\Main.exe
echo ================================================================================
pause
exit /b 0