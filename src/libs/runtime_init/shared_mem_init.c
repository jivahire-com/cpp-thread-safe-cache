//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file shared_mem_init.c
 * This file contains the initialization of the shared memory(RSM/ARSM).
 */

/*------------- Includes -----------------*/

#include <DbgPrint.h>
#include <atu_api.h>
#include <atu_lib.h>
#include <boot_status.h>
#include <fpfw_init.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <kng_atu_mappings.h>
#include <string.h>
#include <system_info.h>
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/
// TODO: Silib should provide these
#define VOYAGER_RSM_ADDRESS_DIEn(n) ((0x2F000000UL) + (0x1000000000UL * (n)))
#define VOYAGER_RSM_SIZE            (0x10000)
#define ATU_MAPPING_VOYAGER_RSM_RAM(die_id)                                                \
    ATU_MAPPING((die_id == 0 ? VOYAGER_RSM_ADDRESS_DIEn(0) : VOYAGER_RSM_ADDRESS_DIEn(1)), \
                0,                                                                         \
                (ALIGN_UP(VOYAGER_RSM_SIZE, ATU_PAGE_SIZE) - 1),                           \
                {ATU_BUS_ATTR_NS})

/*-------------- Functions ---------------*/

PLACED_CODE FPFW_INIT_COMPONENT(shared_mem, FPFW_INIT_DEPENDENCIES("atu_svc", "hw_ver", "sysinfo", "debug_print", "boot_stat"))
{
    int32_t result = FPFW_INIT_STATUS_SUCCESS;

    if (IS_PLATFORM_SVP())
    {
        /*
         * SVP always starts from a clean state for every sim run so ARSM/RSM is
         * guaranteed to be zeroed out on it.
         *
         * More so, SVP offers a pre-loading feature to load code/data on
         * the ARSM at simulation start time. SCP should not clobber ARSM state
         * to allow using this feature.
         */
        FPFW_DBGPRINT_INFO("Skip shared_mem_init on SVP\n");
        goto done;
    }

    // Only clear the ARSM/RSM on a cold boot
    if (!system_info_is_warm_start())
    {
        // ARSM zero-init
        if (idsw_get_die_id() == DIE_0)
        {
            // Clear DIE0 ARSM ROOT
            uintptr_t arsm_base = MSCP_ATU_AP_WINDOW_ARSM_DIE_0_ROOT_BASE_ADDR;
            memset((void*)arsm_base, 0, MSCP_ATU_AP_WINDOW_ARSM_DIE_0_ROOT_SIZE);
            FPFW_DBGPRINT_INFO("Cleared ARSM ROOT at 0x%08lX\n", arsm_base);

            atu_map_entry_t arsm_atu_entry = {
                .ap_base_address = D0_ARSM_TFA_RMM_SHARED_BASE,
                .mscp_start_address = 0,
                .mscp_end_address = D0_MSCP_ARSM_SRAM_SIZE - D0_ARSM_TFA_RMM_SHARED_BASE - 1,
                .attribute = {ATU_BUS_ATTR_NS},
            };

            // Clear DIE0 ARSM TFA region
            result = atu_map(ATU_ID_MSCP, &arsm_atu_entry);
            if (result != SILIBS_SUCCESS)
            {
                FPFW_DBGPRINT_ERROR("Failed to map ARSM TFA shared region, status=%d\n", result);
                goto done;
            }
            memset((void*)(uintptr_t)arsm_atu_entry.mscp_start_address, 0, D0_MSCP_ARSM_SRAM_SIZE - D0_ARSM_TFA_RMM_SHARED_BASE);
            FPFW_DBGPRINT_INFO("Cleared ARSM TFA shared region at 0x%08lX\n", arsm_atu_entry.mscp_start_address);
            result = atu_unmap(ATU_ID_MSCP, &arsm_atu_entry);
            if (result != SILIBS_SUCCESS)
            {
                FPFW_DBGPRINT_ERROR("Failed to unmap ARSM TFA shared region, status=%d\n", result);
                goto done;
            }
        }
        else
        {
            // Clear DIE1 ARSM ROOT
            uintptr_t arsm_base = MSCP_ATU_AP_WINDOW_ARSM_DIE_1_ROOT_BASE_ADDR;
            memset((void*)arsm_base, 0, MSCP_ATU_AP_WINDOW_ARSM_DIE_1_ROOT_SIZE);
            FPFW_DBGPRINT_INFO("Cleared ARSM ROOT at 0x%08lX\n", arsm_base);

            // Clear DIE1 ARSM NS
            arsm_base = MSCP_ATU_AP_WINDOW_ARSM_DIE_1_NS_BASE_ADDR;
            memset((void*)arsm_base, 0, MSCP_ATU_AP_WINDOW_ARSM_DIE_1_NS_SIZE);
            FPFW_DBGPRINT_INFO("Cleared ARSM NS at 0x%08lX\n", arsm_base);
        }

        // RSM zero-init
        atu_map_entry_t rsm_atu_entry = ATU_MAPPING_VOYAGER_RSM_RAM(idsw_get_die_id());
        result = atu_map(ATU_ID_MSCP, &rsm_atu_entry);

        if (result != SILIBS_SUCCESS)
        {
            FPFW_DBGPRINT_ERROR("Failed to map RSM on shared_mem_init, status=%d\n", result);
            goto done;
        }

        memset((void*)(uintptr_t)rsm_atu_entry.mscp_start_address, 0, VOYAGER_RSM_SIZE);
        FPFW_DBGPRINT_INFO("Cleared RSM at 0x%08lX\n", rsm_atu_entry.mscp_start_address);

        result = atu_unmap(ATU_ID_MSCP, &rsm_atu_entry);
    }
done:
    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(&boot_status_req,
                            (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_SHARED_MEM_INIT_END
                                                             : MSCP_BOOT_STATUS_CODE_MCP_SHARED_MEM_INIT_END,
                            GEN_BOOT_STATUS_EX_GENERIC_CODE(
                                (idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                MSCP_GENERIC,
                                (idsw_get_die_id() == DIE_0)
                                    ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                    : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){result, NULL};
}
