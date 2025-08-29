set(TARGET kng.lib.pldm_common_headers)

add_library(${TARGET} INTERFACE)

target_link_libraries(${TARGET}
    INTERFACE
        silibs::icc_mhu
)

target_include_directories(${TARGET}
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
add_library(ms::lib::pldm_common_headers ALIAS ${TARGET})

