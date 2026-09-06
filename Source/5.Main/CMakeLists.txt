cmake_minimum_required(VERSION 3.24)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(Main LANGUAGES C CXX)
if(WIN32 AND NOT ANDROID AND NOT (CMAKE_SYSTEM_NAME STREQUAL "Android"))
    enable_language(RC)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Platform and Architecture Check
# ---------------------------------------------------------------------------
if(WIN32 AND NOT ANDROID)
    message(STATUS "Target system: Windows (${CMAKE_SYSTEM_PROCESSOR} / CMAKE_SIZEOF_VOID_P=${CMAKE_SIZEOF_VOID_P})")
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        message(STATUS "Architecture: Win32 (x86 32-bit) - Compatible with bundled 32-bit libraries")
    else()
        message(WARNING "Architecture is 64-bit (x64). Note: bundled .lib in dependencies/lib are 32-bit x86. Consider generating with -A Win32")
    endif()
endif()

# ---------------------------------------------------------------------------
# Preprocessor Definitions and Compiler Options
# ---------------------------------------------------------------------------
if(ANDROID)
    add_compile_definitions(
        __ANDROID__
        ANDROID
        _LANGUAGE_FOREIGN
        _LANGUAGE_ENG
        LDS_PATCH_GLOBAL_100520
        PROTO_EXTRA
        MAIN_UPDATE=603
        FT2_BUILD_LIBRARY
        $<$<CONFIG:Debug>:DEBUG>
        $<$<CONFIG:Debug>:_FOREIGN_DEBUG>
        $<$<CONFIG:Debug>:CONSOLE_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
        $<$<CONFIG:Release>:_FOREIGN_NDEBUG>
    )
else()
    add_compile_definitions(
        WIN32
        _WINDOWS
        _USE_32BIT_TIME_T
        _LANGUAGE_FOREIGN
        _LANGUAGE_ENG
        LDS_PATCH_GLOBAL_100520
        PROTO_EXTRA
        MAIN_UPDATE=603
        FT2_BUILD_LIBRARY
        _CRT_SECURE_NO_WARNINGS
        _CRT_NONSTDC_NO_DEPRECATE
        _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_WARNING
        _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
        _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
        $<$<CONFIG:Debug>:DEBUG>
        $<$<CONFIG:Debug>:_FOREIGN_DEBUG>
        $<$<CONFIG:Debug>:CONSOLE_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
        $<$<CONFIG:Release>:_FOREIGN_NDEBUG>
    )
endif()

if(ANDROID OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
        -Wno-error
        -Wno-format-security
        -Wno-narrowing
        -Wno-writable-strings
        -Wno-int-to-pointer-cast
        -Wno-invalid-source-encoding
    )
endif()

if(ANDROID)
    add_link_options(-Wl,-z,max-page-size=16384)
endif()

if(MSVC)
    # Set static CRT (/MT) matching Main.vcxproj configuration
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

    add_compile_options(
        /EHsc
        /permissive
        /Zc:wchar_t
        /W0
        /MP
    )
    add_link_options(
        /LARGEADDRESSAWARE
        /SUBSYSTEM:WINDOWS
    )
endif()

# ---------------------------------------------------------------------------
# Vulkan SDK Setup
# ---------------------------------------------------------------------------
find_package(Vulkan REQUIRED)
message(STATUS "Found Vulkan SDK: ${Vulkan_INCLUDE_DIRS}")

# Look for shader compiler (glslc or glslangValidator) in Vulkan SDK
find_program(VULKAN_GLSLC glslc HINTS ${Vulkan_GLSLC_EXECUTABLE} "$ENV{VULKAN_SDK}/Bin")
find_program(VULKAN_GLSLANG glslangValidator HINTS ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} "$ENV{VULKAN_SDK}/Bin")

set(SHADER_COMPILER "")
if(VULKAN_GLSLC)
    set(SHADER_COMPILER "${VULKAN_GLSLC}")
    set(SHADER_FLAGS "")
elseif(VULKAN_GLSLANG)
    set(SHADER_COMPILER "${VULKAN_GLSLANG}")
    set(SHADER_FLAGS "-V")
endif()

# ---------------------------------------------------------------------------
# SDL3 Integration (via FetchContent)
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.2.4
    GIT_SHALLOW    TRUE
)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL3)
message(STATUS "SDL3 integrated successfully")

