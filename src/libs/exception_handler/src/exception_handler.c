//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file exception_handler_init.c
 *  Sets up the exception handler for the Cortex-M7
 */

/*------------- Includes -----------------*/
#include "exception_handler_i.h"

#include <atu_api.h>
#include <atu_lib.h>
#include <bug_check.h> // for BUG_CHECK
#include <cmsdk_wd.h>  // for wdog_cmsdk_apb_disable
#include <cper.h>
#include <crash_dump.h> // for crash_dump_handler, crash_dump_bug_check_initiated_dump
#include <health_monitor.h>
#include <idsw_kng.h>
#include <interrupts.h>
#include <kng_atu_mappings.h> // for ATU_MAPPING_SDMSS_BASE, ATU_MAPPING_SDM_RCIEP_ECAM_BASE ...
#include <kng_error.h>        // for KNG_CD_DEFAULT_EXCEPTION, KNG_CD_HARDFAULT_EXCEPTION ...
#include <mscp_error_domain.h>
#include <nvic.h> // for nvic_set_isr_fault, nvic_irq_set_isr ...
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#if defined(MCP_RUNTIME_INIT)
    #include <mcp_exp_csr_regs.h>
    #include <mcp_exp_top_regs.h>
    #include <mcp_top_regs.h>
#elif defined(SCP_RUNTIME_INIT)
    #include <scp_exp_csr_regs.h>
    #include <scp_exp_top_regs.h>
    #include <scp_top_regs.h>
#endif
#include <shared_sram_ecc_ras_registers_regs.h>
#include <stddef.h> // for NULL
#include <stdint.h> // for uint32_t
#include <tx_api.h> // for TX_THREAD, tx_thread_stack_error_notify

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
#define KNG_CD_CTI_TRIGGER_ERROR_CODE 0
#define AP_RSM_ADDRESS_DIEn(n)        ((uint64_t)0x2F000000UL + 0x1000000000ULL * (n))
#define AP_RSM_SIZE                   (0x10000)

/** Size of the basic exception stack frame (R0, R1, R2, R3, R12, LR, PC, xPSR) */
#define EXCEPTION_FRAME_BASIC_SIZE (8U * sizeof(uint32_t))

/** Size of the extended exception stack frame including FP context (S0-S15, FPSCR, reserved) */
#define EXCEPTION_FRAME_FP_EXT_SIZE (18U * sizeof(uint32_t))

/** EXC_RETURN bit 4: 0 = FP context stacked, 1 = no FP context */
#define EXC_RETURN_FTYPE_MASK (1U << 4)

/** xPSR bit 9: 1 = SP was force-aligned to 8 bytes (4-byte pad inserted) */
#define XPSR_STKALIGN_MASK (1U << 9)

/*------------- Functions ----------------*/
static void print_context_info(core_crash_context_t* crash_context)
{
    FPFwCDPrintf("Crash Context:\n");
    FPFwCDPrintf("==============\n");

    FPFwCDPrintf("R0 : [%08lX]\n", crash_context->r0);
    FPFwCDPrintf("R1 : [%08lX]\n", crash_context->r1);
    FPFwCDPrintf("R2 : [%08lX]\n", crash_context->r2);
    FPFwCDPrintf("R3 : [%08lX]\n", crash_context->r3);
    FPFwCDPrintf("R4 : [%08lX]\n", crash_context->r4);
    FPFwCDPrintf("R5 : [%08lX]\n", crash_context->r5);
    FPFwCDPrintf("R6 : [%08lX]\n", crash_context->r6);
    FPFwCDPrintf("R7 : [%08lX]\n", crash_context->r7);
    FPFwCDPrintf("R8 : [%08lX]\n", crash_context->r8);
    FPFwCDPrintf("R9 : [%08lX]\n", crash_context->r9);
    FPFwCDPrintf("R10: [%08lX]\n", crash_context->r10);
    FPFwCDPrintf("R11: [%08lX]\n", crash_context->r11);
    FPFwCDPrintf("R12: [%08lX]\n", crash_context->r12);
    FPFwCDPrintf("SP : [%08lX]\n", crash_context->sp);
    FPFwCDPrintf("LR : [%08lX]\n", crash_context->lr);
    FPFwCDPrintf("PC : [%08lX]\n", crash_context->pc);
    FPFwCDPrintf("xPSR: [%08lX]\n", crash_context->xpsr);
    FPFwCDPrintf("==============\n");

    FPFwCDPrintf("HFSR  : [%08lX]\n", SCB->HFSR);
    FPFwCDPrintf("CFSR  : [%08lX]\n", SCB->CFSR);
    FPFwCDPrintf("DFSR  : [%08lX]\n", SCB->DFSR);
    FPFwCDPrintf("AFSR  : [%08lX]\n", SCB->AFSR);
    FPFwCDPrintf("MMFAR : [%08lX]\n", SCB->MMFAR);
    FPFwCDPrintf("BFAR  : [%08lX]\n", SCB->BFAR);
    FPFwCDPrintf("==============\n");
}

