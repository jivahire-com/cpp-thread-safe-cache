//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Implements PCIe config manager thread.
 */

#include "FpFwAssert.h" // for FPFW_RUNTIME_ASSERT

#include <DfwkClient.h>   // for DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE
#include <ErrorHandler.h> // for FPFwErrorRaise
#include <idsw_kng.h>
#include <pcie_config_variable.h>
#include <pcie_manager_i.h>   // for rpss_req_completion_cb, rpss_service_t...
#include <scp_pcie_manager.h> // for pcie_manager_context_t, pciess_complet...
#include <silibs_kng_soc.h>
#include <stdbool.h> // for true
#include <stdint.h>  // for uint8_t
#include <stdio.h>   // for fflush, printf, stdout
#include <tx_api.h>  // for TX_WAIT_FOREVER, ULONG, tx_queue_receive

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void config_variable_service_thread_fn(ULONG thread_input)
{
    pcie_config_manager_context_t* ctx = (pcie_config_manager_context_t*)(thread_input);
    ULONG event;
    kingsgate_pcie_root_bridge_config* rb_config_var = ctx->rb_config_var;
    kingsgate_pcie_vab_config* vab_config_var = ctx->vab_config_var;
    KNG_DIE_ID current_die_instance = idsw_get_die_id();

    int status = tx_event_flags_get(ctx->event_ptr, ctx->event_flags_mask, TX_AND, &event, TX_WAIT_FOREVER);
    if (status != TX_SUCCESS)
    {
        printf("%s: Failed to get event flags! TX_STATUS: %d\n", __func__, status);
        FPFwErrorRaise(status, 0, 0, 0, 0);
    }

    printf("PCIe configs are set\n");
    tx_event_flags_delete(ctx->event_ptr);

    /* The BARs for accelerators should be added here, see definitions in silibs_kng_soc.h*/
    /* Note that there is no MMIOL for either RB*/
    if (current_die_instance == DIE_0)
    {
        rb_config_var->rootbridge_config[PCIE_RPSS_COUNT * ROOT_PORTS_PER_RPSS].mmioh.base = D0_SDM_MMIOH_START;
        rb_config_var->rootbridge_config[PCIE_RPSS_COUNT * ROOT_PORTS_PER_RPSS].mmioh.limit = D0_SDM_MMIOH_END;
        rb_config_var->rootbridge_config[PCIE_RPSS_COUNT * ROOT_PORTS_PER_RPSS].bus.base = D0_SDM_RCIEP_BUS;
        rb_config_var->rootbridge_config[PCIE_RPSS_COUNT * ROOT_PORTS_PER_RPSS].bus.limit = D0_SDM_RCEC_BUS;
        rb_config_var->rootbridge_config[PCIE_RPSS_COUNT * ROOT_PORTS_PER_RPSS].flags.is_enabled = true;
        rb_config_var->rootbridge_config[PCIE_RPSS_COUNT * ROOT_PORTS_PER_RPSS].flags.is_integrated_endpoint = true;

        rb_config_var->rootbridge_config[PCIE_RPSS_COUNT * ROOT_PORTS_PER_RPSS + 1].mmioh.base = D0_CDED_MMIOH_START;
        rb_config_var->rootbridge_config[PCIE_RPSS_COUNT * ROOT_PORTS_PER_RPSS + 1].mmioh.limit = D0_CDED_MMIOH_END;
        rb_config_var->rootbridge_config[PCIE_RPSS_COUNT * ROOT_PORTS_PER_RPSS + 1].bus.base = D0_CDED_RCIEP_BUS;
        rb_config_var->rootbridge_config[PCIE_RPSS_COUNT * ROOT_PORTS_PER_RPSS + 1].bus.limit = D0_CDED_RCEC_BUS;
        rb_config_var->rootbridge_config[PCIE_RPSS_COUNT * ROOT_PORTS_PER_RPSS + 1].flags.is_enabled = true;
        rb_config_var->rootbridge_config[PCIE_RPSS_COUNT * ROOT_PORTS_PER_RPSS + 1].flags.is_integrated_endpoint = true;
    }
    else
    {
        rb_config_var->rootbridge_config[PCIE_RPSS_COUNT * ROOT_PORTS_PER_RPSS].mmioh.base = D1_SDM_MMIOH_START;
        rb_config_var->rootbridge_config[PCIE_RPSS_COUNT * ROOT_PORTS_PER_RPSS].mmioh.limit = D1_SDM_MMIOH_END;
        rb_config_var->rootbridge_config[PCIE_RPSS_COUNT * ROOT_PORTS_PER_RPSS].bus.base = D1_SDM_RCIEP_BUS;
        rb_config_var->rootbridge_config[PCIE_RPSS_COUNT * ROOT_PORTS_PER_RPSS].bus.limit = D1_SDM_RCEC_BUS;
        rb_config_var->rootbridge_config[PCIE_RPSS_COUNT * ROOT_PORTS_PER_RPSS].flags.is_enabled = true;
        rb_config_var->rootbridge_config[PCIE_RPSS_COUNT * ROOT_PORTS_PER_RPSS].flags.is_integrated_endpoint = true;

        rb_config_var->rootbridge_config[PCIE_RPSS_COUNT * ROOT_PORTS_PER_RPSS + 1].mmioh.base = D1_CDED_MMIOH_START;
        rb_config_var->rootbridge_config[PCIE_RPSS_COUNT * ROOT_PORTS_PER_RPSS + 1].mmioh.limit = D1_CDED_MMIOH_END;
        rb_config_var->rootbridge_config[PCIE_RPSS_COUNT * ROOT_PORTS_PER_RPSS + 1].bus.base = D1_CDED_RCIEP_BUS;
        rb_config_var->rootbridge_config[PCIE_RPSS_COUNT * ROOT_PORTS_PER_RPSS + 1].bus.limit = D1_CDED_RCEC_BUS;
        rb_config_var->rootbridge_config[PCIE_RPSS_COUNT * ROOT_PORTS_PER_RPSS + 1].flags.is_enabled = true;
        rb_config_var->rootbridge_config[PCIE_RPSS_COUNT * ROOT_PORTS_PER_RPSS + 1].flags.is_integrated_endpoint = true;
    }

    /* Print RB configurations */
    for (uint8_t i = 0; i < PCIE_CONFIG_VAR_NUM_RB; i++)
    {
        /* Copy over the rb config */
        pcie_root_bridge_config* rb_config = &(rb_config_var->rootbridge_config[i]);

        if (rb_config->flags.is_enabled == false)
        {
            continue;
        }

        printf("RB config %d is enabled\n", i);

        printf("MMIO64 range 0x%08lx%08lx - 0x%08lx%08lx\n",
               (unsigned long)(rb_config->mmioh.base >> 32),
               (unsigned long)(rb_config->mmioh.base),
               (unsigned long)(rb_config->mmioh.limit >> 32),
               (unsigned long)(rb_config->mmioh.limit));

        printf("MMIO32 range 0x%lx - 0x%lx\n",
               (unsigned long)rb_config->mmiol.base,
               (unsigned long)rb_config->mmiol.limit);

        printf("bus range 0x%lx - 0x%lx\n", (unsigned long)rb_config->bus.base, (unsigned long)rb_config->bus.limit);
    }

    /* Print VAB configurations */
    for (uint8_t i = 0; i < PCIE_CONFIG_VAR_NUM_VAB; i++)
    {
        printf("VAB %d smmu_bypass %d\n", i, (int)(vab_config_var->perf_config[i].smmu_bypass));
    }

    /* publish NV variable with RB aperture info */
    guid_t rb_config_guid_die_0 = PCIE_RB_CONFIG_VAR_DIE_0_GUID;
    guid_t vab_config_guid_die_0 = VAB_CONFIG_VAR_DIE_0_GUID;
    uint16_t rb_config_varname_die_0[] = PCIE_RB_CONFIG_VAR_DIE_0_NAME;
    uint16_t vab_config_varname_die_0[] = VAB_CONFIG_VAR_DIE_0_NAME;

    guid_t rb_config_guid_die_1 = PCIE_RB_CONFIG_VAR_DIE_1_GUID;
    guid_t vab_config_guid_die_1 = VAB_CONFIG_VAR_DIE_1_GUID;
    uint16_t rb_config_varname_die_1[] = PCIE_RB_CONFIG_VAR_DIE_1_NAME;
    uint16_t vab_config_varname_die_1[] = VAB_CONFIG_VAR_DIE_1_NAME;

    guid_t* rb_config_guid;
    guid_t* vab_config_guid;
    uint16_t* rb_config_varname;
    uint16_t* vab_config_varname;
    if (current_die_instance == DIE_0)
    {
        rb_config_guid = &rb_config_guid_die_0;
        vab_config_guid = &vab_config_guid_die_0;
        rb_config_varname = rb_config_varname_die_0;
        vab_config_varname = vab_config_varname_die_0;
    }
    else
    {
        rb_config_guid = &rb_config_guid_die_1;
        vab_config_guid = &vab_config_guid_die_1;
        rb_config_varname = rb_config_varname_die_1;
        vab_config_varname = vab_config_varname_die_1;
    }

    printf("RPSS CONFIG GUID - {%x-%x-%x-%x%x-%x%x%x%x%x%x}\n",
           (unsigned)rb_config_guid->guid1,
           (unsigned)rb_config_guid->guid2,
           (unsigned)rb_config_guid->guid3,
           (unsigned)rb_config_guid->guid4[0],
           (unsigned)rb_config_guid->guid4[1],
           (unsigned)rb_config_guid->guid4[2],
           (unsigned)rb_config_guid->guid4[3],
           (unsigned)rb_config_guid->guid4[4],
           (unsigned)rb_config_guid->guid4[5],
           (unsigned)rb_config_guid->guid4[6],
           (unsigned)rb_config_guid->guid4[7]);

    printf("VAB CONFIG GUID - {%x-%x-%x-%x%x-%x%x%x%x%x%x}\n",
           (unsigned)vab_config_guid->guid1,
           (unsigned)vab_config_guid->guid2,
           (unsigned)vab_config_guid->guid3,
           (unsigned)vab_config_guid->guid4[0],
           (unsigned)vab_config_guid->guid4[1],
           (unsigned)vab_config_guid->guid4[2],
           (unsigned)vab_config_guid->guid4[3],
           (unsigned)vab_config_guid->guid4[4],
           (unsigned)vab_config_guid->guid4[5],
           (unsigned)vab_config_guid->guid4[6],
           (unsigned)vab_config_guid->guid4[7]);
    printf("rb_config_varname ");
    for (uint8_t i = 0; i < sizeof(rb_config_varname_die_0) / sizeof(uint16_t); i++)
    {
        printf("%c", (char)rb_config_varname[i]);
    }
    printf("\nvab_config_varname ");
    for (uint8_t i = 0; i < sizeof(vab_config_varname_die_0) / sizeof(uint16_t); i++)
    {
        printf("%c", (char)vab_config_varname[i]);
    }
    printf("\n");

    /*TODO https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1886092 Use ICC library to publish the PCIe config variables*/
}