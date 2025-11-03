set(TARGET ddr_manager_library)
set(LIBS PUBLIC fwlib.dfwk fwlib.lib.assert)

# Add target for the service
add_library(${TARGET}
    STATIC
        ddr_manager_acpi_smbios_tables.c
        ddr_manager_bwl.c
        ddr_manager_events.c
        ddr_manager_gpio.c
        ddr_manager_i3c.c
        ddr_manager_memory_map.c
        ddr_manager_platform.c
        ddr_manager_polling.c
        ddr_manager_rhtlm_service.c
        ddr_manager_telemetry.c
        ddr_manager_top.c
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
        ${REPO_ROOT_DIR}/.externs/repo/silibs/soc_fw_shared/include
        ${REPO_ROOT_DIR}/.externs/repo/silibs/libraries/atu/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET}
    PRIVATE
        silibs::ddr_i3c
        silibs::ddrss
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        silibs::kng_silibs_platform
        silibs::platform
        silibs::ras_arm
        silibs::spi_bridge
        fwlib.dfwk
        fwlib.lib.init
        ms::lib::atomics
        ms::lib::crash_dump
        ms::lib::ddrss_init
        ms::lib::error_codes
        ms::lib::event_trace::trace
        ms::lib::gtimer
        ms::lib::hsp_firmware_headers
        ms::lib::i3c_controller
        ms::lib::icc_platform_defines
        ms::lib::scp_interrupts
        ms::lib::system_info
        ms::service::sds_service
    PUBLIC
        ms::service::sensor_fifo
        silibs::atu
        fwlib.platform.dfwkhost.threadx
        fwlib.lib.cfg_mgr
        fwlib.dfwk
        fwlib.lib.interface.platform.errorhandler
        fwlib.lib.interface.platform.interrupts
        fwlib.lib.interface.platform.threadx
        fwlib.service.iccBase
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

###### Extract all starting/ending reserved ranges to a separate file #######
# Define the path to the .h file
set(HEADER_FILE_PATH "${REPO_ROOT_DIR}/.externs/repo/silibs/soc_fw_shared/include/memory_map/ddrss_reserved_regions.h")

# Read the contents of the .h file
file(READ ${HEADER_FILE_PATH} HEADER_CONTENTS)

# Apply the regex to extract the last instance of *_RESERVATION_END  0xHEXNUMBER
# Accept white spaces and parentheses
string(REGEX MATCHALL "(_RESERVATION_END[ ]* [(]?0x[0-9A-Fa-f]+[)]?)" MATCHES ${HEADER_CONTENTS})

# Get the last match from the list of matches
list(GET MATCHES -1 LAST_RESERVATION_END)

# Strip "_RESERVATION_END " from the front
string(REGEX REPLACE "_RESERVATION_END " "" LAST_RESERVATION_END ${LAST_RESERVATION_END})

# Write the results to a file in the build directory
file(WRITE "${CMAKE_BINARY_DIR}/start_of_borgens_range.txt" "${LAST_RESERVATION_END}")
#############################################################################

add_library(ms::lib::ddr_manager ALIAS ${TARGET})
