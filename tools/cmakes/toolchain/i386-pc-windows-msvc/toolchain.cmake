# Set target
set(CLANG_TARGET_TRIPLE i386-pc-windows-msvc)

include($ENV{REPO_APP_ROOT}/tools/cmakes/scripts/enabled_on.cmake)
set_valid_enable_flags(${CLANG_TARGET_TRIPLE})

# For LLVM linker instead of MSVC for coverage compatibility
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-fuse-ld=lld")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld")
set(CMAKE_EXECUTABLE_SUFFIX_C ".exe")
set(CMAKE_EXECUTABLE_SUFFIX_CXX ".exe")

# In order for clang to locate MSVC some enviroment variables are required
set(CMAKE_C_COMPILER "${CMAKE_SOURCE_DIR}/tools/cmakes/toolchain/${CLANG_TARGET_TRIPLE}/clang.cmd")
set(CMAKE_CXX_COMPILER "${CMAKE_SOURCE_DIR}/tools/cmakes/toolchain/${CLANG_TARGET_TRIPLE}/clang++.cmd")
set(CMAKE_RC_COMPILER $ENV{REPO_APP_PATH_llvm.win64}/bin/llvm-rc.exe)
set(CMAKE_AR $ENV{REPO_APP_PATH_llvm.win64}/bin/llvm-ar.exe)
set(CMAKE_RANLIB $ENV{REPO_APP_PATH_llvm.win64}/bin/llvm-ranlib.exe)
set(CMAKE_STRIP "")

set(CMAKE_C_COMPILER_AR "${CMAKE_AR}")
set(CMAKE_CXX_COMPILER_AR "${CMAKE_AR}")
set(CMAKE_C_COMPILER_RANLIB "${CMAKE_RANLIB}")
set(CMAKE_CXX_COMPILER_RANLIB "${CMAKE_RANLIB}")

# Set compilation to enable debug symbols
# Set compilation to respect target architecture
set(CMAKE_C_FLAGS "-g -m32 --target=${CLANG_TARGET_TRIPLE} -Wall -Wextra -Werror -Wno-missing-field-initializers")
set(CMAKE_CXX_FLAGS "-g -m32 --target=${CLANG_TARGET_TRIPLE} -Wall -Wextra -Werror -Wno-missing-field-initializers")

# Set repo utilities
# set(REPO_CLANG_TIDY "${CMAKE_SOURCE_DIR}/tools/cmakes/toolchain/${CLANG_TARGET_TRIPLE}/clang-tidy.cmd" --quiet)
set(REPO_IWYU "${CMAKE_SOURCE_DIR}/tools/cmakes/toolchain/${CLANG_TARGET_TRIPLE}/iwyu.cmd" "-Xiwyu" "--quoted_includes_first" "-Xiwyu" "--mapping_file=${CMAKE_SOURCE_DIR}/.iwyu")
set(REPO_CLANG_FORMAT $ENV{REPO_APP_PATH_llvm.win64}/bin/clang-format.exe)

# Enable features
option(REPO_ENABLE_TESTS "Compiles unit tests" ON)
option(REPO_ENABLE_COVERAGE "Enables code coverage in compilation" ON)
