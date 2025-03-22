/**
 * @file icc_d2dmbx_init.c
 * Instantiates icc base ctx for d2d mailbox transport interface for the platform
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <DfwkHost.h>        // for DfwkDeviceInitialize
#include <DfwkStatus.h>      // for DFWK_SUCCESS
#include <DfwkThreadXHost.h> // for PDFWK_THREADX_HOST
#include <FpFwAssert.h>
#include <MboxPrimitives.h>          // for FPFW_MBX_FIFO_DEPTH, FPFW_MBX_I...
#include <MboxReg.h>                 // for FPFW_MBX_FIFO_DEPTH, FPFW_MBX_I...
#include <fpfw_icc_base.h>           // for fpfw_icc_base_init, fpfw_icc_ba...
#include <fpfw_icc_base_i.h>         // for fpfw_icc_base_ctx_t
#include <fpfw_icc_dispatcher.h>     // for fpfw_icc_dispatcher_start
#include <fpfw_init.h>               // for fpfw_init_result_t, fpfw_init_g...
#include <fpfw_mbox_icc_transport.h> // for ICC_MBX_ASYNC_RECV, ICC_MBX_ASY...
#include <fpfw_status.h>             // for FPFW_STATUS_SUCCESS, fpfw_status_t
#include <fpfw_timer_port.h>         // for _fpfw_timer_t
#include <icc_platform_defines.h>
#include <idhw.h> // for idhw_is_single_die_boot_en
#include <idsw.h>
#include <idsw_kng.h>
#include <interrupts.h>
#include <silibs_mcp_exp_top_regs.h> // for SCP_EXP_TOP_SCF_RAM_ADDRESS, SCP_EXP_TOP_SCF_RAM_SIZE
#include <silibs_mcp_top_regs.h>
#include <silibs_platform.h>
#include <silibs_scp_exp_top_regs.h> // for SCP_EXP_TOP_SCF_RAM_ADDRESS, SCP_EXP_TOP_SCF_RAM_SIZE
#include <silibs_scp_top_regs.h>
#include <spi_bridge.h>
#include <stdbool.h> // for true
#include <stddef.h>  // for NULL
#include <stdint.h>  // for uint32_t, uint8_t
#include <stdio.h>   // for printf, NULL

/*-------------- Macros ------------------*/
#define D2D_MBOX_SPI_CTRL_BASE_ADDR      (MSCP_TOP_MSCP_EXP_ADDRESS + MSCP_EXP_TOP_SPI_CTRL_ADDRESS)
#define D2D_MBOX_SYNC_OPERATION_LOOP_MAX 0x200000UL
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

/**
 * @brief API for D2D mailbox to write to the outgoing mailbox over SPI
 * Current die is the SPI controller, remote die has SPI bridge that converts
 * SPI transactions into AHB and deposits the data in remote die's incoming fifo.
 * Current die can only write to it's outgoing mbox registers or sender side mbox
 * registers over SPI but can directly access the incoming mbox data over APB.
 *
 * @note typically the registers it writes to are d2d mbx recv push reg
 *
 * @param baseAddr register block base address
 * @param regOffset offset of the desired register
 * @param value 2-bit data value to write
 */
static void SpiControllerWrite32(uint32_t baseAddr, FPFW_MBX_REG_OFFSET regOffset, uint32_t value)
{
    uint32_t mbx_reg_addr = baseAddr + regOffset;
    int status = spi_controller_write_direct_instruction((uintptr_t)D2D_MBOX_SPI_CTRL_BASE_ADDR, mbx_reg_addr, 9, value);
    FPFW_RUNTIME_ASSERT(status == SILIBS_SUCCESS);
}

/**
 * @brief API for D2D mailbox to read it's outgoing mailbox over SPI
 * Current die can only read it's outgoing mbox registers or sender side mbox
 * registers over SPI but can directly access the incoming mbox data over APB.
 *
 * @note typically the registers it reads are the d2d mbx recv inst reg
 *
 * @param baseAddr register block base address
 * @param regOffset offset of the desired register
 * @return uint32_t 2-bit data value read
 */
static uint32_t SpiControllerRead32(uint32_t baseAddr, FPFW_MBX_REG_OFFSET regOffset)
{
    uint32_t readData = 0;
    uint32_t mbx_reg_addr = baseAddr + regOffset;
    int status = spi_controller_read_direct_instruction((uintptr_t)D2D_MBOX_SPI_CTRL_BASE_ADDR, mbx_reg_addr, 8, &readData);
    FPFW_RUNTIME_ASSERT(status == SILIBS_SUCCESS);
    return readData;
}

/**
 * @note ICC Support added for D2D Mailbox or RMSS Mailbox.
 * Current die scp can send, remote die scp or mcp can recv.
 * Remote die scp can send, current die scp or mcp can recv.
 *
 */
