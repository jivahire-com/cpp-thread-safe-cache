//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file ift.c
 *  IFT API definitions
 */

/*------------------------- Includes ------------------------*/

#include "ift_fw.h" // for IFT_RUN_MEM_TESTS_ASYNC, IFT_MEM_TEST_BIT_POS
#include "ift_i.h"  // for ift_dfwk_send_async_req

#include <DbgPrint.h>                 // for FPFW_DBGPRINT_INFO
#include <DfwkDriver.h>               // for DfwkAsyncRequestComplete
#include <FpFwUtils.h>                // for FPFW_UNUSED, FPFW_ARRAY_SIZE
#include <atu_api.h>                  // for atu_api_function
#include <bug_check.h>                // for BUG_ASSERT, BUG_ASSERT_PARAM
#include <fpfw_init.h>                // for fpfw_init_get_handle
#include <fpfw_status.h>              // for fpfw_status_t, FPFW_ICC_BASE_STATUS_SUCCESS
#include <fuse_common.h>              // for fuse_read_data
#include <idsw.h>                     // for idsw_get_die_id
#include <idsw_kng.h>                 // for DIE_0
#include <ift.h>                      // for ift_get_status, ift_get_intent_type
#include <kingsgate_fuse_defines.h>   // for CORE_DEFECT_MFG_MASK_CORE_DEFECT_MFG_31_0_BIT_OFFSET
#include <mpu.h>                      // for mpu_init
#include <mscp_exp_rmss_memory_map.h> // for SCP_EXP_MSCP_BOOT_DATA_BASE
#include <nvic.h>                     // for nvic_irq_disable
#include <scp_exp_top_regs.h>         // for SCP_EXP_TOP_SPI_CTRL_ADDRESS
#include <scp_top_regs.h>             // for SCP_TOP_DBGR_CSR_ADDRESS, SCP_TOP_SCP_EXP_ADDRESS
#include <silibs_common.h>            // for SL_1KB, BYTES_IN_WORD32
#include <silibs_status.h>            // for silibs_status_t, SILIBS_SUCCESS
#include <spi_bridge.h>               // for spi_controller_bulk_write
#include <startup_shutdown.h>         // for sos_start_phase
#include <startup_shutdown_ssi.h>     // for IFT_BOOT, STARTUP_PHASE_IFT_MEM_TEST_LOAD
#include <stdbool.h>                  // for bool, false
#include <system_info.h>              // for system_info_is_init_complete

/*-------------------- Structure Definitions -----------------*/

/*------------------------- Typedefs -------------------------*/

/*--------------------- Static Declarations ------------------*/

static uint8_t s_ift_enabled = IFT_STATUS_DISABLED; // IFT enabled status
static uint32_t s_ift_intent_type = 0;              // IFT intent type
static uint16_t s_ift_fw_idx = 0;
static uint32_t s_ift_current_fw_size = 0;
static uint32_t s_ift_skip_irq_num = 0; // IRQ number to skip during IFT

/*--------------------- Global Declarations ------------------*/

fpfw_icc_base_ctx_t* g_hsp_icc_ctx = NULL; // HSP ICC context
uint32_t g_ift_result_offset = 0;
silibs_status_t g_ift_execute_status;

/*----------------------- Static Prototype -------------------*/

/*-------------------------- Defines -------------------------*/

#define BYTES_TO_WORDS(x) ((x) / BYTES_IN_WORD32)

#define SPI_WRITE_ACTUAL_WAIT_CYCLES 9

/*---------------------- Static Functions --------------------*/

static uint32_t ift_get_pattern_version()
{
    return IFT_PATTERN_VERSION;
}

