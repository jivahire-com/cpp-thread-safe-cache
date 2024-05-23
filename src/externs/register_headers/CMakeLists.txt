
set(TARGET register_headers)

add_library(${TARGET} INTERFACE)
 
target_include_directories(${TARGET} INTERFACE
        $ENV{REPO_APP_PATH_kingsgate_register_header}/ap_map
        $ENV{REPO_APP_PATH_kingsgate_register_header}/hsss_dfp_map
        $ENV{REPO_APP_PATH_kingsgate_register_header}/mcp_map
        $ENV{REPO_APP_PATH_kingsgate_register_header}/scp_map
)

add_library(ms::lib::register_headers ALIAS ${TARGET})