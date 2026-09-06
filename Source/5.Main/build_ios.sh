#!/usr/bin/env bash
set -euo pipefail

# ==============================================================================
# SCRIPT BUILD MU CLIENT CHO APPLE IOS QUA CMAKE + XCODE
# ==============================================================================
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build-ios"
CONFIGURATION="${1:-Release}"

echo "================================================================================"
echo "                   BUILD MU CLIENT (APPLE IOS) BANG CMAKE"
echo "================================================================================"
echo " Source Dir : ${SCRIPT_DIR}"
echo " Build Dir  : ${BUILD_DIR}"
echo " Config     : ${CONFIGURATION}"
echo "================================================================================"

# Tạo Xcode project cho iOS
cmake -B "${BUILD_DIR}" -S "${SCRIPT_DIR}" -G Xcode \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 \
    -DCMAKE_BUILD_TYPE="${CONFIGURATION}"

echo ""
echo "[V] Tao Xcode project thanh cong tai:"
echo "    ${BUILD_DIR}/MuOnlineClient.xcodeproj"
echo ""
echo "Lenh bien dich tu terminal:"
echo "    cmake --build ${BUILD_DIR} --config ${CONFIGURATION}"