if(ANDROID)
    if(TARGET SDL3-shared)
        target_link_options(SDL3-shared PRIVATE -Wl,-z,max-page-size=16384)
        set_target_properties(SDL3-shared PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../android/app/src/main/jniLibs/${ANDROID_ABI}"
        )
    endif()
    if(TARGET SDL3)
        target_link_options(SDL3 PRIVATE -Wl,-z,max-page-size=16384)
        set_target_properties(SDL3 PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../android/app/src/main/jniLibs/${ANDROID_ABI}"
        )
    endif()
    FetchContent_Declare(
        freetype
        GIT_REPOSITORY https://github.com/freetype/freetype.git
        GIT_TAG        VER-2-13-2
        GIT_SHALLOW    TRUE
    )
    set(FT_DISABLE_ZLIB TRUE CACHE BOOL "" FORCE)
    set(FT_DISABLE_BZIP2 TRUE CACHE BOOL "" FORCE)
    set(FT_DISABLE_PNG TRUE CACHE BOOL "" FORCE)
    set(FT_DISABLE_HARFBUZZ TRUE CACHE BOOL "" FORCE)
    set(FT_DISABLE_BROTLI TRUE CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(freetype)
endif()

# ---------------------------------------------------------------------------
# Source Files Collection
# ---------------------------------------------------------------------------
file(GLOB_RECURSE SOURCEMAIN_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.c"
)

file(GLOB_RECURSE SOURCEMAIN_HEADERS
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.hpp"
)

# Exclude standalone / mobile / unused files
list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "ExternalObject/detours\\.cpp$")
list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "Utilities/Dump/CrashHandler\\.cpp$")
list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "Utilities/Dump/CrashReporter\\.cpp$")
list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "Utilities/Dump/Logger\\.cpp$")
list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "Utilities/Dump/Uploader\\.cpp$")
list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "android_main\\.cpp$")
list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "android_link_stubs\\.cpp$")
if(NOT ANDROID)
    list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "android_turbojpeg_stubs\\.cpp$")
endif()
list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "Platform/AndroidGDI\\.cpp$")
list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "Platform/MobilePlatform\\.cpp$")
list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "Platform/MobileTime\\.cpp$")
list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "Platform/gl_compat\\.cpp$")
list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "Platform/SokolRuntime\\.cpp$")
list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "Platform/RenderBackend\\.cpp$")
list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "UIWindows - Copy\\.cpp$")
list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "Utilities/cryptopp/.*")
list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "Utilities/mapm/.*")
list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "Utilities/Math\\.cpp$")
list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "Scenes/.*\\.cpp$")
list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "Camera/.*\\.cpp$")
list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "CSResourceManager\\.cpp$")
list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "VulkanHTMLNotice\\.cpp$")

if(ANDROID)
    list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "ExternalObject/Leaf/.*\\.cpp$")
    list(APPEND SOURCEMAIN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/source/ExternalObject/Leaf/xstreambuf.cpp")
    list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "GCCertification\\.cpp$")
    list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "StackWalker.*\\.cpp$")
    list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "TrayMode\\.cpp$")
    list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "Utilities/Log/WindowsConsole\\.cpp$")
    list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "ProtectSysKey.*\\.cpp$")
    list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "WSctlc\\.cpp$")
    list(FILTER SOURCEMAIN_SOURCES EXCLUDE REGEX "Stack\\.cpp$")
endif()

# Resource File
set(RESOURCE_FILES "")
if(WIN32 AND NOT ANDROID)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/source/resource.rc")
        set(RESOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/source/resource.rc")
    endif()
endif()

# Combine all sources
set(ALL_PROJECT_SOURCES
    ${SOURCEMAIN_SOURCES}
    ${SOURCEMAIN_HEADERS}
    ${RESOURCE_FILES}
)

# ---------------------------------------------------------------------------
# Target Definition
# ---------------------------------------------------------------------------
if(ANDROID)
    add_library(${PROJECT_NAME} SHARED ${ALL_PROJECT_SOURCES})
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "main")
else()
    add_executable(${PROJECT_NAME} WIN32 ${ALL_PROJECT_SOURCES})
endif()

# ---------------------------------------------------------------------------
# Precompiled Headers (PCH)
# ---------------------------------------------------------------------------
if(WIN32 AND NOT ANDROID)
    target_precompile_headers(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/source/stdafx.h"
    )
endif()

# ---------------------------------------------------------------------------
# Include Directories
# ---------------------------------------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/source"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/Platform"
    "${CMAKE_CURRENT_SOURCE_DIR}/boost_1_80_0"
    "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/include/glm"
    "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/include/turbojpeg"
    "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/include/asio"
    ${Vulkan_INCLUDE_DIRS}
)

if(ANDROID)
    target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../../android/app/src/main/cpp/compat_include"
    )
endif()

# ---------------------------------------------------------------------------
# Link Libraries & Directories
# ---------------------------------------------------------------------------
if(ANDROID)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        android
        log
        OpenSLES
        vulkan
        SDL3::SDL3
        freetype
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../android/app/src/main/jniLibs/${ANDROID_ABI}"
    )
