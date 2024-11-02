
set(TARGET fuse_artifacts)

add_library(${TARGET} INTERFACE)
 
target_include_directories(${TARGET} INTERFACE
        $ENV{REPO_APP_PATH_kng_system_fuse_data}/
)

add_library(ms::lib::fuse_artifacts ALIAS ${TARGET})