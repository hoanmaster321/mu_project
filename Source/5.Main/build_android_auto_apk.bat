@echo off
@chcp 65001 >nul
title AUTO BUILD ANDROID APK - MU ONLINE
color 0A
setlocal enabledelayedexpansion

:: ============================================================================
:: 1. XAC DINH THU MUC DU AN CHINH XAC (KHONG CO DAU \ O CUOI BIEN QUOTE)
:: ============================================================================
set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

if exist "%SCRIPT_DIR%\android\app" (
    set "REPO_ROOT=%SCRIPT_DIR%"
) else if exist "%SCRIPT_DIR%\app\src" (
    pushd "%SCRIPT_DIR%\.."
    set "REPO_ROOT=!CD!"
    popd
) else if exist "%SCRIPT_DIR%\..\..\android\app" (
    pushd "%SCRIPT_DIR%\..\.."
    set "REPO_ROOT=!CD!"
    popd
) else (
    set "REPO_ROOT=G:\mu_project"
)

if "%REPO_ROOT:~-1%"=="\" set "REPO_ROOT=%REPO_ROOT:~0,-1%"

set "ANDROID_DIR=%REPO_ROOT%\android"
if exist "%REPO_ROOT%\Source\5.Main" (
    set "CLIENT_DIR=%REPO_ROOT%\Source\5.Main"
) else (
    set "CLIENT_DIR=%REPO_ROOT%\Source_Server-Mobi\5.Main"
)
set "APP_DIR=%ANDROID_DIR%\app"
set "OUTPUT_APK_DIR=%ANDROID_DIR%\outputs"

echo ================================================================================
echo          TOOL TU DONG BUILD LIBMAIN.SO + DONG GOI + SIGN APK (ALL-IN-ONE)
echo ================================================================================
echo  Repo Root : %REPO_ROOT%
echo  Client    : %CLIENT_DIR%
echo  Android   : %ANDROID_DIR%
echo ================================================================================
echo.

:: ============================================================================
:: 2. TU DONG NHAN DIEN ANDROID SDK, NDK, JAVA, APKSIGNER
:: ============================================================================
if not defined ANDROID_HOME (
    if exist "%LOCALAPPDATA%\Android\Sdk" (
        set "ANDROID_HOME=%LOCALAPPDATA%\Android\Sdk"
    ) else if exist "C:\Android\Sdk" (
        set "ANDROID_HOME=C:\Android\Sdk"
    )
)

if not exist "%ANDROID_HOME%" (
    color 0C
    echo [LOI] Khong tim thay Android SDK tai: %ANDROID_HOME%
    pause
    exit /b 1
)

:: Tim NDK
set "NDK_PATH="
if exist "%ANDROID_HOME%\ndk\25.1.8937393\build\cmake\android.toolchain.cmake" (
    set "NDK_PATH=%ANDROID_HOME%\ndk\25.1.8937393"
) else (
    for /d %%d in ("%ANDROID_HOME%\ndk\*") do (
        if exist "%%d\build\cmake\android.toolchain.cmake" (
            set "NDK_PATH=%%d"
        )
    )
)

if not defined NDK_PATH (
    color 0C
    echo [LOI] Khong tim thay Android NDK hop le!
    pause
    exit /b 1
)

set "TOOLCHAIN=%NDK_PATH%\build\cmake\android.toolchain.cmake"
set "STRIP_EXE=%NDK_PATH%\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-strip.exe"

:: Tim apksigner
set "APKSIGNER_BAT="
for /d %%d in ("%ANDROID_HOME%\build-tools\*") do (
    if exist "%%d\apksigner.bat" (
        set "APKSIGNER_BAT=%%d\apksigner.bat"
    )
)

:: Tim Java JDK
if not defined JAVA_HOME (
    if exist "C:\Program Files\Android\Android Studio\jbr" (
        set "JAVA_HOME=C:\Program Files\Android\Android Studio\jbr"
    )
)

echo [OK] Android SDK : %ANDROID_HOME%
echo [OK] Android NDK : %NDK_PATH%
if defined APKSIGNER_BAT echo [OK] apksigner   : %APKSIGNER_BAT%
echo.

:: ============================================================================
:: 3. CHON CHE DO BUILD (HO TRO TRUYEN THAM SO CLI HOAC MENU)
:: ============================================================================
set "ABI_CHOICE=%~1"
if defined ABI_CHOICE goto :HAS_ABI

