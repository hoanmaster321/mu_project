@echo off
@chcp 65001 >nul
title AUTO BUILD MU CLIENT WINDOWS (CMAKE)
color 0A
setlocal enabledelayedexpansion

:: ============================================================================
:: 1. XAC DINH THU MUC DU AN CHINH XAC (KHONG CO DAU \ O CUOI BIEN QUOTE)
:: ============================================================================
set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

if exist "%SCRIPT_DIR%\CMakeLists.txt" (
    set "CLIENT_DIR=%SCRIPT_DIR%"
    pushd "%SCRIPT_DIR%\..\.."
    set "REPO_ROOT=!CD!"
    popd
) else if exist "%SCRIPT_DIR%\Source\5.Main\CMakeLists.txt" (
    set "REPO_ROOT=%SCRIPT_DIR%"
    set "CLIENT_DIR=%SCRIPT_DIR%\Source\5.Main"
) else (
    set "REPO_ROOT=G:\mu_project"
    set "CLIENT_DIR=%REPO_ROOT%\Source\5.Main"
)

if "%REPO_ROOT:~-1%"=="\" set "REPO_ROOT=%REPO_ROOT:~0,-1%"
if "%CLIENT_DIR:~-1%"=="\" set "CLIENT_DIR=%CLIENT_DIR:~0,-1%"

set "BUILD_DIR=%CLIENT_DIR%\build-windows"
set "OUT_CLIENT_IP=%REPO_ROOT%\ClientBuild_192.168.1.117"
set "OUT_CLIENT_MAIN=%REPO_ROOT%\Client"

echo ================================================================================
echo             TOOL TU DONG BUILD MU CLIENT (WINDOWS) BANG CMAKE
echo ================================================================================
echo  Repo Root  : %REPO_ROOT%
echo  Client Src : %CLIENT_DIR%
echo  Build Dir  : %BUILD_DIR%
echo ================================================================================
echo.

:: ============================================================================
:: 2. TU DONG NHAN DIEN CONG CU (CMAKE, VISUAL STUDIO, PYTHON, VULKAN SDK)
:: ============================================================================
echo [BƯỚC 1/4] KIEM TRA MOI TRUONG VA CONG CU HE THONG...

:: 2.1. Tim CMake
set "CMAKE_EXE="
where cmake.exe >nul 2>nul
if not errorlevel 1 (
    for /f "tokens=*" %%i in ('where cmake.exe') do (
        if not defined CMAKE_EXE set "CMAKE_EXE=%%i"
    )
)

if not defined CMAKE_EXE (
    if exist "C:\Program Files\CMake\bin\cmake.exe" (
        set "CMAKE_EXE=C:\Program Files\CMake\bin\cmake.exe"
    ) else if exist "%LOCALAPPDATA%\Android\Sdk\cmake\3.22.1\bin\cmake.exe" (
        set "CMAKE_EXE=%LOCALAPPDATA%\Android\Sdk\cmake\3.22.1\bin\cmake.exe"
    ) else (
        for /d %%d in ("%LOCALAPPDATA%\Android\Sdk\cmake\*") do (
            if exist "%%d\bin\cmake.exe" set "CMAKE_EXE=%%d\bin\cmake.exe"
        )
    )
)

if not defined CMAKE_EXE (
    color 0C
    echo [LOI] Khong tim thay CMake tren he thong!
    echo Vui long cai dat CMake hoac them CMake vao bien moi truong PATH.
    pause
    exit /b 1
)
echo [OK] CMake         : %CMAKE_EXE%

:: 2.2. Tim Visual Studio va Generator qua vswhere
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VS_PATH="
set "CMAKE_GENERATOR="

if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
        if exist "%%i" set "VS_PATH=%%i"
    )
)

if not defined VS_PATH (
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community" (
        set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional" (
        set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise" (
        set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise"
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community" (
        set "VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community"
    )
)

if defined VS_PATH (
    echo [OK] Visual Studio : %VS_PATH%
    echo %VS_PATH% | find "2022" >nul
    if not errorlevel 1 (
        set "CMAKE_GENERATOR=Visual Studio 17 2022"
    ) else (
        echo %VS_PATH% | find "2019" >nul
        if not errorlevel 1 (
            set "CMAKE_GENERATOR=Visual Studio 16 2019"
        )
    )
)

if not defined CMAKE_GENERATOR (
    set "CMAKE_GENERATOR=Visual Studio 17 2022"
)
echo [OK] Generator     : %CMAKE_GENERATOR% (Platform Win32)