/**
 * @brief Save the crash context into the global crash context object.
 *
 * @param stack_frame Pointer to the exception stack frame
 * @param exc_return  The EXC_RETURN value from LR at exception entry
 */
#ifndef _WIN32
static inline __attribute__((always_inline)) void save_crash_context(exception_stack_frame_t* stack_frame, uint32_t exc_return)
#else
__attribute__((__weak__)) void save_crash_context(exception_stack_frame_t* stack_frame, uint32_t exc_return)
#endif
{
#ifndef _WIN32
    // Pick up static crash context object
    __asm__ volatile(".global g_core_crash_context   \n"
                     "ldr r12, =g_core_crash_context \n"
                     // Store registers not in stack_frame into crash context
                     "str r4,  [r12, %[r4_offset]]   \n"
                     "str r5,  [r12, %[r5_offset]]   \n"
                     "str r6,  [r12, %[r6_offset]]   \n"
                     "str r7,  [r12, %[r7_offset]]   \n"
                     "str r8,  [r12, %[r8_offset]]   \n"
                     "str r9,  [r12, %[r9_offset]]   \n"
                     "str r10, [r12, %[r10_offset]]  \n"
                     "str r11, [r12, %[r11_offset]]  \n"
                     : // No outputs
                     : // Inputs
                     [r4_offset] "i"(offsetof(core_crash_context_t, r4)),
                     [r5_offset] "i"(offsetof(core_crash_context_t, r5)),
                     [r6_offset] "i"(offsetof(core_crash_context_t, r6)),
                     [r7_offset] "i"(offsetof(core_crash_context_t, r7)),
                     [r8_offset] "i"(offsetof(core_crash_context_t, r8)),
                     [r9_offset] "i"(offsetof(core_crash_context_t, r9)),
                     [r10_offset] "i"(offsetof(core_crash_context_t, r10)),
                     [r11_offset] "i"(offsetof(core_crash_context_t, r11))
                     : // Clobbers
                     "r12");
#endif
    // Store registers from exception stack frame into crash context
    g_core_crash_context.r0 = stack_frame->R0;
    g_core_crash_context.r1 = stack_frame->R1;
    g_core_crash_context.r2 = stack_frame->R2;
    g_core_crash_context.r3 = stack_frame->R3;
    g_core_crash_context.r12 = stack_frame->R12;
    g_core_crash_context.lr = stack_frame->LR;
    g_core_crash_context.pc = stack_frame->PC;
    g_core_crash_context.xpsr = stack_frame->PSR;

    // Calculate the pre-exception SP.
    // Start with the basic exception frame size (8 words: R0-R3, R12, LR, PC, xPSR).
    uint32_t frame_size = EXCEPTION_FRAME_BASIC_SIZE;

    // If EXC_RETURN bit 4 (FTYPE) is 0, the hardware also stacked FP context
    // (S0-S15, FPSCR, and a reserved word = 18 additional 32-bit words).
    if ((exc_return & EXC_RETURN_FTYPE_MASK) == 0)
    {
        frame_size += EXCEPTION_FRAME_FP_EXT_SIZE;
    }

    // If xPSR bit 9 (STKALIGN) is set, the hardware inserted a 4-byte padding
    // word to enforce 8-byte stack alignment before pushing the exception frame.
    if (stack_frame->PSR & XPSR_STKALIGN_MASK)
    {
        frame_size += sizeof(uint32_t);
    }

    g_core_crash_context.sp = (uint32_t)(stack_frame) + frame_size;
}

