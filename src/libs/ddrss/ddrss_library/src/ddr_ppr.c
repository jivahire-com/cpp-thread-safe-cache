//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file ddr_ppr.c
 *  Contains Post-Package Repair (PPR) related APIs
 */

/*------------- Includes -----------------*/
#include "ddr_ppr.h"

#include "ddrss_sdl.h"

#include <DbgPrint.h>
#include <FPFwInterrupts.h>
#include <arm_intrinsic.h> // for __DSB on Windows builds (empty define)
#include <atu_api.h>
#include <atu_lib.h>
#include <boot_status.h>
#include <bug_check.h>
#include <ddr_i3c.h>
#include <ddr_manager.h>
#include <ddr_manager_events.h>
#include <ddr_manager_i3c.h>
#include <ddrss.h>
#include <ddrss_knobs.h>
#include <ddrss_lib.h>
#include <fpfw_cfg_mgr.h>
#include <fpfw_icc_base.h>
#include <gtimer_prodfw.h>
#include <health_monitor_icc.h>
#include <icc_mhu.h>
#include <icc_platform_defines.h>
#include <idhw.h> // for idhw_is_single_die_boot_en
#include <kng_scp_tfa_shared.h>
#include <mscp_exp_rmss_memory_map.h>
#include <mscp_exp_spi_synchronize_dies.h>
#include <nvic.h> // Has nested include of cmsis_gcc_m.h for __DSB() intrinsic
#include <sel.h>

#ifndef PPR_DEBUG
    #define PPR_DEBUG 1
#endif

#if PPR_DEBUG
    #define PPR_DBG(...) FPFW_DBGPRINT_INFO(__VA_ARGS__)
#else
    #define PPR_DBG(...) \
        do               \
        {                \
        } while (0)
#endif

/*-- Symbolic Constant Macros (defines) --*/
#define ICC_MODULE_DDR (0x000A)

#define ICC_D2D_PPR_TYPE     ICC_GEN_CMD(ICC_MODULE_DDR, 0x1) // 0x000A_0001
#define SPD_OFFSET_DDR5      (640U)
#define SPD_STRUCT_SIZE_DDR5 SIZE_16_BYTES
// #define SPD_UPDATE_TESTING           // Defined to prevent SPD readback from being updated for testing
// #define SPD_READ_DEBUGPRINT_ENABLE        // Define to enable debug prints for SPD readback
#define SPD_VERSION                       (3U)
#define PROD_DDRSS_MAX_DIMM_DQ_PER_SUB_CH (40)
#define TIMEOUT_MS                        (10000U)

/* -- Prototypes --*/
static void init_ppr_shared_memory_arsm0(ddrss_cfg_knobs_t* ddrss_cfgs);
static void synchronize_dies_for_ppr(DIE_INSTANCE die_id);
static void synchronize_dies_before_init(DIE_INSTANCE die_id);
static DDRSS_PPR_TYPE handle_die1_ppr_reception(volatile ddr_ppr_sync_msg_t* ppr_sync_msg);
static DDRSS_PPR_TYPE d0_determine_ppr_type(ddrss_cfg_knobs_t* ddrss_cfgs);

/*-- Declarations (Statics and globals) --*/
static var_service_req_ctx_t ppr_get_var_svc_ctx = {};
static var_service_req_ctx_t ppr_set_var_svc_ctx = {};

// Compiler was unhappy with defining array size with the const variable
const int MAX_PPR_DEFECTS_SUPPORTED_PER_DIE = 48;
ddrss_addr_t g_ddrss_defect_list[48]; // No pre-init to keep in .bss section

/*------------- Functions ----------------*/

// ============================================================================
// PPR Setup Flow (entry + early sync)
// ============================================================================

