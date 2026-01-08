//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file error_domain_shared.c
 * This file contains scp and mcp ras related functionality.
 */

/*------------- Includes -----------------*/
// #include <atu_lib.h>
#include <DbgPrint.h>
#include <FpFwUtils.h>
#include <arm_intrinsic.h> // for __DSB on Windows builds (empty define)
#include <atu_api.h>
#include <bug_check.h>
#include <cper.h>
#include <error_domain_i.h>
#include <fpfw_icc_base.h>
#include <health_monitor.h>
#include <idsw_kng.h>
#include <mscp_error_domain.h>
#include <mscp_exp_rmss_memory_map.h>
#include <mscp_ras_and_init_ctrl_registers_regs.h>
#include <nvic.h>
#include <scp_exp_csr_regs.h>
#include <scp_exp_top_regs.h>
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <mcp_top_regs.h>
#include <scp_top_regs.h>
#include <shared_sram_ecc_ras_registers_regs.h>

// clang-format off
#include <cmsis_m7.h>
#ifdef PLATFORM_CACHING_ENABLED
#include <cmsis_compiler.h>
#include <m-profile/armv7m_cachel1.h>
#endif
// clang-format on

/*-- Symbolic Constant Macros (defines) --*/
#if defined(SCP_RUNTIME_INIT)
#else
    #define MCP_EXP_TOP_RAM0_BASE          (MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM0_ADDRESS)
    // // CACHEABLE SECTION 1856KB
    #define MCP_EXP_CACHEABLE_SIZE         (1856 * SL_1KB)
    #define MCP_EXP_CACHEABLE_SECTION_BASE (MCP_EXP_TOP_RAM0_BASE)
    #define MCP_EXP_CACHEABLE_SECTION_END  (MCP_EXP_CACHEABLE_SECTION_BASE + MCP_EXP_CACHEABLE_SIZE - 1)
#endif
#define RSM_RAM_DEFAULT_OFFSET (0x08)
#define AP_RSM_ADDRESS_DIEn(n) ((0x2F000000UL) + (0x1000000000UL * (n)))
#define AP_RSM_SIZE            (0x10000)
#define ATU_MAPPING_RSM_RAM(die_id)                                              \
    ATU_MAPPING((die_id == 0 ? AP_RSM_ADDRESS_DIEn(0) : AP_RSM_ADDRESS_DIEn(1)), \
                0,                                                               \
                (ALIGN_UP(AP_RSM_SIZE, ATU_PAGE_SIZE) - 1),                      \
                {ATU_BUS_ATTR_NS})

const atu_map_entry_t s_hm_arsm_atu_entries[2][MSCP_ARSM_RAM_COUNT] =
#if defined(SCP_RUNTIME_INIT)
    {{ATU_MAPPING_SCP_S_ARSM_RAM_ECC(DIE_0),
      ATU_MAPPING_SCP_NS_ARSM_RAM_ECC(DIE_0),
      ATU_MAPPING_SCP_RT_ARSM_RAM_ECC(DIE_0),
      ATU_MAPPING_SCP_RL_ARSM_RAM_ECC(DIE_0)},
     {ATU_MAPPING_SCP_S_ARSM_RAM_ECC(DIE_1),
      ATU_MAPPING_SCP_NS_ARSM_RAM_ECC(DIE_1),
      ATU_MAPPING_SCP_RT_ARSM_RAM_ECC(DIE_1),
      ATU_MAPPING_SCP_RL_ARSM_RAM_ECC(DIE_1)}};
#elif defined(MCP_RUNTIME_INIT)
    {{ATU_MAPPING_MCP_S_ARSM_RAM_ECC(DIE_0),
      ATU_MAPPING_MCP_NS_ARSM_RAM_ECC(DIE_0),
      ATU_MAPPING_MCP_RT_ARSM_RAM_ECC(DIE_0),
      ATU_MAPPING_MCP_RL_ARSM_RAM_ECC(DIE_0)},
     {ATU_MAPPING_MCP_S_ARSM_RAM_ECC(DIE_1),
      ATU_MAPPING_MCP_NS_ARSM_RAM_ECC(DIE_1),
      ATU_MAPPING_MCP_RT_ARSM_RAM_ECC(DIE_1),
      ATU_MAPPING_MCP_RL_ARSM_RAM_ECC(DIE_1)}};
