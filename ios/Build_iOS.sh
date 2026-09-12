#!/usr/bin/env bash
# ==============================================================================
# AUTO BUILD SCRIPT FOR MU ONLINE iOS (CLIENT)
# ==============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
CLIENT_DIR="${REPO_ROOT}/Source/5.Main"
OUTPUT_DIR="${SCRIPT_DIR}/outputs"
TOOLCHAIN="${SCRIPT_DIR}/ios.toolchain.cmake"

echo "================================================================================"
echo "          AUTO BUILD iOS CLIENT - MU ONLINE (SDL3 + VULKAN/METAL)"
echo "================================================================================"
echo " Repo Root  : ${REPO_ROOT}"
echo " Client Dir : ${CLIENT_DIR}"
echo " iOS Dir    : ${SCRIPT_DIR}"
echo " Toolchain  : ${TOOLCHAIN}"
echo "================================================================================"
echo ""

# 1. Check requirements
command -v cmake >/dev/null 2>&1 || { echo "[ERROR] cmake is required but not installed."; exit 1; }
command -v xcodebuild >/dev/null 2>&1 || { echo "[ERROR] Xcode command line tools are required."; exit 1; }

# 2. Select build target
BUILD_MODE="${1:-1}"
if [ -z "$1" ]; then
    echo "Select build mode:"
    echo "  [1] Real Device (arm64) - Generate Xcode & Build .ipa [Default]"
    echo "  [2] Simulator (arm64/x86_64) - Build .app for iOS Simulator"
    echo "  [3] Generate Xcode project only (.xcodeproj for Xcode GUI)"
    read -p ">> Enter choice [1-3, default 1]: " USER_CHOICE
    BUILD_MODE="${USER_CHOICE:-1}"
fi

BUILD_DIR=""
PLATFORM="OS64"
CONFIG="Release"

case "$BUILD_MODE" in
    1)
        PLATFORM="OS64"
        CONFIG="Release"
        BUILD_DIR="${SCRIPT_DIR}/build-device"
        ;;
    2)
        PLATFORM="SIMULATORARM64"
        CONFIG="Debug"
        BUILD_DIR="${SCRIPT_DIR}/build-simulator"
        ;;
    3)
        PLATFORM="OS64"
        CONFIG="Debug"
        BUILD_DIR="${SCRIPT_DIR}/build-xcode"
        ;;
    *)
        echo "[ERROR] Invalid choice: ${BUILD_MODE}"; exit 1 ;;
esac

echo ""
echo "=== Step 1: Generating Xcode project with CMake ==="
mkdir -p "${BUILD_DIR}"
mkdir -p "${OUTPUT_DIR}"

cmake -B "${BUILD_DIR}" -S "${CLIENT_DIR}" -G Xcode \
    -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" \
    -DPLATFORM="${PLATFORM}" \
    -DCMAKE_BUILD_TYPE="${CONFIG}" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="14.0"

if [ "$BUILD_MODE" == "3" ]; then
    echo ""
    echo "[SUCCESS] Xcode project generated at: ${BUILD_DIR}/Main.xcodeproj"
    echo "You can open it now by running: open '${BUILD_DIR}/Main.xcodeproj'"
    exit 0
fi

echo ""
echo "=== Step 2: Compiling iOS App via xcodebuild (${CONFIG}) ==="
if [ "$PLATFORM" == "OS64" ]; then
    xcodebuild -project "${BUILD_DIR}/Main.xcodeproj" \
        -scheme "Main" \
        -configuration "${CONFIG}" \
        -sdk iphoneos \
        -destination "generic/platform=iOS" \
        CODE_SIGNING_ALLOWED=NO \
        CODE_SIGNING_REQUIRED=NO \
        build
    
    APP_PATH="${BUILD_DIR}/${CONFIG}-iphoneos/MUOnline.app"
else
    xcodebuild -project "${BUILD_DIR}/Main.xcodeproj" \
        -scheme "Main" \
        -configuration "${CONFIG}" \
        -sdk iphonesimulator \
        -destination "generic/platform=iOS Simulator" \
        CODE_SIGNING_ALLOWED=NO \
        CODE_SIGNING_REQUIRED=NO \
        build
    
    APP_PATH="${BUILD_DIR}/${CONFIG}-iphonesimulator/MUOnline.app"
fi

if [ ! -d "${APP_PATH}" ]; then
    echo "[ERROR] MUOnline.app not found at: ${APP_PATH}"
    exit 1
fi

echo "[OK] Built application bundle: ${APP_PATH}"

# 3. Embed game assets if available
if [ -d "${REPO_ROOT}/Client/Data" ]; then
    echo ""
    echo "=== Step 3: Syncing game Data assets into App Bundle ==="
    mkdir -p "${APP_PATH}/Data"
    cp -R "${REPO_ROOT}/Client/Data/"* "${APP_PATH}/Data/" 2>/dev/null || true
    echo "[OK] Game Data synced into MUOnline.app/Data"
fi

# 4. Packaging IPA for Real Device
if [ "$PLATFORM" == "OS64" ]; then
    echo ""
    echo "=== Step 4: Packaging Payload into .ipa ==="
    PAYLOAD_DIR="${BUILD_DIR}/Payload"
    rm -rf "${PAYLOAD_DIR}"
    mkdir -p "${PAYLOAD_DIR}"
    cp -R "${APP_PATH}" "${PAYLOAD_DIR}/"
    
    IPA_PATH="${OUTPUT_DIR}/MUOnline_Client.ipa"
    rm -f "${IPA_PATH}"
    
    cd "${BUILD_DIR}"
    zip -q -r "${IPA_PATH}" Payload
    rm -rf "${PAYLOAD_DIR}"
    
    echo ""
    echo "================================================================================"
    echo "                   BUILD iOS IPA COMPLETED SUCCESSFULLY!"
    echo "================================================================================"
    echo " Output IPA: ${IPA_PATH}"
    echo " Dung lượng : $(du -h "${IPA_PATH}" | cut -f1)"
    echo ""
    echo " Hướng dẫn cài đặt:"
    echo "  1. TrollStore: Cài trực tiếp không cần ký (vĩnh viễn, không revoke)."
    echo "  2. AltStore / SideStore: Ký bằng Apple ID cá nhân."
    echo "  3. Sideloadly: Cài qua USB từ máy tính."
    echo "================================================================================"
else
    echo ""
    echo "================================================================================"
    echo "             BUILD iOS SIMULATOR APP COMPLETED SUCCESSFULLY!"
    echo "================================================================================"
    echo " Simulator App: ${APP_PATH}"
    echo " Để chạy trên Simulator: xcrun simctl install booted '${APP_PATH}'"
    echo "================================================================================"
fi