void ppr_setup(ddrss_cfg_knobs_t* ddrss_cfgs)
{
    DDRSS_PPR_TYPE ppr_type = DDRSS_PPR_NONE;
    DIE_INSTANCE die_id = ddrss_cfgs->die_id;
    volatile ddr_ppr_sync_msg_t* ppr_sync_msg = NULL;
    atu_map_entry_t arsm0_map_entry = {0};

    // Event Trace (ppr_setup begin)
    DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_PPR_SETUP_BEGIN);

    if (idhw_is_single_die_boot_en())
    {
        PPR_DBG("PPR: Single die boot - skipping PPR setup\n");
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_PPR_SETUP_SKIPPED_SINGLE_DIE);
        ddrss_cfgs->ext_knobs.ppr_type = DDRSS_PPR_NONE;
        return;
    }

    // Step 1: Initialize shared memory & sets global pointer to ppr_sync_msg
    init_ppr_shared_memory_arsm0(ddrss_cfgs);
    ppr_sync_msg = get_ppr_sync_msg_ptr(&arsm0_map_entry);

    // Step 2: Die 0 determines PPR type and writes it to shared memory before sync
    if (die_id == SOC_D0)
    {
        ppr_type = d0_determine_ppr_type(ddrss_cfgs);
        send_ppr_type_to_die1(ppr_type, ppr_sync_msg);
    }

    // Step 3: Synchronize dies - guarantees Die 0's write is visible to Die 1
    synchronize_dies_for_ppr(die_id);

    // Step 4: Die 1 reads PPR type from shared memory after sync
    if (die_id == SOC_D1)
    {
        ppr_type = handle_die1_ppr_reception(ppr_sync_msg);
    }

    // Step 5: Configure PPR type for ddrss library
    ddrss_cfgs->ext_knobs.ppr_type = ppr_type;

    // Step 6: Build defect lists if needed (Todo - next PR)

    // Step 7: Final sync before ddr_init()
    FPFW_DBGPRINT_INFO("PPR: Final die sync before ddr_init()\n");
    synchronize_dies_before_init(die_id);

    unmap_sdl_arsm0(&arsm0_map_entry);

    // Event Trace (ppr_setup end)
    DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_PPR_SETUP_END);
}

/**
 * @brief Initialize shared memory for PPR operations
 * @return None
 */
static void init_ppr_shared_memory_arsm0(ddrss_cfg_knobs_t* ddrss_cfgs)
{
    DIE_INSTANCE die_id = ddrss_cfgs->die_id;

    // Only Die 0: Initialize ARSM0 shared memory for PPR operations
    if (die_id == SOC_D0)
    {
        atu_map_entry_t arsm0_map_entry = {0};
        memset((void*)get_sdl_arsm0_addr(&arsm0_map_entry), 0, D0_ARSM_SDL_RESERVED_SIZE);
        __DSB();
        unmap_sdl_arsm0(&arsm0_map_entry);
    }
}

/**
 * @brief Get pointer to PPR sync message structure
 * @return Pointer to sync message structure
 */
volatile ddr_ppr_sync_msg_t* get_ppr_sync_msg_ptr(atu_map_entry_t* map_entry)
{
    return (volatile ddr_ppr_sync_msg_t*)((uint8_t*)get_sdl_arsm0_addr(map_entry) + SDL_MAX_SIZE);
}

/**
 * @brief Get pointer to defect list (now in DTCM)
 * @return Pointer to defect array located .bss section
 */
ddrss_addr_t* ppr_get_defect_array_ptr(void)
{
    return (ddrss_addr_t*)&g_ddrss_defect_list[0];
}

/**
 * @brief Synchronize both dies before PPR operations
 * @param die_id Current die instance
 * @param ppr_sync_msg Pointer to sync message structure
 */
static void synchronize_dies_for_ppr(DIE_INSTANCE die_id)
{
    int sts = SILIBS_SUCCESS;

    // FPFW_DBGPRINT_INFO("PPR: Syncing dies\n");
    mscp_exp_spi_sync_point_t d2d_ddr_sync_point;
    d2d_ddr_sync_point.local_write_addr = SCP_EXP_D2D_SYNC_DDR_BDAT_CRC_BASE;
    d2d_ddr_sync_point.remote_write_addr = SCP_EXP_D2D_SYNC_DDR_BDAT_CRC_BASE + sizeof(uint32_t);
    d2d_ddr_sync_point.value = RMSS_D2D_DDR_PPR_SYNC_POINT_BEFORE_BUILD_DEFECT_LIST;

    sts = mscp_exp_spi_synchronize_dies_timeout(d2d_ddr_sync_point, die_id, (uint32_t)TIMEOUT_MS);
    if (sts != SILIBS_SUCCESS)
    {
        PPR_DBG("Error Syncing dies for ppr_setup\n");
        BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, sts, 0);
    }
}

