//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pdr_dummy_init.c
 * @brief Provides an example of registering dummy sensors and effecters with the fpfw pldm service. 
 * 
 */

/*------------- Includes -----------------*/

#include <fpfw_init.h>      // for FPFW_INIT_COMPONENT, FPFW_INIT_D...
#include <fpfw_pldm_service.h> // for fpfw_pldm_service_t, fpfw_pldm_pdr_state_sensor_COMPOSITE_1_STATES_1_t
#include <idsw_kng.h>
#include <libpldm/state_set.h>

#include <pldm_pdr.h>

#include <stdbool.h>
#include <stddef.h>

#include <DbgPrint.h>         // for FPFW_DBGPRINT_ALWAYS, FPFW_DBGPRINT_VE...

/*-- Symbolic Constant Macros (defines) --*/

/*-- Declarations (Statics and globals) --*/

static bool platform_supports_oob(idsw_plat_id_t sdv_id)
{
    switch (sdv_id)
    {
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
    case PLATFORM_RVP_EVT_SILICON:
        return true;
    default:
        return false;
    }
}

/*------------- Functions ----------------*/

static void on_state_sensor_get(pldm_state_sensor_context_t* p_sensor, void* p_context)
{
    (void)p_context;
    fpfw_pldm_composite_value_t composite_value = {0};
    uint16_t sensor_id = p_sensor->sensor_state.config.sensor_id;
    
    if (sensor_id == PLDM_SENSOR_ID_MCP_DUMMY_0 || sensor_id == PLDM_SENSOR_ID_MCP_DUMMY_2)
    {
        composite_value.state.states[0] = PLDM_STATE_SET_AVAILABILITY_REBOOTING;
    }
    else if (sensor_id == PLDM_SENSOR_ID_MCP_DUMMY_5)
    {
        composite_value.state.states[0] = PLDM_STATE_SET_OPERATIONAL_RUNNING_STATUS_STARTING;
    }
    fpfw_status_t status = fpfw_pldm_service_state_sensor_set(p_sensor, composite_value, 0b00000001);
    if (FPFW_STATUS_FAILED(status))
    {
        FPFW_DBGPRINT_ALWAYS("[pldm] failed state sensor set: %d\n", status);
    } 
}

static void on_numeric_sensor_get(pldm_numeric_sensor_context_t* p_sensor, void* p_context)
{
    (void)p_context;
    fpfw_pldm_composite_value_t composite_value = {0};
    uint16_t sensor_id = p_sensor->sensor_state.config.sensor_id;
    switch (sensor_id)
    {
        case PLDM_SENSOR_ID_MCP_DUMMY_1:
            composite_value.numeric.u32 = 12345;
            break;
            
        case PLDM_SENSOR_ID_MCP_DUMMY_3:
            composite_value.numeric.u32 = 0xABCD1234;
            break;
            
        case PLDM_SENSOR_ID_MCP_DUMMY_4:
            composite_value.numeric.i32 = -25;
            break;
            
        case PLDM_SENSOR_ID_MCP_DUMMY_6:
            composite_value.numeric.u32 = 75;
            break;
            
        default:
            composite_value.numeric.u32 = 0;
            break;
    }
    fpfw_status_t status = fpfw_pldm_service_numeric_sensor_set(p_sensor, composite_value);
    if (FPFW_STATUS_FAILED(status))
    {
        FPFW_DBGPRINT_ALWAYS("[pldm] failed num sensor set: %d\n", status);
    } 
}

static void on_state_effecter_set(pldm_state_effecter_context_t* effecter, fpfw_pldm_composite_value_t value, uint8_t set_mask, void* p_context)
{
    (void)p_context;
    FPFW_DBGPRINT_ALWAYS("[pldm] effecter %d set to states: ", effecter->effecter_state.config.effecter_id);
    for (int i = 0; i < FPFW_PLDM_MAX_COMPOSITE_COUNT; i++)
    {
        if (set_mask & (1 << i))
        {
            FPFW_DBGPRINT_ALWAYS("[pldm] composite %d set to state %d\n", i, value.state.states[i]);
        }
    }

    fpfw_status_t status = fpfw_pldm_service_state_effecter_set_complete(effecter);
    if (FPFW_STATUS_FAILED(status))
    {
        FPFW_DBGPRINT_ALWAYS("[pldm] failed state effecter set: %d\n", status);
    }
}

static void on_numeric_effecter_set(pldm_numeric_effecter_context_t* effecter, fpfw_pldm_composite_value_t value, void* p_context)
{
    (void)p_context;
    FPFW_DBGPRINT_ALWAYS("[pldm] effecter %d set to value: %u\n", effecter->effecter_state.config.effecter_id, value.numeric.u32);
    fpfw_status_t status = fpfw_pldm_service_numeric_effecter_set_complete(effecter);
    if (FPFW_STATUS_FAILED(status))
    {
        FPFW_DBGPRINT_ALWAYS("[pldm] failed numeric effecter set: %d\n", status);
    }
}

