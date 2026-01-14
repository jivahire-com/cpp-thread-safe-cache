# Copyright (c) Microsoft Corporation. All rights reserved.


set(TARGET fuse_artifacts_lib)

add_library(${TARGET} INTERFACE)
 
target_include_directories(${TARGET} INTERFACE
        $ENV{REPO_APP_PATH_kng_system_fuse_data}/A1/
)

add_library(ms::lib::fuse_artifacts_lib ALIAS ${TARGET})