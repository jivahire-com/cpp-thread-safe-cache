//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_ip.c
 * This file provides interface to initializes the Accelerator IP(s) available
 * on the SoC.
 */

/*-------------------------------- Features ---------------------------------*/
/*
 * TODO: ADO 1831262: Remove the below flag and related code once HSP supports
 * initialization of CDEDSS Tower.
 */
#define FEATURE_SVP_WA_INIT_CDEDSS_TOWER_ON_BEHALF_OF_HSP

/*-------------------------------- Includes ---------------------------------*/
#include "accelerator_ip.h"

#include <FpFwAssert.h>          // for FPFW_RUNTIME_ASSERT
#include <accelerator_ip_priv.h> // for get_accelerator_ctxt
#include <atu_lib.h>             // for atu_map, atu_unmap, atu_map...
#include <idsw.h>                // for idsw_get_platform_sdv, idsw...
#include <idsw_kng.h>
#include <kng_soc_constants.h> // for DIE_INSTANCE
#include <silibs_platform.h>   // for debug_print, MMIO_UPDATE32
#include <silibs_status.h>     // for SILIBS_SUCCESS
#include <stdint.h>            // for int32_t, uintptr_t, uint32_t
#include <stdio.h>             // for printf, NULL
#include <string.h>            // for memcpy

#if defined(FEATURE_SVP_WA_INIT_CDEDSS_TOWER_ON_BEHALF_OF_HSP)
    #include <silibs_common.h> // for ALIGN_UP
    #include <tower_cdedss.h>  // for configure_cdedss_hsp_system_addr_map()
#endif                         /* FEATURE_SVP_WA_INIT_CDEDSS_TOWER_ON_BEHALF_OF_HSP */

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

#define ACCEL_PCR_CONFIG_TIMEOUT (10)

#define SLEEP_100_MS                                (100)
#define MAX_RETRY_CNT_FOR_SMMU_CONFIGURE_GPBA_CHECK (50)

#if defined(FEATURE_SVP_WA_INIT_CDEDSS_TOWER_ON_BEHALF_OF_HSP)
    #define HSSS_DFP_TOP_KD_CDEDSS_HSP_AXI_ADDRESS (0xffffff0000000U)
#endif /* FEATURE_SVP_WA_INIT_CDEDSS_TOWER_ON_BEHALF_OF_HSP */

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/
#if defined(FEATURE_SVP_WA_INIT_CDEDSS_TOWER_ON_BEHALF_OF_HSP)
static int32_t init_hsp_cdedss_tower_atu_map(atu_map_entry_t* p_cdeedss_tower_atu_map_entry)
{
    p_cdeedss_tower_atu_map_entry->mscp_start_address = 0;                                    // 0x78000000;
    p_cdeedss_tower_atu_map_entry->mscp_end_address = ALIGN_UP(0x8000000, ATU_PAGE_SIZE) - 1; // 0x7fffffff;

    p_cdeedss_tower_atu_map_entry->attribute.axprot0 = 0x3;
    p_cdeedss_tower_atu_map_entry->attribute.axprot1 = 0x2;
    p_cdeedss_tower_atu_map_entry->attribute.axnse = 0x3;

    int32_t ret = atu_map(ATU_ID_MSCP, p_cdeedss_tower_atu_map_entry);
    if (ret != SILIBS_SUCCESS)
    {
        debug_print("accel_lib: WA: CDEDSS Tower ATU Mapping failed\n");
        return ACCEL_RET_FAIL_ATU_MAP;
    }

    return ACCEL_RET_SUCCESS;
}

static int32_t init_hsp_cdedss_tower_atu_unmap(atu_map_entry_t* p_cdeedss_tower_atu_map_entry)
{
    // TODO: WA until HSP configures CDEDSS Tower
    int32_t ret = atu_unmap(ATU_ID_MSCP, p_cdeedss_tower_atu_map_entry);
    if (ret != SILIBS_SUCCESS)
    {
        debug_print("Accel IP CDEDSS Tower ATU Unmapping failed\n");
        return ACCEL_RET_FAIL_ATU_UNMAP;
    }

    return ACCEL_RET_SUCCESS;
}

