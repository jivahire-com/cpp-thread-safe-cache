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
set(PROD_FW_KNOBS_XML_FILENAME "kng_mscp_config_knobs.xml")
set(EXPLICIT_PATH_PROD_FW_KNOBS ${CMAKE_SOURCE_DIR}/tools/config_knobs/${PROD_FW_KNOBS_XML_FILENAME})

# Merge all XMLs into one TOP level XML ---------------------------------------------------------------------
set(MERGE_COMMAND ${Python3_EXECUTABLE} ${EXPLICIT_PATH_MERGE_KNOBS_SCRIPT} -t ${EXPLICIT_PATH_SILIBS_KNOBS} ${EXPLICIT_PATH_PROD_FW_KNOBS} -p ${PROD_FW_COMBINED_GENERATED_KNOBS})
add_custom_command(
    OUTPUT ${PROD_FW_COMBINED_GENERATED_KNOBS}
    COMMAND ${MERGE_COMMAND}
    DEPENDS ${SILIBS_KNOBS_XML} ${MERGE_KNOBS_SCRIPT} ${PROD_FW_KNOBS_XML}
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