@echo off
title Start GameServerCS Only - 192.168.1.117
color 0B
setlocal

set "MUSERVER=%~dp0"
if "%MUSERVER:~-1%"=="\" set "MUSERVER=%MUSERVER:~0,-1%"

echo ==================================================
echo Starting GameServerCS only on 192.168.1.117
echo Requirement: ConnectServer, DataServer, JoinServer already running
echo ==================================================
echo.

if not exist "%MUSERVER%\4.GameServer\GameServer\GameServerCS.exe" (
	echo [ERROR] Missing %MUSERVER%\4.GameServer\GameServer\GameServerCS.exe
	pause
	exit /b 1
)

if not exist "%MUSERVER%\4.GameServer\GameServer\Data\GameServerInfo - Common.ini" (
	echo [ERROR] Missing %MUSERVER%\4.GameServer\GameServer\Data\GameServerInfo - Common.ini
	pause
	exit /b 1
)

echo [INFO] Core services must already be online:
echo        ConnectServer.exe
echo        DataServer.exe
echo        JoinServer.exe
echo.
echo [START] GameServerCS
start "GameServerCS" /D "%MUSERVER%\4.GameServer\GameServer" "%MUSERVER%\4.GameServer\GameServer\GameServerCS.exe"

echo.
echo Done.
pause
endlocal