static int32_t init_hsp_cdedss_tower(atu_map_entry_t* p_cdeedss_tower_atu_map_entry)
{
    // Create ATU Mapping (SCP View) for the CDEDSS Tower
    p_cdeedss_tower_atu_map_entry->ap_base_address = (uint64_t)HSSS_DFP_TOP_KD_CDEDSS_HSP_AXI_ADDRESS;

    // Create ATU Mapping (SCP View) for the Accel IP CDEDSS Tower
    int32_t ret = init_hsp_cdedss_tower_atu_map(p_cdeedss_tower_atu_map_entry);
    if (ret != ACCEL_RET_SUCCESS)
    {
        return ret;
    }

    uint32_t cdedss_tower_atu_mapped_addr = p_cdeedss_tower_atu_map_entry->mscp_start_address;
    CDEDSS_INSTANCE cdedss_id = 0;

    debug_print("accel_lib: WA: Initializing CDEDSS Tower\n");

    configure_cdedss_hsp_system_addr_map(HSSS_DFP_TOP_KD_CDEDSS_HSP_AXI_ADDRESS, cdedss_tower_atu_mapped_addr);
    configure_cdedss_vab_system_addr_map(cdedss_id, cdedss_tower_atu_mapped_addr);

    // Destroy ATU mapping
    ret = init_hsp_cdedss_tower_atu_unmap(p_cdeedss_tower_atu_map_entry);
    if (ret != ACCEL_RET_SUCCESS)
    {
        return ret;
    }

    debug_print("accel_lib: WA: CDEDSS Tower initialization complete.\n");

    return ret;
}
#endif /* FEATURE_SVP_WA_INIT_CDEDSS_TOWER_ON_BEHALF_OF_HSP */

static eACCELERATOR_TYPE get_accelip_type(ACCELIP_SS_INSTANCE accel_type)
{
    switch (accel_type)
    {
    case D0_ACCELIP_SDMSS:
    case D1_ACCELIP_SDMSS: {
        return ACCELERATOR_SDMSS;
    }
    case D0_ACCELIP_CDEDSS:
    case D1_ACCELIP_CDEDSS: {
        return ACCELERATOR_CDEDSS;
    }
    default: {
        return MAX_ACCELERATOR_TYPES;
    }
    }
}

static int32_t init_accelerator(subsystem_ctxt_t* p_ss_ctxt)
{
    int32_t ret = ACCEL_RET_SUCCESS;
    eACCELERATOR_TYPE accel_type = get_accelip_type(p_ss_ctxt->accelip_metadata.accel_type);

    if (accel_type == MAX_ACCELERATOR_TYPES)
    {
        debug_print("accel_lib: Invalid accel type\n");
        return ACCEL_RET_FAIL_INVALID_PARAMS;
    }

    if (accel_type == ACCELERATOR_CDEDSS)
    {
#ifdef FEATURE_SVP_WA_INIT_CDEDSS_TOWER_ON_BEHALF_OF_HSP
        // TODO: WA until HSP configures CDEDSS Tower
        atu_map_entry_t cdeedss_tower_atu_map_entry = {0};

        if (idsw_get_platform_sdv() == PLATFORM_SVP_SIM)
        {
            ret = init_hsp_cdedss_tower(&cdeedss_tower_atu_map_entry);
            if (ret != ACCEL_RET_SUCCESS)
            {
                debug_print("accel_lib: WA: CDEDSS Tower Init failed\n");
                return ret;
            }
        }
        else
#endif /* FEATURE_SVP_WA_INIT_CDEDSS_TOWER_ON_BEHALF_OF_HSP */
        {
            /*
             * Since HSP FW is not ready for the handshake, we would be consuming
             * the cmm scripts from FPGA team to configure the CDEDSS Tower.
             *
             * Once HSP FW is available, we need to follow the below steps:
             *   - Send request to HSP to configure the CDEDSS Tower via Mailbox.
             *   - Wait for the confirmation from HSP via Mailbox about the status
             *     to configure the CDEDSS Tower.
             *   - Take further action based on SUCCESS/FAIL.
             *
             * This ADO catpures the status of HSP FW readiness: 1568266
             */
        }
    }

    atu_map_entry_t atu_map_entry;
    memcpy((void*)&atu_map_entry, (void*)p_ss_ctxt->p_accelip_atu_map, sizeof(atu_map_entry_t));

    ret = atu_map(ATU_ID_MSCP, &atu_map_entry);
    if (ret != SILIBS_SUCCESS)
    {
        critical_print("Accel IP: init_accelerator: ATU MAP failed.\n");
        return ACCEL_RET_FAIL_ACCEL_IP;
    }
    debug_print("atu mapped for accel ip\n");
    
    if (idsw_get_platform_sdv() != PLATFORM_SVP_SIM)
    {
        // On FPGA, SDM and CDED Firmware are not preloaded in the respective ITCMs. (Hence setting to false for FPGA)
        // This will enable silibs to load the spin loop firmware in the ITCM before releasing the cores from reset.
        p_ss_ctxt->p_init_params->fw_preload_enabled = false;
    }

    ret = accelip_ss_init(atu_map_entry.mscp_start_address,
                          p_ss_ctxt->accelip_metadata.accel_type,
                          p_ss_ctxt->p_init_params);
    if (ret != SILIBS_SUCCESS)
    {
        critical_print("Accel IP: init_accelerator: accelip ss init failed.\n");
        return ACCEL_RET_FAIL_SS_INIT;
    }

    ret = atu_unmap(ATU_ID_MSCP, &atu_map_entry);
    if (ret != SILIBS_SUCCESS)
    {
        critical_print("Accel IP: init_accelerator: ATU UNMAP failed.\n");
        return ACCEL_RET_FAIL_ACCEL_IP;
    }

    return ACCEL_RET_SUCCESS;
}

