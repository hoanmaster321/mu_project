@echo off
@chcp 65001 >nul
title MAKE UPDATE PACKAGE (data.zip) - 192.168.1.117/data.zip
cd /d "%~dp0"

echo ================================================================================
echo  DONG GOI UPDATE DU LIEU CLIENT (data.zip) CHO 192.168.1.117/data.zip
echo ================================================================================

echo [*] Dang tao goi data.zip tu thu muc Client\Data (Vui long cho vai giay)...
tar -a -c -f "data.zip" -C "Client" "Data"

if exist "data.zip" (
    if not exist "update" mkdir "update"
    copy /y "data.zip" "update\data.zip" >nul
    echo [V] Da dong goi thanh cong: data.zip
    echo [*] Link update Android: http://192.168.1.117/data.zip
) else (
    echo [X] Co loi xay ra khi dong goi!
)
echo ================================================================================
pause