echo Chon ABI muon bien dich:
echo   [1] arm64-v8a  - Thiet bi 64-bit thuc te [Khuyen dung]
echo   [2] armeabi-v7a - Thiet bi 32-bit thuc te
echo   [3] Real Devices - Build ca arm64-v8a va armeabi-v7a
echo   [4] Universal - Build tat ca 4 ABI: arm64 + v7a + x86 + x86_64
echo.
set /p "ABI_CHOICE=>> Nhap lua chon [1-4, mac dinh la 1]: "

:HAS_ABI
if "%ABI_CHOICE%"=="" set "ABI_CHOICE=1"
set "ABI_CHOICE=%ABI_CHOICE: =%"

if "%ABI_CHOICE%"=="arm64" set "ABI_CHOICE=1"
if "%ABI_CHOICE%"=="v7a"   set "ABI_CHOICE=2"
if "%ABI_CHOICE%"=="all"   set "ABI_CHOICE=4"

:: ============================================================================
:: 4. DON RAC TRUOC KHI BUILD
:: ============================================================================
echo.
echo ================================================================================
echo [BƯỚC 1/5] DANG DON RAC VA XOA CACHE BUILD CU...
echo ================================================================================
if exist "%APP_DIR%\.cxx" (
    echo [-] Xoa cache .cxx cu...
    rd /s /q "%APP_DIR%\.cxx" 2>nul
)
if exist "%APP_DIR%\src\main\jniLibs" (
    echo [-] Xoa cac file *.bak trong jniLibs...
    del /s /q "%APP_DIR%\src\main\jniLibs\*.bak" 2>nul
)
:: Giu lai build cache de bien dich nhanh hon (Ninja tu dong xu ly incremental build)
:: for /d %%d in ("%CLIENT_DIR%\build-android-*") do (
::     if exist "%%d\" (
::         echo [-] Xoa thu muc build tam: %%d
::         rd /s /q "%%d" 2>nul
::     )
:: )
echo [V] Don rac hoan tat.

:: ============================================================================
:: 5. BIEN DICH LIBMAIN.SO
:: ============================================================================
echo.
echo ================================================================================
echo [BƯỚC 2/5] DANG BIEN DICH NATIVE LIBMAIN.SO QUA CMAKE + NDK...
echo ================================================================================

if "%ABI_CHOICE%"=="1" (
    call :COMPILE_LIBMAIN "arm64-v8a"
    if errorlevel 1 goto :BUILD_FAIL
)
if "%ABI_CHOICE%"=="2" (
    call :COMPILE_LIBMAIN "armeabi-v7a"
    if errorlevel 1 goto :BUILD_FAIL
)
if "%ABI_CHOICE%"=="3" (
    call :COMPILE_LIBMAIN "arm64-v8a"
    if errorlevel 1 goto :BUILD_FAIL
    call :COMPILE_LIBMAIN "armeabi-v7a"
    if errorlevel 1 goto :BUILD_FAIL
)
if "%ABI_CHOICE%"=="4" (
    call :COMPILE_LIBMAIN "arm64-v8a"
    if errorlevel 1 goto :BUILD_FAIL
    call :COMPILE_LIBMAIN "armeabi-v7a"
    if errorlevel 1 goto :BUILD_FAIL
    call :COMPILE_LIBMAIN "x86_64"
    if errorlevel 1 goto :BUILD_FAIL
    call :COMPILE_LIBMAIN "x86"
    if errorlevel 1 goto :BUILD_FAIL
)

:: ============================================================================
:: 6. DONG GOI VA TU DONG SIGN APK QUA GRADLE
:: ============================================================================
echo.
echo ================================================================================
echo [BƯỚC 3/5] DANG DONG GOI VA TU DONG KY (SIGN) APK BANG GRADLE...
echo ================================================================================

cd /d "%ANDROID_DIR%"
if "%ABI_CHOICE%"=="1" (
    call gradlew.bat :app:assembleRealDeviceRelease -PmuRealDeviceAbis=arm64-v8a
) else if "%ABI_CHOICE%"=="2" (
    call gradlew.bat :app:assembleRealDeviceRelease -PmuRealDeviceAbis=armeabi-v7a
) else if "%ABI_CHOICE%"=="3" (
    call gradlew.bat :app:assembleRealDeviceRelease
) else (
    call gradlew.bat :app:assembleUniversalRelease
)

if errorlevel 1 (
    color 0C
    echo [X] Dong goi APK that bai!
    pause
    exit /b 1
)