/*----------------------------- Global Functions ----------------------------*/
int32_t scp_accelerators_init(void)
{
    DIE_INSTANCE current_die_instance = (DIE_INSTANCE)idsw_get_die_id();
    int ret = ACCEL_RET_SUCCESS;
    uint32_t accel_ctxt_size = 0;

    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);

    FPFW_RUNTIME_ASSERT(p_ss_ctxt != NULL);

    printf("Number of Accelerator instances present: %d\n", (int)accel_ctxt_size);

    // Init all available Accelerator instances
    for (uint32_t index = 0; index < accel_ctxt_size; index++)
    {
        // TODO (ADO 1728772) : init any particular accelerator instance only if that is enabled in fuse
        if (p_ss_ctxt[index].accelip_metadata.die_instance == current_die_instance)
        {
            printf("accel lib: Initializing for die_id = %d, accel_type = %d, accel_instance = %d\n",
                   p_ss_ctxt[index].accelip_metadata.die_instance,
                   p_ss_ctxt[index].accelip_metadata.accel_type,
                   p_ss_ctxt[index].accelip_metadata.accel_instance);

            ret = init_accelerator(&p_ss_ctxt[index]);

            FPFW_RUNTIME_ASSERT(ret == ACCEL_RET_SUCCESS);
        }
    }

    return ret;
}

int32_t scp_accelerators_isolation_control(void)
{
    // TODO: read the knob (and fuse if implemented).  For now we are hardcoding to de-isolate
    DIE_INSTANCE current_die_instance = (DIE_INSTANCE)idsw_get_die_id();
    int32_t ret = ACCEL_RET_SUCCESS;
    uint32_t accel_ctxt_size = 0;

    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);

    FPFW_RUNTIME_ASSERT(p_ss_ctxt != NULL);

    printf("Number of Accelerator instances present: %d\n", (int)accel_ctxt_size);

    // Init all available Accelerator instances
    for (uint32_t index = 0; index < accel_ctxt_size; index++)
    {
        // TODO (ADO 1728772) : init any particular accelerator instance only if that is enabled in fuse
        if (p_ss_ctxt[index].accelip_metadata.die_instance == current_die_instance)
        {
            debug_print("accel lib: Initializing for die_id = %d, accel_type = %d, accel_instance = %d\n",
                        p_ss_ctxt[index].accelip_metadata.die_instance,
                        p_ss_ctxt[index].accelip_metadata.accel_type,
                        p_ss_ctxt[index].accelip_metadata.accel_instance);

            atu_map_entry_t atu_map_entry;
            memcpy((void*)&atu_map_entry, (void*)p_ss_ctxt[index].p_accelip_atu_map, sizeof(atu_map_entry_t));

            ret = atu_map(ATU_ID_MSCP, &atu_map_entry);
            FPFW_RUNTIME_ASSERT(ret == ACCEL_RET_SUCCESS);

            ret = accelip_ss_enable_ip_isolation(atu_map_entry.mscp_start_address,
                                                 p_ss_ctxt[index].accelip_metadata.accel_type,
                                                 p_ss_ctxt[index].p_init_params->accelip_ss_cfg->isolation_enable);
            FPFW_RUNTIME_ASSERT(ret == ACCEL_RET_SUCCESS);

            ret = atu_unmap(ATU_ID_MSCP, &atu_map_entry);
            FPFW_RUNTIME_ASSERT(ret == ACCEL_RET_SUCCESS);
        }
    }
    return ret;
}
