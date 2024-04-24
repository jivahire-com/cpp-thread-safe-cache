
set(TARGET register_headers)

add_library(${TARGET} INTERFACE)
 
target_include_directories(${TARGET} INTERFACE
        $ENV{REPO_APP_PATH_kng_soc_mason_csr_collateral}/ap_addr_map/registerheader/ap_addr_map/registerheader
        $ENV{REPO_APP_PATH_kng_soc_mason_csr_collateral}/hsss_dfp_addr_map/registerheader
        $ENV{REPO_APP_PATH_kng_soc_mason_csr_collateral}/mcp_addr_map/registerheader
        $ENV{REPO_APP_PATH_kng_soc_mason_csr_collateral}/scp_addr_map/registerheader
)