#endif

static const atu_map_entry_t s_hm_rsm_atu_entries[2][MSCP_RSM_RAM_COUNT] = {
#if defined(SCP_RUNTIME_INIT)
    {ATU_MAPPING_SCP_S_RSM_RAM_ECC(DIE_0), ATU_MAPPING_SCP_NS_RSM_RAM_ECC(DIE_0)},
    {ATU_MAPPING_SCP_S_RSM_RAM_ECC(DIE_1), ATU_MAPPING_SCP_NS_RSM_RAM_ECC(DIE_1)}
#elif defined(MCP_RUNTIME_INIT)
    {ATU_MAPPING_MCP_S_RSM_RAM_ECC(DIE_0), ATU_MAPPING_MCP_NS_RSM_RAM_ECC(DIE_0)},
    {ATU_MAPPING_MCP_S_RSM_RAM_ECC(DIE_1), ATU_MAPPING_MCP_NS_RSM_RAM_ECC(DIE_1)}
#endif
};

/*-------- Function Prototypes -----------*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
static fpfw_icc_base_ctx_t* mhu_handle = NULL;

/*-------------- Functions ---------------*/
bool is_cached_space(uint32_t addr)
{
#if defined(SCP_RUNTIME_INIT)
    if (SCP_EXP_CACHEABLE_SECTION_BASE <= addr && addr <= SCP_EXP_CACHEABLE_SECTION_END)
#else
    if (MCP_EXP_CACHEABLE_SECTION_BASE <= addr && addr <= MCP_EXP_CACHEABLE_SECTION_END)
#endif
    {
        return true;
    }
    return false;
}

void inject_err_by_access(uint32_t addr)
{
    __DSB();

    if (is_cached_space(addr))
    {
        SCB_InvalidateDCache_by_Addr((uint32_t*)addr, sizeof(uint32_t));
    }

    MMIO_READ32(addr);
}

void get_arsm_ecc_atu_entry_die(mscp_arsm_ram_type_t type, uint8_t die_id, atu_map_entry_t* atu_entry)
{
    BUG_ASSERT(type < MSCP_ARSM_RAM_COUNT);
    *atu_entry = s_hm_arsm_atu_entries[die_id][type];
}

void get_arsm_ecc_atu_entry(mscp_arsm_ram_type_t type, atu_map_entry_t* atu_entry)
{
    get_arsm_ecc_atu_entry_die(type, idsw_get_die_id(), atu_entry);
}

void get_rsm_ecc_atu_entry_die(mscp_rsm_ram_type_t type, uint8_t die_id, atu_map_entry_t* atu_entry)
{
    BUG_ASSERT(type < MSCP_RSM_RAM_COUNT);
    *atu_entry = s_hm_rsm_atu_entries[die_id][type];
}

void get_rsm_ecc_atu_entry(mscp_rsm_ram_type_t type, atu_map_entry_t* atu_entry)
{
    get_rsm_ecc_atu_entry_die(type, idsw_get_die_id(), atu_entry);
}

