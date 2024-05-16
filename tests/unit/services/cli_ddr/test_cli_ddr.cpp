//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_cli_ddr.cpp
 * Tests the init of cli ddr component
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_non_null, CmockaWrapperTest, TEST_...

extern "C" {
#include <FpFwCli.h> // for _FPFW_CLI_COMMAND, FPFW_CLI_STATUS, _FPFW...
#include <stddef.h>  // for NULL, size_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
extern FPFW_CLI_STATUS ecc_ce_error_injection(int Argc, const char** Argv);
extern FPFW_CLI_STATUS ecc_ue_error_injection(int Argc, const char** Argv);
extern FPFW_CLI_STATUS cmd_addr_parity_error_injection(int Argc, const char** Argv);
extern void cli_ddr_init(void);

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

//
// Mocks
//
void __wrap_FpFwCliRegisterTable(PFPFW_CLI_COMMAND pTable, size_t TableLength)
{
    size_t loop = 0;
    assert_non_null(pTable);
    assert_true(TableLength > 0);

    while (loop < TableLength)
    {
        assert_non_null(&pTable[loop]);
        assert_non_null(pTable[loop].MenuName);
        assert_non_null(pTable[loop].Syntax);
        assert_non_null(pTable[loop].Routine);
        assert_non_null(pTable[loop].Description);
        assert_non_null(pTable[loop].Usage);

        loop++;
    }
}

//
// Tests
//
TEST_FUNCTION(test_cli_ddr_init, nullptr, nullptr)
{
    //! Call API under test
    cli_ddr_init();
}

TEST_FUNCTION(test_ecc_ce_error_injection, nullptr, nullptr)
{
    //! Call API under test
    // TODO Silibs to provide APIs with arguments list
    // https://dev.azure.com/ms-tsd/Base_IP/_workitems/edit/584974
    assert_int_equal(ecc_ce_error_injection(0, NULL), CLI_SUCCESS);
}

TEST_FUNCTION(test_ecc_ue_error_injection, nullptr, nullptr)
{
    //! Call API under test
    // TODO Silibs to provide APIs with arguments list
    // https://dev.azure.com/ms-tsd/Base_IP/_workitems/edit/584974
    assert_int_equal(ecc_ue_error_injection(0, NULL), CLI_SUCCESS);
}

TEST_FUNCTION(test_cmd_addr_parity_error_injection, nullptr, nullptr)
{
    //! Call API under test
    // TODO Silibs to provide APIs with arguments list
    // https://dev.azure.com/ms-tsd/Base_IP/_workitems/edit/584974
    assert_int_equal(cmd_addr_parity_error_injection(0, NULL), CLI_SUCCESS);
}
}
