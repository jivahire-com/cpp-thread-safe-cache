//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hm_error_domain_register.c
 * Implements mcp error domain related functions.
 */

/*------------- Includes -----------------*/
#include <bug_check.h>
#include <health_monitor_i.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static hm_error_domain_info_t error_domain_info[ACPI_ERROR_DOMAIN_COUNT] = {0};

/*------------- Functions ----------------*/
void hm_register_error_domain(uint16_t error_domain_idx,
                              const guid_t* error_domain_guid,
                              const char* error_domain_name,
                              hm_error_injection_cb_t err_inject_cb,
                              void* err_inject_ctx)
{
    if ((error_domain_idx < ACPI_ERROR_DOMAIN_COUNT) && (err_inject_cb != NULL))
    {
        memset(&error_domain_info[error_domain_idx], 0, sizeof(hm_error_domain_info_t));
        error_domain_info[error_domain_idx].error_domain_idx = error_domain_idx;

        if (error_domain_guid != NULL)
        {
            memcpy(&error_domain_info[error_domain_idx].fru_id, error_domain_guid, sizeof(guid_t));
            error_domain_info[error_domain_idx].valid_fru_id = true;
        }
        else
        {
            error_domain_info[error_domain_idx].valid_fru_id = false;
        }

        if (error_domain_name != NULL)
        {
            if (strlen(error_domain_name) < ACPI_FRU_TEXT_LENGTH)
            {
                memcpy(&error_domain_info[error_domain_idx].fru_text, error_domain_name, ACPI_FRU_TEXT_LENGTH);
            }
            else
            {
                BUG_ASSERT_PARAM(false, error_domain_name, ACPI_FRU_TEXT_LENGTH);
            }

            error_domain_info[error_domain_idx].valid_fru_str = true;
        }
        else
        {
            error_domain_info[error_domain_idx].valid_fru_str = false;
        }

        error_domain_info[error_domain_idx].injection_cb = err_inject_cb;
        error_domain_info[error_domain_idx].err_inject_ctx = err_inject_ctx;

        if (ddr_subsystem_enabled())
        {
            if (error_domain_info[error_domain_idx].activated == false)
            {
                activate_error_domain(error_domain_idx, error_domain_guid, error_domain_name);
                error_domain_info[error_domain_idx].activated = true;
            }
        }
        else
        {
            HM_LOG_INFO("activation pending for %s, waiting DDR init", get_error_domain_name(error_domain_idx));
            error_domain_info[error_domain_idx].activated = false;
        }
    }
    else
    {
        BUG_ASSERT_PARAM(false, err_inject_cb, error_domain_idx);
    }
}

void hm_register_cached_error_domain()
{
    for (uint32_t idx = 0; idx < sizeof(error_domain_info) / sizeof(error_domain_info[0]); idx++)
    {
        if (error_domain_info[idx].injection_cb != NULL && error_domain_info[idx].activated == false)
        {
            activate_error_domain(error_domain_info[idx].error_domain_idx,
                                  error_domain_info[idx].valid_fru_id ? &error_domain_info[idx].fru_id : NULL,
                                  error_domain_info[idx].valid_fru_str ? error_domain_info[idx].fru_text : NULL);

            error_domain_info[idx].activated = true;
        }
    }
}

hm_error_domain_info_t* hm_get_registered_error_domain(acpi_error_domain_t error_domain_idx)
{
    if (error_domain_idx < ACPI_ERROR_DOMAIN_COUNT && error_domain_info[error_domain_idx].injection_cb != NULL &&
        error_domain_info[error_domain_idx].activated == true)
    {
        return &error_domain_info[error_domain_idx];
    }

    return NULL;
}