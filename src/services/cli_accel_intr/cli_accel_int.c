//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_accel_int.c
 * Source file to implement cli for accel_interrupt_handler
 */

/*-------------------------------- Includes ---------------------------------*/
#include "IFpFwEventTracingGeneration.h" // for FPFW_ET_LOG

#include <FpFwLinkedList.h>       // for NULL_LIST_ENTRY
#include <FpFwUtils.h>            // for FPFW_ARRAY_SIZE
#include <accelip_id.h>           // for ACCEL_ID_CDED, ACCEL_ID_SDM
#include <accelip_ss_init.h>      // for D0_ACCELIP_SDMSS, D0_ACCELIP_CDEDSS
#include <atu_init.h>             // for atu_svc_accel_atu_addr
#include <cli_accel_int.h>        // for READ_WRITE, READ_ONLY, READ...
#include <cli_accel_int_events.h> // for EventWriteREG_OPER_FAILED
#include <errno.h>                // for EPERM, EINVAL
#include <sdm_ext_cfg_regs.h>     // for sdm_ext_cfg_regs
#include <silibs_platform.h>      // for MMIO_WRITE32, MMIO_READ32
#include <stdint.h>               // for uint32_t, uint8_t
#include <stdlib.h>               // for errno, strtol

/*------------------- Symbolic Constant Macros (defines) --------------------*/
// Word length in bytes
#define DEFAULT_WORD_SIZE sizeof(uint32_t)

#define ACCEL_CLI_GET_CFG_ADDRESS(addr) (SDM_EXT_CFG__ADDRESSBLOCK_0X100000_ADDRESS + addr)

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/
static FPFW_CLI_STATUS cli_accel_int_reg_rd(int argc, const char** argv);
static FPFW_CLI_STATUS cli_accel_int_reg_wr(int argc, const char** argv);
static FPFW_CLI_STATUS cli_accel_int_reg_set(int argc, const char** argv);
static FPFW_CLI_STATUS cli_accel_int_reg_clr(int argc, const char** argv);

/*------------------- Declarations (Statics and globals) --------------------*/
// clang-format off
static FPFW_CLI_COMMAND s_accel_int_commands_table[] = {
    {NULL_LIST_ENTRY, "accel_int", "reg_rd",  cli_accel_int_reg_rd,  "Read value",                       "reg_rd [addr(hex)]"},
    {NULL_LIST_ENTRY, "accel_int", "reg_wr",  cli_accel_int_reg_wr,  "Write value",               "reg_wr [addr(hex)] [value(hex)]"},
    {NULL_LIST_ENTRY, "accel_int", "reg_set", cli_accel_int_reg_set, "Set register bits",    "reg_set [addr(hex)] [bitmask(hex)]"},
    {NULL_LIST_ENTRY, "accel_int", "reg_clr", cli_accel_int_reg_clr, "Clr register bits",    "reg_clr [addr(hex)] [bitmask(hex)]"},
};

