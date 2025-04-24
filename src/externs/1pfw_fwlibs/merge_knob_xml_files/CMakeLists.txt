# Merge SiLibs and Prod FW Config XMLs

set(TARGET kng.knob_service)
add_library(${TARGET} INTERFACE)

set(LIBS
    ${GENERATED_PUBLIC_PATH}
    ${TARGET_DIR_SILIBS}/libraries
    ${TARGET_DIR_SILIBS_PLATFORM}
)

# This header is intended for public access by any module
file(MAKE_DIRECTORY ${GENERATED_PUBLIC_PATH})

# Note that PARENT_SCOPE only sets the variable in the parent scope - and not in local scope
set(FPFW_CFG_MGR_HEADER_REF_DIRS ${LIBS} PARENT_SCOPE)

# Find Python
find_package(Python3 COMPONENTS Interpreter)

# Find Merge Knobs script (maintained by SiLibs)-------------------------------------------------------------
set(MERGE_KNOBS_SCRIPT_FILENAME mergeKnobs.py)
set(EXPLICIT_PATH_MERGE_KNOBS_SCRIPT ${TARGET_DIR_SILIBS_COMMON}/knobs/${MERGE_KNOBS_SCRIPT_FILENAME})

# Find SiLibs top-level XML ---------------------------------------------------------------------------------
set(SILIBS_KNOBS_XML_FILENAME "silibs_knobs.xml")
set(EXPLICIT_PATH_SILIBS_KNOBS ${TARGET_DIR_SILIBS}/libraries/${SILIBS_KNOBS_XML_FILENAME})

# Find Production FW config knob XML ------------------------------------------------------------------------
set(EXPLICIT_PATH_PROD_FW_DDRSS_KNOBS ${CMAKE_SOURCE_DIR}/config/knobs_config/kng_mscp_ddrss_config_knobs.xml)
set(EXPLICIT_PATH_PROD_FW_I3C_KNOBS ${CMAKE_SOURCE_DIR}/config/knobs_config/kng_mscp_i3c_config_knobs.xml)
set(EXPLICIT_PATH_PROD_FW_MESH_KNOBS ${CMAKE_SOURCE_DIR}/config/knobs_config/kng_mscp_mesh_config_knobs.xml)
set(EXPLICIT_PATH_PROD_FW_PCIE_KNOBS ${CMAKE_SOURCE_DIR}/config/knobs_config/kng_mscp_pcie_config_knobs.xml)
set(EXPLICIT_PATH_PROD_FW_POWER_KNOBS ${CMAKE_SOURCE_DIR}/config/knobs_config/kng_mscp_power_config_knobs.xml)
set(EXPLICIT_PATH_PROD_FW_ACCEL_KNOBS ${CMAKE_SOURCE_DIR}/config/knobs_config/kng_mscp_accel_config_knobs.xml)
set(EXPLICIT_PATH_PROD_FW_FUNC_TEST_KNOBS ${CMAKE_SOURCE_DIR}/config/knobs_config/kng_mscp_func_test_config_knobs.xml)
set(EXPLICIT_PATH_PROD_FW_UNIT_TEST_KNOBS ${CMAKE_SOURCE_DIR}/config/knobs_config/kng_mscp_unit_test_config_knobs.xml)
set(EXPLICIT_PATH_PROD_FW_USB_KNOBS ${CMAKE_SOURCE_DIR}/config/knobs_config/kng_mscp_usb_config_knobs.xml)
set(EXPLICIT_PATH_PROD_FW_GPIO_KNOBS ${CMAKE_SOURCE_DIR}/config/knobs_config/kng_mscp_gpio_config_knobs.xml)
set(EXPLICIT_PATH_PROD_FW_CRASHDUMP_KNOBS ${CMAKE_SOURCE_DIR}/config/knobs_config/kng_mscp_crash_dump_config_knobs.xml)
set(EXPLICIT_PATH_PROD_FW_SYSTEM_KNOBS ${CMAKE_SOURCE_DIR}/config/knobs_config/kng_mscp_system_config_knobs.xml)
set(EXPLICIT_PATH_PROD_FW_TOWER_KNOBS ${CMAKE_SOURCE_DIR}/config/knobs_config/kng_mscp_tower_config_knobs.xml)
set(EXPLICIT_PATH_PROD_FW_VAB_KNOBS ${CMAKE_SOURCE_DIR}/config/knobs_config/kng_mscp_vab_config_knobs.xml)
set(EXPLICIT_PATH_PROD_FW_FUSE_KNOBS ${CMAKE_SOURCE_DIR}/config/knobs_config/kng_mscp_fuse_config_knobs.xml)
set(EXPLICIT_PATH_PROD_FW_SMMU_VAB_KNOBS ${CMAKE_SOURCE_DIR}/config/knobs_config/kng_mscp_smmu_config_knobs.xml)
set(EXPLICIT_PATH_PROD_FW_POWER_TLM_KNOBS ${CMAKE_SOURCE_DIR}/config/knobs_config/kng_mscp_power_tlm_knobs.xml)

