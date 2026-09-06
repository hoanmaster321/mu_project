@echo off
@chcp 65001 >nul
title BUILD MU CLIENT ANDROID (CMAKE)
color 0A
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
pushd "%SCRIPT_DIR%..\.."
set "REPO_ROOT=!CD!\"
popd

set "ANDROID_DIR=%REPO_ROOT%android\"

:: Tu dong tim Android SDK & NDK
if not defined ANDROID_HOME (
    if exist "%LOCALAPPDATA%\Android\Sdk" (
        set "ANDROID_HOME=%LOCALAPPDATA%\Android\Sdk"
    )
)

if not defined ANDROID_NDK_HOME (
    for /d %%d in ("%ANDROID_HOME%\ndk\*") do (
        if exist "%%d\build\cmake\android.toolchain.cmake" (
            set "ANDROID_NDK_HOME=%%d"
        )
    )
)

echo ================================================================================
echo                   BUILD MU CLIENT (ANDROID) BANG CMAKE
echo ================================================================================
echo  SDK Path : %ANDROID_HOME%
echo  NDK Path : %ANDROID_NDK_HOME%
echo ================================================================================
echo.
echo    [1] Build native libmain.so (arm64-v8a)
echo    [2] Build native libmain.so (armeabi-v7a)
echo    [3] Build native libmain.so (x86_64 - Emulator)
echo    [4] Build native libmain.so (x86 - Emulator)
echo    [5] Build TAT CA cac ABI (arm64 + armeabi + x86 + x86_64)
echo    [6] Build APK hoan chinh qua Gradle (assembleRealDeviceDebug)
echo    [7] Build APK Universal qua Gradle (assembleUniversalDebug)
echo    [0] Thoat
echo.
echo ================================================================================
set /p "CHOICE=>> Lua chon cua ban (0-7): "

if "%CHOICE%"=="1" (
    call :BUILD_ABI "arm64-v8a"
    pause
    goto :EOF
)
if "%CHOICE%"=="2" (
    call :BUILD_ABI "armeabi-v7a"
    pause
    goto :EOF
)
if "%CHOICE%"=="3" (
    call :BUILD_ABI "x86_64"
    pause
    goto :EOF
)
if "%CHOICE%"=="4" (
    call :BUILD_ABI "x86"
    pause
    goto :EOF
)
if "%CHOICE%"=="5" (
    call :BUILD_ABI "arm64-v8a"
    call :BUILD_ABI "armeabi-v7a"
    call :BUILD_ABI "x86_64"
    call :BUILD_ABI "x86"
    pause
    goto :EOF
)
if "%CHOICE%"=="6" (
    cd /d "%ANDROID_DIR%"
    call gradlew.bat :app:assembleRealDeviceDebug
    pause
    goto :EOF
)
if "%CHOICE%"=="7" (
    cd /d "%ANDROID_DIR%"
    call gradlew.bat :app:assembleUniversalDebug
    pause
    goto :EOF
)
if "%CHOICE%"=="0" exit /b 0

exit /b 0

:: ----------------------------------------------------------------------------
:BUILD_ABI
set "ABI=%~1"
set "BUILD_DIR=%SCRIPT_DIR%build-android-%ABI%"
echo.
echo [*] Dang build ABI: %ABI%...
cmake -B "%BUILD_DIR%" -S "%SCRIPT_DIR%" -G "Ninja" ^
    -DCMAKE_TOOLCHAIN_FILE="%ANDROID_NDK_HOME%\build\cmake\android.toolchain.cmake" ^
    -DANDROID_ABI="%ABI%" ^
    -DANDROID_PLATFORM="android-28" ^
    -DCMAKE_BUILD_TYPE="Release"

if errorlevel 1 (
    color 0C
    echo [X] Cau hinh CMake cho %ABI% that bai!
    exit /b 1
)

cmake --build "%BUILD_DIR%" -j %NUMBER_OF_PROCESSORS%
if errorlevel 1 (
    color 0C
    echo [X] Bien dich %ABI% that bai!
    exit /b 1
)

echo [V] Build %ABI% thanh cong! Output da duoc copy sang:
echo     %REPO_ROOT%android\app\src\main\jniLibs\%ABI%\libmain.so
exit /b 0