//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pdr_repo_init.c
 * @brief Defines the static platform description record table of the kingsgate platform on the out of band path.
 * @link
 * For context, this file has elements ported from the pioneer firmware.
 * Pioneer reference: https://azurecsi.visualstudio.com/Woodinville/_git/scp?path=/product/pioneer/mcp_ramfw/config_pldm_service.c
 * To add a PDR
 *    - Define a struct statically below based on PDR type
 *    - Add to pdr_list
 *
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>      // for FPFW_INIT_COMPONENT, FPFW_INIT_D...
#include <fpfw_pldm_service.h> // for fpfw_pldm_service_t, fpfw_pldm_pdr_state_sensor_COMPOSITE_1_STATES_1_t

#include <libpldm/entity.h>
#include <libpldm/platform.h>
#include <libpldm/state_set.h>
#include <platform_management_component/platform_description_record.h>
#include <platform_management_component/platform_description_types.h>

#include <pldm_pdr.h>

#include <stdbool.h>
#include <stddef.h>

#include <silibs_mcp_top_regs.h>

#include <DbgPrint.h>         // for FPFW_DBGPRINT_ALWAYS, FPFW_DBGPRINT_VE...

/*-- Symbolic Constant Macros (defines) --*/

// Step 1: expand argument then stringify
#define STRINGIFY_STEP1(x)  #x
#define STRINGIFY(x)        STRINGIFY_STEP1(x)

// Step 2: paste u + "..."
#define U16_PASTE_INNER(x)  u##x
#define U16_PASTE(x)        U16_PASTE_INNER(x)

// Final macro
#define UTF16_STRINGIFY(x)  U16_PASTE(STRINGIFY(x))


#define DEFINE_POWER_TLM_TEMPERATURE_SENSOR(NAME)                                                       \
static fpfw_pldm_pdr_numeric_sensor_UINT16_UINT16_t s_##NAME##_pdr    =                                 \
{                                                                                                       \
    .hdr.record_handle = 0,                                                                             \
    .hdr.version = PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION,                                          \
    .hdr.type = PLDM_NUMERIC_SENSOR_PDR,                                                                \
    .hdr.length = sizeof(fpfw_pldm_pdr_numeric_sensor_UINT16_UINT16_t) - sizeof(fpfw_pmc_pdr_header_t), \
    .hdr.record_change_num = 0,                                                                         \
    .terminus_handle = 0,                                                                               \
    .sensor_id = PLDM_SENSOR_ID_POWER_TLM_##NAME##_NUM_SENS,                                            \
    .entity_type = PLDM_ENTITY_SYS_FIRMWARE,                                                            \
    .entity_instance      = 0,                                                                          \
    .container_id         = 0,                                                                          \
    .sensor_init          = PLDM_NO_INIT,                                                               \
    .auxiliar_init_pdr    = false,                                                                      \
    .base_unit            = PLDM_BASE_UNIT_CELCIUS,                                                     \
    .unit_modifier        = -1,                                                                         \
    .rate_unit            = 0,                                                                          \
    .base_oem_unit_handle = 0,                                                                          \
    .aux_unit             = 0,                                                                          \
    .aux_unit_modifier    = 0,                                                                          \
    .aux_rate_unit        = 0,                                                                          \
    .rel                  = 0,                                                                          \
    .aux_oem_unit_handle  = 0,                                                                          \
    .is_linear            = true,                                                                       \
    .sensor_data_size     = PLDM_SENSOR_DATA_SIZE_UINT16,                                               \
    .resolution = 1,                                                                                    \
    .offset = 0,                                                                                        \
    .accuracy = 0,                                                                                      \
    .plus_tolerance = 0,                                                                                \
    .minus_tolerance = 0,                                                                               \
    .hysteresis                           = 0,                                                          \
    .supported_thresholds                 = 0,                                                          \
    .thresholds_and_hysteresis_volatility = 0x1F,                                                       \
    .state_transition_interval            = 1,                                                          \
    .update_interval                      = 0.5,                                                        \
    .max_readable                         = 1000,                                                       \
    .min_readable                         = 0,                                                          \
    .range_field_format                   = PLDM_SENSOR_DATA_SIZE_UINT16,                               \
    .range_field_support                  = 0,                                                          \
    .nominal_value                        = 250,                                                        \
    .normal_min                           = 0,                                                          \
    .normal_max                           = 0,                                                          \
    .warning_high                         = 0,                                                          \
    .warning_low                          = 0,                                                          \
    .critical_low                         = 0,                                                          \
    .critical_high                        = 0,                                                          \
    .fatal_low                            = 0,                                                          \
    .fatal_high                           = 0,                                                          \
};                                                                                                      \
static fpfw_pldm_pdr_sensor_auxiliary_name_t s_##NAME##_pdr_aux_name =                                  \
{                                                                                                       \
    .hdr.version = PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION,                                          \
    .hdr.type = PLDM_SENSOR_AUXILIARY_NAMES_PDR,                                                        \
    .hdr.length = sizeof(fpfw_pldm_pdr_sensor_auxiliary_name_t) - sizeof(fpfw_pmc_pdr_header_t),        \
    .hdr.record_change_num = 0,                                                                         \
    .sensor_id = PLDM_SENSOR_ID_POWER_TLM_##NAME##_NUM_SENS,                                            \
    .sensor_count = 1,                                                                                  \
    .name_string_count = 0x01,                                                                          \
    .name_language_tag = "en",                                                                          \
    .sensor_name = UTF16_STRINGIFY(NAME)                                                                \
};                                                                                                      \