set(PROD_FW_KNOBS_NAMESPACE "FE3ED49F-B173-41ED-9076-356661D46A42")

# Merge all XMLs into one TOP level XML ---------------------------------------------------------------------
if (i386-pc-windows-msvc)
    set(MERGE_COMMAND ${Python3_EXECUTABLE} ${EXPLICIT_PATH_MERGE_KNOBS_SCRIPT}
                -t ${EXPLICIT_PATH_SILIBS_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_DDRSS_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_I3C_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_MESH_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_PCIE_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_POWER_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_ACCEL_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_UNIT_TEST_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_USB_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_GPIO_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_CRASHDUMP_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_SYSTEM_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_TOWER_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_VAB_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_FUSE_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_SMMU_VAB_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_POWER_TLM_KNOBS}
                -p ${PROD_FW_COMBINED_GENERATED_KNOBS} -n ${PROD_FW_KNOBS_NAMESPACE}
    )
else()
    set(MERGE_COMMAND ${Python3_EXECUTABLE} ${EXPLICIT_PATH_MERGE_KNOBS_SCRIPT}
                -t ${EXPLICIT_PATH_SILIBS_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_DDRSS_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_I3C_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_MESH_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_PCIE_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_POWER_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_ACCEL_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_FUNC_TEST_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_USB_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_GPIO_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_CRASHDUMP_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_SYSTEM_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_TOWER_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_VAB_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_FUSE_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_SMMU_VAB_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_POWER_TLM_KNOBS}
                -p ${PROD_FW_COMBINED_GENERATED_KNOBS} -n ${PROD_FW_KNOBS_NAMESPACE}
    )
endif()

set(KNOB_FILES_DEPENDS ${EXPLICIT_PATH_SILIBS_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_DDRSS_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_I3C_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_MESH_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_PCIE_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_POWER_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_ACCEL_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_FUNC_TEST_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_UNIT_TEST_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_USB_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_GPIO_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_TOWER_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_VAB_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_FUSE_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_SMMU_VAB_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_POWER_TLM_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_CRASHDUMP_KNOBS}
                   ${EXPLICIT_PATH_PROD_FW_SYSTEM_KNOBS}
    )
add_custom_command(
    OUTPUT ${PROD_FW_COMBINED_GENERATED_KNOBS}
    COMMAND ${MERGE_COMMAND}
    DEPENDS ${KNOB_FILES_DEPENDS}
)

message(STATUS "Merged XML: ${PROD_FW_COMBINED_GENERATED_KNOBS}")

add_custom_target(
    gen_config_headers_target ALL
    DEPENDS ${PROD_FW_COMBINED_GENERATED_KNOBS}
    COMMENT "generate combined production fw xml"
    VERBATIM
)

add_dependencies(${TARGET} gen_config_headers_target)

target_include_directories(
    ${TARGET}
    SYSTEM INTERFACE
        ${LIBS}
)

add_library(kng_knob_service ALIAS ${TARGET})