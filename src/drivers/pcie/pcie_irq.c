//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Implements handlers and calls the relevant silibs APIs to walk the PCIe RPSS
 * INTU tree to understand what event might have triggered an RPSS interrupt.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <FpFwAssert.h>
#include <bug_check.h>
#include <health_monitor.h>
#include <idsw.h>
#include <interrupts.h>
#include <intu_lib.h>
#include <kng_soc_constants.h>
#include <oi_pcie.h>
#include <pcie_common.h>
#include <pcie_dfwk.h>
#include <pcie_dfwk_i.h>
#include <pcie_events.h>
#include <pcie_irq.h>
#include <pcie_rp_common.h>
#include <pcie_rp_sii.h>
#include <pcie_ss_common.h>
#include <pciess.h>
#include <pciess_int.h>
#include <rpss_addr_map_regs.h>
#include <silibs_common.h>
#include <stdbool.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/
typedef struct _pcie_bugcheck_info_t
{
    KNG_STATUS error_code;
    union
    {
        struct
        {
            uint16_t rpss_instance;
            uint16_t rp_index;
        };
        uint32_t info;
    };
    uint32_t error_type;
    bool valid;
} pcie_bugcheck_info_t;

/*-------- Function Prototypes -----------*/
static bool rpss_record_callback(ras_error_record_t* record, KNG_STATUS errcode);

/*-- Declarations (Statics and globals) --*/
static pcie_bugcheck_info_t bugcheck_info = {0};

/*------------- Functions ----------------*/
static bool rpss_record_callback(ras_error_record_t* record, KNG_STATUS errcode)
{
    RPSS_INSTANCE rpss_instance = NUM_RPSS;
    unsigned rp_index = 0;

    /* Check record */
    ras_agent_entity_t* agent = (ras_agent_entity_t*)record->reporting_agent;
    if (agent)
    {
        pcie_rp_entity_t* rp = (pcie_rp_entity_t*)agent->parent;
        rpss_instance = rp->ss_id;
        rp_index = rp->id;
    }

    acpi_cper_section_t cper_section = {0};
    acpi_err_sec_pcie_vendor_t* pcie_cper = &cper_section.sec_pcie_vendor;
    uint32_t severity;
    if (ras_agent_record_to_cper(record, pcie_cper, sizeof(acpi_err_sec_pcie_vendor_t), &severity))
    {
        FPFW_DBGPRINT_ALWAYS("Unable to convert RAS record to CPER!\n");
    }
    else
    {
        hm_submit_cper(ACPI_ERROR_DOMAIN_PCIE, severity, &cper_section, sizeof(cper_section));
    }

    pciess_ras_process_record(record);

    /* Return true if a bugcheck is required */
    if (severity == ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL)
    {
        /* Setup bugcheck information if this is the first fatal error */
        if (!bugcheck_info.valid)
        {
            bugcheck_info.error_code = errcode;
            bugcheck_info.rpss_instance = rpss_instance;
            bugcheck_info.rp_index = rp_index;
            bugcheck_info.error_type = pcie_cper->error_type;
            bugcheck_info.valid = true;
        }
        return true;
    }

    return false;
}