#define DEFINE_POWER_TLM_POWER_SENSOR(NAME)                                                             \
static fpfw_pldm_pdr_numeric_sensor_UINT32_UINT32_t s_##NAME##_pdr    =                                 \
{                                                                                                       \
    .hdr.record_handle = 0,                                                                             \
    .hdr.version = PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION,                                          \
    .hdr.type = PLDM_NUMERIC_SENSOR_PDR,                                                                \
    .hdr.length = sizeof(fpfw_pldm_pdr_numeric_sensor_UINT32_UINT32_t) - sizeof(fpfw_pmc_pdr_header_t), \
    .hdr.record_change_num = 0,                                                                         \
    .terminus_handle = 0,                                                                               \
    .sensor_id = PLDM_SENSOR_ID_POWER_TLM_##NAME##_NUM_SENS,                                            \
    .entity_type = PLDM_ENTITY_SYS_FIRMWARE,                                                            \
    .entity_instance      = 0,                                                                          \
    .container_id         = 0,                                                                          \
    .sensor_init          = PLDM_NO_INIT,                                                               \
    .auxiliar_init_pdr    = false,                                                                      \
    .base_unit            = PLDM_BASE_UNIT_WATTS,                                                       \
    .unit_modifier        = -3,                                                                         \
    .rate_unit            = 0,                                                                          \
    .base_oem_unit_handle = 0,                                                                          \
    .aux_unit             = 0,                                                                          \
    .aux_unit_modifier    = 0,                                                                          \
    .aux_rate_unit        = 0,                                                                          \
    .rel                  = 0,                                                                          \
    .aux_oem_unit_handle  = 0,                                                                          \
    .is_linear            = true,                                                                       \
    .sensor_data_size     = PLDM_SENSOR_DATA_SIZE_UINT32,                                               \
    .resolution = 1,                                                                                    \
    .offset = 0,                                                                                        \
    .accuracy = 0,                                                                                      \
    .plus_tolerance = 0,                                                                                \
    .minus_tolerance = 0,                                                                               \
    .hysteresis                           = 0,                                                          \
    .supported_thresholds                 = 0,                                                          \
    .thresholds_and_hysteresis_volatility = 0x1F,                                                       \
    .state_transition_interval            = 1,                                                          \
    .update_interval                      = 0.25,                                                       \
    .max_readable                         = 4294967295U,                                                \
    .min_readable                         = 0,                                                          \
    .range_field_format                   = PLDM_SENSOR_DATA_SIZE_UINT32,                               \
    .range_field_support                  = 0,                                                          \
    .nominal_value                        = 0,                                                          \
    .normal_min                           = 0,                                                          \
    .normal_max                           = 0,                                                          \
    .warning_high                         = 0,                                                          \
    .warning_low                          = 0,                                                          \
    .critical_low                         = 0,                                                          \
    .critical_high                        = 0,                                                          \
    .fatal_low                            = 0,                                                          \
    .fatal_high                           = 0,                                                          \
};                                                                                                      \
static fpfw_pldm_pdr_sensor_auxiliary_name_t s_##NAME##_pdr_aux_name =                                  \
{                                                                                                       \
    .hdr.version = PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION,                                          \
    .hdr.type = PLDM_SENSOR_AUXILIARY_NAMES_PDR,                                                        \
    .hdr.length = sizeof(fpfw_pldm_pdr_sensor_auxiliary_name_t) - sizeof(fpfw_pmc_pdr_header_t),        \
    .hdr.record_change_num = 0,                                                                         \
    .sensor_id = PLDM_SENSOR_ID_POWER_TLM_##NAME##_NUM_SENS,                                            \
    .sensor_count = 1,                                                                                  \
    .name_string_count = 0x01,                                                                          \
    .name_language_tag = "en",                                                                          \
    .sensor_name = UTF16_STRINGIFY(NAME)                                                                \
};                                                                                                      


