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
 *    - Add to s_pdr_list
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
#include <pldm_common_power.h>
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


#define DEFINE_POWER_TLM_TEMPERATURE_SENSOR(NAME, UPDATE_RATE)                                          \
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
    .auxiliar_init_pdr    = true,                                                                       \
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
    .update_interval                      = UPDATE_RATE,                                                \
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

#define DEFINE_POWER_TLM_POWER_SENSOR(NAME, UPDATE_RATE)                                                \
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
    .auxiliar_init_pdr    = true,                                                                       \
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
    .update_interval                      = UPDATE_RATE,                                                \
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

#define DEFINE_POWER_TLM_FREQUENCY_SENSOR(NAME)                                                         \
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
    .auxiliar_init_pdr    = true,                                                                       \
    .base_unit            = PLDM_BASE_UNIT_HERTZ,                                                       \
    .unit_modifier        = 6,                                                                          \
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
    .max_readable                         = 4000,                                                       \
    .min_readable                         = 0,                                                          \
    .range_field_format                   = PLDM_SENSOR_DATA_SIZE_UINT16,                               \
    .range_field_support                  = 0,                                                          \
    .nominal_value                        = 2100,                                                       \
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
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(SOC_TMP_MAX, 0.5);
    DEFINE_POWER_TLM_POWER_SENSOR(SOC_PWR, 0.5);
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(DIMM_TMP_MAX, 0.5);
    DEFINE_POWER_TLM_POWER_SENSOR(DIMM_TOTAL_PWR, 0.5);
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(VR_TMP_MAX, 0.5);
    DEFINE_POWER_TLM_FREQUENCY_SENSOR(SOC_AVG_FREQ);

    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(DIMM_AVG_TMP_00, 15.0);
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(DIMM_AVG_TMP_01, 15.0);
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(DIMM_AVG_TMP_02, 15.0);
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(DIMM_AVG_TMP_03, 15.0);
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(DIMM_AVG_TMP_04, 15.0);
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(DIMM_AVG_TMP_05, 15.0);
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(DIMM_AVG_TMP_06, 15.0);
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(DIMM_AVG_TMP_07, 15.0);
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(DIMM_AVG_TMP_08, 15.0);
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(DIMM_AVG_TMP_09, 15.0);
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(DIMM_AVG_TMP_10, 15.0);
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(DIMM_AVG_TMP_11, 15.0);

    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(DIMM_MAX_TMP_00, 15.0);
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(DIMM_MAX_TMP_01, 15.0);
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(DIMM_MAX_TMP_02, 15.0);
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(DIMM_MAX_TMP_03, 15.0);
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(DIMM_MAX_TMP_04, 15.0);
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(DIMM_MAX_TMP_05, 15.0);
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(DIMM_MAX_TMP_06, 15.0);
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(DIMM_MAX_TMP_07, 15.0);
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(DIMM_MAX_TMP_08, 15.0);
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(DIMM_MAX_TMP_09, 15.0);
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(DIMM_MAX_TMP_10, 15.0);
    DEFINE_POWER_TLM_TEMPERATURE_SENSOR(DIMM_MAX_TMP_11, 15.0);

    DEFINE_POWER_TLM_POWER_SENSOR(DIMM_AVG_PWR_00, 15.0);
    DEFINE_POWER_TLM_POWER_SENSOR(DIMM_AVG_PWR_01, 15.0);
    DEFINE_POWER_TLM_POWER_SENSOR(DIMM_AVG_PWR_02, 15.0);
    DEFINE_POWER_TLM_POWER_SENSOR(DIMM_AVG_PWR_03, 15.0);
    DEFINE_POWER_TLM_POWER_SENSOR(DIMM_AVG_PWR_04, 15.0);
    DEFINE_POWER_TLM_POWER_SENSOR(DIMM_AVG_PWR_05, 15.0);
    DEFINE_POWER_TLM_POWER_SENSOR(DIMM_AVG_PWR_06, 15.0);
    DEFINE_POWER_TLM_POWER_SENSOR(DIMM_AVG_PWR_07, 15.0);
    DEFINE_POWER_TLM_POWER_SENSOR(DIMM_AVG_PWR_08, 15.0);
    DEFINE_POWER_TLM_POWER_SENSOR(DIMM_AVG_PWR_09, 15.0);
    DEFINE_POWER_TLM_POWER_SENSOR(DIMM_AVG_PWR_10, 15.0);
    DEFINE_POWER_TLM_POWER_SENSOR(DIMM_AVG_PWR_11, 15.0);



    /*----------Power Pldm PDR entries----------*/
    static fpfw_pldm_pdr_sensor_auxiliary_name_t s_POWER_THROTTLING_STATE_pdr_aux_name = {
        .hdr.version = PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION,
        .hdr.type = PLDM_SENSOR_AUXILIARY_NAMES_PDR,
        .hdr.length = sizeof(fpfw_pldm_pdr_sensor_auxiliary_name_t) - sizeof(fpfw_pmc_pdr_header_t),
        .hdr.record_change_num = 0,

        .sensor_id = PLDM_SENSOR_ID_POWER_THROTTLING_STATE_SENSOR,
        .sensor_count = 1,
        .name_string_count = 0x01,
        .name_language_tag = "en", // Language tag for the name. 3 bytes, null terminated. "en" = English
        .sensor_name = u"THROTTLING_STATE" // Sensor name in UTF-16 format
    };

    static fpfw_pldm_pdr_state_sensor_COMPOSITE_1_STATES_3_t s_POWER_THROTTLING_STATE_pdr = {
        .hdr.version = PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION,
        .hdr.type = PLDM_STATE_SENSOR_PDR,
        .hdr.length = sizeof(fpfw_pldm_pdr_state_sensor_COMPOSITE_1_STATES_3_t) - sizeof(fpfw_pmc_pdr_header_t),
        .hdr.record_change_num = 0,

        .sensor_id = PLDM_SENSOR_ID_POWER_THROTTLING_STATE_SENSOR, // Unique id across sensors. Used for registration in the PldmService API

        .entity_type = PLDM_ENTITY_PROC_MODULE, // Entity for which this sensor is associated. Section 7.1 https://www.dmtf.org/sites/default/files/standards/documents/DSP0249_1.0.0.pdf
        .entity_instance = 0, // Instance id in case there are multiple entities of the same time in the same container
        .container_id = 0,           // 0 = SYSTEM, related to entity associations in DSP0248_1.2.1 (Section 28)
        .sensor_init = PLDM_NO_INIT, // Only no-init/enable supported
        .auxiliar_init_pdr = true,  // Required if sensor has an auxiliary name
        .composite_sensor_count = 1,
        .possible_states = {
            {.state_set_id = PLDM_STATE_SET_PERFORMANCE,
                .possible_states_size = 3,
                .possible_states = {PLDM_PERFORMANCE_NORMAL, PLDM_PERFORMANCE_THROTTLED, PLDM_PERFORMANCE_DEGRADED} //! 1 - Normal, 2 - Throttled, 3 - Degraded
            }
        }
    };

    static fpfw_pldm_pdr_effecter_auxiliary_name_t s_POWER_CAP_NUM_EFFECTER_pdr_aux_name = {
        .hdr.version = PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION,
        .hdr.type = PLDM_EFFECTER_AUXILIARY_NAMES_PDR,
        .hdr.length = sizeof(fpfw_pldm_pdr_effecter_auxiliary_name_t) - sizeof(fpfw_pmc_pdr_header_t),
        .hdr.record_change_num = 0,
        .terminus_handle = 0,                       // Populated by PLDM lib
        .effecter_id = PLDM_EFFECTER_ID_POWER_CAP_NUM_EFFECTER, // Unique id across sensors. Used for registration in the PldmService API
        .effecter_count = 1,
        .name_string_count = 0x01,
        .name_language_tag = "en", // Language tag for the name. 3 bytes, null terminated. "en" = English
        .effecter_name = u"PWRCAP_LIMIT" // Effecter name in UTF-16 format
    };

    static fpfw_pldm_pdr_numeric_effecter_UINT16_UINT16_t s_POWER_CAP_NUM_EFFECTER_pdr = {
        .hdr.version = PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION,
        .hdr.type = PLDM_NUMERIC_EFFECTER_PDR,
        .hdr.length = sizeof(fpfw_pldm_pdr_numeric_effecter_UINT16_UINT16_t) - sizeof(fpfw_pmc_pdr_header_t),
        .hdr.record_change_num = 0,

        .effecter_id = PLDM_EFFECTER_ID_POWER_CAP_NUM_EFFECTER, // Unique id across sensors. Used for registration in the PldmService API
        .entity_type = PLDM_ENTITY_PROC_MODULE, // Entity for which this sensor is associated. Section 7.1 https://www.dmtf.org/sites/default/files/standards/documents/DSP0249_1.0.0.pdf
        .entity_instance = 0, // Instance id in case there are multiple entities of the same time in the same container
        .container_id = 0,             // 0 = SYSTEM, related to entity associations in DSP0248_1.2.1 (Section 28)
        .effecter_init = PLDM_NO_INIT, // Only no-init/enable supported
        .effecter_aux_pdr = true,      // Required if effecter has an auxiliary name

        .base_unit = PLDM_BASE_UNIT_WATTS,
        .unit_modifier = 0,                                 // None
        .rate_unit = 0,                                     // per second
        .base_oem_unit_handle = 0,                          // Standard unit used
        .aux_unit = 0,                                      // No aux unit
        .aux_unit_modifier = 0,                             // No aux unit
        .aux_rate_unit = 0,                                 // No aux unit
        .aux_oem_unit_handle = 0,                           // No aux unit
        .is_linear = true,                                  // Range of this sensor is non-dynamic
        .effecter_data_size = PLDM_SENSOR_DATA_SIZE_UINT16, // This is an u32 counter
        .resolution = 1,
        .offset = 0,
        .accuracy = 0,
        .plus_tolerance = 0,
        .minus_tolerance = 0,
        .state_transition_interval = 0.5, // 0.5 seconds for sensor to be enabled (to be characterized)
        .transition_interval = 1,         // Sensor is updated every 1 seconds
        .max_settable = 65535, //! Max electrical limit is 400 watts, overridable by knobs, 65535 for uncapped power
        .min_settable = 50, //! Soc Idle Power 50 Watts
        .range_field_format = PLDM_SENSOR_DATA_SIZE_UINT16, // Range units
        .rangel_field_support = 0,                          // Only supports warning ranges
        .nominal_value = 0,
        .normal_min = 0, // Not set
        .normal_max = 0, // Not set
        .rated_max = 700, //! @todo: Based on pioneer
        .rated_min = 300,
    };

    static void* s_pdr_list[] = {
        &s_SOC_TMP_MAX_pdr,
        &s_SOC_TMP_MAX_pdr_aux_name,
        &s_SOC_PWR_pdr,
        &s_SOC_PWR_pdr_aux_name,
        &s_DIMM_TMP_MAX_pdr,
        &s_DIMM_TMP_MAX_pdr_aux_name,
        &s_DIMM_TOTAL_PWR_pdr,
        &s_DIMM_TOTAL_PWR_pdr_aux_name,
        &s_VR_TMP_MAX_pdr,
        &s_VR_TMP_MAX_pdr_aux_name,
        &s_SOC_AVG_FREQ_pdr,
        &s_SOC_AVG_FREQ_pdr_aux_name,
        &s_DIMM_AVG_TMP_00_pdr,
        &s_DIMM_AVG_TMP_00_pdr_aux_name,
        &s_DIMM_AVG_TMP_01_pdr,
        &s_DIMM_AVG_TMP_01_pdr_aux_name,
        &s_DIMM_AVG_TMP_02_pdr,
        &s_DIMM_AVG_TMP_02_pdr_aux_name,
        &s_DIMM_AVG_TMP_03_pdr,
        &s_DIMM_AVG_TMP_03_pdr_aux_name,
        &s_DIMM_AVG_TMP_04_pdr,
        &s_DIMM_AVG_TMP_04_pdr_aux_name,
        &s_DIMM_AVG_TMP_05_pdr,
        &s_DIMM_AVG_TMP_05_pdr_aux_name,
        &s_DIMM_AVG_TMP_06_pdr,
        &s_DIMM_AVG_TMP_06_pdr_aux_name,
        &s_DIMM_AVG_TMP_07_pdr,
        &s_DIMM_AVG_TMP_07_pdr_aux_name,
        &s_DIMM_AVG_TMP_08_pdr,
        &s_DIMM_AVG_TMP_08_pdr_aux_name,
        &s_DIMM_AVG_TMP_09_pdr,
        &s_DIMM_AVG_TMP_09_pdr_aux_name,
        &s_DIMM_AVG_TMP_10_pdr,
        &s_DIMM_AVG_TMP_10_pdr_aux_name,
        &s_DIMM_AVG_TMP_11_pdr,
        &s_DIMM_AVG_TMP_11_pdr_aux_name,
        &s_DIMM_MAX_TMP_00_pdr,
        &s_DIMM_MAX_TMP_00_pdr_aux_name,
        &s_DIMM_MAX_TMP_01_pdr,
        &s_DIMM_MAX_TMP_01_pdr_aux_name,
        &s_DIMM_MAX_TMP_02_pdr,
        &s_DIMM_MAX_TMP_02_pdr_aux_name,
        &s_DIMM_MAX_TMP_03_pdr,
        &s_DIMM_MAX_TMP_03_pdr_aux_name,
        &s_DIMM_MAX_TMP_04_pdr,
        &s_DIMM_MAX_TMP_04_pdr_aux_name,
        &s_DIMM_MAX_TMP_05_pdr,
        &s_DIMM_MAX_TMP_05_pdr_aux_name,
        &s_DIMM_MAX_TMP_06_pdr,
        &s_DIMM_MAX_TMP_06_pdr_aux_name,
        &s_DIMM_MAX_TMP_07_pdr,
        &s_DIMM_MAX_TMP_07_pdr_aux_name,
        &s_DIMM_MAX_TMP_08_pdr,
        &s_DIMM_MAX_TMP_08_pdr_aux_name,
        &s_DIMM_MAX_TMP_09_pdr,
        &s_DIMM_MAX_TMP_09_pdr_aux_name,
        &s_DIMM_MAX_TMP_10_pdr,
        &s_DIMM_MAX_TMP_10_pdr_aux_name,
        &s_DIMM_MAX_TMP_11_pdr,
        &s_DIMM_MAX_TMP_11_pdr_aux_name,
        &s_DIMM_AVG_PWR_00_pdr,
        &s_DIMM_AVG_PWR_00_pdr_aux_name,
        &s_DIMM_AVG_PWR_01_pdr,
        &s_DIMM_AVG_PWR_01_pdr_aux_name,
        &s_DIMM_AVG_PWR_02_pdr,
        &s_DIMM_AVG_PWR_02_pdr_aux_name,
        &s_DIMM_AVG_PWR_03_pdr,
        &s_DIMM_AVG_PWR_03_pdr_aux_name,
        &s_DIMM_AVG_PWR_04_pdr,
        &s_DIMM_AVG_PWR_04_pdr_aux_name,
        &s_DIMM_AVG_PWR_05_pdr,
        &s_DIMM_AVG_PWR_05_pdr_aux_name,
        &s_DIMM_AVG_PWR_06_pdr,
        &s_DIMM_AVG_PWR_06_pdr_aux_name,
        &s_DIMM_AVG_PWR_07_pdr,
        &s_DIMM_AVG_PWR_07_pdr_aux_name,
        &s_DIMM_AVG_PWR_08_pdr,
        &s_DIMM_AVG_PWR_08_pdr_aux_name,
        &s_DIMM_AVG_PWR_09_pdr,
        &s_DIMM_AVG_PWR_09_pdr_aux_name,
        &s_DIMM_AVG_PWR_10_pdr,
        &s_DIMM_AVG_PWR_10_pdr_aux_name,
        &s_DIMM_AVG_PWR_11_pdr,
        &s_DIMM_AVG_PWR_11_pdr_aux_name,
        &s_POWER_THROTTLING_STATE_pdr,
        &s_POWER_THROTTLING_STATE_pdr_aux_name,
        &s_POWER_CAP_NUM_EFFECTER_pdr,
        &s_POWER_CAP_NUM_EFFECTER_pdr_aux_name,
        NULL
    };

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &s_pdr_list};
}
