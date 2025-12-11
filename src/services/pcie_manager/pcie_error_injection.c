//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_error_injection.c
 * Implements PCIe error injection functions.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <DfwkPtrTypes.h>
#include <FpFwUtils.h>
#include <cper.h>
#include <idsw.h>
#include <kng_soc_constants.h>
#include <pcie_einj_structs.h>
#include <pcie_manager_events.h>
#include <pcie_manager_i.h>
#include <pcie_phy.h>
#include <pcie_rp_rasdes.h>
#include <pcie_ss.h>
#include <pcie_sync_requests_i.h>
#include <scp_pcie_manager.h>
#include <silibs_status.h>
#include <stdbool.h>
#include <stdint.h>
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
/* PCIe bus ranges */
#define PCIE_RPSS0_BUS_MAX (PCIE_RPSS_BUSES_PER_SS_STRIDE - 1)
#define PCIE_RPSS1_BUS_MAX (PCIE_RPSS0_BUS_MAX + PCIE_RPSS_BUSES_PER_SS_STRIDE)
#define PCIE_RPSS2_BUS_MAX (PCIE_RPSS1_BUS_MAX + PCIE_RPSS_BUSES_PER_SS_STRIDE)
#define PCIE_RPSS3_BUS_MAX (PCIE_RPSS2_BUS_MAX + PCIE_RPSS_BUSES_PER_SS_STRIDE)
#define PCIE_RPSS4_BUS_MAX (PCIE_RPSS3_BUS_MAX + PCIE_RPSS_BUSES_PER_SS_STRIDE)
#define PCIE_RPSS5_BUS_MAX (PCIE_RPSS4_BUS_MAX + PCIE_RPSS_BUSES_PER_SS_STRIDE)
#define PCIE_RPSS6_BUS_MAX (PCIE_RPSS5_BUS_MAX + PCIE_RPSS_BUSES_PER_SS_STRIDE)
#define PCIE_RPSS7_BUS_MAX (PCIE_RPSS6_BUS_MAX + PCIE_RPSS_BUSES_PER_SS_STRIDE)