FPFW_INIT_COMPONENT(icc_d2dmbx, FPFW_INIT_DEPENDENCIES("dfwk", "hw_ver", "debug_print"))
{
    //! Statics declarations required for mailbox primitive
    static struct _fpfw_timer_t d2d_timer[ICC_MAX_ASYNC_REQ_TYPE] = {};
    static fpfw_mbox_icc_transport_config_t d2d_cfg = {
        .mbox_dev_cfg =
            {
                .MbxFifoDepth = D2D_MBX_FIFO_DEPTH, //! 16
                .MbxMesgHandlingType = MBX_MESG_HANDLING_SINGLE_MESG_AT_A_TIME,
                .MbxImplementation = MBX_IMPL_POLLING,             //! polling to begin with
                .MsgSizeBytes = D2D_FIFO_MBOX_MAX_MESG_SIZE_BYTES, //! each message is 64 bytes as of now
                .MbxBaseAddr = MSCP_TOP_MSCP_EXP_ADDRESS + MSCP_EXP_TOP_MSCP_MBOX_ADDRESS,
                .MbxSendBaseAddr = MSCP_TOP_MSCP_EXP_ADDRESS + MSCP_EXP_TOP_MSCP_MBOX_ADDRESS,
                .MbxRecvBaseAddr = MSCP_TOP_MSCP_EXP_ADDRESS + MSCP_EXP_TOP_MSCP_MBOX_ADDRESS,
                .RemoteMbxInstRegAccessRW0Clear = true,
            },
        .timer_period = KG_D2D_MBOX_POLL_INTERVAL_NS,
        .timer_handle = {&d2d_timer[ICC_MBX_ASYNC_SEND], &d2d_timer[ICC_MBX_ASYNC_RECV]},
        .min_mesg_size = sizeof(rmss_d2d_mailbox_msg_header),
    };

    if (idsw_get_cpu_type() == CPU_SCP)
    {
        //! For SCP core, set the d2d mbox as interrupt based, intr only available for SCP
        d2d_cfg.mbox_dev_cfg.MbxImplementation = MBX_IMPL_INTERRUPT; //! polling to begin with
        d2d_cfg.mbx_irq_num = RMSS_D2D_SCP_MB_HW_INT;                //! HW_INT_MSCP_PEER_SCP_MB_INT = 226
        //! set the SPI read/write APIs for d2d in mbox primitives config
        d2d_cfg.mbox_dev_cfg.RemoteRegWrite32 = SpiControllerWrite32;
        d2d_cfg.mbox_dev_cfg.RemoteRegRead32 = SpiControllerRead32;
    }

    //! Statics declarations required for mailbox transport driver
    static fpfw_mbox_icc_transport_device_t d2d_mbx_dev = {};
    static fpfw_mbox_icc_transport_intrf_t d2d_mbx_intf = {};

    //! Statics declarations required for icc base service
    static uint8_t s_d2d_dispatch_buff[D2D_FIFO_MBOX_MAX_MESG_SIZE_BYTES] = {0};
    static fpfw_icc_base_ctx_t s_d2d_icc_base_ctx;
    static fpfw_icc_base_config s_d2d_icc_cfg = {
        .transport_interface = &d2d_mbx_intf.header,
        .dispatch_cfg =
            {
                .transport_interface = &d2d_mbx_intf,
                .dispatcher_buffer = &s_d2d_dispatch_buff,
                .dispatcher_buffer_size = sizeof(s_d2d_dispatch_buff),
                .strategy =
                    {
                        .cmd_code =
                            {
                                .is_used = true,
                                .start_pos = D2D_MBOX_CMD_CODE_START_POS,
                                .size_bits = D2D_MBOX_CMD_CODE_SIZE,
                                .valid_max = D2D_MBOX_MAX_CMD_CODE,
                                .valid_min = D2D_MBOX_MIN_CMD_CODE,
                            },
                        .seq_num =
                            {
                                .is_used = true,
                                .start_pos = D2D_MBOX_SEQ_NUM_START_POS,
                                .size_bits = D2D_MBOX_SEQ_NUM_SIZE,
                                .valid_max = D2D_MBOX_MAX_SEQ_NUM,
                                .valid_min = D2D_MBOX_MIN_SEQ_NUM,
                            },
                    },
                .match_strategy_cb = NULL,
                .match_strategy_ctx = NULL,
            },
        .ctx = NULL,
        .sync_loop_count = D2D_MBOX_SYNC_OPERATION_LOOP_MAX,
    };

    //! Initialize d2d mailbox transport driver
    fpfw_status_t status = fpfw_mbox_icc_transport_dfwk_device_init(&d2d_mbx_dev, &d2d_cfg);
    if (status != DFWK_SUCCESS)
    {
        return (fpfw_init_result_t){status, NULL};
    }
    DfwkDeviceInitialize(&d2d_mbx_dev.header, &((PDFWK_THREADX_HOST)fpfw_init_get_handle("dfwk"))->Schedule);
    status = fpfw_mbox_icc_transport_dfwk_interface_init(&d2d_mbx_dev, &d2d_mbx_intf);
    if (status != DFWK_SUCCESS)
    {
        return (fpfw_init_result_t){status, NULL};
    }

    //! Initialize ICC base ctx for d2d mailbox transport driver
    status = fpfw_icc_base_init(&s_d2d_icc_base_ctx, &s_d2d_icc_cfg);
    if (status == FPFW_STATUS_SUCCESS)
    {
        //! start the dispatcher to receive data over d2d mailbox
        status = fpfw_icc_dispatcher_start(&s_d2d_icc_base_ctx.dispatch_ctx);
        if (status != FPFW_ICC_DISPATCH_STATUS_SUCCESS)
        {
            return (fpfw_init_result_t){status, NULL};
        }
    }
    //! pass in status & ref to ctx to the clients
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &s_d2d_icc_base_ctx};
}