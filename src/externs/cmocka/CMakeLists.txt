enabled_on(i386-pc-windows-msvc)

include(FetchContent)

set(TARGET extern.populate.cmocka)
set(TARGET_URL $ENV{REPO_APP_GIT_CMOCKA.URL})
set(TARGET_DIR ${REPO_ROOT_DIR}/.externs/repo/cmocka)

# Options copied from cmocka/src/DefineOptions.cmake. 
option(WITH_STATIC_LIB "Build with a static library" OFF)
option(WITH_SHARED_LIB "Build with a shared library" ON)
option(WITH_CMOCKERY_SUPPORT "Install a cmockery header" OFF)
option(WITH_EXAMPLES "Build examples" ON)
option(UNIT_TESTING "Build with unit testing" OFF)
option(PICKY_DEVELOPER "Build with picky developer flags" OFF)
option(WITH_POSITION_INDEPENDENT_CODE "Build with PIC/PIE" ON)
option(WITH_TINY_CONFIG "Build with tiny config" OFF)

# Set desired options to build the cmocka library
set(HAVE_STRSIGNAL 0)
set(HAVE_SIGLONGJMP 0)
set(HAVE_SNPRINTF 1)
set(HAVE_VSNPRINTF 1)
set(HAVE_MALLOC_H 0)
set(WITH_STATIC_LIB ON)
set(WITH_SHARED_LIB OFF)
set(WITH_EXAMPLES OFF)
set(UNIT_TESTING OFF)
set(CMAKE_COMPILER_IS_GNUCC OFF)
set(WITH_POSITION_INDEPENDENT_CODE "Build with PIC/PIE" OFF)

# Add special compile options for subdirectory
add_compile_options(-Wno-deprecated-declarations -Wno-invalid-noreturn)

FetchContent_Declare(
    ${TARGET}
    GIT_REPOSITORY ${TARGET_URL}
    GIT_TAG        $ENV{REPO_APP_GIT_CMOCKA.TAG}
    SOURCE_DIR     ${TARGET_DIR}
)

FetchContent_GetProperties(${TARGET})

if (NOT ${TARGET}_POPULATED)
    FetchContent_Populate(${TARGET})
    add_subdirectory(${TARGET_DIR} ${CMAKE_CURRENT_BINARY_DIR}/src/cmocka)
endif()

add_subdirectory(cmocka)
add_subdirectory(wrapper)