// Registers supported as a part of accel_interrupt_handler commands.
// reg_address : Address of register
// reg_rw_type : Type of operation supported by the register
reg_data_t accel_interrupt_handler_registers[] = {
    // sdm_ext_cfg_regs.emcpu_cfg.wdt_intr_set
    {
        .reg_address = ACCEL_CLI_GET_CFG_ADDRESS(_ADDRESSBLOCK_0X100000_EMCPU_CFG_WDT_INTR_SET_ADDRESS),
        .reg_rw_type = READ_WRITE_ONE_TO_SET,
        .word_size   = DEFAULT_WORD_SIZE
    },
    // sdm_ext_cfg_regs_regs.bcfg.boot_cfg.fab_wdt_intr_set
    {
        .reg_address = ACCEL_CLI_GET_CFG_ADDRESS(_ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_FAB_WDT_INTR_SET_ADDRESS),
        .reg_rw_type = READ_WRITE_ONE_TO_SET,
        .word_size   = DEFAULT_WORD_SIZE
    },
    // sdm_ext_cfg_regs_regs.bcfg.boot_cfg.sdm_wdt_intr_set
    {
        .reg_address = ACCEL_CLI_GET_CFG_ADDRESS(_ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_SDM_WDT_INTR_SET_ADDRESS),
        .reg_rw_type = READ_WRITE_ONE_TO_SET,
        .word_size   = DEFAULT_WORD_SIZE
    },
    // sdm_ext_cfg_regs.bcfg.boot_cfg.fab_err_intr_set
    {
        .reg_address = ACCEL_CLI_GET_CFG_ADDRESS(_ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_FAB_ERR_INTR_SET_ADDRESS),
        .reg_rw_type = READ_WRITE_ONE_TO_SET,
        .word_size   = DEFAULT_WORD_SIZE
    },
    // sdm_ext_cfg_regs.misc.sys_ext_intr2.msk
    {
        .reg_address = ACCEL_CLI_GET_CFG_ADDRESS(_ADDRESSBLOCK_0X100000_MISC_SYS_EXT_INTR2_MSK_ADDRESS),
        .reg_rw_type = READ_WRITE,
        .word_size   = DEFAULT_WORD_SIZE
    },
    // sdm_ext_cfg_regs.misc.sys_ext_intr2.stat
    {
        .reg_address = ACCEL_CLI_GET_CFG_ADDRESS(_ADDRESSBLOCK_0X100000_MISC_SYS_EXT_INTR2_STAT_ADDRESS),
        .reg_rw_type = READ_ONLY,        // Setting this to Read Only over CLI for protection. HW settings: READ_WRITE_ONE_TO_CLEAR
        .word_size   = DEFAULT_WORD_SIZE
    },
    // sdm_ext_cfg_regs.misc.sys_ext_intr2.stat_set
    {
        .reg_address = ACCEL_CLI_GET_CFG_ADDRESS(_ADDRESSBLOCK_0X100000_MISC_SYS_EXT_INTR2_STAT_SET_ADDRESS),
        .reg_rw_type = READ_WRITE_ONE_TO_SET,
        .word_size   = DEFAULT_WORD_SIZE
    },
    // sdm_ext_cfg_regs.misc.sys_ext_intr2.msg_send_intr
    {
        .reg_address = ACCEL_CLI_GET_CFG_ADDRESS(_ADDRESSBLOCK_0X100000_MISC_SYS_EXT_INTR2_MSG_SEND_INTR_ADDRESS),
        .reg_rw_type = READ_WRITE_ONE_TO_SET,
        .word_size   = DEFAULT_WORD_SIZE
    },
    // sdm_ext_cfg_regs.misc.sys_ext_intr2.msg_send_intr_mask
    {
        .reg_address = ACCEL_CLI_GET_CFG_ADDRESS(_ADDRESSBLOCK_0X100000_MISC_SYS_EXT_INTR2_MSG_SEND_INTR_MSK_ADDRESS),
        .reg_rw_type = READ_ONLY,
        .word_size   = DEFAULT_WORD_SIZE
    },
    // ITCM
    {
        .reg_address = SDM_EXT_CFG_EMCPU_TCM_ITCM_ADDRESS + DEFAULT_WORD_SIZE,
        .reg_rw_type = READ_ONLY,
        .word_size   = DEFAULT_WORD_SIZE
    },
    // D0TCM
    {
        .reg_address = SDM_EXT_CFG_EMCPU_TCM_DTCM_ADDRESS + DEFAULT_WORD_SIZE,
        .reg_rw_type = READ_ONLY,
        .word_size   = DEFAULT_WORD_SIZE
    },
    // D1TCM
    {
        .reg_address = SDM_EXT_CFG_EMCPU_TCM_DTCM_ADDRESS + (SDM_EXT_CFG_EMCPU_TCM_DTCM_SIZE >> 1) + DEFAULT_WORD_SIZE,
        .reg_rw_type = READ_ONLY,
        .word_size   = DEFAULT_WORD_SIZE
    },
};
// clang-format on

#define SDM_INTERRUPT_HANDLER_REGISTERS_COUNT (sizeof(accel_interrupt_handler_registers) / sizeof(reg_data_t))

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/**
 * @brief Get index of register from accel_interrupt_handler_registers
 *
 * \b Description:
 *
 * @param[in]
 * address : Address of the register to search for
 *
 * @retval
 *  On success, 0x0
 *  On failure, -1
 * */