/**
 * @brief Synchronize both dies before ddr_init
 * @param die_id Current die instance
 */
static void synchronize_dies_before_init(DIE_INSTANCE die_id)
{
    PPR_DBG("PPR: Syncing dies\n");
    mscp_exp_spi_sync_point_t sync_pt = {.local_write_addr = SCP_EXP_D2D_SYNC_DDR_BDAT_CRC_BASE,
                                         .remote_write_addr = SCP_EXP_D2D_SYNC_DDR_BDAT_CRC_BASE + sizeof(uint32_t),
                                         .value = RMSS_D2D_DDR_PPR_SYNC_POINT_READY_FOR_DDRSS_INIT};

    int sts = mscp_exp_spi_synchronize_dies_timeout(sync_pt, die_id, (uint32_t)TIMEOUT_MS);
    if (sts != SILIBS_SUCCESS)
    {
        PPR_DBG("Error Syncing dies at end of ppr_setup\n");
        BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, sts, 0);
    }
    else
    {
        PPR_DBG("Dies synced successfully\n");
    }
}

// ============================================================================
// Variable Service Helpers (PPR configuration)
// ============================================================================

/**
 * @brief Retrieve PPR run configuration from UEFI variable synchronously
 *
 * Reads the PPR_RUN UEFI variable to determine what type of Post-Package Repair
 * operation should be performed (hPPR, sPPR, mPPR, or NONE). This variable is
 * typically set by BIOS/UEFI to request firmware-assisted memory repair.
 *
 * @param[out] ppr_type_req Pointer to store retrieved PPR type (DDRSS_PPR_TYPE)
 * @return SILIBS_SUCCESS if variable read successfully (or not found)
 * @return SILIBS_E_PARAM if ppr_type_req is NULL or PPR type is invalid
 * @return SILIBS_E_SUPPORT if variable service fails unexpectedly
 */
int32_t get_ppr_run_var_sync(void* ppr_type_req)
{
    uint16_t ppr_var_name[] = PPR_RUN_CFG_VAR_NAME;
    static const guid_t ppr_var_guid = PPR_RUN_CFG_VAR_GUID;
    var_service_shared_mem_t mem_ctx = {0};

    mem_ctx.payload_base = (uintptr_t)SCP_EXP_SCP_DDRSS_PPR_VARIABLE_SERVICE_PAYLOAD_BASE;
    mem_ctx.max_payload_size = SCP_EXP_SCP_DDRSS_PPR_VARIABLE_SERVICE_PAYLOAD_SIZE;

    //! initialize variable services ctx for the region
    memset((void*)mem_ctx.payload_base, 0, mem_ctx.max_payload_size);
    variable_service_initialize_ctx(&ppr_get_var_svc_ctx, &mem_ctx);

    var_service_req_params_t ppr_get_var_req = {0};
    ppr_get_var_req.variable_name_ptr = (uint16_t*)ppr_var_name;
    ppr_get_var_req.variable_name_size = sizeof(ppr_var_name);
    memcpy(&ppr_get_var_req.vendor_namespace_guid, &ppr_var_guid, sizeof(ppr_get_var_req.vendor_namespace_guid));
    if (ppr_type_req == NULL)
    {
        return SILIBS_E_PARAM;
    }
    ppr_get_var_req.data_size = sizeof(DDRSS_PPR_TYPE);
    ppr_get_var_req.data = (uint8_t*)ppr_type_req; // Pointer to store the retrieved PPR type
    ppr_get_var_req.attributes_size = 0;

    DDR_MANAGER_ET_STATUS_PARAM(DDR_MANAGER_ET_TYPE_PPR_GET_VARIABLE, 0);
    int status = variable_service_sync_get_variable(&ppr_get_var_svc_ctx, &ppr_get_var_req);

    // Other variable service error
    if ((status != KNG_SUCCESS) && (status != KNG_E_NOT_FOUND))
    {
        FPFW_DBGPRINT_INFO("[DDRPPR]  failed, sts 0x%x\n", status);
        DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_PPR_GET_VARIABLE_FAIL, status);
        variable_service_unlock_get_var_ctx(&ppr_get_var_svc_ctx);

        return status;
    }

    variable_service_unlock_get_var_ctx(&ppr_get_var_svc_ctx);
    DDR_MANAGER_ET_STATUS_PARAM(DDR_MANAGER_ET_TYPE_PPR_VARIABLE_FOUND, *((uint32_t*)(ppr_type_req)));
    FPFW_DBGPRINT_INFO("[DDRPPR]   OK\n");

    switch (*((int*)ppr_type_req))
    {
    case DDRSS_PPR_NONE:
        FPFW_DBGPRINT_INFO("DDRSS_PPR_NONE\n");
        break;
    case DDRSS_HPPR:
        FPFW_DBGPRINT_INFO("hPPR\n");
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_PPR_HPPR);
        break;
    case DDRSS_SPPR:
        FPFW_DBGPRINT_INFO("sPPR (Only to be used for testing)\n");
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_PPR_SPPR);
        break;
    case DDRSS_MPPR:
        FPFW_DBGPRINT_INFO("mPPR\n");
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_PPR_MPPR);
        break;
    default:
        FPFW_DBGPRINT_INFO("Invalid PPR type\n ");
        DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_PPR_INVALID_TYPE, *((uint32_t*)(ppr_type_req)));
        return SILIBS_E_PARAM;
        break;
    }
    return SILIBS_SUCCESS;
}