/**
 * @brief Get the active exception number
 *
 * @return int Active exception number
 */
__attribute__((__weak__)) int get_active_exception(void)
{
    return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) - NVIC_USER_IRQ_OFFSET;
}

// #define DUMP_ARSM_ECC_STATUS_VERBOSE 1
static bool check_shared_sram_ecc_ras_fault_internal(bool is_arsm,
                                                     atu_map_entry_t* atu_entry,
                                                     int32_t* errorCode,
                                                     acpi_err_sec_firmware_t* sec_fw_cper_section)
{
    bool ret = false;
    atu_map(ATU_ID_MSCP, atu_entry);
    uint32_t err_status =
        MMIO_READ32(atu_entry->mscp_start_address + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ADDRESS);
    uint32_t err_addr = MMIO_READ32(atu_entry->mscp_start_address + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRADDR_ADDRESS);

#ifdef DUMP_ARSM_ECC_STATUS_VERBOSE
    if (err_status != 0)
    {
        FPFwCDPrintf("type_0x%llx : SERR = 0x%08lx\n",
                     atu_entry->ap_base_address,
                     err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_SERR_MASK);
        FPFwCDPrintf("type_0x%llx : UET = 0x%08lx\n",
                     atu_entry->ap_base_address,
                     (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UET_MASK) >>
                         SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UET_LSB);
        FPFwCDPrintf("type_0x%llx : PN = 0x%08lx\n",
                     atu_entry->ap_base_address,
                     (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_PN_MASK) >>
                         SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_PN_LSB);
        FPFwCDPrintf("type_0x%llx : DE = 0x%08lx\n",
                     atu_entry->ap_base_address,
                     (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_DE_MASK) >>
                         SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_DE_LSB);
        FPFwCDPrintf("type_0x%llx : CE = 0x%08lx\n",
                     atu_entry->ap_base_address,
                     (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK) >>
                         SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_LSB);
        FPFwCDPrintf("type_0x%llx : MV = 0x%08lx\n",
                     atu_entry->ap_base_address,
                     (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_MV_MASK) >>
                         SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_MV_LSB);
        FPFwCDPrintf("type_0x%llx : OF = 0x%08lx\n",
                     atu_entry->ap_base_address,
                     (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK) >>
                         SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_LSB);
        FPFwCDPrintf("type_0x%llx : ER = 0x%08lx\n",
                     atu_entry->ap_base_address,
                     (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ER_MASK) >>
                         SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ER_LSB);
        FPFwCDPrintf("type_0x%llx : UE = 0x%08lx\n",
                     atu_entry->ap_base_address,
                     (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK) >>
                         SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_LSB);
        FPFwCDPrintf("type_0x%llx : V = 0x%08lx\n",
                     atu_entry->ap_base_address,
                     (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_V_MASK) >>
                         SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_V_LSB);
        FPFwCDPrintf("type_0x%llx : AV = 0x%08lx\n",
                     atu_entry->ap_base_address,
                     (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_AV_MASK) >>
                         SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_AV_LSB);
    }
#endif

    if ((err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_V_MASK) &&
        (err_status & (SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK |
                       SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_PN_MASK)))
    {
        if (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_PN_MASK)
        {
            FPFwCDPrintf("Poisoned memory access at 0x%08lx\n", err_addr);
        }

        *errorCode = (is_arsm) ? KNG_HM_ARSM_UE : KNG_HM_RSM_UE;
        sec_fw_cper_section->severity = ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL;

        sec_fw_cper_section->record_id = (is_arsm) ? RECORD_ID_MSCP_ARSM_RAM : RECORD_ID_MSCP_RSM_RAM;
        sec_fw_cper_section->param[0] = err_status;
        sec_fw_cper_section->param[1] = err_addr;
        sec_fw_cper_section->param[2] = *errorCode;
        sec_fw_cper_section->param[3] = 0;

        // arm_voyager-68c_reference_manual : all fields are W1C
        uint32_t err_clr_mask = err_status;
        err_clr_mask |= SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_SERR_MASK;
        err_clr_mask |= SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_V_MASK;
        err_clr_mask |= (err_status & (SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ER_MASK |
                                       SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_AV_MASK |
                                       SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_DE_MASK |
                                       SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK |
                                       SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_PN_MASK));

        MMIO_WRITE32(atu_entry->mscp_start_address + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ADDRESS, err_clr_mask);

        atu_map_entry_t fault_addr_map_entry = {
            .ap_base_address = err_addr,
            .mscp_start_address = 0,
            .mscp_end_address = 0,
            .attribute.as_uint32 = atu_entry->attribute.as_uint32,
        };

        if (atu_find_map(ATU_ID_MSCP, &fault_addr_map_entry) == SILIBS_SUCCESS)
        {
            clear_poison_memory(fault_addr_map_entry.mscp_start_address);
        }

        ret = true;
    }

    atu_unmap(ATU_ID_MSCP, atu_entry);

    return ret;
}