void trigger_shared_sram_fault(bool arsm, int type, uint32_t target_addr, uint32_t err_mask)
{
    atu_map_entry_t entry;

    if (arsm)
    {
        BUG_ASSERT(type < MSCP_ARSM_RAM_COUNT);
        get_arsm_ecc_atu_entry((mscp_arsm_ram_type_t)type, &entry);
    }
    else
    {
        BUG_ASSERT(type < MSCP_RSM_RAM_COUNT);
        get_rsm_ecc_atu_entry((mscp_rsm_ram_type_t)type, &entry);
    }

    int status = atu_map(ATU_ID_MSCP, &entry);
    BUG_ASSERT(status == SILIBS_SUCCESS);

    if (err_mask & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK)
    {
        MMIO_UPDATE32(entry.mscp_start_address + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_ADDRESS,
                      SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_INJECT_CE_MASK,
                      SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_INJECT_CE_MASK);

        inject_err_by_access(target_addr);
    }
    else if (err_mask & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK)
    {
        MMIO_UPDATE32(entry.mscp_start_address + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_ADDRESS,
                      SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_INJECT_UE_MASK,
                      SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_INJECT_UE_MASK);
        inject_err_by_access(target_addr);
    }
    else if (err_mask & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK)
    {
        nvic_global_disable();
        MMIO_UPDATE32(entry.mscp_start_address + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_ADDRESS,
                      SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_INJECT_CE_MASK,
                      SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_INJECT_CE_MASK);
        inject_err_by_access(target_addr);
        MMIO_UPDATE32(entry.mscp_start_address + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_ADDRESS,
                      SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_INJECT_CE_MASK,
                      SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_INJECT_CE_MASK);
        inject_err_by_access(target_addr);
        nvic_global_enable();
    }

    status = atu_unmap(ATU_ID_MSCP, &entry);
    BUG_ASSERT(status == SILIBS_SUCCESS);
}

void trigger_shared_sram_arsm_fault(mscp_arsm_ram_type_t type, uint32_t target_addr, uint32_t err_mask)
{
    BUG_ASSERT(type < MSCP_ARSM_RAM_COUNT);
    trigger_shared_sram_fault(true, (int)type, target_addr, err_mask);
}

void shared_sram_ecc_isr(void* ctx)
{
    bool is_rsm = false;
    uint32_t err_status = 0;
    uint32_t err_addr = 0;
    KNG_STATUS err_code = KNG_SUCCESS;
    acpi_error_severity_t severity = ACPI_ERROR_SEVERITY_INFORMATIONAL;
    atu_map_entry_t atu_entry = *(atu_map_entry_t*)ctx;
    KNG_DIE_ID die_id = idsw_get_die_id();

    for (int rsm_idx = 0; rsm_idx < MSCP_RSM_RAM_COUNT; ++rsm_idx)
    {
        if (atu_entry.ap_base_address == s_hm_rsm_atu_entries[die_id][rsm_idx].ap_base_address)
        {
            is_rsm = true;
            break;
        }
    }

    // Map the shared SRAM ECC registers
    int status = atu_map(ATU_ID_MSCP, &atu_entry);
    BUG_ASSERT(status == SILIBS_SUCCESS);

    // Read error status
    err_status = MMIO_READ32(atu_entry.mscp_start_address + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ADDRESS);

    if ((err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_V_MASK) && // SRAMECC_ERRSTATUS is valid
        (err_status & (SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK | SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK |
                       SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK | SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_PN_MASK |
                       SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_DE_MASK)))
    {
        uint32_t err_clr_mask = err_status & ~SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_SERR_MASK;

        // Read error address if applicable
        err_addr = (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_AV_MASK)
                       ? MMIO_READ32(atu_entry.mscp_start_address + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRADDR_ADDRESS)
                       : 0;

        // Determine error severity and code based on the error status
        if (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK)
        {
            severity = ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL;
            err_code = (is_rsm) ? KNG_HM_RSM_UE : KNG_HM_ARSM_UE;
        }
        else if (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK)
        {
            severity = ACPI_ERROR_SEVERITY_CORRECTED;
            err_code = KNG_HM_ARSM_OF;
            err_clr_mask |= SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK |
                            SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK; // bits [25:24] | [27]
        }
        else if (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_PN_MASK)
        {
            severity = ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL;
            err_code = KNG_HM_ARSM_UE;

            // Write to the poison address register
            MMIO_WRITE32(err_addr, 0x00000000);
        }
        else if (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK)
        {
            severity = ACPI_ERROR_SEVERITY_CORRECTED;
            err_code = (is_rsm) ? KNG_HM_RSM_CE : KNG_HM_ARSM_CE;
            err_clr_mask |= SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK; // bits [25:24]
        }

        // Clear error status
        MMIO_WRITE32(atu_entry.mscp_start_address + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ADDRESS, err_clr_mask);

        // Submit CPER
#if defined(SCP_RUNTIME_INIT)
        uint32_t err_record_id = (is_rsm) ? RECORD_ID_SCP_RSM_RAM : RECORD_ID_SCP_ARSM_RAM;
#else
        uint32_t err_record_id = (is_rsm) ? RECORD_ID_MCP_RSM_RAM : RECORD_ID_MCP_ARSM_RAM;
#endif

        acpi_err_sec_firmware_t sec_fw_cper_section = {.severity = severity,
                                                       .record_id = err_record_id,
                                                       .param = {err_status, err_addr, err_code, 0}};

        acpi_cper_section_t cper_section = {0};
        cper_section.sec_fw = sec_fw_cper_section;
#if defined(SCP_RUNTIME_INIT)
        hm_submit_cper(ACPI_ERROR_DOMAIN_SCP_PROC, severity, &cper_section, sizeof(cper_section));
#else
        hm_submit_cper(ACPI_ERROR_DOMAIN_MCP_PROC, severity, &cper_section, sizeof(cper_section));
#endif
    }

    // Unmap the shared SRAM ECC registers
    status = atu_unmap(ATU_ID_MSCP, &atu_entry);
    BUG_ASSERT(status == SILIBS_SUCCESS);

    // Bug check if the error is uncorrectable fatal
    if (severity == ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL)
    {
        BUG_CHECK(err_code, err_status, err_addr);
    }
}