else()
    target_link_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/source"
        "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/lib"
    )

    # Windows System Libraries (100% PURE - ZERO OpenGL / GLU)
    set(SYSTEM_LIBS
        ws2_32
        winmm
        imm32
        vfw32
        dsound
        dxguid
        version
        dbghelp
        shlwapi
        wininet
    )

    # Precompiled dependencies in dependencies/lib/
    set(DEPENDENCY_LIBS
        "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/lib/ShareMemory.lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/lib/SimpleModulus.lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/lib/wzAudio.lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/lib/freetype.lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/lib/turbojpeg.lib"
    )

    # Vulkan Library (Win32 x86 uses 32-bit import lib in dependencies/lib)
    if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(VULKAN_TARGET_LIB "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/lib/vulkan-1.lib")
    else()
        set(VULKAN_TARGET_LIB Vulkan::Vulkan)
    endif()

    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${SYSTEM_LIBS}
        ${DEPENDENCY_LIBS}
        ${VULKAN_TARGET_LIB}
        SDL3::SDL3
    )

    if(MSVC)
        target_link_options(${PROJECT_NAME} PRIVATE
            /NODEFAULTLIB:libc.lib
            /NODEFAULTLIB:MSVCRT.lib
            /NODEFAULTLIB:MSVCRTD.lib
            "/MANIFESTUAC:level='requireAdministrator' uiAccess='false'"
        )
    endif()

    # Output Directory
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../../ClientBuild_192.168.99.200"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/../../ClientBuild_192.168.99.200"
    )
endif()

# ---------------------------------------------------------------------------
# ---------------------------------------------------------------------------
# SPIR-V Shaders Compilation (All shaders in Shaders/ directory)
# ---------------------------------------------------------------------------
file(GLOB ALL_GLSL_SHADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.vert.glsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.frag.glsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.comp.glsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.comp"
)

if(SHADER_COMPILER AND ALL_GLSL_SHADERS)
    set(SPV_OUTPUTS "")
    foreach(SHADER_SRC ${ALL_GLSL_SHADERS})
        get_filename_component(SHADER_NAME ${SHADER_SRC} NAME)
        string(REGEX REPLACE "\\.glsl$" "" SHADER_BASE ${SHADER_NAME})
        set(SHADER_OUT_NAME "${SHADER_BASE}.spv")
        set(SHADER_SPV "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/${SHADER_OUT_NAME}")

        set(STAGE_FLAG "")
        if(VULKAN_GLSLC)
            if(SHADER_NAME MATCHES "\\.vert\\.glsl$")
                set(STAGE_FLAG "-fshader-stage=vertex")
            elseif(SHADER_NAME MATCHES "\\.frag\\.glsl$")
                set(STAGE_FLAG "-fshader-stage=fragment")
            elseif(SHADER_NAME MATCHES "\\.comp(\\.glsl)?$")
                set(STAGE_FLAG "-fshader-stage=compute")
            endif()
        elseif(VULKAN_GLSLANG)
            if(SHADER_NAME MATCHES "\\.vert\\.glsl$")
                set(STAGE_FLAG "-S vert")
            elseif(SHADER_NAME MATCHES "\\.frag\\.glsl$")
                set(STAGE_FLAG "-S frag")
            elseif(SHADER_NAME MATCHES "\\.comp(\\.glsl)?$")
                set(STAGE_FLAG "-S comp")
            endif()
        endif()

        add_custom_command(
            OUTPUT ${SHADER_SPV}
            COMMAND ${SHADER_COMPILER} ${SHADER_FLAGS} ${STAGE_FLAG} ${SHADER_SRC} -o ${SHADER_SPV}
            DEPENDS ${SHADER_SRC}
            COMMENT "Compiling SPIR-V shader: ${SHADER_NAME} -> ${SHADER_OUT_NAME}"
        )
        list(APPEND SPV_OUTPUTS ${SHADER_SPV})
    endforeach()

    if(SPV_OUTPUTS)
        find_package(Python3 QUIET COMPONENTS Interpreter)
        if(NOT Python3_EXECUTABLE)
            set(Python3_EXECUTABLE "python")
        endif()

        add_custom_target(CompileShaders ALL DEPENDS ${SPV_OUTPUTS}
            COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/generate_embedded_shaders.py"
            COMMENT "Embedding all SPIR-V shaders into SourceMain/EmbeddedShaders.h"
        )
        add_dependencies(${PROJECT_NAME} CompileShaders)
    endif()
endif()

# ---------------------------------------------------------------------------
# Post-Build Steps: Copy SDL3.dll to Output Directory
# ---------------------------------------------------------------------------
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3-shared>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying SDL3.dll to output directory"
    )
endif()

