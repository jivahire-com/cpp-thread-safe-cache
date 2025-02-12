//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_ddr.c
 * Source file to implement ddr cli
 */

/*------------- Includes -----------------*/
#include <FpFwCli.h>        // for FPFW_CLI_STATUS, CLI_SUCCESS, FpFwCliReg...
#include <FpFwLinkedList.h> // for NULL_LIST_ENTRY
#include <FpFwUtils.h>      // for FPFW_UNUSED, FPFW_ARRAY_SIZE
#include <ddr_err_inj.h>
#include <ddr_memory_map.h>
#include <ddrss_lib.h>
#include <stdio.h> // for printf
#include <stdlib.h>

#ifdef UNIT_TEST
    #define STATIC
#else
    #define STATIC static
#endif

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/* Added 3 placeholder commands to set the structure - Functions to be plugged when Silibs apis are available */
STATIC FPFW_CLI_STATUS ecc_ce_error_injection(int Argc, const char** Argv);
STATIC FPFW_CLI_STATUS ecc_ue_error_injection(int Argc, const char** Argv);
STATIC FPFW_CLI_STATUS cmd_addr_parity_error_injection(int Argc, const char** Argv);
STATIC FPFW_CLI_STATUS wrrtydat_ue_error_injection(int Argc, const char** Argv);

/*-- Declarations (Statics and globals) --*/

// TODO Silibs to provide APIs with arguments list
// https://dev.azure.com/ms-tsd/Base_IP/_workitems/edit/584974
STATIC FPFW_CLI_COMMAND cli_ddr_commands[] = {
    {NULL_LIST_ENTRY, "ddr_err_inj", "ecc_ce_err", ecc_ce_error_injection, "ECC CE error injection", "Usage: ecc_ce_err <parameter list undefined>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "ecc_ue_err", ecc_ue_error_injection, "ECC UE error injection", "Usage: ecc_ue_err <parameter list undefined>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "capar_err", cmd_addr_parity_error_injection, "CAPAR (Cmd/Addr Parity) error", "Usage: caddr_parity_err <parameter list undefined>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "wrrtydat_ue_err", wrrtydat_ue_error_injection, "WRRTYDAT UE error injection", "Usage: wrrtydat_ue_err <parameter list undefined>"},
};

// ecc_ce_err (mc) (phy_add) {error bit}
STATIC FPFW_CLI_STATUS ecc_ce_error_injection(int Argc, const char** Argv)
{
    uint32_t die = 0;
    uint32_t mc = 0;
    uint64_t p_addr = 0x20080000000ULL;
    uint8_t bit = 0;
    uint16_t BIT_Value;
    bool check_null = true;

    if (Argc > 4)
    {
        FpFwCliPrint("Invalid Argument! Please provide necessary arguments!");
        return CLI_ERROR;
    }

    if (Argc > 1) // only mc is passed as argument
    {
        mc = (uint32_t)(strtoul(Argv[1], 0, 0));
    }

    if (Argc > 2) // only mc & phy_addr is passed as argument
    {
        p_addr = (uint64_t)(strtoull(Argv[2], 0, 0));
    }

    if (Argc > 3) // Mc, phy_addr & one bit is passed as argument.
    {
        bit = (uint8_t)(strtoul(Argv[3], 0, 0));
    }

    if (bit >= 10)
    {
        check_null = false;
        BIT_Value = 1;
    }
    else
    {
        BIT_Value = (1 << bit);
    }

    if (!ddrss_ue_ce_err_inj_validation(mc, BIT_Value))
    {
        check_null = false;
    }

    if (check_null == false)
    {
        FpFwCliPrint("Invalid Argument! Please provide necessary arguments!");
        return CLI_ERROR;
    }

    die = 0;
    ddrss_ue_ce_error_injection(die, mc, p_addr, BIT_Value);
    FpFwCliPrint("DDR: ecc_ce_error_injection - Done!!\n");

    return CLI_SUCCESS;
}

STATIC FPFW_CLI_STATUS ecc_ue_error_injection(int Argc, const char** Argv)
{
    int32_t die = 0;
    uint32_t mc = 0;
    uint64_t p_addr = 0;
    uint16_t BIT_Value = 0;
    bool check_null = false;

    if (Argc == 1) // no argument passed
    {
        mc = 0;
        p_addr = 0;
        BIT_Value = BIT0 | BIT1;
        check_null |= true;
    }
    else if (Argc == 2) // only mc is passed as argument
    {
        if (Argv[1] != NULL)
        {
            mc = (uint32_t)(strtoul(Argv[1], 0, 0));
            check_null |= true;
        }
        p_addr = 0;
        BIT_Value = BIT0 | BIT1;
    }
    else if (Argc == 3) // only mc & phy_addr is passed as argument
    {
        if ((Argv[1] != NULL) && (Argv[2] != NULL))
        {
            mc = (uint32_t)(strtoul(Argv[1], 0, 0));
            p_addr = (uint64_t)(strtoul(Argv[2], 0, 0));
            check_null |= true;
        }
        BIT_Value = BIT0 | BIT1;
    }
    else if (Argc >= 5) // Mc, phy_addr & atleast two bit is passed as argument.
    {
        int i;
        int bit;
        if ((Argv[1] != NULL) && (Argv[2] != NULL))
        {
            mc = (uint32_t)(strtoul(Argv[1], 0, 0));
            p_addr = (uint64_t)(strtoul(Argv[2], 0, 0));
            check_null |= true;
        }
        for (i = 3; i < Argc; i++)
        {
            if (Argv[i] != NULL)
            {
                bit = (uint16_t)(strtoul(Argv[i], 0, 0));
                if (bit >= 0 && bit < 10)
                {
                    BIT_Value |= 1 << bit;
                    check_null |= true;
                }
                else
                {
                    check_null &= false;
                    break;
                }
            }
            else
            {
                check_null &= false;
                break;
            }
        }
    }
    else
    {
        FpFwCliPrint("Invalid Argument! Please provide necessary arguments!");
        return CLI_ERROR;
    }

    if ((check_null == false) || (!ddrss_ue_ce_err_inj_validation(mc, BIT_Value)))
    {
        FpFwCliPrint("Invalid Argument! Please provide necessary arguments!");
        return CLI_ERROR;
    }

    ddrss_ue_ce_error_injection(die, mc, p_addr, BIT_Value);
    FpFwCliPrint("DDR: ecc_ue_error_injection - Done!!\n");
    return CLI_SUCCESS;
}

STATIC FPFW_CLI_STATUS cmd_addr_parity_error_injection(int Argc, const char** Argv)
{
    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    // TODO Silibs to provide APIs with exact arguments list
    // https://dev.azure.com/ms-tsd/Base_IP/_workitems/edit/584974
    FpFwCliPrint("Work in progress: cmd_addr_parity_error_injection!!\n");

    return CLI_SUCCESS;
}

STATIC FPFW_CLI_STATUS wrrtydat_ue_error_injection(int Argc, const char** Argv)
{
    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    // TODO Silibs to provide APIs with exact arguments list
    // https://dev.azure.com/ms-tsd/Base_IP/_workitems/edit/584974
    FpFwCliPrint("Work in progress: wrrtydat_ue_error_injection!!\n");

    return CLI_SUCCESS;
}

void cli_ddr_init(void)
{
    FpFwCliRegisterTable(&cli_ddr_commands[0], FPFW_ARRAY_SIZE(cli_ddr_commands));
}
