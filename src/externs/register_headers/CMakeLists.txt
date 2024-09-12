
set(TARGET register_headers)

add_library(${TARGET} INTERFACE)
 
target_include_directories(${TARGET} INTERFACE
        ${REPO_ROOT_DIR}/.externs/repo/silibs/registerheader_overrides
        $ENV{REPO_APP_PATH_kingsgate_register_header}/ap_addr_map/registerheader
        $ENV{REPO_APP_PATH_kingsgate_register_header}/hsss_dfp_addr_map/registerheader
        $ENV{REPO_APP_PATH_kingsgate_register_header}/mcp_addr_map/registerheader
        $ENV{REPO_APP_PATH_kingsgate_register_header}/scp_addr_map/registerheader
)

add_library(ms::lib::register_headers ALIAS ${TARGET})