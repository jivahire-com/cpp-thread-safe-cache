set(TARGET kng.lib.mesh)

add_library(${TARGET} 
    STATIC 
        src/mesh.c
)

target_link_libraries(${TARGET} 
    PRIVATE
    cmn800
    cmn800_skus
    cmn800_sequence
    idsw
    fwlib.assert
    kng_silibs_platform
    ppu_v1
    silibs_platform
)

target_include_directories(${TARGET}
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
        ${REPO_ROOT_DIR}/.externs/azpkg/kingsgate_register_header.0.9.3/ap_addr_map/registerheader
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::mesh ALIAS ${TARGET})
