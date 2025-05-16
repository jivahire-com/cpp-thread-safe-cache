/**
 * @file avs_scp_init.c
 * Instantiates AVS for the SCP
 */

/*------------- Includes -----------------*/
#include <DfwkClient.h>
#include <DfwkThreadXHost.h> // for DFWK_THREADX_HOST
#include <fpfw_init.h>
#include <idsw_kng.h>
#include <interrupts.h>
#include <padring_southeast_regs.h>
#include <scp_avs.h>
#include <scp_avs_driver.h>
#include <silibs_scp_exp_top_regs.h>
#include <silibs_scp_top_regs.h>
#include <stdint.h>
#include <stdio.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(avs0, FPFW_INIT_DEPENDENCIES("dfwk", "std_io", "nvic", "cfg_mgr"))
{
    static scp_avs_device_t avs_device = {
        .config = {
            .avs_irq = HW_INT_AVS_CTRL_0_INT,
            .reg_base_addr = (uintptr_t)SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_AVS_0_ADDRESS,
            .rail_count = 2,
            .afm_csr_avs_clk_addr = (uintptr_t)SCP_TOP_AFM_SE_ADDRESS + PADRING_SOUTHEAST_SOUTHEAST_AFM_CSR_AVS0_CLK_ADDRESS,
            .afm_csr_mdata_addr = (uintptr_t)SCP_TOP_AFM_SE_ADDRESS + PADRING_SOUTHEAST_SOUTHEAST_AFM_CSR_AVS0_MDATA_ADDRESS,
        }};
    avs_device.avs_bus_num = AVS_BUS0;

    fpfw_init_component_id_t dfwk_id = "dfwk";
    DFWK_THREADX_HOST* dfwk_host = (DFWK_THREADX_HOST*)fpfw_init_get_handle(dfwk_id);

    scp_avs_init(&avs_device, &dfwk_host->Schedule);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &avs_device};
}

FPFW_INIT_COMPONENT(avs0_int, FPFW_INIT_DEPENDENCIES("avs0"))
{
    static scp_avs_interface_t avs_interface = {};
    fpfw_init_component_id_t avs_id = "avs0";

    scp_avs_interface_initialize(fpfw_init_get_handle(avs_id), &avs_interface);
    DfwkClientInterfaceOpen(&avs_interface.Header);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &avs_interface};
}

FPFW_INIT_COMPONENT(avs1, FPFW_INIT_DEPENDENCIES("dfwk", "std_io", "nvic", "hw_ver", "cfg_mgr"))
{
    if (idsw_get_die_id() == DIE_1)
    { // DIE_1 only has one AVSBus.
        return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
    }

    static scp_avs_device_t avs_device = {
        .config = {
            .avs_irq = HW_INT_AVS_CTRL_1_INT,
            .reg_base_addr = (uintptr_t)SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_AVS_1_ADDRESS,
            .rail_count = 2,
            .afm_csr_avs_clk_addr = (uintptr_t)SCP_TOP_AFM_SE_ADDRESS + PADRING_SOUTHEAST_SOUTHEAST_AFM_CSR_AVS1_CLK_ADDRESS,
            .afm_csr_mdata_addr = (uintptr_t)SCP_TOP_AFM_SE_ADDRESS + PADRING_SOUTHEAST_SOUTHEAST_AFM_CSR_AVS1_MDATA_ADDRESS,
        }};

    avs_device.avs_bus_num = AVS_BUS1;

    fpfw_init_component_id_t dfwk_id = "dfwk";
    DFWK_THREADX_HOST* dfwk_host = (DFWK_THREADX_HOST*)fpfw_init_get_handle(dfwk_id);

    scp_avs_init(&avs_device, &dfwk_host->Schedule);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &avs_device};
}

FPFW_INIT_COMPONENT(avs1_int, FPFW_INIT_DEPENDENCIES("avs1"))
{
    if (idsw_get_die_id() == DIE_1)
    {
        return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
    }

    static scp_avs_interface_t avs_interface = {};
    fpfw_init_component_id_t avs_id = "avs1";

    scp_avs_interface_initialize(fpfw_init_get_handle(avs_id), &avs_interface);
    DfwkClientInterfaceOpen(&avs_interface.Header);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &avs_interface};
}

