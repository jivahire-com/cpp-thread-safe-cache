# Copyright (c) Microsoft Corporation. All rights reserved.

message("----------------------------------")
message("-- Adding Silibs KNG Platform ----")

set(TARGET extern.silibs.kng_silibs_platform)

add_library(${TARGET} INTERFACE)

target_include_directories(
    ${TARGET}
        SYSTEM INTERFACE
            ${TARGET_DIR}/libraries/kng_silibs_platform/include
            ${TARGET_DIR}/soc_fw_shared/include
            ${TARGET_DIR}/soc_fw_shared/include/memory_map

)

target_link_libraries(${TARGET}
    INTERFACE
        silibs::platform
        ms::lib::register_headers
)

add_library(kng_silibs_platform ALIAS ${TARGET})
add_library(silibs::kng_silibs_platform ALIAS ${TARGET})