static bool check_rmss_ram_ecc_ue(int32_t* errorCode,
                                  acpi_err_sec_firmware_t* sec_fw_cper_section,
                                  uint32_t err_msk,
                                  uint32_t ue_msk,
                                  uint32_t status_reg_addr,
                                  uint32_t address_reg_addr,
                                  uint32_t err_code,
                                  uint16_t record_id,
                                  const char* fault_log,
                                  acpi_error_severity_t report_severity)
{
    bool ret = false;

    uint32_t status = MMIO_READ32(status_reg_addr);

    if (status & err_msk)
    {
        if (status & ue_msk)
        {
            FPFwCDPrintf("%s ECC RAS UE Fault\n", fault_log);
            *errorCode = err_code;

            // Set CPER section
            sec_fw_cper_section->severity = report_severity;
            sec_fw_cper_section->record_id = record_id;
            sec_fw_cper_section->param[0] = status;
            sec_fw_cper_section->param[1] = MMIO_READ32(address_reg_addr);
            sec_fw_cper_section->param[2] = err_code;
            sec_fw_cper_section->param[3] = 0;

            ret = true;
        }

        // clear Error status
        MMIO_UPDATE32(status_reg_addr, status, err_msk);
    }

    return ret;
}

static bool check_ecc_fault(int32_t* errorCode, acpi_err_sec_firmware_t* sec_fw_cper_section)
{
    bool ret = false;
    uint32_t fault_addr = 0;

    if (SCB->CFSR & SCB_CFSR_MMARVALID_Msk) // MMFA valid
    {
        // Check with MMFAR
        fault_addr = SCB->MMFAR;
    }
    else if (SCB->CFSR & SCB_CFSR_BFARVALID_Msk)
    {
        // Check with BFAR
        fault_addr = SCB->BFAR;
    }

    if (fault_addr)
    {
        // Look for ATU entry to check if fault address is mapped AP address
        atu_map_entry_t atu_entry = {.ap_base_address = UINT64_MAX, .mscp_start_address = fault_addr};

        if (atu_find_map(ATU_ID_MSCP, &atu_entry) == SILIBS_SUCCESS)
        {
            // This is mapped address.
            if (atu_entry.ap_base_address < (AP_TOP_D0_ARSM_SHARED_SRAM_ADDRESS + AP_TOP_D0_ARSM_SHARED_SRAM_SIZE))
            // && atu_entry.ap_base_address >= (AP_TOP_D0_ARSM_SHARED_SRAM_ADDRESS) : Always true on uint
            {
                // Check D0 ARSM ECC Status
                for (mscp_arsm_ram_type_t i = MSCP_S_ARSM_RAM; i < MSCP_ARSM_RAM_COUNT; i++)
                {
                    get_arsm_ecc_atu_entry_die(i, 0, &atu_entry);
                    if (check_shared_sram_ecc_ras_fault_internal(true, &atu_entry, errorCode, sec_fw_cper_section))
                    {
                        FPFwCDPrintf("ARSM0 ECC RAS UE Fault\n");
                        ret = true;
                        break;
                    }
                }
            }
            else if (atu_entry.ap_base_address >= (AP_TOP_D1_ARSM_SHARED_SRAM_ADDRESS) &&
                     atu_entry.ap_base_address < (AP_TOP_D1_ARSM_SHARED_SRAM_ADDRESS + AP_TOP_D1_ARSM_SHARED_SRAM_SIZE))
            {
                // Check D1 ARSM ECC Status
                for (mscp_arsm_ram_type_t i = MSCP_S_ARSM_RAM; i < MSCP_ARSM_RAM_COUNT; i++)
                {
                    get_arsm_ecc_atu_entry_die(i, 1, &atu_entry);
                    if (check_shared_sram_ecc_ras_fault_internal(true, &atu_entry, errorCode, sec_fw_cper_section))
                    {
                        FPFwCDPrintf("ARSM1 ECC RAS UE Fault\n");
                        ret = true;
                        break;
                    }
                }
            }
            else if (atu_entry.ap_base_address >= (AP_RSM_ADDRESS_DIEn(0)) &&
                     atu_entry.ap_base_address < (AP_RSM_ADDRESS_DIEn(0) + AP_RSM_SIZE))
            {
                // Check D0 RSM ECC Status
                for (mscp_rsm_ram_type_t i = MSCP_S_RSM_RAM; i < MSCP_RSM_RAM_COUNT; i++)
                {
                    get_rsm_ecc_atu_entry_die(i, 0, &atu_entry);
                    if (check_shared_sram_ecc_ras_fault_internal(false, &atu_entry, errorCode, sec_fw_cper_section))
                    {
                        FPFwCDPrintf("RSM0 ECC RAS UE Fault\n");
                        ret = true;
                        break;
                    }
                }
            }
            else if (atu_entry.ap_base_address >= (AP_RSM_ADDRESS_DIEn(1)) &&
                     atu_entry.ap_base_address < (AP_RSM_ADDRESS_DIEn(1) + AP_RSM_SIZE))
            {
                // Check D1 RSM ECC Status
                for (mscp_rsm_ram_type_t i = MSCP_S_RSM_RAM; i < MSCP_RSM_RAM_COUNT; i++)
                {
                    get_rsm_ecc_atu_entry_die(i, 1, &atu_entry);
                    if (check_shared_sram_ecc_ras_fault_internal(false, &atu_entry, errorCode, sec_fw_cper_section))
                    {
                        FPFwCDPrintf("RSM1 ECC RAS UE Fault\n");
                        ret = true;
                        break;
                    }
                }
            }
        }
        else
        {
            // Fault address is MSCP local address space
            if (fault_addr >= (MSCP_TOP_MSCP_EXP_ADDRESS + MSCP_EXP_TOP_SCF_RAM_ADDRESS) &&
                fault_addr < (MSCP_TOP_MSCP_EXP_ADDRESS + MSCP_EXP_TOP_SCF_RAM_ADDRESS + MSCP_EXP_TOP_SCF_RAM_SIZE))
            {
                // SCF RAM
                ret = check_rmss_ram_ecc_ue(
                    errorCode,
                    sec_fw_cper_section,
                    MSCP_EXP_CSR_SCFRAM_MSCP_ERRSTATUS_REG_UE_MASK | MSCP_EXP_CSR_SCFRAM_MSCP_ERRSTATUS_REG_CE_MASK |
                        MSCP_EXP_CSR_SCFRAM_MSCP_ERRSTATUS_REG_OF_MASK,
                    MSCP_EXP_CSR_SCFRAM_MSCP_ERRSTATUS_REG_UE_MASK,
                    MSCP_TOP_MSCP_EXP_ADDRESS + MSCP_EXP_TOP_MSCP_EXP_CSR_ADDRESS + MSCP_EXP_CSR_SCFRAM_MSCP_ERRSTATUS_REG_ADDRESS,
                    MSCP_TOP_MSCP_EXP_ADDRESS + MSCP_EXP_TOP_MSCP_EXP_CSR_ADDRESS + MSCP_EXP_CSR_SCFRAM_MSCP_ERRADDR_REG_ADDRESS,
                    KNG_HM_SCF_UE,
                    RECORD_ID_MSCP_SCF_RAM,
                    "SCF",
                    ACPI_ERROR_SEVERITY_CORRECTED);
            }
            else if (fault_addr >= (MSCP_TOP_MSCP_EXP_ADDRESS + MSCP_EXP_TOP_RAM0_ADDRESS) &&
                     fault_addr < (MSCP_TOP_MSCP_EXP_ADDRESS + MSCP_EXP_TOP_RAM0_ADDRESS + MSCP_EXP_TOP_RAM0_SIZE))
            {
                // RAM0
                ret = check_rmss_ram_ecc_ue(errorCode,
                                            sec_fw_cper_section,
                                            MSCP_EXP_CSR_RMSS_RAM0_MSCP_ERRSTATUS_REG_CE_MASK |
                                                MSCP_EXP_CSR_RMSS_RAM0_MSCP_ERRSTATUS_REG_UE_MASK |
                                                MSCP_EXP_CSR_RMSS_RAM0_MSCP_ERRSTATUS_REG_OF_MASK,
                                            MSCP_EXP_CSR_RMSS_RAM0_MSCP_ERRSTATUS_REG_UE_MASK,
                                            MSCP_TOP_MSCP_EXP_ADDRESS + MSCP_EXP_TOP_MSCP_EXP_CSR_ADDRESS +
                                                MSCP_EXP_CSR_RMSS_RAM0_MSCP_ERRSTATUS_REG_ADDRESS,
                                            MSCP_TOP_MSCP_EXP_ADDRESS + MSCP_EXP_TOP_MSCP_EXP_CSR_ADDRESS +
                                                MSCP_EXP_CSR_RMSS_RAM0_MSCP_ERRADDR_REG_ADDRESS,
                                            KNG_HM_RMSS_RAM0_UE,
                                            RECORD_ID_MSCP_RMSS_RAM0,
                                            "RMSS RAM0",
                                            ACPI_ERROR_SEVERITY_CORRECTED);
            }
            else if (fault_addr >= (MSCP_TOP_MSCP_EXP_ADDRESS + MSCP_EXP_TOP_RAM1_ADDRESS) &&
                     fault_addr < (MSCP_TOP_MSCP_EXP_ADDRESS + MSCP_EXP_TOP_RAM1_ADDRESS + MSCP_EXP_TOP_RAM1_SIZE))
            {
                // RAM1
                ret = check_rmss_ram_ecc_ue(errorCode,
                                            sec_fw_cper_section,
                                            MSCP_EXP_CSR_RMSS_RAM1_MSCP_ERRSTATUS_REG_CE_MASK |
                                                MSCP_EXP_CSR_RMSS_RAM1_MSCP_ERRSTATUS_REG_UE_MASK |
                                                MSCP_EXP_CSR_RMSS_RAM1_MSCP_ERRSTATUS_REG_OF_MASK,
                                            MSCP_EXP_CSR_RMSS_RAM1_MSCP_ERRSTATUS_REG_UE_MASK,
                                            MSCP_TOP_MSCP_EXP_ADDRESS + MSCP_EXP_TOP_MSCP_EXP_CSR_ADDRESS +
                                                MSCP_EXP_CSR_RMSS_RAM1_MSCP_ERRSTATUS_REG_ADDRESS,
                                            MSCP_TOP_MSCP_EXP_ADDRESS + MSCP_EXP_TOP_MSCP_EXP_CSR_ADDRESS +
                                                MSCP_EXP_CSR_RMSS_RAM1_MSCP_ERRADDR_REG_ADDRESS,
                                            KNG_HM_RMSS_RAM1_UE,
                                            RECORD_ID_MSCP_RMSS_RAM1,
                                            "RMSS RAM1",
                                            ACPI_ERROR_SEVERITY_CORRECTED);
            }
        }
    }

    return ret;
}

