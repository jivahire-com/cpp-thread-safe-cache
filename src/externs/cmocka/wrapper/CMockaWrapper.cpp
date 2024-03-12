//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file Example.c
 * This file serves as an example for our repo
 */

/*------------- Includes -----------------*/

#include "inc/CMockaWrapper.h"

#include <cstdlib> // for _putenv_s

/*-- Symbolic Constant Macros (defines) --*/

#ifdef _WIN32
    #define SET_ENV(key, value) _putenv_s(key, value)
#else
    #define SET_ENV(key, value) setenv(key, value, 1)
#endif

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

CmockaWrapperTestManager __attribute__((init_priority(101))) CmockaWrapperTest::Manager;

/*------------- Functions ----------------*/

void CmockaWrapperTestManager::RegisterTest(CMUnitTest test)
{
    s_tests.push_back(test);
}

CmockaWrapperTest::CmockaWrapperTest(const char* name, CMUnitTestFunction test_func, CMFixtureFunction setup_func, CMFixtureFunction teardown_func)
{
    CMUnitTest test = {name, test_func, setup_func, teardown_func, nullptr};
    Manager.RegisterTest(test);
}

int main(int argc, char** argv)
{
    // Write to file if specified, for test frameworks
    if (2 == argc)
    {
        SET_ENV("CMOCKA_XML_FILE", argv[1]);
        cmocka_set_message_output(CM_OUTPUT_XML);
    }

    return _cmocka_run_group_tests("TestSuite",
                                   CmockaWrapperTest::Manager.s_tests.data(),
                                   CmockaWrapperTest::Manager.s_tests.size(),
                                   nullptr,
                                   nullptr);

    return -1;
}
