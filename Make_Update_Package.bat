@echo off
@chcp 65001 >nul
title MAKE UPDATE PACKAGE (data.zip) - 192.168.1.117
cd /d "%~dp0"

echo ================================================================================
echo  DONG GOI UPDATE DU LIEU CLIENT (data.zip) CHO 192.168.1.117
echo ================================================================================

if not exist "update" mkdir "update"
echo [*] Dang tao goi data.zip tu thu muc Client\Data (Vui long cho vai giay)...
tar -a -c -f "update\data.zip" -C "Client" "Data"

if exist "update\data.zip" (
    echo [V] Da dong goi thanh cong: update\data.zip
    echo [*] Link update Android: http://192.168.1.117:8080/update/data.zip
) else (
    echo [X] Co loi xay ra khi dong goi!
)
echo ================================================================================
pause