static void ift_get_core_defect_mfg_mask(uint32_t* core_defect_mfg_mask)
{
    core_defect_mfg_mask[0] = (uint32_t)fuse_read_data((uintptr_t)SYSTEM_FUSE_RAM_BASE_ADDR,
                                                       CORE_DEFECT_MFG_MASK_CORE_DEFECT_MFG_31_0_BIT_OFFSET,
                                                       CORE_DEFECT_MFG_MASK_CORE_DEFECT_MFG_31_0_WIDTH);
    core_defect_mfg_mask[1] = (uint32_t)fuse_read_data((uintptr_t)SYSTEM_FUSE_RAM_BASE_ADDR,
                                                       CORE_DEFECT_MFG_MASK_CORE_DEFECT_MFG_63_32_BIT_OFFSET,
                                                       CORE_DEFECT_MFG_MASK_CORE_DEFECT_MFG_63_32_WIDTH);
    core_defect_mfg_mask[2] = (uint32_t)fuse_read_data((uintptr_t)SYSTEM_FUSE_RAM_BASE_ADDR,
                                                       CORE_DEFECT_MFG_MASK_CORE_DEFECT_MFG_67_64_BIT_OFFSET,
                                                       CORE_DEFECT_MFG_MASK_CORE_DEFECT_MFG_67_64_WIDTH);
}

static void ift_inc_test_fw_idx(void)
{
    uint32_t intent_type = ift_get_intent_type();

    s_ift_fw_idx++;

    if (intent_type == IFT_MGR_IFT_INTENT_MEM_TEST)
    {
        BUG_ASSERT(s_ift_fw_idx <= NUM_IFT_MEM_TEST_FW_COUNT);
    }
    else
    {
        BUG_ASSERT(s_ift_fw_idx <= NUM_IFT_CORE_TEST_FW_COUNT);
    }
}

static bool ift_is_test_done()
{
    uint32_t intent_type = ift_get_intent_type();

    if (intent_type == IFT_MGR_IFT_INTENT_MEM_TEST)
    {
        return (s_ift_fw_idx == NUM_IFT_MEM_TEST_FW_COUNT);
    }

    return (s_ift_fw_idx == NUM_IFT_CORE_TEST_FW_COUNT);
}

static void ift_d1_set_d1_test_done()
{
    /**
     * SCP die 1 sets IFT test done MAGIC NUMBER to indicate SCP
     * die 0 that IFT test is completed on SCP die 1
     */
    int status = spi_controller_write_direct_instruction((uintptr_t)SPI_CTRL_MASTER_REG,
                                                         (uintptr_t)SCP_EXP_IFT_SYNC_MAGIC_NUM_ADDR,
                                                         SPI_WRITE_ACTUAL_WAIT_CYCLES,
                                                         SCP_EXP_IFT_SYNC_MAGIC_NUM);
    BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, 0);
}

static void ift_d0_reset_d1_test_done()
{
    MMIO_WRITE32((void*)SCP_EXP_IFT_SYNC_MAGIC_NUM_ADDR, 0);
}

static bool ift_d0_is_d1_test_done()
{
    return MMIO_READ32((void*)SCP_EXP_IFT_SYNC_MAGIC_NUM_ADDR) == SCP_EXP_IFT_SYNC_MAGIC_NUM;
}

/**
 * @brief Disables all IRQs except the specified one for IFT. While running
 * IFT tests, all interrupts are disabled as IFT pattern execution raises
 * all kind of error interrupt which is expected and should be ignored to
 * avoid interference with the test execution. However, HSP mailbox
 * interrupt needs to be kept enabled to allow communication with HSP.
 */
static void ift_disable_irq()
{
    for (uint32_t irq_num = 0; irq_num < 256; irq_num++)
    {
        if (irq_num == s_ift_skip_irq_num)
        {
            continue;
        }

        nvic_irq_disable(irq_num);
    }
}

static void ift_d0_transfer_fw_to_d1()
{
    uint32_t ift_fw_sz = ift_get_current_fw_size();

    /* Transfer the IFT firmware binary from D0 to D1 using SPI bridge */
    int status = spi_controller_bulk_write((uintptr_t)SPI_CTRL_MASTER_REG,
                                           (uintptr_t)SCP_EXP_IFT_BIN_DATA_BASE,
                                           (void*)SCP_EXP_IFT_BIN_DATA_BASE,
                                           BYTES_TO_WORDS(ift_fw_sz));
    BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, 0);
}