:: 2.3. Nhan dien Vulkan SDK (Dung cho glslc SPIR-V shader)
if not defined VULKAN_SDK (
    if exist "C:\VulkanSDK" (
        for /d %%d in ("C:\VulkanSDK\*") do (
            if exist "%%d\Bin\glslc.exe" set "VULKAN_SDK=%%d"
        )
    )
)
if defined VULKAN_SDK (
    echo [OK] Vulkan SDK    : %VULKAN_SDK%
    if exist "%VULKAN_SDK%\Bin" (
        set "PATH=%VULKAN_SDK%\Bin;%PATH%"
    )
) else (
    echo [INFO] Vulkan SDK  : Khong tim thay (CMake se su dung shader da bien dich san neu co)
)

:: 2.4. Nhan dien Python (Dung cho generate_embedded_shaders.py)
set "PYTHON_EXE="
where python.exe >nul 2>nul
if not errorlevel 1 (
    for /f "tokens=*" %%i in ('where python.exe') do (
        if not defined PYTHON_EXE set "PYTHON_EXE=%%i"
    )
)
if defined PYTHON_EXE (
    echo [OK] Python        : %PYTHON_EXE%
) else (
    echo [INFO] Python       : Khong tim thay (bo qua neu shader khong can sinh lai)
)
echo.

:: ============================================================================
:: 3. KIEM TRA VA DONG TIEN TRINH MAIN.EXE DANG CHAY (TRANH KHOA FILE)
:: ============================================================================
tasklist /fi "imagename eq Main.exe" 2>nul | find /i "Main.exe" >nul
if not errorlevel 1 (
    echo [CANH BAO] Main.exe dang chay! Dang dong tien trinh de tranh loi khoa file...
    taskkill /f /im Main.exe >nul 2>nul
    timeout /t 1 /nobreak >nul
)

:: ============================================================================
:: 4. XU LY THAM SO DONG LENH HOAC HIEN THI MENU
:: ============================================================================
set "MODE=%~1"
set "NOPROMPT=0"

if /i "%~1"=="--nopause" (
    set "MODE=release"
    set "NOPROMPT=1"
)
if /i "%~2"=="--nopause" set "NOPROMPT=1"

if defined MODE (
    if /i "%MODE%"=="release" goto :DO_RELEASE
    if /i "%MODE%"=="debug"   goto :DO_DEBUG
    if /i "%MODE%"=="clean"   goto :DO_CLEAN
    if /i "%MODE%"=="rebuild" goto :DO_REBUILD
)

:MENU
echo ================================================================================
echo Chon che do build:
echo   [1] Build Release (Win32) [Khuyen dung - Chay game muot ma]
echo   [2] Rebuild Release (Clean cache + Cau hinh lai + Build Release)
echo   [3] Build Debug (Win32) [Dung cho go loi / debug]
echo   [4] Clean thu muc build-windows
echo   [0] Thoat
echo ================================================================================
set "CHOICE="
set /p "CHOICE=>> Nhap lua chon [0-4, mac dinh la 1]: "

if "%CHOICE%"=="" set "CHOICE=1"
set "CHOICE=%CHOICE: =%"

if "%CHOICE%"=="1" goto :DO_RELEASE
if "%CHOICE%"=="2" goto :DO_REBUILD
if "%CHOICE%"=="3" goto :DO_DEBUG
if "%CHOICE%"=="4" goto :DO_CLEAN
if "%CHOICE%"=="0" exit /b 0

echo [X] Lua chon khong hop le!
goto :MENU

:DO_CLEAN
echo.
echo [*] Dang don dep thu muc build-windows...
if exist "%BUILD_DIR%" (
    rd /s /q "%BUILD_DIR%" 2>nul
    echo [V] Da xoa thu muc build cu: %BUILD_DIR%
) else (
    echo [V] Thu muc build khong ton tai, khong can xoa.
)
if "%NOPROMPT%"=="0" pause
exit /b 0

:DO_REBUILD
echo.
echo [*] Dang xoa cache build de rebuild toan bo...
if exist "%BUILD_DIR%" (
    rd /s /q "%BUILD_DIR%" 2>nul
    echo [V] Da xoa cache build cu.
)
set "CONFIG=Release"
goto :START_BUILD

:DO_DEBUG
set "CONFIG=Debug"
goto :START_BUILD

:DO_RELEASE
set "CONFIG=Release"
goto :START_BUILD

:: ============================================================================
:: 5. CAU HINH CMAKE CHO PROJECT
:: ============================================================================
:START_BUILD
echo.
echo ================================================================================
echo [BƯỚC 2/4] CAU HINH PROJECT CMAKE (Win32)...
echo ================================================================================

if not exist "%BUILD_DIR%\CMakeCache.txt" (
    echo [*] Tao moi va cau hinh project CMake: %BUILD_DIR%
    "%CMAKE_EXE%" -B "%BUILD_DIR%" -S "%CLIENT_DIR%" -G "%CMAKE_GENERATOR%" -A Win32
    if errorlevel 1 (
        color 0C
        echo.
        echo [X] Cau hinh CMake THAT BAI!
        if "%NOPROMPT%"=="0" pause
        exit /b 1
    )
) else (
    echo [V] Project CMake da duoc cau hinh san trong: %BUILD_DIR%
)