:: ============================================================================
:: 7. KIEM TRA VA SAO CHEP APK DA SIGN
:: ============================================================================
echo.
echo ================================================================================
echo [BƯỚC 4/5] SAO CHEP VA XUAT FILE APK HOAN CHINH...
echo ================================================================================

set "SOURCE_APK="
if exist "%APP_DIR%\build\outputs\apk\realDevice\release\app-realDevice-release.apk" (
    set "SOURCE_APK=%APP_DIR%\build\outputs\apk\realDevice\release\app-realDevice-release.apk"
) else if exist "%APP_DIR%\build\outputs\apk\universal\release\app-universal-release.apk" (
    set "SOURCE_APK=%APP_DIR%\build\outputs\apk\universal\release\app-universal-release.apk"
)

if not defined SOURCE_APK (
    color 0C
    echo [X] Khong tim thay file APK sau khi build!
    pause
    exit /b 1
)

if not exist "%OUTPUT_APK_DIR%" mkdir "%OUTPUT_APK_DIR%"
set "FINAL_APK=%OUTPUT_APK_DIR%\MuOnline_Client_Signed.apk"
copy /Y "%SOURCE_APK%" "%FINAL_APK%" >nul
copy /Y "%SOURCE_APK%" "%REPO_ROOT%\MuOnline_Client_Signed.apk" >nul

echo [V] File APK da duoc xuat ra:
echo     1. %FINAL_APK%
echo     2. %REPO_ROOT%\MuOnline_Client_Signed.apk

:: ============================================================================
:: 8. XAC MINH CHU KY APK
:: ============================================================================
echo.
echo ================================================================================
echo [BƯỚC 5/5] XAC MINH CHU KY SO CUA APK...
echo ================================================================================
if not defined APKSIGNER_BAT goto :NO_APKSIGNER
call "%APKSIGNER_BAT%" verify --verbose "%FINAL_APK%"
goto :VERIFY_DONE

:NO_APKSIGNER
echo [INFO] Bo qua buoc verify chi tiet do khong co apksigner (APK da duoc Gradle sign qua mu_release.keystore).

:VERIFY_DONE
echo.
echo ================================================================================
echo                 HOAN TAT BUILD VA SIGN APK THANH CONG 100%!
echo ================================================================================
echo File APK da san sang de cai dat tren dien thoai Android:
echo adb install -r "%FINAL_APK%"
echo ================================================================================
echo.
if not "%~2"=="--nopause" pause
exit /b 0

:: ============================================================================
:: HAM BUILD & STRIP & COPY LIBMAIN.SO
:: ============================================================================
:COMPILE_LIBMAIN
set "CURR_ABI=%~1"
set "BUILD_DIR=%CLIENT_DIR%\build-android-%CURR_ABI%"
set "DEST_SO=%APP_DIR%\src\main\jniLibs\%CURR_ABI%\libmain.so"

echo.
echo ----------------------------------------------------------------
echo [*] Dang bien dich libmain.so [%CURR_ABI%]...
echo ----------------------------------------------------------------

cmake -B "%BUILD_DIR%" -S "%CLIENT_DIR%" -G "Ninja" ^
    -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN%" ^
    -DANDROID_ABI="%CURR_ABI%" ^
    -DANDROID_PLATFORM="android-28" ^
    -DCMAKE_BUILD_TYPE="Release" ^
    -DCMAKE_SHARED_LINKER_FLAGS="-Wl,-z,max-page-size=16384"

if errorlevel 1 exit /b 1

cmake --build "%BUILD_DIR%" -j %NUMBER_OF_PROCESSORS%
if errorlevel 1 exit /b 1

if exist "%BUILD_DIR%\_deps\sdl3-build\libSDL3.so" (
    copy /y "%BUILD_DIR%\_deps\sdl3-build\libSDL3.so" "%APP_DIR%\src\main\jniLibs\%CURR_ABI%\libSDL3.so" >nul
)

if exist "%STRIP_EXE%" (
    echo [*] Dang strip debug symbol giam dung luong libmain.so va libSDL3.so...
    if exist "%DEST_SO%" "%STRIP_EXE%" --strip-all "%DEST_SO%"
    if exist "%APP_DIR%\src\main\jniLibs\%CURR_ABI%\libSDL3.so" "%STRIP_EXE%" --strip-all "%APP_DIR%\src\main\jniLibs\%CURR_ABI%\libSDL3.so"
)

echo [V] Da build va copy: %DEST_SO%
exit /b 0

:BUILD_FAIL
color 0C
echo.
echo [X] CO LOI XAY RA TRONG QUA TRINH BUILD!
pause
exit /b 1
