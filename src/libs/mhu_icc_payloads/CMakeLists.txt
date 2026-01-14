# Copyright (c) Microsoft Corporation. All rights reserved.


set(TARGET mhu_icc_payloads)

add_library(${TARGET} INTERFACE)

target_link_libraries(${TARGET}
    INTERFACE
        ms::lib::utils
        ms::service::atu_service
)

target_include_directories(
    ${TARGET}
        INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(ms::lib::interface::mhu_icc_payloads ALIAS ${TARGET})
