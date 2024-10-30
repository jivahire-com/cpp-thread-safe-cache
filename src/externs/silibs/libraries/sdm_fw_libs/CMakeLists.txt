# Copyright (c) Microsoft Corporation. All rights reserved.

message("----------------------------------")
message("----- Adding Silibs SDM LIBS -----")


set(TARGET_DIR ${REPO_ROOT_DIR}/.externs/repo/silibs/libraries/sdm)
set(LIB_OUTS_DIR ${CMAKE_BINARY_DIR}/src/externs/silibs/libraries/sdm_fw_libs)

set(SDM_FW_LIBS_LIST
        sdm_command_desc)

foreach(SDM_FW_LIB IN LISTS SDM_FW_LIBS_LIST)
    message("Adding sdm_fw_libs/${SDM_FW_LIB} ...")
    add_subdirectory(${TARGET_DIR}/libraries/${SDM_FW_LIB} ${LIB_OUTS_DIR}/${SDM_FW_LIB})
endforeach()

message("----------------------------------")