static int cli_accel_int_get_register_idx(uint32_t address)
{
    for (int i = 0; i < (int)SDM_INTERRUPT_HANDLER_REGISTERS_COUNT; i++)
    {
        uint32_t start_address = accel_interrupt_handler_registers[i].reg_address;
        // We are reading and writing in 32 bit size so maximum address available is start + word_size - DEFAULT_WORD_SIZE
        uint32_t end_address = start_address + accel_interrupt_handler_registers[i].word_size - DEFAULT_WORD_SIZE;

        if (address >= start_address && address <= end_address)
        {
            return i;
        }
    }

    return -1;
}

/**
 * @brief Reads / write given register address
 *
 * \b Description:
 *
 * @param[in]
 * reg_addr : Address of the register to read / write
 * reg_value : Incase of READ this is where the value is stored.
 *             Incase of write this value is written to register.
 * operation : 0x0 : Read operation
 *             0x1 : Write operation
 *
 * @retval
 *  On success, 0x0
 *  On failure, -1, error is set in errno
 * */
static int cli_accel_int_read_write_reg(uint32_t reg_offset, uint32_t* reg_value, uint8_t operation)
{
    int index = cli_accel_int_get_register_idx(reg_offset);

    uint32_t reg_addr = atu_svc_accel_atu_addr(ACCEL_ID_SDM) + reg_offset;

    if (index < 0)
    {
        // This address is not processed by interrupt handler
        errno = EINVAL;
        return -1;
    }

    switch (operation)
    {
    case REG_WRITE:
        if (accel_interrupt_handler_registers[index].reg_rw_type != READ_WRITE)
        {
            errno = EPERM;
            return -1;
        }
        else
        {
            MMIO_WRITE32(reg_addr, *reg_value);
        }
        break;
    case REG_SET:
        switch (accel_interrupt_handler_registers[index].reg_rw_type)
        {
        // In case of write we need to read current register and set only bit set in reg_value.
        case READ_WRITE: {
            uint32_t cur_reg_value = MMIO_READ32(reg_addr);
            MMIO_WRITE32(reg_addr, cur_reg_value | *reg_value);
        }
        break;
        // Incase of READ_WRITE_ONE_TO_SET we can directly pass the value since hardware will take care of this
        case READ_WRITE_ONE_TO_SET:
            MMIO_WRITE32(reg_addr, *reg_value);
            break;
        default:
            errno = EPERM;
            return -1;
        }
        break;
    case REG_CLR:
        switch (accel_interrupt_handler_registers[index].reg_rw_type)
        {
        // In case of write we need to read current register and clear only bit set in reg_value.
        case READ_WRITE: {
            uint32_t cur_reg_value = MMIO_READ32(reg_addr);
            MMIO_WRITE32(reg_addr, cur_reg_value & (~(*reg_value)));
        }
        break;
        // Incase of READ_WRITE_ONE_TO_CLEAR we can directly pass the value since hardware will take care of this
        case READ_WRITE_ONE_TO_CLEAR:
            MMIO_WRITE32(reg_addr, *reg_value);
            break;
        default:
            errno = EPERM;
            return -1;
        }
        break;
    }

    // Read final value of the register
    *reg_value = MMIO_READ32(reg_addr);

    return 0;
}

/**
 * @brief Extract Address and Value from arguments and perform corresponding function
 *
 * \b Description:
 *
 * @param[in]
 * argc : Number of arguments passed including "reg_rd"
 * argv : Array of strings containing the passed arguments
 * oper : Operation to perform (Write / Clear / Set)
 * oper_name : Operation Name in string
 *
 * @retval
 *  void
 * */
static void cli_accel_int_perform_reg_update(int argc, const char** argv, uint8_t oper)
{
    // Now loop through all addresses and read value
    for (int i = 1; i < argc; i += 2)
    {
        errno = 0;
        char* end = 0x0;
        uint32_t reg_addr = (uint32_t)(strtol(argv[i], &end, 0));

        // Check errno
        if (errno != 0 || reg_addr == 0x0)
        {
            FPFW_ET_LOG(REG_OPER_INVALID_ADDR, errno);
            continue;
        }

        uint32_t reg_value = (uint32_t)(strtol(argv[i + 1], &end, 0));

        // Check errno
        if (errno != 0 || reg_value == 0x0)
        {
            FPFW_ET_LOG(REG_OPER_INVALID_VALUE, errno);
            continue;
        }

        if (cli_accel_int_read_write_reg(reg_addr, &reg_value, oper) == 0x0)
        {
            FPFW_ET_LOG(REG_OPER_SUCCESS, reg_addr, reg_value);
        }
        else
        {
            FPFW_ET_LOG(REG_OPER_FAILED, oper, reg_addr, errno);
        }
    }
}