static void ift_wait_for_scp_die1()
{
    /**
     * HSP Die 0 resets the SoC upon receiving IFT status from SCP Die 0.
     * Make sure that SCP Die 0 synchronizes with SCP Die 1 before sending
     * IFT status to HSP Die 0.
     *
     * This will also help us synchronize between two core tests and ensure
     * that SCP die 0 does not proceed with its test until SCP die 1 has
     * completed its test.
     *
     * Note: HSP die 1 does not reset the SoC upon receiving IFT status
     * from SCP Die 1.
     */
    while (!ift_d0_is_d1_test_done())
    {
        /* Busy wait for SCP Die 1 to complete its IFT test */
        SLEEP_US(IFT_WAIT_FOR_SCP_DIE1_US);
    }
}

static void ift_d0_prepare_scp1()
{
    /* Reset the test done MAGIC NUMBER flag for SCP Die 1 */
    ift_d0_reset_d1_test_done();

    /* Transfer the IFT test vector binary to SCP Die 1 MSCP EXP RAM */
    ift_d0_transfer_fw_to_d1();

    /* Inform SCP die 1 that the IFT test vector binary has been transferred */
    ift_icc_send_d2d_fw_done_msg();
}

static void ift_fw_load_done_sos_cb(PDFWK_ASYNC_REQUEST_HEADER request, void* ctx)
{
    FPFW_UNUSED(request);
    FPFW_UNUSED(ctx);

    /* Prepare SCP Die 1 for the IFT test */
    ift_d0_prepare_scp1();

    /* Execute the IFT core test */
    ift_execute_test_dfwk();
}

static void ift_load_fw(void)
{
    uint32_t intent_type = ift_get_intent_type();
    IFT_DFWK_REQUEST_TYPE request_type;
    static ift_request_t s_dfwk_request = {0};

    /**
     * Queue the IFT memory test for execution if that test
     * is enabled in the intent type.
     */
    if (intent_type == IFT_MGR_IFT_INTENT_MEM_TEST)
    {
        request_type = IFT_MEM_TESTS_FW_LOAD_ASYNC;
    }
    /**
     * Queue the IFT core test for execution if that test
     * is enabled in the intent type.
     */
    else
    {
        request_type = IFT_CORE_TESTS_FW_LOAD_ASYNC;
    }

    ift_dfwk_send_async_req(&s_dfwk_request, request_type);
}

static void ift_mpu_update(void)
{
    const ARM_MPU_Region_t regions[] = {
        /**
         * MPU Region 13~14 - MSCP_EXP SRAM (First 768KB of RAM0) used for IFT post boot
         *                  Strongly Ordered
         *                  Priviledged R/W
         */
        {
            .RBAR = ARM_MPU_RBAR(13, SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS), // NOLINT
            .RASR = ARM_MPU_RASR(DISABLE_EXEC, ARM_MPU_AP_PRIV, TYPE_EXT_0, NON_SHAREABLE, NON_CACHEABLE, NON_BUFFERABLE, DISABLE_SUBREGION, ARM_MPU_REGION_SIZE_512KB),
        },
        {
            .RBAR = ARM_MPU_RBAR(14, SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS + (512 * SL_1KB)), // NOLINT
            .RASR = ARM_MPU_RASR(DISABLE_EXEC, ARM_MPU_AP_PRIV, TYPE_EXT_0, NON_SHAREABLE, NON_CACHEABLE, NON_BUFFERABLE, DISABLE_SUBREGION, ARM_MPU_REGION_SIZE_256KB),
        },
    };
    // Disable Dynamic read allocate mode
    SCnSCB->ACTLR |= SCnSCB_ACTLR_DISRAMODE_Msk;
    uint8_t region_count = FPFW_ARRAY_SIZE(regions);
    mpu_init(regions, region_count);
    // Enable I caches,  ensures that any stale instructions are not used.
    SCB_InvalidateICache();
    SCB_EnableICache();
    // Enable D caches, ensures any stale data is not used.
    SCB_InvalidateDCache();
    SCB_EnableDCache();
}

/*---------------------- Global Functions --------------------*/