void ppr_type_reset_variable()
{
    uint16_t ppr_var_name[] = PPR_RUN_CFG_VAR_NAME;
    static const guid_t ppr_var_guid = PPR_RUN_CFG_VAR_GUID;
    var_service_shared_mem_t mem_ctx = {0};

    mem_ctx.payload_base = (uintptr_t)SCP_EXP_SCP_DDRSS_PPR_VARIABLE_SERVICE_PAYLOAD_BASE;
    mem_ctx.max_payload_size = SCP_EXP_SCP_DDRSS_PPR_VARIABLE_SERVICE_PAYLOAD_SIZE;
    memset((void*)mem_ctx.payload_base, 0, mem_ctx.max_payload_size);

    //! initialize variable services ctx for the region
    variable_service_initialize_ctx(&ppr_set_var_svc_ctx, &mem_ctx);

    DDRSS_PPR_TYPE no_action = DDRSS_PPR_NONE;

    var_service_req_params_t ppr_set_var_req = {0};
    ppr_set_var_req.variable_name_ptr = (uint16_t*)ppr_var_name;
    ppr_set_var_req.variable_name_size = sizeof(ppr_var_name);
    memcpy(&ppr_set_var_req.vendor_namespace_guid, &ppr_var_guid, sizeof(ppr_set_var_req.vendor_namespace_guid));
    ppr_set_var_req.data_size = sizeof(no_action);
    ppr_set_var_req.data = (uint8_t*)&no_action;
    ppr_set_var_req.attributes.as_uint32 =
        EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS;

    DDR_MANAGER_ET_STATUS_PARAM(DDR_MANAGER_ET_TYPE_PPR_VARIABLE_RESET, 0);
    variable_service_sync_set_variable(&ppr_set_var_svc_ctx, &ppr_set_var_req);
    variable_service_unlock_get_var_ctx(&ppr_set_var_svc_ctx);
    DDR_MANAGER_ET_STATUS_PARAM(DDR_MANAGER_ET_TYPE_PPR_VARIABLE_RESET, 1);
}

// ============================================================================
// Die Synchronization Helpers
// ============================================================================

void send_ppr_type_to_die1(DDRSS_PPR_TYPE ppr_type, volatile ddr_ppr_sync_msg_t* ppr_sync_msg)
{
    FPFW_DBGPRINT_INFO("[DIE_0] ENTERED send_ppr_type_to_die1 with ppr_type=%d\n", ppr_type);
    DDR_MANAGER_ET_STATUS_PARAM(DDR_MANAGER_ET_TYPE_PPR_SYNC_DO_SEND_D1, (uint32_t)ppr_type);

    BUG_ASSERT(ppr_sync_msg != NULL);
    memset((void*)ppr_sync_msg, 0, sizeof(ddr_ppr_sync_msg_t));
    __DSB();

    ppr_sync_msg->ppr_type = (uint8_t)ppr_type;
    ppr_sync_msg->valid = DDR_PPR_VALID_DATA;
    __DSB();

    FPFW_DBGPRINT_INFO("[DIE_0] PPR type written to shared memory, awaiting die sync\n");
}

