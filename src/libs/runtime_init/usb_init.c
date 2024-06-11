/**
 * @file usb_init.c
 * Instantiates USBSS
 */

/*------------- Includes -----------------*/
#include "idsw.h" // for idsw_get_platform_sdv, _PLAT_ID
#include "usb.h"  // for usb_init, USBSS_INIT_USB2_0, USBSS_IN...

#include <fpfw_init.h>         // for FPFW_INIT_STATUS_SUCCESS, fpfw_init_r...
#include <idhw.h>              // for idhw_get_die_id
#include <kng_soc_constants.h> // for SOC_D0
#include <stdint.h>            // for uint8_t
#include <stdio.h>             // for printf, NULL

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(usb, FPFW_INIT_DEPENDENCIES("ioss"))
{
    // Re-enable when SVP is ready to test ADO TASK #1793926
    // TODO: ADO 1826681 Re-enable on FPGA once the vab sequence library has been integrated
    if (((idsw_get_platform_sdv() <= PLATFORM_SVP_SIM) && (idsw_get_platform_sdv() >= PLATFORM_FPGA)) ||
        ((idsw_get_platform_sdv() <= PLATFORM_EMU_2D_8C) && (idsw_get_platform_sdv() >= PLATFORM_EMU)))
    {
        printf("%s: skip USB init on SVP, FPGA and ZEBU for now!\n", __func__);
        return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
    }

    uint8_t die_num = (uint8_t)idhw_get_die_id();
    printf("USB init, die_num: [%u]\n", die_num);

    // TODO: get usb_init_block from config knob ADO Task #1508422
    // USB only used in DIE0
    if (die_num == SOC_D0)
    {
        usb_init(USBSS_INIT_USB2_0);
        usb_init(USBSS_INIT_USB2_1);
    }
    else
    {
        printf("USB not used on die: %u... Skipping USB init...\n", die_num);
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}