message("----------------------------------")
message("- Adding Silibs Common Override --")

set(TARGET extern.silibs.silibs_platform)

set(SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/silibs_platform/src/silibs_platform.c
)

add_library(${TARGET} STATIC ${SRCS})

target_include_directories(
    ${TARGET}
        SYSTEM PUBLIC
            ${TARGET_DIR}/libraries/common/silibs_platform/include
        SYSTEM INTERFACE
            $ENV{REPO_APP_PATH_kng_soc_mason_csr_collateral}/ap_addr_map/registerheader
            $ENV{REPO_APP_PATH_kng_soc_mason_csr_collateral}/hsss_dfp_addr_map/registerheader
            $ENV{REPO_APP_PATH_kng_soc_mason_csr_collateral}/mcp_addr_map/registerheader
            $ENV{REPO_APP_PATH_kng_soc_mason_csr_collateral}/scp_addr_map/registerheader
)

target_link_libraries(${TARGET}
    PUBLIC
)

lint_target(${TARGET})
format_target(${TARGET})

#package as silibs_platform due to kng_silibs_platform dependency within silibs
add_library(silibs_platform ALIAS ${TARGET})
add_library(silibs::platform ALIAS ${TARGET})