/**
 * @brief Receive PPR type on Die 1 from Die 0 via shared mailbox in ARSM0.
 *        Must be called after synchronize_dies_for_ppr() guarantees data is available.
 * @param ppr_type Pointer to store the received PPR type
 * @param ppr_sync_msg Pointer to the PPR sync message structure
 */
void receive_ppr_type_from_die0(DDRSS_PPR_TYPE* ppr_type, volatile ddr_ppr_sync_msg_t* ppr_sync_msg)
{
    DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_PPR_SYNC_D1_RECV_D0);

    FPFW_DBGPRINT_INFO("[DIE_1] ENTERED receive_ppr_type_from_die0\n");

    BUG_ASSERT(ppr_sync_msg != NULL);
    __DSB(); // Ensure memory barrier before reading shared memory

    // After die sync, the data must be valid
    BUG_ASSERT_PARAM(ppr_sync_msg->valid == DDR_PPR_VALID_DATA, ppr_sync_msg->valid, DDR_PPR_VALID_DATA);

    DDR_MANAGER_ET_STATUS_PARAM(DDR_MANAGER_ET_TYPE_PPR_SYNC_D1_RECV_D0, (uint32_t)ppr_sync_msg->ppr_type);
    *ppr_type = (DDRSS_PPR_TYPE)ppr_sync_msg->ppr_type;
    FPFW_DBGPRINT_INFO("[DIE_1] Received ppr_type=%d from DIE_0\n", *ppr_type);
}

// ============================================================================
// PPR Type Selection and SDL Load
// ============================================================================

/**
 * @brief Handle Die 1 receiving PPR type from Die 0
 * @param ppr_sync_msg Pointer to sync message structure
 * @return PPR type received from Die 0, or DDRSS_PPR_NONE if should abort
 */
static DDRSS_PPR_TYPE handle_die1_ppr_reception(volatile ddr_ppr_sync_msg_t* ppr_sync_msg)
{
    DDRSS_PPR_TYPE ppr_type = DDRSS_PPR_NONE;

    receive_ppr_type_from_die0(&ppr_type, ppr_sync_msg);
    FPFW_DBGPRINT_INFO("[DIE_1] handle_die1_ppr_reception returning ppr_type=%d\n", ppr_type);

    return ppr_type;
}

/**
 * @brief Determine PPR type and reset variable if needed (Die 0 only).
 *        Does not send to Die 1; that is handled separately.
 * @param ddrss_cfgs DDRSS configuration structure
 * @return Final PPR type to execute
 */
static DDRSS_PPR_TYPE d0_determine_ppr_type(ddrss_cfg_knobs_t* ddrss_cfgs)
{
    DDRSS_PPR_TYPE ppr_type = DDRSS_PPR_NONE;
    int32_t status;

    // Get PPR type from UEFI variable
    status = get_ppr_run_var_sync(&ppr_type);

    if (status != SILIBS_SUCCESS)
    {
        FPFW_DBGPRINT_INFO("Error with variable service but ok to boot: 0x%X\n", status);
        ppr_type = DDRSS_PPR_NONE;
    }
    else if ((ppr_type == DDRSS_PPR_NONE) || (ppr_type > DDRSS_MPPR))
    {
        FPFW_DBGPRINT_INFO("Skip PPR: type=NONE or invalid\n");
        ppr_type = DDRSS_PPR_NONE;
    }
    else if (ddrss_cfgs->reset_reason != DDRSS_SYS_RESET_COLD)
    {
        FPFW_DBGPRINT_INFO("Skip PPR: (reset reason: %d)\n", ddrss_cfgs->reset_reason);
        ppr_type = DDRSS_PPR_NONE;
    }

    // Reset PPR variable to prevent boot loops
    if (ppr_type != DDRSS_PPR_NONE)
    {
        FPFW_DBGPRINT_INFO("Reseting PPR variable to prevent boot loops\n");
        ppr_type_reset_variable();
    }

    if (ppr_type == DDRSS_PPR_NONE)
    {
        FPFW_DBGPRINT_INFO("PPR type is NONE, no PPR will be performed\n");
    }

    return ppr_type;
}