void ift_init(fpfw_icc_base_ctx_t* hsp_icc_ctx)
{
    BUG_ASSERT(hsp_icc_ctx != NULL);

    ift_icc_get_ift_intent(hsp_icc_ctx, &s_ift_enabled, &s_ift_intent_type);

    /**
     * Updates the MPU to make MSCP BOOT DATA in MSCP EXP RAM strongly ordered.
     * SCP0 will be transferring IFT bins from MSCPEXP RAM 0 to MSCP EXP RAM 1.
     * Inorder to avoid any kind of issues and make life simple convert the area
     * used by IFT to strongly ordered memory.
     */
    if (ift_is_enabled())
    {
        ift_mpu_update();
    }

    g_hsp_icc_ctx = hsp_icc_ctx;

    FPFW_DBGPRINT_INFO("IFT Init: Enabled=%d, IntentType=0x%X\n", s_ift_enabled, s_ift_intent_type);
}

void ift_execute_test(PDFWK_ASYNC_REQUEST_HEADER request)
{
    idsw_die_id_t die_id = idsw_get_die_id();

    uint32_t core_defect_mfg_mask[CORE_DEFECT_MFG_MASK_ARRAY_SIZE] = {0};

    FPFW_DBGPRINT_INFO("IFT Execute Test: DieID=%d, FWIdx=%ld\n", die_id, ift_get_fw_idx());

    /* Increment the test firmware index */
    ift_inc_test_fw_idx();

    /* Disabled Interrupts */
    ift_disable_irq();

    /* Run the IFT core test */
    g_ift_execute_status = ift_execute_ist(SCP_TOP_DBGR_IFT_ADDRESS,
                                           (void*)ift_get_fw_addr(),
                                           ift_get_pattern_version(),
                                           SCP_EXP_IFT_RESULT_MAX - (g_ift_result_offset >> 1),
                                           (uint32_t*)SCP_EXP_IFT_RESULT_BASE,
                                           &g_ift_result_offset);

    FPFW_DBGPRINT_INFO("IFT Execute Test: ExecuteStatus=%d, ResultOffset=%ld\n", g_ift_execute_status, g_ift_result_offset);

    /**
     * `ift_execute_ist()` can generate multiple results and each result is size two word(32-bit)
     * `g_ift_result_offset` represents index for array of uint32_t(one word) and this array stores
     * the IFT test results. So the maximum valid value for `g_ift_result_offset` is
     * `SCP_EXP_IFT_RESULT_MAX << 1`
     */
    BUG_ASSERT_PARAM(g_ift_result_offset <= SCP_EXP_IFT_RESULT_MAX << 1, g_ift_result_offset, SCP_EXP_IFT_RESULT_MAX << 1);

    /* If running on Die 0 Wait for SCP Die 1 to complete the IFT test */
    if (die_id == DIE_0)
    {
        ift_wait_for_scp_die1();
    }

    /**
     * Since the test FWs are split into multiple binaries we need
     * to run them all before marking IFT test as done.
     */
    if (g_ift_execute_status != SILIBS_SUCCESS || ift_is_test_done())
    {
        /**
         * Currently we are ignoring the core defect manufacturing mask. Later
         * we will use this mask to filter out any defective cores from the
         * test results.
         *
         * Core defect MFG fuse contains information regarding faulty cores.
         * Since this cores are know to be faulty in future we need filter them
         * out of test results before sending them to HSP.
         *
         * Since currently parsing the test results logic is still in development.
         * Just read this fuses for now and use them in future.

         * TODO: 2820614 - SCP processes IFT errors
         */
        ift_get_core_defect_mfg_mask(core_defect_mfg_mask);

        /**
         * Send the IFT core test results to HSP. HSP will further pushout this
         * logs to BMC as SEL logs and at last send the IFT core test status to HSP
         *
         * Once the IFT test results and status are sent to HSP, SCP1 will notify it to
         * SCP0 so that SCP0 can proceed with its flow.
         */
        ift_icc_send_test_result_and_status(g_hsp_icc_ctx);
    }
    else
    {
        /* If running on Die 1, notify Die 0 that the IFT test has completed */
        if (die_id == DIE_1)
        {
            ift_notify_scp_die0();
        }

        /* Load the next IFT test firmware */
        ift_load_fw();
    }

    DfwkAsyncRequestComplete(request);
}

