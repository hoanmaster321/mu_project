@echo off
@chcp 65001 >nul
title MU SERVER BUILD TOOL - CS DS JS GS
color 0A
setlocal enabledelayedexpansion

:: ============================================================================
:: XAC DINH THU MUC GOC (ROOT DIR)
:: ============================================================================
set "SCRIPT_DIR=%~dp0"
if exist "%SCRIPT_DIR%Source_Server-Mobi" (
    set "ROOT_DIR=%SCRIPT_DIR%"
) else if exist "%SCRIPT_DIR%1.ConnectServer" (
    pushd "%SCRIPT_DIR%.."
    set "ROOT_DIR=!CD!\"
    popd
) else (
    set "ROOT_DIR=G:\mu_project\"
)

if exist "%ROOT_DIR%Source\" (
    set "SRC_DIR=%ROOT_DIR%Source\"
) else (
    set "SRC_DIR=%ROOT_DIR%Source_Server-Mobi\"
)
set "MUSERVER_DIR=%ROOT_DIR%MuServer\"
set "BUILDLOG_DIR=%ROOT_DIR%BuildLog\"

:: ============================================================================
:: TIM KIEM MSBUILD.EXE (VS2022 / VS2019 / VSWHERE)
:: ============================================================================
set "MSBUILD_EXE="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
        if exist "%%i" (
            set "MSBUILD_EXE=%%i"
            goto :FOUND_MSBUILD
        )
    )
)

:: Thu tim o cac thu muc mac dinh
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" (
    set "MSBUILD_EXE=C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
    goto :FOUND_MSBUILD
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe" (
    set "MSBUILD_EXE=C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
    goto :FOUND_MSBUILD
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe" (
    set "MSBUILD_EXE=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
    goto :FOUND_MSBUILD
)
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" (
    set "MSBUILD_EXE=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
    goto :FOUND_MSBUILD
)

:FOUND_MSBUILD
if not defined MSBUILD_EXE (
    color 0C
    echo [LOI] Khong tim thay MSBuild.exe tren he thong!
    echo Vui long kiem tra Visual Studio hoac Build Tools da duoc cai dat.
    pause
    exit /b 1
)

:: Tham so chung cho MSBuild (Build x86/Win32 EX603 bang toolset v143)
set "BUILD_OPTS=/p:Platform=Win32 /p:PlatformToolset=v143 /v:m /m /nologo"

:: Kiem tra neu co tham so dong lenh truyen vao
if /i "%~1"=="all"     goto :CLI_ALL
if /i "%~1"=="cs"      goto :CLI_CS
if /i "%~1"=="ds"      goto :CLI_DS
if /i "%~1"=="js"      goto :CLI_JS
if /i "%~1"=="gs"      goto :CLI_GS
if /i "%~1"=="gscs"    goto :CLI_GSCS
if /i "%~1"=="clean"   goto :CLI_CLEAN
if /i "%~1"=="rebuild" goto :CLI_REBUILD

:: ============================================================================
:: MENU CHINH
:: ============================================================================
:MENU
cls
echo ================================================================================
echo                    TOOL BUILD MU SERVER (CS - DS - JS - GS)
echo ================================================================================
echo  Thu muc goc : %ROOT_DIR%
echo  MSBuild     : %MSBUILD_EXE%
echo ================================================================================
echo.
echo    [1] Build TAT CA (CS + DS + JS + GS + GSCS)
echo    [2] Build ConnectServer (CS)
echo    [3] Build DataServer    (DS)
echo    [4] Build JoinServer    (JS)
echo    [5] Build GameServer    (GS - Sub 1)
echo    [6] Build GameServerCS  (GS Castle Siege)
echo    [7] Don rac va xoa file tam (Clean .vs, BuildLog, crash dumps, logs)
echo    [8] Rebuild Full (Don rac roi Build tat ca tu dau)
echo    [0] Thoat
echo.
echo ================================================================================
set /p "CHOICE=>> Lua chon cua ban (0-8): "

