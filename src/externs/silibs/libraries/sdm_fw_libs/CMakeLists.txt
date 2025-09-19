# Copyright (c) Microsoft Corporation. All rights reserved.

message("----------------------------------")
message("----- Adding Silibs SDM LIBS -----")


set(TARGET_DIR ${REPO_ROOT_DIR}/.externs/repo/silibs/libraries/sdm)
set(LIB_OUTS_DIR ${CMAKE_BINARY_DIR}/src/externs/silibs/libraries/sdm_fw_libs)

add_compile_options(-I${REPO_ROOT_DIR}/.externs/repo/silibs/libraries/common/silibs_platform/include
                    -I$ENV{REPO_APP_PATH_sdm_register_header}
)

set(SDM_FW_LIBS_LIST
        sdm_command_desc
        sdm_fw_libs_platform
        sdm_qos)

foreach(SDM_FW_LIB IN LISTS SDM_FW_LIBS_LIST)
    message("Adding sdm_fw_libs/${SDM_FW_LIB} ...")
    add_subdirectory(${TARGET_DIR}/libraries/${SDM_FW_LIB} ${LIB_OUTS_DIR}/${SDM_FW_LIB})
endforeach()

message("----------------------------------")