/**
 * @brief Exception handler for Cortex-M7. This function is called when an exception occurs.
 *
 * @param stack_frame Pointer to the exception stack frame
 * @param exc_return  The EXC_RETURN value from LR at exception entry
 */
FPFW_NORETURN void exception_handler(exception_stack_frame_t* stack_frame, uint32_t exc_return)
{
    save_crash_context(stack_frame, exc_return);

    acpi_err_sec_firmware_t sec_fw_cper_section = {0};
    bool sec_fw_cper_section_valid = false;

    // Disable Watchdog
    wdog_cmsdk_apb_disable();         // Disable watchdog
    wdog_cmsdk_apb_lock_unlock(true); // Lock counter

    int32_t errorCode = KNG_CD_DEFAULT_EXCEPTION;
    uint32_t bugCheckParams[4] = {};
    const int exceptionIdx = get_active_exception();

    switch (exceptionIdx)
    {
    case HardFault_IRQn:
        FPFwCDPrintf("Hard Fault occurred\n");
        errorCode = KNG_CD_HARDFAULT_EXCEPTION;
        sec_fw_cper_section_valid = check_ecc_fault(&errorCode, &sec_fw_cper_section);
        bugCheckParams[0] = (uint32_t)__FILE__;
        bugCheckParams[1] = __LINE__;
        break;
    case MemoryManagement_IRQn:
        FPFwCDPrintf("Memory Management Fault occurred\n");
        errorCode = KNG_CD_MEMORYMANAGEMENT_EXCEPTION;
        bugCheckParams[0] = (uint32_t)__FILE__;
        bugCheckParams[1] = __LINE__;
        break;
    case BusFault_IRQn:
        FPFwCDPrintf("Bus Fault occurred\n");
        errorCode = KNG_CD_BUSFAULT_EXCEPTION;
        sec_fw_cper_section_valid = check_ecc_fault(&errorCode, &sec_fw_cper_section);
        bugCheckParams[0] = (uint32_t)__FILE__;
        bugCheckParams[1] = __LINE__;
        break;
    case UsageFault_IRQn:
        FPFwCDPrintf("Usage Fault occurred\n");
        errorCode = KNG_CD_USAGEFAULT_EXCEPTION;
        bugCheckParams[0] = (uint32_t)__FILE__;
        bugCheckParams[1] = __LINE__;
        break;
    case DebugMonitor_IRQn:
        FPFwCDPrintf("Debug Monitor Exception occurred\n");
        if (crash_dump_bug_check_initiated_dump())
        {
            errorCode = g_core_crash_context.r0;
            bugCheckParams[0] = g_core_crash_context.r1;
            bugCheckParams[1] = g_core_crash_context.r2;
            bugCheckParams[2] = g_core_crash_context.r3;
            bugCheckParams[3] = g_core_crash_context.r4;
        }
        else
        {
            FPFwCDPrintf("CTI-triggered Debug Monitor Exception\n");
            errorCode = KNG_CD_CTI_TRIGGER_ERROR_CODE;
        }
        break;
    case NonMaskableInt_IRQn:
        FPFwCDPrintf("WDT timeout occurred\n");
        errorCode = KNG_CD_WDT_TIMEOUT;
        break;
    default:
        FPFwCDPrintf("other fault occurred\n");
        errorCode = KNG_CD_DEFAULT_EXCEPTION;
        break;
    }

    // Provide printout for debugging
    print_context_info(&g_core_crash_context);

    if (errorCode != KNG_CD_EXTERNAL_REQUEST && errorCode != KNG_CD_CTI_TRIGGER_ERROR_CODE)
    {
        acpi_cper_section_t cper_section;

        if (!sec_fw_cper_section_valid)
        {
            sec_fw_cper_section = (acpi_err_sec_firmware_t){
                .severity = ACPI_ERROR_SEVERITY_CORRECTED,
                .param = {errorCode, bugCheckParams[0], bugCheckParams[1], bugCheckParams[2]}};

            sec_fw_cper_section.record_id = exceptionIdx == NonMaskableInt_IRQn ? RECORD_ID_MSCP_WD : RECORD_ID_MSCP_EXCEPTION;
        }

        cper_section.sec_fw = sec_fw_cper_section;
        hm_submit_cper_cd_state(ACPI_ERROR_DOMAIN_MSCP_PROC, sec_fw_cper_section.severity, &cper_section, sizeof(cper_section));
    }

    // Call the crash dump handler
    crash_dump_handler(errorCode, bugCheckParams[0], bugCheckParams[1], bugCheckParams[2], bugCheckParams[3]);

    // Hang the core
    crash_dump_wait_forever();
}