uint32_t map_rsm_address(mscp_rsm_ram_type_t type, atu_map_entry_t* atu_entry)
{
    BUG_ASSERT(atu_entry != NULL);
    KNG_DIE_ID die_id = idsw_get_die_id();

    *atu_entry = (atu_map_entry_t)ATU_MAPPING_RSM_RAM(die_id);

    if (type == MSCP_S_RSM_RAM)
    {
        atu_entry_attr_t attributes = {ATU_BUS_ATTR_S};
        atu_entry->attribute.as_uint32 = attributes.as_uint32;
    }
    else
    {
        atu_entry_attr_t attributes = {ATU_BUS_ATTR_NS};
        atu_entry->attribute.as_uint32 = attributes.as_uint32;
    }

    int status = atu_map(ATU_ID_MSCP, atu_entry);
    BUG_ASSERT(status == SILIBS_SUCCESS);

    return atu_entry->mscp_start_address + RSM_RAM_DEFAULT_OFFSET;
}

void trigger_shared_sram_rsm_fault(mscp_rsm_ram_type_t type, uint32_t target_addr, uint32_t err_mask)
{
    BUG_ASSERT(type < MSCP_RSM_RAM_COUNT);
    trigger_shared_sram_fault(false, (int)type, target_addr, err_mask);
}

void unmap_rsm_address(atu_map_entry_t* atu_entry)
{
    int status = atu_unmap(ATU_ID_MSCP, atu_entry);
    BUG_ASSERT(status == SILIBS_SUCCESS);
}

void shared_sram_ecc_isr_ext()
{
    // RSM secure/non-secure share same interrupt
    for (int idx = MSCP_S_RSM_RAM; idx < MSCP_RSM_RAM_COUNT; idx++)
    {
        atu_map_entry_t atu_entry = s_hm_rsm_atu_entries[idsw_get_die_id()][idx];
        shared_sram_ecc_isr(&atu_entry);
    }
}

void get_rsm_ecc_atu_entry_wrapper(int type, atu_map_entry_t* entry)
{
    get_rsm_ecc_atu_entry((mscp_rsm_ram_type_t)type, entry);
}