/**
 * @brief Routine to implement CLI reg_rd
 *
 * \b Description:
 * This will traverse through all given addresses and print their value one after the other
 *
 * @param[in]
 * argc : Number of arguments passed including "reg_rd"
 * argv : Array of strings containing the passed arguments
 *
 * @retval
 *  On success, CLI_SUCCESS
 *  On failure, CLI_ERROR
 * */
static FPFW_CLI_STATUS cli_accel_int_reg_rd(int argc, const char** argv)
{
    if (argc <= 1)
    {
        // Print Usage
        FpFwCliDisplayUsage(cli_accel_int_reg_rd);
        return CLI_ERROR;
    }

    // Now loop through all addresses and read value
    for (int i = 1; i < argc; i++)
    {
        errno = 0;
        char* end = 0x0;
        uint32_t reg_addr = (uint32_t)(strtol(argv[i], &end, 0));

        // Check errno
        if (errno != 0 || reg_addr == 0x0)
        {
            FPFW_ET_LOG(REG_OPER_INVALID_ADDR, errno);
            continue;
        }

        uint32_t reg_value = 0x0;

        if (cli_accel_int_read_write_reg(reg_addr, &reg_value, REG_READ) == 0x0)
        {
            FPFW_ET_LOG(REG_OPER_SUCCESS, reg_addr, reg_value);
        }
        else
        {
            FPFW_ET_LOG(REG_OPER_FAILED, REG_READ, reg_addr, errno);
        }
    }

    return CLI_SUCCESS;
}

/**
 * @brief Routine to implement CLI reg_wr
 *
 * \b Description:
 * This will traverse through all given addresses and set them to the values passed
 *
 * @param[in]
 * argc : Number of arguments passed including "reg_wr"
 * argv : Array of strings containing the passed arguments
 *
 * @retval
 *  On success, CLI_SUCCESS
 *  On failure, CLI_ERROR
 * */
static FPFW_CLI_STATUS cli_accel_int_reg_wr(int argc, const char** argv)
{
    if (argc <= 2)
    {
        // Print Usage
        FpFwCliDisplayUsage(cli_accel_int_reg_wr);
        return CLI_ERROR;
    }

    cli_accel_int_perform_reg_update(argc, argv, REG_WRITE);

    return CLI_SUCCESS;
}

/**
 * @brief Routine to implement CLI reg_set
 *
 * \b Description:
 * This will traverse through all given addresses and set their bits according to the value passed
 *
 * @param[in]
 * argc : Number of arguments passed including "reg_set"
 * argv : Array of strings containing the passed arguments
 *
 * @retval
 *  On success, CLI_SUCCESS
 *  On failure, CLI_ERROR
 * */
static FPFW_CLI_STATUS cli_accel_int_reg_set(int argc, const char** argv)
{
    if (argc <= 2)
    {
        // Print Usage
        FpFwCliDisplayUsage(cli_accel_int_reg_set);
        return CLI_ERROR;
    }

    cli_accel_int_perform_reg_update(argc, argv, REG_SET);

    return CLI_SUCCESS;
}

/**
 * @brief Routine to implement CLI reg_clr
 *
 * \b Description:
 * This will traverse through all given addresses and clear their bits according to the value passed
 *
 * @param[in]
 * argc : Number of arguments passed including "reg_clr"
 * argv : Array of strings containing the passed arguments
 *
 * @retval
 *  On success, CLI_SUCCESS
 *  On failure, CLI_ERROR
 * */
static FPFW_CLI_STATUS cli_accel_int_reg_clr(int argc, const char** argv)
{
    if (argc <= 2)
    {
        // Print Usage
        FpFwCliDisplayUsage(cli_accel_int_reg_clr);
        return CLI_ERROR;
    }

    cli_accel_int_perform_reg_update(argc, argv, REG_CLR);

    return CLI_SUCCESS;
}

/*----------------------------- Global Functions ----------------------------*/
FPFW_CLI_STATUS cli_accel_int_init(void)
{
    FpFwCliRegisterTable(s_accel_int_commands_table, FPFW_ARRAY_SIZE(s_accel_int_commands_table));

    return CLI_SUCCESS;
}