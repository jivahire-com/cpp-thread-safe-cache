set(TARGET kng.mock.silibs_platform)

add_library(${TARGET} 
    STATIC 
        silibs_platform_mock.c
)

target_include_directories(
    ${TARGET} PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${REPO_ROOT_DIR}/.externs/repo/silibs/libraries/common/silibs_platform/include
)

target_link_libraries(
    ${TARGET}
    PRIVATE
        ms::cmocka::mocks
    PUBLIC
        IFPFW.EventTracing
        ms::lib::utils
        silibs::platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

# Call out what mock functions from the event trace mock we need
mock_target(${TARGET} 
    mmio_read32
    mmio_update32
    mmio_write32
)

add_library(ms::mocks::silibs_platform ALIAS ${TARGET})