/*------------- Functions ----------------*/
static bool validate_einj_payload(ras_einj_info_t* info)
{
    bool is_valid_payload = false;

    if (info == NULL)
    {
        FPFW_DBGPRINT_ERROR("[PCIe EINJ]: NULL einj payload received!\n");
        PCIE_MANAGER_ET_ERROR(PCIE_MANAGER_ET_TYPE_EINJ_NULL_PAYLOAD);
        goto done;
    }

    pcie_einj_operation_t* op = (pcie_einj_operation_t*)&(info->status_operation);
    pcie_einj_params_t* pcie_params = (pcie_einj_params_t*)&(info->param_type.error_parameters[0]);

    if (info->component_instance != idsw_get_die_id())
    {
        FPFW_DBGPRINT_ERROR("[PCIe EINJ]: Component Instance: %d does not match Die Id: %d",
                            info->component_instance,
                            idsw_get_die_id());
        PCIE_MANAGER_ET_ERROR_PARAM(PCIE_MANAGER_ET_TYPE_EINJ_INSTANCE_MISMATCH, info->component_instance);
        goto done;
    }

    if (info->component_group != ACPI_ERROR_DOMAIN_PCIE)
    {
        FPFW_DBGPRINT_ERROR("[PCIe EINJ]: Invalid component group: 0x%x\n", info->component_group);
        PCIE_MANAGER_ET_ERROR_PARAM(PCIE_MANAGER_ET_TYPE_EINJ_INVALID_COMPONENT_GROUP, info->component_group);
        goto done;
    }

    if (pcie_params->sbdf.segment != 0)
    {
        FPFW_DBGPRINT_ERROR("[PCIe EINJ]: Invalid segment number: %d\n", pcie_params->sbdf.segment);
        PCIE_MANAGER_ET_ERROR_PARAM(PCIE_MANAGER_ET_TYPE_EINJ_INVALID_SEGMENT, pcie_params->sbdf.segment);
        goto done;
    }

    if (pcie_params->sbdf.bus > PCIE_RPSS7_BUS_MAX)
    {
        FPFW_DBGPRINT_ERROR("[PCIe EINJ]: Invalid bus number: %d\n", pcie_params->sbdf.bus);
        PCIE_MANAGER_ET_ERROR_PARAM(PCIE_MANAGER_ET_TYPE_EINJ_INVALID_BUS, pcie_params->sbdf.bus);
        goto done;
    }

    if (op->error_type >= PCIE_ERROR_TYPE_MAX)
    {
        FPFW_DBGPRINT_ERROR("[PCIe EINJ]: Invalid error type: %d\n", op->error_type);
        PCIE_MANAGER_ET_ERROR_PARAM(PCIE_MANAGER_ET_TYPE_EINJ_INVALID_TYPE, op->error_type);
        goto done;
    }

    if (op->error_type == PCIE_ERROR_TYPE_AER && (pcie_params->error_data.aer > PCIE_SS_APP_ERR_HEADER_LOG_OVERFLOW))
    {
        FPFW_DBGPRINT_ERROR("[PCIe EINJ]: Invalid AER error data: %d\n", pcie_params->error_data.aer);
        PCIE_MANAGER_ET_ERROR_PARAM(PCIE_MANAGER_ET_TYPE_EINJ_INVALID_AER_DATA, pcie_params->error_data.aer);
        goto done;
    }

    if (op->error_type == PCIE_ERROR_TYPE_RP_INTERNAL_CRC && (pcie_params->error_data.crc > PCIE_RASDES_INJ_ECRC))
    {
        FPFW_DBGPRINT_ERROR("[PCIe EINJ]: Invalid CRC error data: %d\n", pcie_params->error_data.crc);
        PCIE_MANAGER_ET_ERROR_PARAM(PCIE_MANAGER_ET_TYPE_EINJ_INVALID_CRC_DATA, pcie_params->error_data.crc);
        goto done;
    }

    if (op->error_type == PCIE_ERROR_TYPE_RP_INTERNAL_SEQNUM &&
        (pcie_params->error_data.seqnum > PCIE_RASDES_INJ_DLLP_ACK_NACK_SEQNUM))
    {
        FPFW_DBGPRINT_ERROR("[PCIe EINJ]: Invalid SEQNUM error data: %d\n", pcie_params->error_data.seqnum);
        PCIE_MANAGER_ET_ERROR_PARAM(PCIE_MANAGER_ET_TYPE_EINJ_INVALID_SEQNUM_DATA, pcie_params->error_data.seqnum);
        goto done;
    }

    if (op->error_type == PCIE_ERROR_TYPE_RP_INTERNAL_DLLP && (pcie_params->error_data.dllp > PCIE_RASDES_INJ_DLLP_NACK_TIMEOUT))
    {
        FPFW_DBGPRINT_ERROR("[PCIe EINJ]: Invalid DLLP error data: %d\n", pcie_params->error_data.dllp);
        PCIE_MANAGER_ET_ERROR_PARAM(PCIE_MANAGER_ET_TYPE_EINJ_INVALID_DLLP_DATA, pcie_params->error_data.dllp);
        goto done;
    }

    if (op->error_type == PCIE_ERROR_TYPE_RP_INTERNAL_SYMBOL && (pcie_params->error_data.symbol > PCIE_RASDES_INJ_SKP_SYMBOL))
    {
        FPFW_DBGPRINT_ERROR("[PCIe EINJ]: Invalid SYMBOL error data: %d\n", pcie_params->error_data.symbol);
        PCIE_MANAGER_ET_ERROR_PARAM(PCIE_MANAGER_ET_TYPE_EINJ_INVALID_SYMBOL_DATA, pcie_params->error_data.symbol);
        goto done;
    }

    if (op->error_type == PCIE_ERROR_TYPE_RP_INTERNAL_FC && (pcie_params->error_data.fc > PCIE_RASDES_INJ_CPL_TLP_DATA_CREDIT))
    {
        FPFW_DBGPRINT_ERROR("[PCIe EINJ]: Invalid FC error data: %d\n", pcie_params->error_data.fc);
        PCIE_MANAGER_ET_ERROR_PARAM(PCIE_MANAGER_ET_TYPE_EINJ_INVALID_FC_DATA, pcie_params->error_data.fc);
        goto done;
    }

    if (op->error_type == PCIE_ERROR_TYPE_RP_INTERNAL_RETRY_TLP &&
        (pcie_params->error_data.retry_tlp > PCIE_RASDES_INJ_NULLIFIED_TLP))
    {
        FPFW_DBGPRINT_ERROR("[PCIe EINJ]: Invalid RETRY TLP error data: %d\n", pcie_params->error_data.retry_tlp);
        PCIE_MANAGER_ET_ERROR_PARAM(PCIE_MANAGER_ET_TYPE_EINJ_INVALID_RETRY_TLP_DATA, pcie_params->error_data.retry_tlp);
        goto done;
    }

    if (op->error_type == PCIE_ERROR_TYPE_RP_INTERNAL_PHY_SET_SRAM_ERROR ||
        op->error_type == PCIE_ERROR_TYPE_RP_INTERNAL_PHY_CLEAR_SRAM_ERROR)
    {
        /* We cannot validate addresses or masks, just check that the phy instance is within bounds */
        if (pcie_params->error_data.phy_sram.phy_inst > PCIE_PHY_BCAST)
        {
            FPFW_DBGPRINT_ERROR("[PCIe EINJ]: Invalid PHY instance: %d\n", pcie_params->error_data.phy_sram.phy_inst);
            PCIE_MANAGER_ET_ERROR_PARAM(PCIE_MANAGER_ET_TYPE_EINJ_INVALID_PHY_INSTANCE,
                                        pcie_params->error_data.phy_sram.phy_inst);
            goto done;
        }
    }

    is_valid_payload = true;

done:
    return is_valid_payload;
}

