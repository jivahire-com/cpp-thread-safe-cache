/**
 * @file mscp_hspmbx_init.c
 * Instantiates baremetal, polling based, Hsp Mailbox IP for both SCP & MCP cores
 */

/*------------- Includes -----------------*/
#include <DfwkHost.h>                // for DfwkDeviceInitialize
#include <DfwkThreadXHost.h>         // for PDFWK_THREADX_HOST
#include <MboxPrimitives.h>          // for HSP_MBX_FIFO_DEPTH, MBX_IMPL_PO...
#include <fpfw_init.h>               // for fpfw_init_get_handle, FPFW_INIT...
#include <fpfw_mbox_icc_transport.h> // for ICC_MBX_ASYNC_RECV, ICC_MBX_ASY...
#include <fpfw_status.h>             // for fpfw_status_t
#include <fpfw_timer_port.h>         // for _fpfw_timer_t
#include <silibs_mcp_top_regs.h>     // for MCP_TOP_MCP2HSP_MAILBOX_ADDRESS
#include <silibs_scp_top_regs.h>     // for SCP_TOP_SCP2HSP_MAILBOX_ADDRESS
#include <stdint.h>                  // for uint32_t

/*-------------- Macros ------------------*/
#define KG_HSP_MBOX_POLL_INTERVAL_NS (100000ULL)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(hspmbox, FPFW_INIT_DEPENDENCIES("dfwk"))
{
    fpfw_init_component_id_t dfwk_id = "dfwk";
    static struct _fpfw_timer_t timer[ICC_MAX_ASYNC_REQ_TYPE] = {};
    static fpfw_mbox_icc_transport_device_t mscp_mbx_dev = {};
    static fpfw_mbox_icc_transport_config_t cfg = {
        .mbox_dev_cfg = {.MbxFifoDepth = HSP_MBX_FIFO_DEPTH,
                         .MbxImplementation = MBX_IMPL_POLLING,
                         .MsgSizeBytes = (HSP_MBX_FIFO_DEPTH * sizeof(uint32_t)),
                         .MbxBaseAddr = MSCP2HSP_MAILBOX_BASE_ADDRESS},
        .timer_period = KG_HSP_MBOX_POLL_INTERVAL_NS,
        .timer_handle = {&timer[ICC_MBX_ASYNC_SEND], &timer[ICC_MBX_ASYNC_RECV]},
    };

    fpfw_status_t status = fpfw_mbox_icc_transport_dfwk_device_init(&mscp_mbx_dev, &cfg);
    DfwkDeviceInitialize(&mscp_mbx_dev.header, &((PDFWK_THREADX_HOST)fpfw_init_get_handle(dfwk_id))->Schedule);

    return (fpfw_init_result_t){status, &mscp_mbx_dev};
}

FPFW_INIT_COMPONENT(icc_mbx, FPFW_INIT_DEPENDENCIES("hspmbox"))
{
    static fpfw_mbox_icc_transport_intrf_t mscp_mbx_intf = {};
    fpfw_init_component_id_t hspmbox_id = "hspmbox";

    fpfw_status_t status = fpfw_mbox_icc_transport_dfwk_interface_init(fpfw_init_get_handle(hspmbox_id), &mscp_mbx_intf);
    return (fpfw_init_result_t){status, &mscp_mbx_intf};
}
