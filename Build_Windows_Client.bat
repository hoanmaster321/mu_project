@echo off
@chcp 65001 >nul
title AUTO BUILD MU CLIENT WINDOWS (CMAKE)
set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
if exist "%SCRIPT_DIR%\Source\5.Main\build_windows_cmake.bat" (
    call "%SCRIPT_DIR%\Source\5.Main\build_windows_cmake.bat" %*
) else (
    echo [LOI] Khong tim thay Source\5.Main\build_windows_cmake.bat!
    pause
    exit /b 1
)
