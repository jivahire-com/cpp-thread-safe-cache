set(TARGET 1pfw_fwlibs)
set(TARGET_DIR ${REPO_ROOT_DIR}/.externs/repo/1pfw_fwlibs)

# Disable warnings for shared libs
add_compile_options(-w)

# Options for cmake module
option(FPFW_ENABLE_TESTS "Compiles unit tests" OFF)
option(FPFW_ENABLE_COVERAGE "Enables code coverage in compilation" OFF)
option(FPFW_ENABLE_LINT "Enables additional lints during compilation" OFF)
option(FPFW_ENABLE_DOCS "Creates doxygen documentation" OFF)

# Generic threadx lock port provided by fwlibs
set(FPFW_PLATFORM_IMPL_LOCK "fwlib.platform.lock.threadx")

# Generic ports implmented within this project
set(FPFW_PLATFORM_IMPL_THREADX "ms::lib::threadx")

include(FetchContent)
FetchContent_Declare(
    ${TARGET}
    GIT_REPOSITORY $ENV{REPO_APP_GIT_1PFW_FWLIBS.URL}
    GIT_TAG        $ENV{REPO_APP_GIT_1PFW_FWLIBS.TAG}
    SOURCE_DIR     ${TARGET_DIR}
)

FetchContent_GetProperties(${TARGET})

if (NOT ${TARGET}_POPULATED)
    FetchContent_Populate(${TARGET})    
    add_subdirectory(${TARGET_DIR} ${CMAKE_CURRENT_BINARY_DIR}/src/1pfw.fwlibs EXCLUDE_FROM_ALL)
endif()

include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/lint.cmake)
include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/format.cmake)
include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/coverage.cmake)
include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/mock.cmake)