void ift_load_fw_sos(PDFWK_ASYNC_REQUEST_HEADER request)
{
    static startup_start_phase_request_t startup_ift_fw_load_request;
    idsw_die_id_t die_id = idsw_get_die_id();
    ssi_startup_stage_t ift_startup_stage;

    if (request->RequestType == IFT_MEM_TESTS_FW_LOAD_ASYNC)
    {
        ift_startup_stage = STARTUP_PHASE_IFT_MEM_FW_LOAD;
    }
    else
    {
        ift_startup_stage = STARTUP_PHASE_IFT_CORE_FW_LOAD;
    }

    FPFW_DBGPRINT_INFO("IFT: Send SoS request to download IFT FW.\n");

    /**
     * HSP die 1 does not has access to Flash 1 where IFT binaries reside
     * therefore SCP die 1 cannot request HSP die 1 to load IFT binaries.
     * Workaround here is SCP die 0 will copy the IFT binaries from
     * MSCP EXP RAM die 0 to MSCP EXP RAM die 1.
     *
     * This co-ordination is manage using ICC msg. SCP Die 0 will send a
     * mailbox message to SCP Die 1 once it has copied the IFT binaries
     * in MSCP EXP RAM die 1.
     */

    if (die_id == DIE_0)
    {
        DfwkAsyncRequestInitialize((void*)&startup_ift_fw_load_request.header.async, sizeof(startup_ift_fw_load_request));
        /**
         * Start the test by downloading the IFT core test firmware into MSCP EXP
         * memory from FLASH and then run the test.
         *
         * In order to that send a FW download mailbox request to HSP to load IFT
         * core test binaries
         *
         * Since IFT CORE test firmware is split into multiple firmware binaries,
         * we will load the first one and then run the test. The next firmware will
         * be loaded automatically by the IFT core test callback once the current
         * test firmware has completed execution.
         */
        sos_start_phase(fpfw_init_get_handle("sos_int"), &startup_ift_fw_load_request, IFT_BOOT, ift_startup_stage, ift_fw_load_done_sos_cb, NULL);
    }
    else
    {
        ift_icc_register_d2d_fw_done();
    }

    DfwkAsyncRequestComplete(request);
}

void ift_start_tests()
{
    ift_load_fw();
}

void ift_execute_test_dfwk(void)
{
    static ift_request_t s_dfwk_exec_request = {0};

    ift_dfwk_send_async_req(&s_dfwk_exec_request, IFT_EXECUTE_FW_ASYNC);
}

/***************************************************************
 *                      Util APIs                              *
 ***************************************************************/

void ift_set_skip_irq(uint32_t skip_irq_num)
{
    s_ift_skip_irq_num = skip_irq_num;
}

uint32_t ift_get_intent_type(void)
{
    return s_ift_intent_type;
}

bool ift_is_enabled(void)
{
    return s_ift_enabled == IFT_STATUS_ENABLED;
}

uint16_t ift_get_fw_idx(void)
{
    return s_ift_fw_idx;
}

uint32_t ift_get_fw_addr(void)
{
    return SCP_EXP_IFT_BIN_DATA_BASE;
}

void ift_set_current_fw_size(uint32_t size)
{
    BUG_ASSERT_PARAM(size <= SCP_EXP_IFT_BIN_DATA_MAX_SIZE, size, SCP_EXP_IFT_BIN_DATA_MAX_SIZE);
    s_ift_current_fw_size = size;
}

uint32_t ift_get_current_fw_size(void)
{
    return s_ift_current_fw_size;
}

void ift_notify_scp_die0()
{
    /* Notify SCP Die 0 that the IFT test has completed */
    ift_d1_set_d1_test_done();
}

#ifdef _WIN32
void reset_core_test_fw_idx(void)
{
    s_ift_fw_idx = 0;
}
#endif
