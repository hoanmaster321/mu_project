@echo off
setlocal enabledelayedexpansion
title DON RAC VA CACHE BUILD DU AN MU ONLINE
color 0B

set "ROOT_DIR=%~dp0"
if "%ROOT_DIR:~-1%"=="\" set "ROOT_DIR=%ROOT_DIR:~0,-1%"

echo ================================================================================
echo                     DON DEP FILE RAC VA CACHE DU AN
echo ================================================================================
echo  Thu muc goc: %ROOT_DIR%
echo ================================================================================
echo.

echo [*] Dang xoa cac file anh chup man hinh debug...
del /q "%ROOT_DIR%\screen*.png" 2>nul
del /q "%ROOT_DIR%\screen*.jpg" 2>nul

echo [*] Dang xoa logcat va log tam...
del /q "%ROOT_DIR%\logcat_mu.txt" 2>nul
del /q "%ROOT_DIR%\MuOnline_Client_Signed.apk" 2>nul
del /q "%ROOT_DIR%\android\hs_err_*.log" 2>nul
del /q "%ROOT_DIR%\android\build_*.txt" 2>nul

echo [*] Dang don thu muc BuildLog cu...
if exist "%ROOT_DIR%\BuildLog" (
    rd /s /q "%ROOT_DIR%\BuildLog" 2>nul
)

echo [*] Dang don cac thu muc ClientBuild tam...
if exist "%ROOT_DIR%\ClientBuild_192.168.1.117" (
    rd /s /q "%ROOT_DIR%\ClientBuild_192.168.1.117" 2>nul
)
if exist "%ROOT_DIR%\ClientBuild_192.168.99.200" (
    rd /s /q "%ROOT_DIR%\ClientBuild_192.168.99.200" 2>nul
)

echo [*] Dang don cache build trong Source\5.Main...
del /q "%ROOT_DIR%\Source\5.Main\build.log" 2>nul
for /d %%d in ("%ROOT_DIR%\Source\5.Main\build*") do (
    echo     [-] Xoa %%d
    rd /s /q "%%d" 2>nul
)
if exist "%ROOT_DIR%\Source\5.Main\.vs" (
    rd /s /q "%ROOT_DIR%\Source\5.Main\.vs" 2>nul
)

echo [*] Dang don cache build Android...
if exist "%ROOT_DIR%\android\.gradle" (
    rd /s /q "%ROOT_DIR%\android\.gradle" 2>nul
)
if exist "%ROOT_DIR%\android\app\build" (
    rd /s /q "%ROOT_DIR%\android\app\build" 2>nul
)
if exist "%ROOT_DIR%\android\app\.cxx" (
    rd /s /q "%ROOT_DIR%\android\app\.cxx" 2>nul
)

echo [*] Dang don log client va server...
del /q "%ROOT_DIR%\Client\*.log" 2>nul
del /q "%ROOT_DIR%\Client\Main.pdb" 2>nul
if exist "%ROOT_DIR%\Client\STACK_ERROR" (
    rd /s /q "%ROOT_DIR%\Client\STACK_ERROR" 2>nul
)
if exist "%ROOT_DIR%\Client\Data\Interface - Copy" (
    rd /s /q "%ROOT_DIR%\Client\Data\Interface - Copy" 2>nul
)
del /q "%ROOT_DIR%\Client\Data\Logo\MU-logo - Copy.ozt" 2>nul
del /s /q /a:h /a "%ROOT_DIR%\Client\Data\desktop.ini" 2>nul

if exist "%ROOT_DIR%\MuServer\7.Log" (
    del /s /q "%ROOT_DIR%\MuServer\7.Log\*.*" 2>nul
)
if exist "%ROOT_DIR%\MuServer\4.GameServer\Sub 1\GameServer\LOG" (
    del /s /q "%ROOT_DIR%\MuServer\4.GameServer\Sub 1\GameServer\LOG\*.*" 2>nul
)

echo.
echo ================================================================================
echo                     DON RAC HOAN TAT THANH CONG!
echo ================================================================================
if not "%~1"=="--nopause" pause
