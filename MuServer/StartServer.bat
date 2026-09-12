@echo off
title Start MuServer - 192.168.1.117
color 0A

echo =====================================
echo        STARTING MU SERVER
echo        IP: 192.168.1.117
echo =====================================
echo.

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

echo [1/5] Starting ConnectServer (Port 44405)...
start "ConnectServer" /D "%ROOT%\1.ConnectServer" "%ROOT%\1.ConnectServer\ConnectServer.exe"
timeout /t 3 /nobreak >nul

echo [2/5] Starting DataServer (Port 55960)...
start "DataServer" /D "%ROOT%\2.DataServer" "%ROOT%\2.DataServer\DataServer.exe"
timeout /t 3 /nobreak >nul

echo [3/5] Starting JoinServer (Port 55970)...
start "JoinServer" /D "%ROOT%\3.JoinServer" "%ROOT%\3.JoinServer\JoinServer.exe"
timeout /t 3 /nobreak >nul

echo [4/5] Starting AntiHack XShield...
start "XShield" /D "%ROOT%\5.Antihack" "%ROOT%\5.Antihack\XShield.exe"
timeout /t 3 /nobreak >nul

echo [5/5] Starting GameServer Sub 1 (Port 55800)...
start "GameServer" /D "%ROOT%\4.GameServer\Sub 1\GameServer" "%ROOT%\4.GameServer\Sub 1\GameServer\GameServer.exe"

echo.
echo =====================================
echo Done. Check each server window for errors.
echo Neu loi DB thi kiem tra ODBC MuOnline 32-bit va SQL dang chay.
echo =====================================
pause
