# ==============================================================================
# iOS CMake Toolchain
# Supports:
#   - PLATFORM=OS64 (Physical iOS devices, arm64)
#   - PLATFORM=SIMULATORARM64 (Apple Silicon Simulator, arm64)
#   - PLATFORM=SIMULATOR64 (Intel Simulator, x86_64)
# ==============================================================================

cmake_minimum_required(VERSION 3.20)

if(DEFINED CMAKE_CROSSCOMPILING)
    return()
endif()

set(CMAKE_SYSTEM_NAME iOS CACHE INTERNAL "")
set(UNIX TRUE CACHE BOOL "")
set(APPLE TRUE CACHE BOOL "")
set(IOS TRUE CACHE BOOL "")

# Default platform
if(NOT DEFINED PLATFORM)
    set(PLATFORM "OS64")
endif()

set(PLATFORM "${PLATFORM}" CACHE STRING "iOS Target Platform: OS64, SIMULATORARM64, SIMULATOR64")

# Deployment target
if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "Minimum iOS deployment version")
endif()

# Determine SDK and Architecture based on PLATFORM
if(PLATFORM STREQUAL "OS64")
    set(SDK_NAME "iphoneos")
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Target architectures")
elseif(PLATFORM STREQUAL "SIMULATORARM64")
    set(SDK_NAME "iphonesimulator")
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Target architectures")
elseif(PLATFORM STREQUAL "SIMULATOR64")
    set(SDK_NAME "iphonesimulator")
    set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "Target architectures")
else()
    message(FATAL_ERROR "Unsupported PLATFORM: ${PLATFORM}. Valid: OS64, SIMULATORARM64, SIMULATOR64")
endif()

# Find SDK root via xcrun
execute_process(
    COMMAND xcrun --sdk ${SDK_NAME} --show-sdk-path
    OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(NOT EXISTS "${CMAKE_OSX_SYSROOT}")
    message(WARNING "Could not find SDK path for ${SDK_NAME} via xcrun. Using default sysroot.")
    set(CMAKE_OSX_SYSROOT "${SDK_NAME}")
endif()

set(CMAKE_OSX_SYSROOT "${CMAKE_OSX_SYSROOT}" CACHE PATH "Path to iOS SDK")

# Set search paths
set(CMAKE_FIND_ROOT_PATH
    "${CMAKE_OSX_SYSROOT}"
    "${CMAKE_PREFIX_PATH}"
    CACHE STRING "Search path for iOS"
)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Compiler flags
set(CMAKE_C_FLAGS_INIT "-fobjc-arc -Wno-error")
set(CMAKE_CXX_FLAGS_INIT "-fobjc-arc -Wno-error")

# Code signing settings for Xcode generator
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO" CACHE STRING "")
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "YES" CACHE STRING "")
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "" CACHE STRING "")
set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "" CACHE STRING "")

message(STATUS "=== iOS Toolchain Configured ===")
message(STATUS "Platform:      ${PLATFORM}")
message(STATUS "Architecture:  ${CMAKE_OSX_ARCHITECTURES}")
message(STATUS "SDK:           ${SDK_NAME}")
message(STATUS "Sysroot:       ${CMAKE_OSX_SYSROOT}")
message(STATUS "Deploy Target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
message(STATUS "================================")