static RPSS_INSTANCE get_rpss_instance_from_sbdf(pcie_sbdf_t* sbdf)
{
    /*
     * Kingsgate divides bus distributions (0-255) per RPSS, we simply need to
     * get the bus number from the SBDF and choose the RPSS bucket it falls into.
     *
     * Note: SDM/CDED do not support error injections through this interface, so
     *       we can safely reject the SBDF ranges for those devices.
     */
    RPSS_INSTANCE rpss_idx = NUM_RPSS;
    uint32_t bus_number = sbdf->bus;

    if (bus_number <= PCIE_RPSS0_BUS_MAX)
    {
        rpss_idx = RPSS0;
    }
    else if (bus_number <= PCIE_RPSS1_BUS_MAX)
    {
        rpss_idx = RPSS1;
    }
    else if (bus_number <= PCIE_RPSS2_BUS_MAX)
    {
        rpss_idx = RPSS2;
    }
    else if (bus_number <= PCIE_RPSS3_BUS_MAX)
    {
        rpss_idx = RPSS3;
    }
    else if (bus_number <= PCIE_RPSS4_BUS_MAX)
    {
        rpss_idx = RPSS4;
    }
    else if (bus_number <= PCIE_RPSS5_BUS_MAX)
    {
        rpss_idx = RPSS5;
    }
    else if (bus_number <= PCIE_RPSS6_BUS_MAX)
    {
        rpss_idx = RPSS6;
    }
    else if (bus_number <= PCIE_RPSS7_BUS_MAX)
    {
        rpss_idx = RPSS7;
    }

    return rpss_idx;
}

PLACED_CODE acpi_einj_cmd_status_t pcie_error_injection_cb(ras_einj_info_t* einj_payload, void* error_domain_context)
{
    FPFW_UNUSED(error_domain_context);

    if (validate_einj_payload(einj_payload) == false)
    {
        return ACPI_EINJ_INVALID_ACCESS;
    }

    /* Get error injection parameters specific to PCIe */
    pcie_einj_params_t* pcie_params = (pcie_einj_params_t*)&(einj_payload->param_type.error_parameters[0]);

    RPSS_INSTANCE rpss_idx = get_rpss_instance_from_sbdf(&(pcie_params->sbdf));
    if (rpss_idx >= NUM_RPSS)
    {
        FPFW_DBGPRINT_ERROR("[PCIe EINJ]: SBDF from payload cannot be matched to an rpss!\n");
        PCIE_MANAGER_ET_ERROR_PARAM(PCIE_MANAGER_ET_TYPE_EINJ_RPSS_LOOKUP_FAIL, pcie_params->sbdf.bus);
        return ACPI_EINJ_INVALID_ACCESS;
    }

    /* Request the driver to inject the requested error */
    pcie_manager_context_t* ctx = scp_pcie_get_manager_context(rpss_idx);
    silibs_status_t status = send_sync_rpss_inject_pcie_error((PDFWK_INTERFACE_HEADER)ctx->iface, rpss_idx, einj_payload);
    if (status != SILIBS_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR("[PCIe EINJ]: The PCIe driver failed to inject an error! Status: %d\n", status);
        PCIE_MANAGER_ET_ERROR_PARAM(PCIE_MANAGER_ET_TYPE_EINJ_DRIVER_INJECT_FAIL, status);
        return ACPI_EINJ_UNKNOWN_FAILURE;
    }

    return ACPI_EINJ_SUCCESS;
}