/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/


FPFW_INIT_COMPONENT(pdr_repo, FPFW_INIT_NULL_NODE)
{
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(SOC_TEMP_MAX);
    DEFINE_POWER_TLM_POWER_SENSOR(SOC_PWR);
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(DIMM_TEMP_MAX);
    DEFINE_POWER_TLM_POWER_SENSOR(DIMM_TOTAL_PWR);

    static fpfw_pldm_pdr_sensor_auxiliary_name_t s_dummy0_name = {
        .hdr.version = PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION,
        .hdr.type = PLDM_SENSOR_AUXILIARY_NAMES_PDR,
        .hdr.length = sizeof(fpfw_pldm_pdr_sensor_auxiliary_name_t) - sizeof(fpfw_pmc_pdr_header_t),
        .hdr.record_change_num = 0,

        .sensor_id = PLDM_SENSOR_ID_MCP_DUMMY_0,
        .sensor_count = 1,
        .name_string_count = 0x01,
        .name_language_tag = "en", // Language tag for the name. 3 bytes, null terminated. "en" = English
        .sensor_name = u"Dummy_State_Sensor" // Sensor name in UTF-16 format
    };

    static fpfw_pldm_pdr_state_sensor_COMPOSITE_1_STATES_1_t s_dummy0 = {
        .hdr.version = PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION,
        .hdr.type = PLDM_STATE_SENSOR_PDR,
        .hdr.length = sizeof(fpfw_pldm_pdr_state_sensor_COMPOSITE_1_STATES_1_t) - sizeof(fpfw_pmc_pdr_header_t),
        .hdr.record_change_num = 0,

        .sensor_id = PLDM_SENSOR_ID_MCP_DUMMY_0, // Unique id across sensors. Used for registration in the PldmService API

        .entity_type = PLDM_ENTITY_PROC_MODULE, // Entity for which this sensor is associated. Section 7.1 https://www.dmtf.org/sites/default/files/standards/documents/DSP0249_1.0.0.pdf
        .entity_instance = 0, // Instance id in case there are multiple entities of the same time in the same container
        .container_id = 0,           // 0 = SYSTEM, related to entity associations in DSP0248_1.2.1 (Section 28)
        .sensor_init = PLDM_NO_INIT, // Only no-init/enable supported
        .auxiliar_init_pdr = true,  // Required if sensor has an auxiliary name
        .composite_sensor_count = 1,
        .possible_states = {
            {.state_set_id = PLDM_STATE_SET_AVAILABILITY,
                .possible_states_size = 1,
                .possible_states = {
                   PLDM_STATE_SET_AVAILABILITY_REBOOTING,
                }
            }
        }
    };

    static fpfw_pldm_pdr_sensor_auxiliary_name_t s_dummy1_name = {
        .hdr.version = PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION,
        .hdr.type = PLDM_SENSOR_AUXILIARY_NAMES_PDR,
        .hdr.length = sizeof(fpfw_pldm_pdr_sensor_auxiliary_name_t) - sizeof(fpfw_pmc_pdr_header_t),
        .hdr.record_change_num = 0,

        .sensor_id = PLDM_SENSOR_ID_MCP_DUMMY_1,
        .sensor_count = 1,
        .name_string_count = 0x01,
        .name_language_tag = "en", // Language tag for the name. 3 bytes, null terminated. "en" = English
        .sensor_name = u"Dummy_Numeric_Sensor" // Sensor name in UTF-16 format
    };

    static fpfw_pldm_pdr_numeric_sensor_UINT32_UINT32_t s_dummy1 = {
        .hdr.version = PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION,
        .hdr.type = PLDM_NUMERIC_SENSOR_PDR,
        .hdr.length = sizeof(fpfw_pldm_pdr_numeric_sensor_UINT32_UINT32_t) - sizeof(fpfw_pmc_pdr_header_t),
        .hdr.record_change_num = 0,

        .sensor_id = PLDM_SENSOR_ID_MCP_DUMMY_1, // Unique id across sensors. Used for registration in the PldmService API
        .entity_type = PLDM_ENTITY_PROC, // Entity for which this sensor is associated. Section 7.1 https://www.dmtf.org/sites/default/files/standards/documents/DSP0249_1.0.0.pdf
        .entity_instance = 0, // Instance id in case there are multiple entities of the same time in the same container
        .container_id = 0,           // 0 = SYSTEM, related to entity associations in DSP0248_1.2.1 (Section 28)
        .sensor_init = PLDM_NO_INIT, // Only no-init/enable supported
        .auxiliar_init_pdr = true,   // Required if sensor has an auxiliary name

        .base_unit = PLDM_BASE_UNIT_NONE,
        .unit_modifier = 0,                               // None
        .rate_unit = 0,                                   // No rate unit
        .base_oem_unit_handle = 0,                        // Standard unit used
        .aux_unit = 0,                                    // No aux unit
        .aux_unit_modifier = 0,                           // No aux unit
        .aux_rate_unit = 0,                               // No aux unit
        .rel = 0,                                         // No aux to base unit
        .aux_oem_unit_handle = 0,                         // No aux unit
        .is_linear = true,                                // Range of this sensor is non-dynamic
        .sensor_data_size = PLDM_SENSOR_DATA_SIZE_UINT32, // This is an u32 counter
        .resolution = 1,
        .offset = 0,
        .accuracy = 0,
        .plus_tolerance = 0,
        .minus_tolerance = 0,
        .hysteresis = 0,                              // Don't use hysteresis
        .supported_thresholds = 0,                    // Supports upper threshold warning
        .thresholds_and_hysteresis_volatility = 0x1F, // Everything is stored in volatile mem
        .state_transition_interval = 0.5, // 0.5 seconds for sensor to be enabled (to be characterized)
        .update_interval = 0,             // Sensor is updated asychronously
        .max_readable = 10000,            // max uint32 4294967295U
        .min_readable = 10,               // 0
        .range_field_format = PLDM_SENSOR_DATA_SIZE_UINT32, // Range units
        .range_field_support = 0,                           // Only supports warning ranges
        .nominal_value = 100,
        .normal_min = 50,     // Not set
        .normal_max = 0,      // Not set
        .warning_high = 1000, // Should be defined by system engineering
        .warning_low = 0,     // Mandatory, set to lowest value
        .critical_low = 0,    // Not set
        .critical_high = 0,   // Not set
        .fatal_low = 0,       // Not set        .fatal_high = 0,      // Not set
    };

    static fpfw_pldm_pdr_state_sensor_COMPOSITE_1_STATES_1_t s_dummy2 = {
        .hdr.version = PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION,
        .hdr.type = PLDM_STATE_SENSOR_PDR,
        .hdr.length = sizeof(fpfw_pldm_pdr_state_sensor_COMPOSITE_1_STATES_1_t) - sizeof(fpfw_pmc_pdr_header_t),
        .hdr.record_change_num = 0,

        .sensor_id = PLDM_SENSOR_ID_MCP_DUMMY_2,
        .entity_type = PLDM_ENTITY_PROC_MODULE,
        .entity_instance = 0,
        .container_id = 0,
        .sensor_init = PLDM_NO_INIT,
        .auxiliar_init_pdr = false,
        .composite_sensor_count = 1,
        .possible_states = {
            {.state_set_id = PLDM_STATE_SET_AVAILABILITY,
                .possible_states_size = 1,
                .possible_states = {
                   PLDM_STATE_SET_AVAILABILITY_REBOOTING,
                }
            }
        }
    };

    static fpfw_pldm_pdr_numeric_sensor_UINT32_UINT32_t s_dummy3 = {
        .hdr.version = PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION,
        .hdr.type = PLDM_NUMERIC_SENSOR_PDR,
        .hdr.length = sizeof(fpfw_pldm_pdr_numeric_sensor_UINT32_UINT32_t) - sizeof(fpfw_pmc_pdr_header_t),
        .hdr.record_change_num = 0,

        .sensor_id = PLDM_SENSOR_ID_MCP_DUMMY_3,
        .entity_type = PLDM_ENTITY_PROC,
        .entity_instance = 0,
        .container_id = 0,
        .sensor_init = PLDM_NO_INIT,
        .auxiliar_init_pdr = false,

        .base_unit = PLDM_BASE_UNIT_NONE,
        .unit_modifier = 0,
        .rate_unit = 0,
        .base_oem_unit_handle = 0,
        .aux_unit = 0,
        .aux_unit_modifier = 0,
        .aux_rate_unit = 0,
        .rel = 0,
        .aux_oem_unit_handle = 0,
        .is_linear = true,
        .sensor_data_size = PLDM_SENSOR_DATA_SIZE_UINT32,
        .resolution = 1,
        .offset = 0,
        .accuracy = 0,
        .plus_tolerance = 0,
        .minus_tolerance = 0,
        .hysteresis = 0,
        .supported_thresholds = 0,
        .thresholds_and_hysteresis_volatility = 0x1F,
        .state_transition_interval = 0.5,
        .update_interval = 0,
        .max_readable = 4294967295U,
        .min_readable = 0,
        .range_field_format = PLDM_SENSOR_DATA_SIZE_UINT32,
        .range_field_support = 0,
        .nominal_value = 2147483648U,
        .normal_min = 1000,
        .normal_max = 0,
        .warning_high = 4000000000U,
        .warning_low = 0,
        .critical_low = 0,
        .critical_high = 0,
        .fatal_low = 0,
        .fatal_high = 0,
    };

    static fpfw_pldm_pdr_numeric_sensor_SINT32_SINT32_t s_dummy4 = {
        .hdr.version = PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION,
        .hdr.type = PLDM_NUMERIC_SENSOR_PDR,
        .hdr.length = sizeof(fpfw_pldm_pdr_numeric_sensor_SINT32_SINT32_t) - sizeof(fpfw_pmc_pdr_header_t),
        .hdr.record_change_num = 0,

        .sensor_id = PLDM_SENSOR_ID_MCP_DUMMY_4,
        .entity_type = PLDM_ENTITY_PROC,
        .entity_instance = 0,
        .container_id = 0,
        .sensor_init = PLDM_NO_INIT,
        .auxiliar_init_pdr = false,

        .base_unit = PLDM_BASE_UNIT_CELCIUS,
        .unit_modifier = 0,
        .rate_unit = 0,
        .base_oem_unit_handle = 0,
        .aux_unit = 0,
        .aux_unit_modifier = 0,
        .aux_rate_unit = 0,
        .rel = 0,
        .aux_oem_unit_handle = 0,
        .is_linear = true,
        .sensor_data_size = PLDM_SENSOR_DATA_SIZE_SINT32,
        .resolution = 1,
        .offset = 0,
        .accuracy = 100,
        .plus_tolerance = 5,
        .minus_tolerance = 5,
        .hysteresis = -40,
        .supported_thresholds = 0x3F,
        .thresholds_and_hysteresis_volatility = 0x1F,
        .state_transition_interval = 0.5,
        .update_interval = 0,
        .max_readable = 125,
        .min_readable = -40,
        .range_field_format = PLDM_SENSOR_DATA_SIZE_SINT32,
        .range_field_support = 0x3F,
        .nominal_value = 25,
        .normal_min = 0,
        .normal_max = 85,
        .warning_high = 90,
        .warning_low = -10,
        .critical_low = -20,
        .critical_high = 100,
        .fatal_low = -30,
        .fatal_high = 110,
    };

    static fpfw_pldm_pdr_state_sensor_COMPOSITE_1_STATES_2_t s_dummy5 = {
        .hdr.version = PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION,
        .hdr.type = PLDM_STATE_SENSOR_PDR,
        .hdr.length = sizeof(fpfw_pldm_pdr_state_sensor_COMPOSITE_1_STATES_2_t) - sizeof(fpfw_pmc_pdr_header_t),
        .hdr.record_change_num = 0,

        .sensor_id = PLDM_SENSOR_ID_MCP_DUMMY_5,
        .entity_type = PLDM_ENTITY_PROC,
        .entity_instance = 0,
        .container_id = 0,
        .sensor_init = PLDM_NO_INIT,
        .auxiliar_init_pdr = false,
        .composite_sensor_count = 1,
        .possible_states = {
            {.state_set_id = PLDM_STATE_SET_OPERATIONAL_STATUS,
                .possible_states_size = 2,
                .possible_states = {
                   PLDM_STATE_SET_OPERATIONAL_RUNNING_STATUS_STARTING,
                   PLDM_STATE_SET_OPERATIONAL_RUNNING_STATUS_STOPPING,
                }
            }
        }
    };





    static fpfw_pldm_pdr_numeric_sensor_UINT32_UINT32_t s_dummy6 = {
        .hdr.version = PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION,
        .hdr.type = PLDM_NUMERIC_SENSOR_PDR,
        .hdr.length = sizeof(fpfw_pldm_pdr_numeric_sensor_UINT32_UINT32_t) - sizeof(fpfw_pmc_pdr_header_t),
        .hdr.record_change_num = 0,

        .sensor_id = PLDM_SENSOR_ID_MCP_DUMMY_6,
        .entity_type = PLDM_ENTITY_PROC,
        .entity_instance = 0,
        .container_id = 0,
        .sensor_init = PLDM_NO_INIT,
        .auxiliar_init_pdr = false,

        .base_unit = PLDM_BASE_UNIT_NONE,
        .unit_modifier = 0,
        .rate_unit = 0,
        .base_oem_unit_handle = 0,
        .aux_unit = 0,
        .aux_unit_modifier = 0,
        .aux_rate_unit = 0,
        .rel = 0,
        .aux_oem_unit_handle = 0,
        .is_linear = true,
        .sensor_data_size = PLDM_SENSOR_DATA_SIZE_UINT32,
        .resolution = 1,
        .offset = 0,
        .accuracy = 0,
        .plus_tolerance = 2,
        .minus_tolerance = 2,
        .hysteresis = 0,
        .supported_thresholds = 0x18,
        .thresholds_and_hysteresis_volatility = 0x1F,
        .state_transition_interval = 0.1,
        .update_interval = 0,
        .max_readable = 100,
        .min_readable = 0,
        .range_field_format = PLDM_SENSOR_DATA_SIZE_UINT32,
        .range_field_support = 0x18,
        .nominal_value = 50,
        .normal_min = 10,
        .normal_max = 90,
        .warning_high = 95,
        .warning_low = 5,
        .critical_low = 0,
        .critical_high = 0,
        .fatal_low = 0,
        .fatal_high = 0,
    };

    static fpfw_pldm_pdr_effecter_auxiliary_name_t s_dummy_effecter0_name = {
        .hdr.version = PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION,
        .hdr.type = PLDM_EFFECTER_AUXILIARY_NAMES_PDR,
        .hdr.length = sizeof(fpfw_pldm_pdr_sensor_auxiliary_name_t) - sizeof(fpfw_pmc_pdr_header_t),
        .hdr.record_change_num = 0,
        .terminus_handle = 0,                       // Populated by PLDM lib
        .effecter_id = PLDM_EFFECTER_ID_MCP_DUMMY_0, // Unique id across sensors. Used for registration in the PldmService API
        .effecter_count = 1,
        .name_string_count = 0x01,
        .name_language_tag = "en", // Language tag for the name. 3 bytes, null terminated. "en" = English
        .effecter_name = u"Dummy_State_Effecter" // Effecter name in UTF-16 format
    };

    static fpfw_pldm_pdr_state_effecter_COMPOSITE_1_STATES_1_t s_dummy_effecter0 = {
        .hdr.version = PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION,
        .hdr.type = PLDM_STATE_EFFECTER_PDR,
        .hdr.length = sizeof(fpfw_pldm_pdr_state_effecter_COMPOSITE_1_STATES_1_t) - sizeof(fpfw_pmc_pdr_header_t),
        .hdr.record_change_num = 0,

        .effecter_id = PLDM_EFFECTER_ID_MCP_DUMMY_0, // Unique id across sensors. Used for registration in the PldmService API
        .entity_type = PLDM_ENTITY_PROC_MODULE, // Entity for which this sensor is associated. Section 7.1 https://www.dmtf.org/sites/default/files/standards/documents/DSP0249_1.0.0.pdf
        .entity_instance = 0, // Instance id in case there are multiple entities of the same time in the same container
        .container_id = 0,             // 0 = SYSTEM, related to entity associations in DSP0248_1.2.1 (Section 28)
        .effecter_init = PLDM_NO_INIT, // Only no-init/enable supported
        .auxiliar_description_pdr = true, // Required if effecter has an auxiliary name
        .composite_effecter_count = 1,
        .possible_states = {
            {.state_set_id = PLDM_STATE_SET_SOFTWARE_NMI_REQUESTED, .possible_states_size = 1, .possible_states = {0x3}}}
    };

    static fpfw_pldm_pdr_effecter_auxiliary_name_t s_dummy_effecter1_name = {
        .hdr.version = PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION,
        .hdr.type = PLDM_EFFECTER_AUXILIARY_NAMES_PDR,
        .hdr.length = sizeof(fpfw_pldm_pdr_sensor_auxiliary_name_t) - sizeof(fpfw_pmc_pdr_header_t),
        .hdr.record_change_num = 0,
        .terminus_handle = 0,                       // Populated by PLDM lib
        .effecter_id = PLDM_EFFECTER_ID_MCP_DUMMY_1, // Unique id across sensors. Used for registration in the PldmService API
        .effecter_count = 1,
        .name_string_count = 0x01,
        .name_language_tag = "en", // Language tag for the name. 3 bytes, null terminated. "en" = English
        .effecter_name = u"Dummy_Numeric_Effecter" // Effecter name in UTF-16 format
    };

    static fpfw_pldm_pdr_numeric_effecter_UINT32_UINT32_t s_dummy_effecter1 = {
        .hdr.version = PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION,
        .hdr.type = PLDM_NUMERIC_EFFECTER_PDR,
        .hdr.length = sizeof(fpfw_pldm_pdr_numeric_effecter_UINT32_UINT32_t) - sizeof(fpfw_pmc_pdr_header_t),
        .hdr.record_change_num = 0,

        .effecter_id = PLDM_EFFECTER_ID_MCP_DUMMY_1, // Unique id across sensors. Used for registration in the PldmService API
        .entity_type = PLDM_ENTITY_PROC_MODULE, // Entity for which this sensor is associated. Section 7.1 https://www.dmtf.org/sites/default/files/standards/documents/DSP0249_1.0.0.pdf
        .entity_instance = 0, // Instance id in case there are multiple entities of the same time in the same container
        .container_id = 0,             // 0 = SYSTEM, related to entity associations in DSP0248_1.2.1 (Section 28)
        .effecter_init = PLDM_NO_INIT, // Only no-init/enable supported
        .effecter_aux_pdr = true,      // Required if effecter has an auxiliary name

        .base_unit = PLDM_BASE_UNIT_NONE,
        .unit_modifier = 0,                                 // None
        .rate_unit = 3,                                     // per second
        .base_oem_unit_handle = 0,                          // Standard unit used
        .aux_unit = 0,                                      // No aux unit
        .aux_unit_modifier = 0,                             // No aux unit
        .aux_rate_unit = 0,                                 // No aux unit
        .aux_oem_unit_handle = 0,                           // No aux unit
        .is_linear = true,                                  // Range of this sensor is non-dynamic
        .effecter_data_size = PLDM_SENSOR_DATA_SIZE_UINT32, // This is an u32 counter
        .resolution = 1,
        .offset = 0,
        .accuracy = 0,
        .plus_tolerance = 0,
        .minus_tolerance = 0,
        .state_transition_interval = 0.5, // 0.5 seconds for sensor to be enabled (to be characterized)
        .transition_interval = 1,         // Sensor is updated every 1 seconds
        .max_settable = 720,
        .min_settable = 100,
        .range_field_format = PLDM_SENSOR_DATA_SIZE_UINT32, // Range units
        .rangel_field_support = 0,                          // Only supports warning ranges
        .nominal_value = 0,
        .normal_min = 0, // Not set
        .normal_max = 0, // Not set
        .rated_max = 10000,
        .rated_min = 100,
    };

    static void* pdr_list[] = {
        &s_SOC_TEMP_MAX_pdr,
        &s_SOC_TEMP_MAX_pdr_aux_name,
        &s_SOC_PWR_pdr,
        &s_SOC_PWR_pdr_aux_name,
        &s_DIMM_TEMP_MAX_pdr,
        &s_DIMM_TEMP_MAX_pdr_aux_name,
        &s_DIMM_TOTAL_PWR_pdr,
        &s_DIMM_TOTAL_PWR_pdr_aux_name,
        &s_dummy0_name,
        &s_dummy0,
        &s_dummy1_name,
        &s_dummy1,
        &s_dummy2,
        &s_dummy3,
        &s_dummy4,
        &s_dummy5,
        &s_dummy6,
        &s_dummy_effecter0_name,
        &s_dummy_effecter0,
        &s_dummy_effecter1_name,
        &s_dummy_effecter1,
        NULL
    };

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &pdr_list};
}