if "%CHOICE%"=="1" goto :MENU_BUILD_ALL
if "%CHOICE%"=="2" goto :BUILD_CS
if "%CHOICE%"=="3" goto :BUILD_DS
if "%CHOICE%"=="4" goto :BUILD_JS
if "%CHOICE%"=="5" goto :BUILD_GS
if "%CHOICE%"=="6" goto :BUILD_GSCS
if "%CHOICE%"=="7" goto :CLEAN_ALL
if "%CHOICE%"=="8" goto :MENU_REBUILD
if "%CHOICE%"=="0" goto :EXIT
echo Lua chon khong hop le!
timeout /t 1 /nobreak >nul
goto :MENU

:: ============================================================================
:: CAC HAM DIEU HUONG CLI
:: ============================================================================

:CLI_ALL
call :DO_BUILD_CS
call :DO_BUILD_DS
call :DO_BUILD_JS
call :DO_BUILD_GS
call :DO_BUILD_GSCS
goto :EXIT

:CLI_CS
call :DO_BUILD_CS
goto :EXIT

:CLI_DS
call :DO_BUILD_DS
goto :EXIT

:CLI_JS
call :DO_BUILD_JS
goto :EXIT

:CLI_GS
call :DO_BUILD_GS
goto :EXIT

:CLI_GSCS
call :DO_BUILD_GSCS
goto :EXIT

:CLI_CLEAN
call :DO_CLEAN
goto :EXIT

:CLI_REBUILD
call :DO_CLEAN
call :DO_BUILD_CS
call :DO_BUILD_DS
call :DO_BUILD_JS
call :DO_BUILD_GS
call :DO_BUILD_GSCS
goto :EXIT

:: ============================================================================
:: CAC HAM DIEU HUONG MENU
:: ============================================================================

:BUILD_CS
call :DO_BUILD_CS
echo.
pause
goto :MENU

:BUILD_DS
call :DO_BUILD_DS
echo.
pause
goto :MENU

:BUILD_JS
call :DO_BUILD_JS
echo.
pause
goto :MENU

:BUILD_GS
call :DO_BUILD_GS
echo.
pause
goto :MENU

:BUILD_GSCS
call :DO_BUILD_GSCS
echo.
pause
goto :MENU

:MENU_BUILD_ALL
cls
echo ================================================================================
echo                     BAT DAU BUILD TOAN BO MU SERVER
echo ================================================================================
echo.
call :DO_BUILD_CS
echo.
call :DO_BUILD_DS
echo.
call :DO_BUILD_JS
echo.
call :DO_BUILD_GS
echo.
call :DO_BUILD_GSCS
echo.
echo ================================================================================
echo                      HOAN TAT TOAN BO TIEN TRINH BUILD!
echo ================================================================================
echo.
pause
goto :MENU

:MENU_REBUILD
cls
echo ================================================================================
echo                         REBUILD TOAN BO SERVER
echo ================================================================================
echo.
call :DO_CLEAN
echo.
call :DO_BUILD_CS
echo.
call :DO_BUILD_DS
echo.
call :DO_BUILD_JS
echo.
call :DO_BUILD_GS
echo.
call :DO_BUILD_GSCS
echo.
echo ================================================================================
echo                      HOAN TAT REBUILD TOAN BO SERVER!
echo ================================================================================
echo.
pause
goto :MENU

:: ----------------------------------------------------------------------------
:: THUC THI BUILD CHI TIET
:: ----------------------------------------------------------------------------
:DO_BUILD_CS
echo ----------------------------------------------------------------
echo [1/5] Dang build ConnectServer (CS)...
echo ----------------------------------------------------------------
set "SLN=%SRC_DIR%1.ConnectServer\ConnectServer.sln"
"%MSBUILD_EXE%" "%SLN%" /p:Configuration="EX603" %BUILD_OPTS%
if errorlevel 1 (
    color 0C
    echo [X] Build ConnectServer THAT BAI!
    color 0A
) else (
    echo [V] Build ConnectServer THANH CONG!
    echo     =^> %MUSERVER_DIR%1.ConnectServer\ConnectServer.exe
)
exit /b 0

:DO_BUILD_DS
echo ----------------------------------------------------------------
echo [2/5] Dang build DataServer (DS)...
echo ----------------------------------------------------------------
set "SLN=%SRC_DIR%2.DataServer\DataServer.sln"
"%MSBUILD_EXE%" "%SLN%" /p:Configuration="EX603" %BUILD_OPTS%
if errorlevel 1 (
    color 0C
    echo [X] Build DataServer THAT BAI!
    color 0A
) else (
    echo [V] Build DataServer THANH CONG!
    echo     =^> %MUSERVER_DIR%2.DataServer\DataServer.exe
)
exit /b 0

