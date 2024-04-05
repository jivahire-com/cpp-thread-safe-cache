include(FetchContent)

set(TARGET extern.populate.threads)
set(TARGET_URL $ENV{REPO_APP_GIT_THREADX.URL})
set(TARGET_DIR ${REPO_ROOT_DIR}/.externs/repo/threadx)

# Add special compile options for subdirectory
add_compile_options(-w)

FetchContent_Declare(
    ${TARGET}
    GIT_REPOSITORY ${TARGET_URL}
    GIT_TAG        $ENV{REPO_APP_GIT_THREADX.COMMIT}
    SOURCE_DIR     ${TARGET_DIR}
)

FetchContent_GetProperties(${TARGET})

if (NOT ${TARGET}_POPULATED)
    FetchContent_Populate(${TARGET})
endif()

#
# We use Threadx in a few ways:
#
# 1. As a static library built for the platform architecture (cortex_m7). Used for Production.
# 2. As a static library built for win32. Used for tests that require threadx to be initialized.
#
# Each path is built separately for it's toolchain and each path has the alias set so linking 
# is all set.
#
set(TARGET_ALIAS ms::lib::threadx)
add_subdirectory(threadx_lib)