FPFW_INIT_COMPONENT(avs2, FPFW_INIT_DEPENDENCIES("dfwk", "std_io", "nvic", "hw_ver", "cfg_mgr"))
{
    if (idsw_get_die_id() == DIE_1)
    {
        return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
    }

    static scp_avs_device_t avs_device = {
        .config = {
            .avs_irq = HW_INT_AVS_CTRL_2_INT,
            .reg_base_addr = (uintptr_t)SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_AVS_2_ADDRESS,
            .rail_count = 2,
            .afm_csr_avs_clk_addr = (uintptr_t)SCP_TOP_AFM_SE_ADDRESS + PADRING_SOUTHEAST_SOUTHEAST_AFM_CSR_AVS2_CLK_ADDRESS,
            .afm_csr_mdata_addr = (uintptr_t)SCP_TOP_AFM_SE_ADDRESS + PADRING_SOUTHEAST_SOUTHEAST_AFM_CSR_AVS2_MDATA_ADDRESS,
        }};

    avs_device.avs_bus_num = AVS_BUS2;

    fpfw_init_component_id_t dfwk_id = "dfwk";
    DFWK_THREADX_HOST* dfwk_host = (DFWK_THREADX_HOST*)fpfw_init_get_handle(dfwk_id);

    scp_avs_init(&avs_device, &dfwk_host->Schedule);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &avs_device};
}

FPFW_INIT_COMPONENT(avs2_int, FPFW_INIT_DEPENDENCIES("avs2"))
{
    if (idsw_get_die_id() == DIE_1)
    {
        return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
    }

    static scp_avs_interface_t avs_interface = {};
    fpfw_init_component_id_t avs_id = "avs2";

    scp_avs_interface_initialize(fpfw_init_get_handle(avs_id), &avs_interface);
    DfwkClientInterfaceOpen(&avs_interface.Header);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &avs_interface};
}

FPFW_INIT_COMPONENT(avs3, FPFW_INIT_DEPENDENCIES("dfwk", "std_io", "nvic", "hw_ver", "cfg_mgr"))
{
    if (idsw_get_die_id() == DIE_1)
    {
        return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
    }

    static scp_avs_device_t avs_device = {
        .config = {
            .avs_irq = HW_INT_AVS_CTRL_3_INT,
            .reg_base_addr = (uintptr_t)SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_AVS_3_ADDRESS,
            .rail_count = 2,
            .afm_csr_avs_clk_addr = (uintptr_t)SCP_TOP_AFM_SE_ADDRESS + PADRING_SOUTHEAST_SOUTHEAST_AFM_CSR_AVS3_CLK_ADDRESS,
            .afm_csr_mdata_addr = (uintptr_t)SCP_TOP_AFM_SE_ADDRESS + PADRING_SOUTHEAST_SOUTHEAST_AFM_CSR_AVS3_MDATA_ADDRESS,
        }};

    avs_device.avs_bus_num = AVS_BUS3;

    fpfw_init_component_id_t dfwk_id = "dfwk";
    DFWK_THREADX_HOST* dfwk_host = (DFWK_THREADX_HOST*)fpfw_init_get_handle(dfwk_id);

    scp_avs_init(&avs_device, &dfwk_host->Schedule);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &avs_device};
}

FPFW_INIT_COMPONENT(avs3_int, FPFW_INIT_DEPENDENCIES("avs3"))
{
    if (idsw_get_die_id() == DIE_1)
    {
        return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
    }

    static scp_avs_interface_t avs_interface = {};
    fpfw_init_component_id_t avs_id = "avs3";

    scp_avs_interface_initialize(fpfw_init_get_handle(avs_id), &avs_interface);
    DfwkClientInterfaceOpen(&avs_interface.Header);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &avs_interface};
}