void enable_shared_sram_errors(ecc_entry_getter_fn get_entry, int count)
{
    for (int i = 0; i < count; i++)
    {
        atu_map_entry_t atu_entry;
        get_entry(i, &atu_entry);
        int status = atu_map(ATU_ID_MSCP, &atu_entry);
        BUG_ASSERT(status == SILIBS_SUCCESS);

        uint32_t feature = MMIO_READ32(atu_entry.mscp_start_address + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRFR_ADDRESS);
        uint32_t ctrl_mask = 0;
        if (feature & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRFR_ED_MASK) // Error reporting and logging
        {
            ctrl_mask |= SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRCTRL_ED_MASK; // Enable ECC error detection
        }

        if (feature & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRFR_FI_MASK) // Fault handling interrupt
        {
            ctrl_mask |= SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRCTRL_FI_MASK; // Fault handling interrupt is generated for all detected errors
        }

        if (feature & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRFR_UE_MASK) // In-band error response
        {
            ctrl_mask |= SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRCTRL_UE_MASK; // External abort response for uncorrected errors enabled
        }

        if (feature & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRFR_CFI_MASK) // Fault handling interrupt for corrected errors
        {
            ctrl_mask |= SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRCTRL_CFI_MASK; // Fault handling interrupt generated for corrected errors
        }

        // Enable ECC errors for the shared SRAM ECC RAS registers
        MMIO_SET_MASK32(atu_entry.mscp_start_address + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRCTRL_ADDRESS, ctrl_mask);

        status = atu_unmap(ATU_ID_MSCP, &atu_entry);
        BUG_ASSERT(status == SILIBS_SUCCESS);
    }
}

fpfw_icc_base_ctx_t* get_mhu_handle(void)
{
    return (mhu_handle);
}

void set_mhu_handle(fpfw_icc_base_ctx_t* icc_ctx)
{
    mhu_handle = icc_ctx;
}

uint32_t get_arsm_attr_address(uint32_t attributes, atu_map_entry_t* atu_entry, atu_map_entry_t* atu_recover_entry)
{
    BUG_ASSERT(atu_entry != NULL);
    BUG_ASSERT(atu_recover_entry != NULL);

    // Map the ARSM into MSCP with the requested attributes
    *atu_entry = (atu_map_entry_t){
        .ap_base_address = AP_TOP_D0_ARSM_SHARED_SRAM_ADDRESS,
        .mscp_start_address = 0,
        .mscp_end_address = ALIGN_UP(AP_TOP_D0_ARSM_SHARED_SRAM_SIZE, ATU_PAGE_SIZE) - 1,
        .attribute.as_uint32 = attributes,
    };
    if (idsw_get_die_id() == DIE_1)
    {
        atu_entry->ap_base_address = AP_TOP_D1_ARSM_SHARED_SRAM_ADDRESS;
        atu_entry->mscp_end_address = ALIGN_UP(AP_TOP_D1_ARSM_SHARED_SRAM_SIZE, ATU_PAGE_SIZE) - 1;
    }

    // Unmap if already mapped
    *atu_recover_entry = *atu_entry;
    uint32_t sts = atu_find_map(ATU_ID_MSCP, atu_recover_entry);
    FPFW_DBGPRINT("atu_find_map status: %d (0x%08lx)\n", sts, atu_recover_entry->mscp_start_address);
    if (sts == SILIBS_SUCCESS)
    {
        // Already mapped, unmap it.
        BUG_ASSERT(atu_unmap(ATU_ID_MSCP, atu_recover_entry) == SILIBS_SUCCESS);
    }
    else
    {
        // Not mapped, clear the recover entry
        *atu_recover_entry = (atu_map_entry_t){0};
        atu_recover_entry->mscp_start_address = 0;
    }

    // Map with new attributes
    BUG_ASSERT(atu_map(ATU_ID_MSCP, atu_entry) == SILIBS_SUCCESS);

    return atu_entry->mscp_start_address;
}