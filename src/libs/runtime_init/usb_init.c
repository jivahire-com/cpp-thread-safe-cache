/**
 * @file usb_init.c
 * Instantiates USBSS
 */

/*------------- Includes -----------------*/
#include "usb.h" // for usb_init, USBSS_INIT_USB2_0, USBSS_IN...

#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS, fpfw_init_r...
#include <idhw.h>      // for idhw_get_die_id
#include <idsw.h>
#include <idsw_kng.h>
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
    uint8_t die_num = (uint8_t)idhw_get_die_id();
    if (die_num == SOC_D1)
    {
        printf("USB is not enabled on die 1! Skip USB initialization\n");
        goto done;
    }

    /* TODO:
     * Obtain USB configuration information from the configuration manager
     * instead of relying on SDV. ADO - 1508440
     * https://azurecsi.visualstudio.com/Dev/_workitems/edit/1508440
     */
    KNG_PLAT_ID plat = idsw_get_platform_sdv();
    uint32_t usb_init_flags = 0x00;
    switch (plat)
    {
    // Note: FPGAs only support one controller subsytem - USB2_0
    case PLATFORM_FPGA:
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
        usb_init_flags = USBSS_INIT_USB2_0;
        break;

    case PLATFORM_SVP_SIM:
    case PLATFORM_SVP_MIN_CONFIG_SIM:
    case PLATFORM_RVP_EVT_SILICON:
        usb_init_flags = (USBSS_INIT_USB2_0 | USBSS_INIT_USB2_1);
        break;

    case PLATFORM_EMU_1D:
    case PLATFORM_EMU_1D_8C:
    case PLATFORM_EMU_2D:
    case PLATFORM_EMU_2D_8C:
    default:
        printf("Skip USB init on platform id: %d\n", plat);
        break;
    }

    if (usb_init_flags != 0x00)
    {
        usb_init(usb_init_flags);
    }

done:
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}