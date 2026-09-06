@echo off
@chcp 65001 >nul
color 0A
title DON RAC VA XOA LOG MUSERVER
echo ================================================================================
echo                     DANG DON RAC VA XOA LOG MUSERVER...
echo ================================================================================

set "ROOT_DIR=%~dp0"
if "%ROOT_DIR:~-1%"=="\" set "ROOT_DIR=%ROOT_DIR:~0,-1%"

cd /d "%ROOT_DIR%"

echo [*] Xoa file log, dump, txt nhat ky cu...
del /s /q *.log 2>nul
del /s /q *log*.txt 2>nul
del /s /q *.dmp 2>nul
del /s /q *MuError*.dmp 2>nul
del /s /q *PSGG*.txt 2>nul
del /s /q 202*.txt 2>nul
del /s /q 203*.txt 2>nul
del /s /q Thumbs.db 2>nul
del /s /q "* - Copy*" 2>nul

echo [*] Don cac thu muc LOG...
for /d /r "%ROOT_DIR%" %%d in (LOG Log_CS Log_DS Log_JS LogAccount LogText HACK_LOG) do (
    if exist "%%d" (
        del /q "%%d\*.txt" 2>nul
        del /q "%%d\*.log" 2>nul
    )
)

if exist "%ROOT_DIR%\MHP_LOG" rd /s /q "%ROOT_DIR%\MHP_LOG" 2>nul

for /f "tokens=* delims=" %%i in ('dir /s /b /a:d *.svn 2^>nul') do (
    rd /s /q "%%i" 2>nul
)

echo.
echo [V] DA DON RAC VA XOA TOAN BO LOG THANH CONG!
echo ================================================================================
pause
exit /b 0