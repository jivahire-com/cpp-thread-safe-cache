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
#include <scp_mmap.h>
#include <stddef.h>
#include <stdint.h>
#include <unpack_image.h>

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
#endif
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
    arch_init_ccr();

    void* address = load_image(&boot_config);
    if (address)
    {
        launch_image(address, &SCB->VTOR);
    }
    else
    {
        // load_image failure error handling else part
        // but inside load_image all error cases handled
    }
    return 0;
}