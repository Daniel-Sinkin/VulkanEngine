cmake_minimum_required(VERSION 3.10)

project(VulkanEngine)

# Export compile commands & C++ standard
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------------------------------------------
# macOS / Apple Silicon defaults (can be overridden by user/toolchain)
# -------------------------------------------------------------------
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    # Default to arm64 on Apple Silicon
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures for macOS" FORCE)
endif()

# ----------------------------------------------------
# Vulkan SDK (MoltenVK) – expect VULKAN_SDK to be set
# ----------------------------------------------------
if(NOT DEFINED ENV{VULKAN_SDK})
    message(FATAL_ERROR "VULKAN_SDK environment variable is not set")
endif()

set(VULKAN_SDK "$ENV{VULKAN_SDK}")
set(VULKAN_INCLUDE_DIR "${VULKAN_SDK}/include")
set(VULKAN_LIBRARY_DIR "${VULKAN_SDK}/lib")
include_directories(${VULKAN_INCLUDE_DIR})

# ---------------------------------
# Packages: Vulkan, SDL3, and glm
# ---------------------------------
# Tip: on Homebrew macOS, CMAKE_PREFIX_PATH often needs /opt/homebrew
if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew" "/usr/local")
endif()

find_package(Vulkan REQUIRED)
find_package(SDL3 REQUIRED CONFIG)   # replaces GLFW
find_package(glm REQUIRED)

# ---------------------------------
# Project sources
# ---------------------------------
file(GLOB SOURCES "src/*.cpp")

# ---------------------------------
# Dear ImGui (core + SDL3/Vulkan backends)
# ---------------------------------
# You can override IMGUI_DIR at configure time:
#   -DIMGUI_DIR=/path/to/imgui
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/external/imgui" CACHE PATH "Path to Dear ImGui root")

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

# Validate ImGui presence a bit
foreach(_f IN LISTS IMGUI_SOURCES)
    if(NOT EXISTS "${_f}")
        message(FATAL_ERROR "ImGui source not found: ${_f}\nSet IMGUI_DIR to your Dear ImGui root.")
    endif()
endforeach()

include_directories(
    include
    external          # suppress warnings from external libs in your setup
    ${GLM_INCLUDE_DIRS}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# --------------------
# Executable + linking
# --------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
add_executable(VulkanEngine
    ${SOURCES}
    ${IMGUI_SOURCES}
)

target_link_libraries(VulkanEngine
    PRIVATE
        SDL3::SDL3
        Vulkan::Vulkan
)

# macOS frameworks (Makefile links Cocoa, IOKit, CoreVideo)
if(APPLE)
    # MoltenVK: prefer a clean find over hardcoding the dylib
    find_library(MOLTENVK_LIB MoltenVK HINTS "${VULKAN_LIBRARY_DIR}" NO_DEFAULT_PATH)
    if(MOLTENVK_LIB)
        target_link_libraries(VulkanEngine PRIVATE "${MOLTENVK_LIB}")
    endif()

    # These are harmless if already linked transitively by SDL3, but ensure parity with the Makefile
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(IOKIT_FRAMEWORK IOKit)
    find_library(COREVIDEO_FRAMEWORK CoreVideo)
    target_link_libraries(VulkanEngine PRIVATE
        ${COCOA_FRAMEWORK}
        ${IOKIT_FRAMEWORK}
        ${COREVIDEO_FRAMEWORK}
    )
endif()

# -------------
# Build types
# -------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Disable IPO/LTO in Debug like your original
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)
endif()

# Baseline warnings to mirror the Makefile’s -Wall -Wformat
target_compile_options(VulkanEngine PRIVATE
    $<$<CXX_COMPILER_ID:Clang,GNU>:-Wall -Wformat>
)

# Optional extra diagnostics / sanitizers with Clang in Debug
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_link_options(VulkanEngine PRIVATE
            -fsanitize=address
            -fsanitize=undefined
        )
    endif()
endif()