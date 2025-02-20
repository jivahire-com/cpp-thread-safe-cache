//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file mesh_cli.c
 * This file contains the implementation of the Mesh CLI interface
 * and related functionality.
 */

/*------------- Includes -----------------*/
#include <DfwkClient.h> // for DfwkAsyncRequestInititalize, PDFWK_INTER...
#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <FpFwCli.h>
#include <FpFwLinkedList.h>
#include <FpFwUtils.h>
#include <cmn800.h>
#include <cmn800_error_handler.h>
#include <fpfw_icc_base.h>     // for fpfw_icc_base_ctx_t
#include <idhw.h>              // for idhw_is_single_die_boot_en
#include <kng_soc_constants.h> // for NUM_DIE
#include <mesh.h>
#include <mesh_cli.h>
#include <mesh_error_handler.h>
#include <stdio.h>
#include <stdlib.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
static FPFW_CLI_STATUS mesh_echo_cli(int argc, const char** argv);

static FPFW_CLI_STATUS mesh_isr_cli(int argc, const char** argv);

static FPFW_CLI_STATUS mesh_error_inj(int argc, const char** argv);

static FPFW_CLI_STATUS mesh_pseudo_error_inj(int argc, const char** argv);

static FPFW_CLI_STATUS mesh_pseudo_error_inj_test_suite(int argc, const char** argv);

static FPFW_CLI_STATUS d2d_pseudo_error_inj(int argc, const char** argv);

/*-- Declarations (Statics and globals) --*/

static FPFW_CLI_COMMAND mesh_cli_list[] = {
    {NULL_LIST_ENTRY, "mesh", "mesh_echo", mesh_echo_cli, "mesh echo data", "Usage: mesh_echo <32-bit address(in Hex)> <32-bit data(in Hex)>"},
    {NULL_LIST_ENTRY, "mesh", "mesh_isr", mesh_isr_cli, "mesh isr process", "Usage: mesh_isr <Error(0x0) or Fault(0x1)>"},
    {NULL_LIST_ENTRY, "mesh", "mesh_error_inj", mesh_error_inj, "mesh error injection", "Usage: mesh_error_inj <node_type> <node_id> <node_control_reg> <err_inj> <byte_par_err_inj>"},
    {NULL_LIST_ENTRY, "mesh", "mesh_pseudo_error_inj", mesh_pseudo_error_inj, "mesh pseudo fault injection", "Usage: mesh_pseudo_error_inj <secure/non_secure> <node_type> <node_id> <node_control_reg> <err_inj> <err_cnt_down>"},
    {NULL_LIST_ENTRY, "mesh", "mesh_pseudo_error_test_suite", mesh_pseudo_error_inj_test_suite, "mesh pseudo fault injection test suite", "Usage: mesh_pseudo_error_test_suite <secure/non_secure> <node_type> <node_id_start> <node_id_end> <node_control_reg> <err_inj> <err_cnt_down>"},
    {NULL_LIST_ENTRY, "mesh", "d2d_pseudo_error_inj", d2d_pseudo_error_inj, "d2d pseudo fault injection", "Usage: d2d_pseudo_error_inj <node_id> <err_inj> <err_cnt_down>"},

};

/*------------- Functions ----------------*/