:: ============================================================================
:: 6. BIEN DICH CLIENT
:: ============================================================================
echo.
echo ================================================================================
echo [BƯỚC 3/4] BIEN DICH CLIENT (%CONFIG% Win32)...
echo ================================================================================

"%CMAKE_EXE%" --build "%BUILD_DIR%" --config %CONFIG% -j %NUMBER_OF_PROCESSORS%
if errorlevel 1 (
    color 0C
    echo.
    echo [X] Bien dich Client THAT BAI!
    if "%NOPROMPT%"=="0" pause
    exit /b 1
)

:: ============================================================================
:: 7. DONG BO VA KIEM TRA FILE DA XUAT
:: ============================================================================
echo.
echo ================================================================================
echo [BƯỚC 4/4] DONG BO FILE EXECUTABLE VA THU VIEN...
echo ================================================================================

:: Tim file Main.exe vua build
set "BUILT_MAIN="
if exist "%OUT_CLIENT_IP%\Main.exe" (
    set "BUILT_MAIN=%OUT_CLIENT_IP%\Main.exe"
) else if exist "%OUT_CLIENT_MAIN%\Main.exe" (
    set "BUILT_MAIN=%OUT_CLIENT_MAIN%\Main.exe"
) else if exist "%BUILD_DIR%\%CONFIG%\Main.exe" (
    set "BUILT_MAIN=%BUILD_DIR%\%CONFIG%\Main.exe"
)

if not defined BUILT_MAIN (
    color 0C
    echo [X] Khong tim thay file Main.exe sau khi bien dich!
    if "%NOPROMPT%"=="0" pause
    exit /b 1
)

:: Dong bo Main.exe sang ca 2 thu muc Client neu can
if exist "%OUT_CLIENT_IP%" (
    if not "%BUILT_MAIN%"=="%OUT_CLIENT_IP%\Main.exe" (
        copy /y "%BUILT_MAIN%" "%OUT_CLIENT_IP%\Main.exe" >nul
    )
)

if exist "%OUT_CLIENT_MAIN%" (
    if not "%BUILT_MAIN%"=="%OUT_CLIENT_MAIN%\Main.exe" (
        copy /y "%BUILT_MAIN%" "%OUT_CLIENT_MAIN%\Main.exe" >nul
    )
)

:: Dong bo SDL3.dll neu co
set "SDL3_SOURCE="
if exist "%BUILD_DIR%\_deps\sdl3-build\%CONFIG%\SDL3.dll" (
    set "SDL3_SOURCE=%BUILD_DIR%\_deps\sdl3-build\%CONFIG%\SDL3.dll"
) else if exist "%BUILD_DIR%\_deps\sdl3-build\SDL3.dll" (
    set "SDL3_SOURCE=%BUILD_DIR%\_deps\sdl3-build\SDL3.dll"
) else if exist "%OUT_CLIENT_IP%\SDL3.dll" (
    set "SDL3_SOURCE=%OUT_CLIENT_IP%\SDL3.dll"
)

if defined SDL3_SOURCE (
    if exist "%OUT_CLIENT_IP%" copy /y "%SDL3_SOURCE%" "%OUT_CLIENT_IP%\SDL3.dll" >nul 2>nul
    if exist "%OUT_CLIENT_MAIN%" copy /y "%SDL3_SOURCE%" "%OUT_CLIENT_MAIN%\SDL3.dll" >nul 2>nul
)

:: Dong bo Main.pdb (neu co)
if exist "%BUILD_DIR%\%CONFIG%\Main.pdb" (
    if exist "%OUT_CLIENT_IP%" copy /y "%BUILD_DIR%\%CONFIG%\Main.pdb" "%OUT_CLIENT_IP%\Main.pdb" >nul 2>nul
    if exist "%OUT_CLIENT_MAIN%" copy /y "%BUILD_DIR%\%CONFIG%\Main.pdb" "%OUT_CLIENT_MAIN%\Main.pdb" >nul 2>nul
)

echo.
echo ================================================================================
echo                  [V] BIEN DICH CLIENT WINDOWS THANH CONG!
echo ================================================================================
if exist "%OUT_CLIENT_IP%\Main.exe" (
    echo  1. %OUT_CLIENT_IP%\Main.exe
)
if exist "%OUT_CLIENT_MAIN%\Main.exe" (
    echo  2. %OUT_CLIENT_MAIN%\Main.exe
)
echo ================================================================================
echo.

if "%NOPROMPT%"=="0" pause
exit /b 0