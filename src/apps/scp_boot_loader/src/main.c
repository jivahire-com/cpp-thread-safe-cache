//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file main
 * This file contains the main function for the SCP boot loader. It is expected to invoke APIs to parse
 * and decompress the SCP FW and jump to it.
 */

/*------------- Includes -----------------*/
#include <cmsis_m7.h>
#include <kingsgate_boot.h>
#include <mpu.h>
#include <scp_mmap.h>
#include <stddef.h>
#include <stdint.h>

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

void launch_image(void* address, volatile uint32_t* vtor);

/*-- Declarations (Statics and globals) --*/

extern char _mainimage_s;
extern char _mainimage_e;

uint8_t mainImage[SCP_MAX_IMAGE_SIZE] __attribute__((section(".mainimage"))) = {0};

// String containing build path and build type - to reduce confusion while debugging in FPGA platforms
uint8_t bl_rel_str[] = BL_REL_STR;

kingsgate_boot_config_t boot_config = {
    .data_src_base = (size_t)&_mainimage_s,
    .data_src_end = (size_t)&_mainimage_e,

    .itc_ram_base = (size_t)SCP_ITCM_RAM_BASE,
    .itc_ram_size = SCP_ITCM_RAM_SIZE,
    .dtc_ram_base = (size_t)SCP_DTCM_RAM_BASE,
    .dtc_ram_size = SCP_DTCM_RAM_SIZE,
    .rmss_data_base = (size_t)SCP_RMSS_RAM_DATA_BASE,
    .rmss_data_size = SCP_RMSS_RAM_DATA_SIZE,

    .boot_meta_base = (size_t)SCP_BOOT_RAM_BASE,
    .cpu_type = MSCP_CPU_SCP,
};

//! Will be overridden by the SCP firmware
const ARM_MPU_Region_t region[] = {
    /**
     * MPU Region 0 - Complete Memory Map   0x00000000 - 0xFFFFFFFF
     *                Strongly Ordered, non cacheable, non shared, non bufferable
     *                Priviledged R/W
     */
    {
        .RBAR = ARM_MPU_RBAR(0, 0x00000000), // NOLINT
        .RASR = ARM_MPU_RASR(ENABLE_EXEC, ARM_MPU_AP_PRIV, TYPE_EXT_0, NON_SHAREABLE, NON_CACHEABLE, NON_BUFFERABLE, DISABLE_SUBREGION, ARM_MPU_REGION_SIZE_4GB),
    },
};
/*------------- Functions ----------------*/

/**
 *  @brief This function does the following
 *      Set up the Configuration Control Register (CCR) in the System Control
 *      Block (1) by setting the following flag bits:
 *
 *      DIV_0_TRP   [4]: Enable trapping on division by zero.
 *      STKALIGN    [9]: Enable automatic DWORD stack-alignment on exception
 *                  entry (2).
 *
 *      All other bits are left in their default state.
 *
 *      (1) ARM® v7-M Architecture Reference Manual, section B3.2.8.
 *      (2) ARM® v7-M Architecture Reference Manual, section B1.5.7.
 *
 *  @note This function is invoked from main
 *
 *  @param[in] None
 *  @param[out] None
 *
 *  @return
 *      No return value
 */
static void arch_init_ccr(void)
{
    SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;
    SCB->CCR |= SCB_CCR_STKALIGN_Msk;
#if defined(__FPU_USED) && (__FPU_USED == 1U)
    SCB->CPACR |= ((3U << 10U * 2U) | /* set CP10 Full Access */
                   (3U << 11U * 2U)); /* set CP11 Full Access */

    /*
     * Configure the Floating-Point Context Control Register (FPCCR)
     * to enable automatic and lazy FPU state preservation:
     *
     * ASPEN [31]: Automatic State Preservation ENable - When set, the processor
     *             automatically preserves floating-point context on exception entry.
     * LSPEN [30]: Lazy State Preservation ENable - When set, the processor defers
     *             the actual stacking of FPU registers until the FPU is accessed
     *             in the exception handler, reducing interrupt latency.
     *
     * This is critical for ThreadX to properly handle FPU context when threads
     * or ISRs use floating-point operations.
     */
    FPU->FPCCR |= (FPU_FPCCR_ASPEN_Msk | FPU_FPCCR_LSPEN_Msk);
#endif
}

/**
 * @brief  This function sets up the SHCSR register to enable fault handlers.
 *
 */
static void arch_init_shcsr(void)
{
    SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk; // Enable usage fault
    SCB->SHCSR |= SCB_SHCSR_BUSFAULTENA_Msk; // Enable bus fault
    SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk; // Enable memory fault
}

/**
 *  @brief This function is main function of SCP boot loader.
 *
 *  @note This function is invoked from C runtime _start.
 *
 *  @param[in] None
 *  @param[out] None
 *
 *  @return
 *      Currently always returns 0.
 */
int main(void)
{
    //! Set up the Configuration Control Register (CCR) in the System Control
    arch_init_ccr();
    //! Set up the SHCSR register to enable fault handlers
    arch_init_shcsr();
    //! Set the initial MPU settings for bootloader which will be overridden by the SCP firmware
    mpu_init(region, 1);
    //! Parse boot config, decompress and load the image in TCMs & RMSS data region
    //! and return the ITCM base address
    void* address = load_image(&boot_config);
    if (address)
    {
        //! Set the vector table addr, fetch initial stack ptr (1st entry) from the vector table & set it as the main stack ptr
        //! load reset handler address (2nd entry) from vector table into PC & ultimately start executing the reset handler (boot_loader_vectors.S)
        launch_image(address, &SCB->VTOR);
    }
    else
    {
        // load_image failure error handling else part
        // but inside load_image all error cases handled
    }
    return 0;
}