static FPFW_CLI_STATUS mesh_echo_cli(int argc, const char** argv)
{
    FpFwCliPrint("mesh_echo_cli func. call\n");

    if (argc == 3)
    {
        char* endptr;
        unsigned long addr32 = strtoul(argv[1], &endptr, 16);
        if (*endptr != '\0')
        {
            FpFwCliPrint("1st arg %s is Invalid Hex value\n", argv[1]);
            return CLI_ERROR;
        }
        unsigned long data32 = strtoul(argv[2], &endptr, 16);
        if (*endptr != '\0')
        {
            FpFwCliPrint("2nd arg %s is Invalid Hex value\n", argv[2]);
            return CLI_ERROR;
        }

        FpFwCliPrint("mesh echo\n");
        FpFwCliPrint("addr32: 0x%x, data32: 0x%x\n", addr32, data32);
    }
    else
    {
        FpFwCliPrint("Mesh Echo CLI Help\n");
        FpFwCliPrint("Cmds: 2, <32-bit address(in Hex)> <32-bit data(in Hex)\n");
        return CLI_ERROR;
    }
    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS mesh_isr_cli(int argc, const char** argv)
{
    FpFwCliPrint("mesh_isr func. call\n\n");

    if (argc == 2)
    {
        int isr_type = strtoul(argv[1], NULL, 16);
        FpFwCliPrint("mesh isr process\n");
        if (isr_type == MESH_CLI_ERROR_ISR_TYPE)
        {
            mesh_error_isr(NULL);
        }
        else if (isr_type == MESH_CLI_FAULT_ISR_TYPE)
        {
            mesh_fault_isr(NULL);
        }
        else
        {
            FpFwCliPrint("Invalid ISR type\n");
            goto exit_error;
        }
    }
    else
    {
        goto exit_error;
    }
    return CLI_SUCCESS;

exit_error:
    FpFwCliPrint("Mesh ISR CLI Help\n");
    FpFwCliPrint("Cmds: 1, <Error(0x0) or Fault(0x1)>\n");
    return CLI_ERROR;
}

static FPFW_CLI_STATUS mesh_error_inj(int argc, const char** argv)
{
    FpFwCliPrint("mesh_error_inj func. call\n\n");

    uint8_t current_arg = 0x0;
    if (argc == 6)
    {
        char* endptr;
        uint8_t node_type = strtoul(argv[++current_arg], &endptr, 16);
        if (*endptr != '\0')
        {
            goto exit_error1;
        }
        uint8_t node_id = strtoul(argv[++current_arg], &endptr, 16);
        if (*endptr != '\0')
        {
            goto exit_error1;
        }
        uint16_t node_control_reg = strtoul(argv[++current_arg], &endptr, 16);
        if (*endptr != '\0')
        {
            goto exit_error1;
        }
        uint32_t err_inj = strtoul(argv[++current_arg], &endptr, 16);
        if (*endptr != '\0')
        {
            goto exit_error1;
        }
        uint8_t byte_par_err_inj = strtoul(argv[++current_arg], &endptr, 16);
        if (*endptr != '\0')
        {
            goto exit_error1;
        }

        FpFwCliPrint("cmn800_error_injection Start\n");
        FpFwCliPrint("node_type: 0x%x, node_id: 0x%x, node_control_reg: 0x%x, err_inj: 0x%x, "
                     "byte_par_err_inj: 0x%x, die_num: %d\n",
                     node_type,
                     node_id,
                     node_control_reg,
                     err_inj,
                     byte_par_err_inj,
                     (uint8_t)idhw_get_die_id());
        cmn800_error_injection(node_type, node_id, node_control_reg, err_inj, byte_par_err_inj, (uint8_t)idhw_get_die_id());
        FpFwCliPrint("cmn800_error_injection End\n");
    }
    else
    {
        goto exit_error;
    }
    return CLI_SUCCESS;

exit_error1:
    FpFwCliPrint("Arg %s is Invalid Hex value\n", argv[current_arg]);
exit_error:
    FpFwCliPrint("Mesh Error Injection CLI Help\n");
    FpFwCliPrint("Cmds: 5, <node_type> <node_id> <node_control_reg> <err_inj> <byte_par_err_inj>\n");
    return CLI_ERROR;
}

static FPFW_CLI_STATUS mesh_pseudo_error_inj(int argc, const char** argv)
{
    FpFwCliPrint("mesh_pseudo_error_inj func. call\n\n");

    uint8_t current_arg = 0x0;
    if (argc == 7)
    {
        char* endptr;
        uint8_t non_secure = strtoul(argv[++current_arg], &endptr, 16);
        if (*endptr != '\0')
        {
            goto exit_error1;
        }
        uint8_t node_type = strtoul(argv[++current_arg], &endptr, 16);
        if (*endptr != '\0')
        {
            goto exit_error1;
        }
        uint8_t node_id = strtoul(argv[++current_arg], &endptr, 16);
        if (*endptr != '\0')
        {
            goto exit_error1;
        }
        uint16_t node_control_reg = strtoul(argv[++current_arg], &endptr, 16);
        if (*endptr != '\0')
        {
            goto exit_error1;
        }
        uint32_t err_inj = strtoul(argv[++current_arg], &endptr, 16);
        if (*endptr != '\0')
        {
            goto exit_error1;
        }
        uint32_t err_cnt_down = strtoul(argv[++current_arg], &endptr, 16);
        if (*endptr != '\0')
        {
            goto exit_error1;
        }

        FpFwCliPrint("cmn800_pseudo_fault_error_injection Start\n");
        FpFwCliPrint("%s node_type: 0x%x, node_id: 0x%x, node_control_reg: 0x%x, err_inj: 0x%x, "
                     "err_cnt_down: 0x%x, die_num: %d\n",
                     (non_secure == 0x0) ? "Secure" : "Non-Secure",
                     node_type,
                     node_id,
                     node_control_reg,
                     err_inj,
                     err_cnt_down,
                     (uint8_t)idhw_get_die_id());
        cmn800_pseudo_fault_error_injection(node_type,
                                            node_id,
                                            node_control_reg,
                                            err_inj,
                                            err_cnt_down,
                                            (uint8_t)idhw_get_die_id(),
                                            (bool)non_secure);
        FpFwCliPrint("cmn800_pseudo_fault_error_injection End\n");
    }
    else
    {
        goto exit_error;
    }
    return CLI_SUCCESS;

exit_error1:
    FpFwCliPrint("Arg %s is Invalid Hex value\n", argv[current_arg]);
exit_error:
    FpFwCliPrint("Mesh Pseudo Fault Error Injection CLI Help\n");
    FpFwCliPrint(
        "Cmds: 6, <secure/non_secure> <node_type> <node_id> <node_control_reg> <err_inj> <err_cnt_down>\n");
    FpFwCliPrint("HNS Ex: mesh_pseudo_error_inj 0x0 0x1 0xC 0x401 0x80000A20 0x1000\n");
    FpFwCliPrint("HNI Ex: mesh_pseudo_error_inj 0x1 0x3 0x0 0x401 0x80000A20 0x1000\n");
    return CLI_ERROR;
}

static FPFW_CLI_STATUS mesh_pseudo_error_inj_test_suite(int argc, const char** argv)
{
    FpFwCliPrint("mesh_pseudo_error_inj_test_suite func. call\n\n");

    uint8_t current_arg = 0x0;
    if (argc == 8)
    {
        char* endptr;
        uint8_t non_secure = strtoul(argv[++current_arg], &endptr, 16);
        if (*endptr != '\0')
        {
            goto exit_error1;
        }
        uint8_t node_type = strtoul(argv[++current_arg], &endptr, 16);
        if (*endptr != '\0')
        {
            goto exit_error1;
        }
        uint8_t node_id_start = strtoul(argv[++current_arg], &endptr, 16);
        if (*endptr != '\0')
        {
            goto exit_error1;
        }
        uint8_t node_id_end = strtoul(argv[++current_arg], &endptr, 16);
        if (*endptr != '\0')
        {
            goto exit_error1;
        }
        uint16_t node_control_reg = strtoul(argv[++current_arg], &endptr, 16);
        if (*endptr != '\0')
        {
            goto exit_error1;
        }
        uint32_t err_inj = strtoul(argv[++current_arg], &endptr, 16);
        if (*endptr != '\0')
        {
            goto exit_error1;
        }
        uint32_t err_cnt_down = strtoul(argv[++current_arg], &endptr, 16);
        if (*endptr != '\0')
        {
            goto exit_error1;
        }

        FpFwCliPrint("cmn800_pseudo_fault_error_injection Start\n");
        FpFwCliPrint("%s node_type: 0x%x, node_id_start: 0x%x, node_id_end: 0x%x, node_control_reg: 0x%x, "
                     "err_inj: 0x%x, "
                     "err_cnt_down: 0x%x, die_num: %d\n",
                     (non_secure == 0x0) ? "Secure" : "Non-Secure",
                     node_type,
                     node_id_start,
                     node_id_end,
                     node_control_reg,
                     err_inj,
                     err_cnt_down,
                     (uint8_t)idhw_get_die_id());

        for (uint8_t node_id = node_id_start; node_id < node_id_end; node_id++)
        {
            /* code */
            cmn800_pseudo_fault_error_injection(node_type,
                                                node_id,
                                                node_control_reg,
                                                err_inj,
                                                err_cnt_down,
                                                (uint8_t)idhw_get_die_id(),
                                                (bool)non_secure);
            tx_thread_sleep(3);
        }

        FpFwCliPrint("cmn800_pseudo_fault_error_injection End\n");
    }
    else
    {
        goto exit_error;
    }
    return CLI_SUCCESS;

exit_error1:
    FpFwCliPrint("Arg %s is Invalid Hex value\n", argv[current_arg]);
exit_error:
    FpFwCliPrint("Mesh Pseudo Fault Error Injection CLI Help\n");
    FpFwCliPrint("Cmds: 7,  <secure/non_secure> <node_type> <node_id_start> <node_id_end> <node_control_reg> "
                 "<err_inj> <err_cnt_down>\n");
    FpFwCliPrint("HNI Ex: mesh_pseudo_error_test_suite 0x0 0x3 0x0 0x10 0x401 0x80000A20 0x1000\n");
    FpFwCliPrint("MXP Ex: mesh_pseudo_error_test_suite 0x1 0x2 0x0 0x40 0x401 0x80000A20 0x1000\n");
    return CLI_ERROR;
}

static FPFW_CLI_STATUS d2d_pseudo_error_inj(int argc, const char** argv)
{
    FpFwCliPrint("d2d_pseudo_error_inj func. call\n\n");

    uint8_t current_arg = 0x0;
    if (argc == 4)
    {
        char* endptr;
        uint8_t node_id = strtoul(argv[++current_arg], &endptr, 16);
        if (*endptr != '\0')
        {
            goto exit_error1;
        }
        uint32_t err_inj = strtoul(argv[++current_arg], &endptr, 16);
        if (*endptr != '\0')
        {
            goto exit_error1;
        }
        uint32_t err_cnt_down = strtoul(argv[++current_arg], &endptr, 16);
        if (*endptr != '\0')
        {
            goto exit_error1;
        }

        FpFwCliPrint("d2d_pseudo_error_injection Start\n");
        FpFwCliPrint("node_id: 0x%x, err_inj: 0x%x, "
                     "err_cnt_down: 0x%x, die_num: %d\n",
                     node_id,
                     err_inj,
                     err_cnt_down,
                     (uint8_t)idhw_get_die_id());

        d2d_ras_error_inj(node_id, err_inj, err_cnt_down);

        FpFwCliPrint("d2d_pseudo_error_injection End\n");
    }
    else
    {
        goto exit_error;
    }
    return CLI_SUCCESS;

exit_error1:
    FpFwCliPrint("Arg %s is Invalid Hex value\n", argv[current_arg]);
exit_error:
    FpFwCliPrint("Mesh Pseudo Fault Error Injection CLI Help\n");
    FpFwCliPrint("Cmds: 3, <node_id> <err_inj> <err_cnt_down>\n");
    FpFwCliPrint("Ex: d2d_pseudo_error_inj 0x0 0x102000 0xF0000\n");
    return CLI_ERROR;
}

void mesh_cli_initialize(void)
{
    FpFwCliRegisterTable(mesh_cli_list, FPFW_ARRAY_SIZE(mesh_cli_list));

    FpFwCliPrint("Mesh CLI init complete\n");
}