FPFW_INIT_COMPONENT(ex_eff, FPFW_INIT_DEPENDENCIES("pldm"))
{
    idsw_plat_id_t sdv_id = idsw_get_platform_sdv();
    if (idsw_get_die_id() == DIE_1 || !platform_supports_oob(sdv_id))
    {
        // Skip initialization on Die 1
        return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
    }
    
    static pldm_state_effecter_context_t effecter0;
    pldm_state_effecter_config_t config0 = {
        .effecter_id = PLDM_EFFECTER_ID_MCP_DUMMY_0,
        .notifications.on_effecter_set = on_state_effecter_set,
        .notifications.context = NULL 
    };
    fpfw_status_t status = fpfw_pldm_service_register_state_effecter(&effecter0, &config0);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    } 

    static pldm_numeric_effecter_context_t effecter1;
    pldm_numeric_effecter_config_t config1 = {
        .effecter_id = PLDM_EFFECTER_ID_MCP_DUMMY_1,
        .notifications.on_effecter_set = on_numeric_effecter_set,
        .notifications.context = NULL 
    };

    status = fpfw_pldm_service_register_numeric_effecter(&effecter1, &config1);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    } 

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(ex_sens, FPFW_INIT_DEPENDENCIES("pldm"))
{
    idsw_plat_id_t sdv_id = idsw_get_platform_sdv();
    if (idsw_get_die_id() == DIE_1 || !platform_supports_oob(sdv_id))
    {
        // Skip initialization on Die 1
        return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
    }

    // Register dummy0 (state sensor)
    static pldm_state_sensor_context_t sensor0;
    pldm_state_sensor_config_t config0 = {
        .sensor_id = PLDM_SENSOR_ID_MCP_DUMMY_0,
        .notifications.on_sensor_get = on_state_sensor_get,
        .notifications.context = NULL 
    };
    fpfw_status_t status = fpfw_pldm_service_register_state_sensor(&sensor0, &config0);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    // Register dummy1 (numeric sensor)
    static pldm_numeric_sensor_context_t sensor1;
    pldm_numeric_sensor_config_t config1 = {
        .sensor_id = PLDM_SENSOR_ID_MCP_DUMMY_1,
        .notifications.on_sensor_get = on_numeric_sensor_get,
        .notifications.context = NULL 
    };
    status = fpfw_pldm_service_register_numeric_sensor(&sensor1, &config1);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    // Register dummy2 (state sensor)
    static pldm_state_sensor_context_t sensor2;
    pldm_state_sensor_config_t config2 = {
        .sensor_id = PLDM_SENSOR_ID_MCP_DUMMY_2,
        .notifications.on_sensor_get = on_state_sensor_get,
        .notifications.context = NULL 
    };
    status = fpfw_pldm_service_register_state_sensor(&sensor2, &config2);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    // Register dummy3 (numeric sensor)
    static pldm_numeric_sensor_context_t sensor3;
    pldm_numeric_sensor_config_t config3 = {
        .sensor_id = PLDM_SENSOR_ID_MCP_DUMMY_3,
        .notifications.on_sensor_get = on_numeric_sensor_get,
        .notifications.context = NULL 
    };
    status = fpfw_pldm_service_register_numeric_sensor(&sensor3, &config3);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    // Register dummy4 (numeric sensor)
    static pldm_numeric_sensor_context_t sensor4;
    pldm_numeric_sensor_config_t config4 = {
        .sensor_id = PLDM_SENSOR_ID_MCP_DUMMY_4,
        .notifications.on_sensor_get = on_numeric_sensor_get,
        .notifications.context = NULL 
    };
    status = fpfw_pldm_service_register_numeric_sensor(&sensor4, &config4);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    // Register dummy5 (state sensor)
    static pldm_state_sensor_context_t sensor5;
    pldm_state_sensor_config_t config5 = {
        .sensor_id = PLDM_SENSOR_ID_MCP_DUMMY_5,
        .notifications.on_sensor_get = on_state_sensor_get,
        .notifications.context = NULL 
    };
    status = fpfw_pldm_service_register_state_sensor(&sensor5, &config5);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    // Register dummy6 (numeric sensor)
    static pldm_numeric_sensor_context_t sensor6;
    pldm_numeric_sensor_config_t config6 = {
        .sensor_id = PLDM_SENSOR_ID_MCP_DUMMY_6,
        .notifications.on_sensor_get = on_numeric_sensor_get,
        .notifications.context = NULL 
    };
    status = fpfw_pldm_service_register_numeric_sensor(&sensor6, &config6);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}