void rpss_irq_callback(pcie_ss_entity_t* ss, pciess_int_probe_t* info)
{
    pcie_async_request_t* pending_req = {0};
    ras_error_record_t record;
    bool bugcheck_required = false;

    BUG_ASSERT_PARAM((ss != NULL) && (info != NULL), ss, info);

    for (uint8_t rp_index = 0; rp_index < (unsigned)ss->ss_type; rp_index++)
    {
        pcie_rp_entity_t* rp = &ss->rps[rp_index];
        ras_agent_entity_t* ide_agent = &rp->ide.ide_pagent;
        ras_agent_entity_t* dtim_agent = &rp->dtim_pagent;
        ras_agent_entity_t* ltim_agent = &rp->ltim_pagent;
        ras_agent_entity_t* vsecras_agent = &rp->vsecras_pagent;

        /* Async request completion data */
        uint32_t int_mask = 0;
        uint64_t glbl_ide_data = 0;
        uint64_t aes_hcfg_data = 0;

        if (info->rp_ints[rp_index].ints[PCIESS_RP_INT_GLOBAL_IDE].asserted)
        {
            uint64_t status = info->rp_ints[rp_index].ints[PCIESS_RP_INT_GLOBAL_IDE].status;
            PCIE_ET_INFO_PARAM(PCIE_ET_TYPE_INT_GLOBAL_IDE, (ss->id << 16) | rp_index);

            /* Iterate per IDE sub-domain */
            if (status & PCIE_IDE_SRAM_ECC_GLOBAL_INT)
            {
                /* All sources are errors (none are notifiers) */
                while (ras_pcie_ide_agent_probe_by_type(ide_agent, RAS_PCIE_IDE_ALL_KEY_SRAM_ERRORS, &record))
                {
                    bugcheck_required |= rpss_record_callback(&record, KNG_PCIE_IDE_SRAM_ECC_ERROR);
                }
            }
            if (status & PCIE_IDE_DATAPATH_GLOBAL_INT)
            {
                /* All sources are errors (none are notifiers) */
                while (ras_pcie_ide_agent_probe_by_type(ide_agent, RAS_PCIE_IDE_ALL_DATAPATH_ERRORS, &record))
                {
                    bugcheck_required |= rpss_record_callback(&record, KNG_PCIE_IDE_DATAPATH_ERROR);
                }
            }

            if (status & PCIE_IDE_MISC_GLOBAL_INT)
            {
                /* Not all sources are errors (some are notifiers) */
                while (ras_pcie_ide_agent_probe_by_type(ide_agent, RAS_PCIE_IDE_ALL_MISC_ERRORS, &record))
                {
                    bugcheck_required |= rpss_record_callback(&record, KNG_PCIE_IDE_OPERATIONAL_ERROR);
                }

                /* PCIE_IDE_KEY_NEEDED_INT is both an error and a notifier */
                if (status & PCIE_IDE_KEY_NEEDED_INT)
                {
                    /*
                     * This means at least one TX/RX stream has swapped into a K-bit slot without a
                     * valid key. This is considered fatal to the link, and we must forcefully terminate ALL
                     * streams immediately.
                     *
                     * This will cause an INSECURE_TRANSITION event.
                     */
                    silibs_status_t status = pcie_rp_ide_disable_all_streams(rp);
                    if (status)
                    {
                        FPFW_DBGPRINT_ALWAYS("RP[%d, %d] Error: Unable to terminate IDE streams!\n", ss->id, rp_index);
                        BUG_CHECK(KNG_PCIE_FW_ERROR, status, 0x0);
                    }
                }

                /* Advisory notifiers are serviced asynchronously */
                if (status & PCIE_IDE_KEY_NEEDED_INT || status & PCIE_IDE_TX_KBIT_TOGGLE_INT ||
                    status & PCIE_IDE_RX_KBIT_TOGGLE_INT || status & PCIE_IDE_TX_REKEY_REQ_INT ||
                    status & PCIE_IDE_RX_REKEY_REQ_INT)
                {
                    int_mask |= 1 << PCIESS_RP_INT_GLOBAL_IDE;
                    glbl_ide_data = status;
                }
            }
        }

        if (info->rp_ints[rp_index].ints[PCIESS_RP_INT_AES_HCFG].asserted)
        {
            uint64_t status = info->rp_ints[rp_index].ints[PCIESS_RP_INT_AES_HCFG].status;
            PCIE_ET_INFO_PARAM(PCIE_ET_TYPE_INT_AES_HCFG, (ss->id << 16) | rp_index);
            /* Not all sources are errors (some are advisory) */
            while (ras_pcie_ide_agent_probe_by_type(ide_agent, RAS_PCIE_IDE_ALL_AES_HCFG_ERRORS, &record))
            {
                bugcheck_required |= rpss_record_callback(&record, KNG_PCIE_IDE_OPERATIONAL_ERROR);
            }

            /* Advisory notifiers are serviced asynchronously */
            if ((status & PCIE_IDE_TX_KEY_DONE_INT) || (status & PCIE_IDE_RX_KEY_DONE_INT))
            {
                int_mask |= 1 << PCIESS_RP_INT_AES_HCFG;
                aes_hcfg_data = status;
            }
        }

        if (info->rp_ints[rp_index].ints[PCIESS_RP_INT_LINK_DOWN].asserted)
        {
            PCIE_ET_INFO_PARAM(PCIE_ET_TYPE_INT_LINK_DOWN, (ss->id << 16) | rp_index);
            int_mask |= 1 << PCIESS_RP_INT_LINK_DOWN;
        }

        if (info->rp_ints[rp_index].ints[PCIESS_RP_INT_LINK_UP].asserted)
        {
            PCIE_ET_INFO_PARAM(PCIE_ET_TYPE_INT_LINK_UP, (ss->id << 16) | rp_index);
            int_mask |= 1 << PCIESS_RP_INT_LINK_UP;
        }

        if (info->rp_ints[rp_index].ints[PCIESS_RP_INT_DTIM].asserted)
        {
            FPFW_DBGPRINT_ALWAYS("RP[%d, %d]: DTIM asserted! Sources: 0x%x\n",
                                 ss->id,
                                 rp_index,
                                 info->rp_ints[rp_index].ints[PCIESS_RP_INT_DTIM].status);
            PCIE_ET_INFO_PARAM(PCIE_ET_TYPE_INT_DTIM, (ss->id << 16) | rp_index);
            while (ras_pcie_dtim_agent_probe_by_type(dtim_agent, RAS_PCIE_DTIM_ALL_TYPES, &record))
            {
                bugcheck_required |= rpss_record_callback(&record, KNG_PCIE_DTI_ERROR);
            }
        }

        if (info->rp_ints[rp_index].ints[PCIESS_RP_INT_LTIM].asserted)
        {
            FPFW_DBGPRINT_ALWAYS("RP[%d, %d]: LTIM asserted! Sources: 0x%x\n",
                                 ss->id,
                                 rp_index,
                                 info->rp_ints[rp_index].ints[PCIESS_RP_INT_LTIM].status);
            PCIE_ET_INFO_PARAM(PCIE_ET_TYPE_INT_LTIM, (ss->id << 16) | rp_index);
            while (ras_pcie_ltim_agent_probe_by_type(ltim_agent, RAS_PCIE_LTIM_ALL_TYPES, &record))
            {
                bugcheck_required |= rpss_record_callback(&record, KNG_PCIE_LTI_ERROR);
            }
        }

        if (info->rp_ints[rp_index].ints[PCIESS_RP_INT_RASDP].asserted)
        {
            PCIE_ET_INFO_PARAM(PCIE_ET_TYPE_INT_RASDP, (ss->id << 16) | rp_index);

            /*
             * RASDP_ERR_MODE breaks the 'while' fetch pattern as the records fetched cannot be cleared
             * on a per-record basis. Instead, we gather the latest information for both CE and UE
             * independently and clear the error mode on exit
             */
            if (ras_pcie_vsecras_agent_probe_by_type(vsecras_agent, RAS_UNCORRECTABLE_ERROR, &record))
            {
                bugcheck_required |= rpss_record_callback(&record, KNG_PCIE_DATAPATH_ERROR);
            }
            if (ras_pcie_vsecras_agent_probe_by_type(vsecras_agent, RAS_CORRECTABLE_ERROR, &record))
            {
                bugcheck_required |= rpss_record_callback(&record, KNG_PCIE_DATAPATH_ERROR);
            }
            /*
             * Any other FW action prior to error-mode exit must occur here
             * (such as AER upleveling for link-down)
             */
            silibs_status_t status = pcie_rp_vsecras_clear_rasdp_error_mode(rp);
            if (status)
            {
                FPFW_DBGPRINT_ALWAYS("RP[%d, %d] Error: Unable to exit RASDP_ERR_MODE!\n", ss->id, rp_index);
                BUG_CHECK(KNG_PCIE_FW_ERROR, status, 0x0);
            }
        }

        if (info->rp_ints[rp_index].ints[PCIESS_RP_INT_DPC].asserted)
        {
            PCIE_ET_INFO_PARAM(PCIE_ET_TYPE_INT_DPC, (ss->id << 16) | rp_index);

            /*
             * Bug 3360329 - ERRATUM 1081195 workaround commented out until we have
             * a Windows build that supports a longer DL up link time that results
             * due to this workaround. Till then, simply clear RP_BUSY without
             * re-routing to default completer, which means that transactions
             * will continue to be posted to the endpoint even when the link is
             * down. This is not ideal, but should not cause any functional issues
             * for use-cases apart from hotplug.
             */
            silibs_status_t status = oi_pcie_ss_set_rp_dpc_status(ss, rp_index, false);
            if (status)
            {
                FPFW_DBGPRINT_ALWAYS("RP[%d, %d] Error: Unable to clear RP_BUSY!\n", ss->id, rp_index);
            }
#if 0
            /*
             *  Obtain the current LTSSM state, we reroute transactions to
             *  the default completer only if the link is disabled
             */
            PCIE_LTSSM_STATE ltssm_state = pcie_rp_sii_get_link_state(rp);
            if ((ltssm_state == PCIE_LTSSM_STATE_DISABLED_0x17) ||
                (ltssm_state == PCIE_LTSSM_STATE_DISABLED_0x18) || (ltssm_state == PCIE_LTSSM_STATE_DISABLED_0x19))
            {
                /*
                 * ERRATUM 1081195 workaround - Re-route to default completer
                 * Needs to happen only when the link is in disabled state.
                 */
                silibs_status_t status = pciess_fallback_rp_to_default_completer(rp);
                if (status)
                {
                    FPFW_DBGPRINT_ALWAYS("RP[%d, %d] Error: Unable to re-route to default completer!\n", ss->id, rp_index);
                }

                /* Clear RP_BUSY */
                status = oi_pcie_ss_set_rp_dpc_status(ss, rp_index, false);
                if (status)
                {
                    FPFW_DBGPRINT_ALWAYS("RP[%d, %d] Error: Unable to clear RP_BUSY!\n", ss->id, rp_index);
                }
            }
            else
            {
                FPFW_DBGPRINT_ERROR("RP[%d, %d]: Transactions not re-routed to default completer!\n", ss->id, rp_index);
            }
#endif
        }

        /*
         * Unexpected PCIe interrupts (described as non-POR in the HAS)  are left
         * unhandled.
         *
         * There is no need to bugcheck on these interrupts for now as they do
         * not indicate a fatal error condition. One can be added in the future
         * if it is determined that these interrupts indicate a fatal error.
         */
        if (info->rp_ints[rp_index].ints[PCIESS_RP_INT_HP_PME].asserted ||
            info->rp_ints[rp_index].ints[PCIESS_RP_INT_WAKEUP].asserted ||
            info->rp_ints[rp_index].ints[PCIESS_RP_INT_PM_PME].asserted ||
            info->rp_ints[rp_index].ints[PCIESS_RP_INT_PM_TO_ACK].asserted ||
            info->rp_ints[rp_index].ints[PCIESS_RP_INT_SEND_C].asserted ||
            info->rp_ints[rp_index].ints[PCIESS_RP_INT_SEND_NF].asserted ||
            info->rp_ints[rp_index].ints[PCIESS_RP_INT_SEND_F].asserted)
        {
            PCIE_ET_INFO_PARAM(PCIE_ET_TYPE_INT_UNEXPECTED, (ss->id << 16) | rp_index);
        }

        if (int_mask)
        {
            pending_req = get_pending_async_req_for_this_rp(ss->id, rp_index, WAIT_FOR_EVENT);
            if (pending_req)
            {
                pending_req->async_data.int_mask = int_mask;
                pending_req->async_data.aes_hcfg_data = aes_hcfg_data;
                pending_req->async_data.glbl_ide_data = glbl_ide_data;
                complete_async_req_for_this_rp(pending_req);
            }
        }
    }

    if (bugcheck_required)
    {
        BUG_CHECK(bugcheck_info.error_code, bugcheck_info.info, bugcheck_info.error_type);
    }
}