/**
 * @brief Main exception handler for Cortex-M7. This function is called when an exception occurs.
 *
 */
FPFW_NORETURN __attribute__((naked)) void main_exception_handler(void)
{
#ifndef _WIN32
    __asm__ volatile("CPSID   i     \n" // Disable interrupts
                     "tst lr, #4    \n" // Check LR[2] (LR holds EXC_RETURN)
                     "ite eq        \n"
                     "mrseq r0, msp \n" // Move msp into output register if EXC_RETURN[2] == 0
                     "mrsne r0, psp \n" // Move psp into output register if EXC_RETURN[2] == 1
                     "mov r1, lr    \n" // Pass EXC_RETURN as second argument
                     "b exception_handler"); // Branch to exception handler with the location of the exception stack frame
#endif
}

/**
 * @brief ThreadX thread stack error handler. Generate a bug check with the thread pointer.
 *
 */
void threadx_stack_error_handler(TX_THREAD* tx_thread)
{
    BUG_CHECK(KNG_CD_THREAD_STACK_ERROR, tx_thread, 0);
}

/**
 * @brief Register main exception handler for fault ISR and debug monitor and thread stack error.
 *
 */
KNG_STATUS exception_handler_init(void)
{
    // Register the main exception handler for generic fault.
    if (nvic_set_isr_fault(main_exception_handler) != NVIC_STATUS_SUCCESS)
    {
        return KNG_E_FAIL;
    }

    // Register the main exception handler for DebugMonitor_IRQn
    NVIC_SetVector(DebugMonitor_IRQn, (uint32_t)main_exception_handler);

    // Register the watchdog timeout handler for NonMaskableInt_IRQn
    NVIC_SetVector(NonMaskableInt_IRQn, (uint32_t)main_exception_handler);

    // Register the stack error handler for tx stack error
    if (tx_thread_stack_error_notify(threadx_stack_error_handler) != TX_SUCCESS)
    {
        return KNG_E_FAIL;
    }

    return KNG_SUCCESS;
}
