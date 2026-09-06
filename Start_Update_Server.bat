@echo off
@chcp 65001 >nul
title UPDATE HTTP SERVER - 192.168.1.117:8080
cd /d "%~dp0"

echo ================================================================================
echo  HTTP SERVER PHUC VU UPDATE CLIENT (192.168.1.117:8080)
echo ================================================================================
echo  Thu muc update: %~dp0update\data.zip
echo  URL Android:    http://192.168.1.117:8080/update/data.zip
echo                  http://192.168.1.117/update/data.zip (neu dung IIS / Nginx)
echo.
echo  Dang lang nghe ket noi... (Nhan Ctrl+C de dung server)
echo ================================================================================
python -m http.server 8080
pause
