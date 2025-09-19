
set(TARGET register_headers)

add_library(${TARGET} INTERFACE)
 
target_include_directories(${TARGET} INTERFACE
        ${REPO_ROOT_DIR}/.externs/repo/silibs/registerheader_overrides
        $ENV{REPO_APP_PATH_kingsgate_register_header}/ap_map
        $ENV{REPO_APP_PATH_kingsgate_register_header}/hsss_dfp_map
        $ENV{REPO_APP_PATH_kingsgate_register_header}/mcp_map
        $ENV{REPO_APP_PATH_kingsgate_register_header}/scp_map
        $ENV{REPO_APP_PATH_sdm_register_header}
)

add_library(ms::lib::register_headers ALIAS ${TARGET})