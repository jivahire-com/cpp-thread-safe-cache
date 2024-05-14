cmake_minimum_required(VERSION 3.24)

project(kingsgate.mscp VERSION $ENV{MajorVersion}.$ENV{MinorVersion}.$ENV{PatchVersion}.0 LANGUAGES C CXX ASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
set(CMAKE_OBJECT_PATH_MAX 260) # set Max path length on CMake to be higher then default for Windows build


# Base directories
set(REPO_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)

# Python3 executable path
set (Python3_EXECUTABLE $ENV{REPO_APP_PATH_python.win64}/tools/python.exe)

# Build type defines
add_compile_options(
  "$<$<CONFIG:RELEASE>:-DNDEBUG>"
  "$<$<CONFIG:DEBUG>:-DDEBUG>"
)

# Toolchain scripts
include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/lint.cmake)
include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/format.cmake)
include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/coverage.cmake)
include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/mock.cmake)

# Repo Scripts
include(${REPO_ROOT_DIR}/tools/cmakes/scripts/unit_test.cmake)

# Build sources
add_subdirectory(src)
add_subdirectory(tests)
