set(TARGET 1pfw_fwlibs)
set(TARGET_DIR ${REPO_ROOT_DIR}/.externs/repo/1pfw_fwlibs)

# Disable warnings for shared libs
add_compile_options(-w)

# Options for cmake module
option(FPFW_FWLIBS_ENABLE_TESTS "Compiles unit tests" OFF)
option(FPFW_FWLIBS_ENABLE_COVERAGE "Enables code coverage in compilation" OFF)
option(FPFW_FWLIBS_ENABLE_LINT "Enables additional lints during compilation" OFF)
option(FPFW_FWLIBS_ENABLE_DOCS "Creates doxygen documentation" OFF)
option(FPFW_CFG_MGR_ENABLE "Compiles Config Knob XMLs and Compiles Config Manager" ON)

# Config Manager
if (${FPFW_CFG_MGR_ENABLE})
    set(TARGET_DIR_SILIBS ${REPO_ROOT_DIR}/.externs/repo/silibs)
    set(TARGET_DIR_SILIBS_COMMON ${REPO_ROOT_DIR}/.externs/repo/silibs_common)
    set(TARGET_DIR_SILIBS_PLATFORM ${TARGET_DIR_SILIBS}/libraries/kng_silibs_platform/include)
    set(GENERATED_PUBLIC_PATH ${CMAKE_CURRENT_BINARY_DIR}/include)
    set(PROD_FW_COMBINED_GENERATED_KNOBS ${GENERATED_PUBLIC_PATH}/kng_generated_knobs.xml)

    add_subdirectory(merge_knob_xml_files)

    set(FPFW_CFG_MGR_PYTHON   ${Python3_EXECUTABLE})
    set(FPFW_CFG_MGR_SCHEMA   ${PROD_FW_COMBINED_GENERATED_KNOBS})

    #### Uncomment these lines when silibs common incorporates a local mirror of mu_feature_config #####
    # set(FPFW_CFG_MGR_SCRIPT   ${MU_SCRIPT_PATH}/KnobService.py)
    # set(MU_SCRIPT_PATH        ${TARGET_DIR_SILIBS_COMMON}/mu_feature_config/SetupDataPkg/Tools)
    #### Remove this line when silibs common incorporates a local mirror of mu_feature_config ##########
    set(FPFW_CFG_MGR_SCRIPT   ${CMAKE_CURRENT_SOURCE_DIR}/temp_copies_of_mu_scripts/KnobService.py)
    ####################################################################################################

    set(PROD_FW_KNOBS_PATH    ${CMAKE_SOURCE_DIR}/config/knobs_config)
    set(FPFW_CFG_MGR_PROFILES ${PROD_FW_KNOBS_PATH}/kng_mscp_config_platform.csv
                              ${PROD_FW_KNOBS_PATH}/kng_mscp_config_fpga.csv
                              ${PROD_FW_KNOBS_PATH}/kng_mscp_config_svp.csv
                              ${PROD_FW_KNOBS_PATH}/kng_mscp_config_svp_min.csv
                              ${PROD_FW_KNOBS_PATH}/kng_mscp_config_emu.csv
                              ${PROD_FW_KNOBS_PATH}/kng_mscp_config_bmc_compute.csv)
    set(KNOB_SVC_OPTIONS  -nc)

endif()

# Generic threadx lock port provided by fwlibs
set(FPFW_PLATFORM_IMPL_LOCK "fwlib.lib.platform.impl.lock.threadx")

# Generic ports implmented within this project
set(FPFW_PLATFORM_IMPL_THREADX "ms::lib::threadx")

# Generic timer ports implemented within this project
set(FPFW_PLATFORM_IMPL_TIMER "fwlib.lib.platform.impl.timers.threadx")

set(FPFW_PLATFORM_IMPL_MTS "kng.service.platform.impl.mts")

include(FetchContent)
FetchContent_Declare(
    ${TARGET}
    GIT_REPOSITORY $ENV{REPO_APP_GIT_1PFW_FWLIBS.URL}
    GIT_TAG        $ENV{REPO_APP_GIT_1PFW_FWLIBS.TAG}
    SOURCE_DIR     ${TARGET_DIR}
)

FetchContent_GetProperties(${TARGET})

#add prior to 1pfw.fwlibs in order to set cmake options

# configure platform MTS definitions
option(FPFW_FWLIBS_MTS_DEF_ENABLE "Enable MTS Definitions" ON)

set(FPFW_PLATFORM_IMPL_MTS_CORE_DEFINITION
    "MTS_PLATFORM_CORE_AP::0x0001"
    "MTS_PLATFORM_CORE_SCP::0x0002"
    "MTS_PLATFORM_CORE_MCP::0x0003"
    "MTS_PLATFORM_CORE_HSP::0x0004"
    "MTS_PLATFORM_CORE_TBP::0x0005"
    "MTS_PLATFORM_CORE_SDM::0x0006"
    "MTS_PLATFORM_CORE_CDED_SDM::0x0007"
    "MTS_PLATFORM_CORE_CDED_KMP::0x0008"
)

set(MTS_PLATFORM_PRIMARY_DIE_ID
    "0"
)
set(MTS_PLATFORM_PRIMARY_CORE_ID
    "MTS_PLATFORM_CORE_MCP"
)
set(MTS_PLATFORM_HOST_DIE_ID
    "0"
)
set(MTS_PLATFORM_HOST_CORE_ID
    "MTS_PLATFORM_CORE_AP"
)
set(MTS_DCP_PLATFORM_ID
    "DCP_PLATFORM_COBALT_200"
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ports/inc)

add_subdirectory(message_transfer_service)

if (NOT ${TARGET}_POPULATED)
    FetchContent_Populate(${TARGET})
    add_subdirectory(${TARGET_DIR} ${CMAKE_CURRENT_BINARY_DIR}/src/1pfw.fwlibs EXCLUDE_FROM_ALL)
endif()


include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/lint.cmake)
include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/format.cmake)
include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/coverage.cmake)
include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/mock.cmake)

target_compile_options(fwlib.lib.crashDump PRIVATE "-Os")

# Enable MCTP Event Tracing
add_compile_definitions(FPFW_FWLIBS_ENABLE_MCTP_ET)
add_compile_definitions(FPFW_FWLIBS_ENABLE_MCTP_UART_ET)