:DO_BUILD_JS
echo ----------------------------------------------------------------
echo [3/5] Dang build JoinServer (JS)...
echo ----------------------------------------------------------------
set "SLN=%SRC_DIR%3.JoinServer\JoinServer.sln"
"%MSBUILD_EXE%" "%SLN%" /p:Configuration="EX603" %BUILD_OPTS%
if errorlevel 1 (
    color 0C
    echo [X] Build JoinServer THAT BAI!
    color 0A
) else (
    echo [V] Build JoinServer THANH CONG!
    echo     =^> %MUSERVER_DIR%3.JoinServer\JoinServer.exe
)
exit /b 0

:DO_BUILD_GS
echo ----------------------------------------------------------------
echo [4/5] Dang build GameServer (GS - Sub 1)...
echo ----------------------------------------------------------------
set "SLN=%SRC_DIR%4.GameServer\GameServer.sln"
"%MSBUILD_EXE%" "%SLN%" /p:Configuration="EX603" %BUILD_OPTS%
if errorlevel 1 (
    color 0C
    echo [X] Build GameServer THAT BAI!
    color 0A
) else (
    echo [V] Build GameServer THANH CONG!
    echo     =^> %MUSERVER_DIR%4.GameServer\Sub 1\GameServer\GameServer.exe
)
exit /b 0

:DO_BUILD_GSCS
echo ----------------------------------------------------------------
echo [5/5] Dang build GameServerCS (GS Castle Siege)...
echo ----------------------------------------------------------------
set "SLN=%SRC_DIR%4.GameServer\GameServer.sln"
"%MSBUILD_EXE%" "%SLN%" /p:Configuration="EX603CS" %BUILD_OPTS%
if errorlevel 1 (
    color 0C
    echo [X] Build GameServerCS THAT BAI!
    color 0A
) else (
    echo [V] Build GameServerCS THANH CONG!
    echo     =^> %MUSERVER_DIR%4.GameServer\GameServer\GameServerCS.exe
)
exit /b 0

:: ============================================================================
:: DON RAC VA XOA FILE TAM
:: ============================================================================
:CLEAN_ALL
call :DO_CLEAN
echo.
pause
goto :MENU

:DO_CLEAN
echo ================================================================================
echo                     DANG DON RAC VA XOA FILE KO CAN THIET...
echo ================================================================================

:: 1. Xoa thu muc cache .vs
echo [-] Xoa cac thu muc cache Visual Studio (.vs)...
for /f "delims=" %%d in ('dir /s /b /a:d "%SRC_DIR%*.vs" 2^>nul') do (
    if exist "%%d" (
        echo     Xoa: %%d
        rd /s /q "%%d" 2>nul
    )
)

:: 2. Xoa thu muc build trung gian BuildLog
if exist "%BUILDLOG_DIR%" (
    echo [-] Xoa thu muc trung gian BuildLog...
    rd /s /q "%BUILDLOG_DIR%" 2>nul
)

:: 3. Xoa thu muc Debug cu trong GameServer
if exist "%SRC_DIR%4.GameServer\GameServer\Debug" (
    echo [-] Xoa thu muc Debug cu cua GameServer...
    rd /s /q "%SRC_DIR%4.GameServer\GameServer\Debug" 2>nul
)

:: 4. Xoa crash dumps, log rac va file tam
echo [-] Xoa crash dumps (.dmp), file log va screenshot tam...
del /s /q "%MUSERVER_DIR%*.dmp" 2>nul
del /s /q "%SRC_DIR%*.dmp" 2>nul
del /s /q "%SRC_DIR%*.tmp" 2>nul
del /s /q "%SRC_DIR%latest_logcat*.txt" 2>nul
del /s /q "%SRC_DIR%perf_latest.log" 2>nul
del /s /q "%SRC_DIR%device_screen_takumi.png" 2>nul
del /s /q "%SRC_DIR%emulator_hud_check*.png" 2>nul

:: 5. Xoa Thumbs.db
del /s /q "%ROOT_DIR%Thumbs.db" 2>nul

echo.
echo [V] Don rac hoan tat!
exit /b 0

:EXIT
exit /b 0
