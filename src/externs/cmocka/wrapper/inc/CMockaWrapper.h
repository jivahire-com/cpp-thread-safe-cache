//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file CMockaWrapper.h
 * This file serves as wrapper to tests
 */

#pragma once

/*----------- Nested includes ------------*/

#include <setjmp.h> // IWYU pragma: keep
#include <stdarg.h> // IWYU pragma: keep
#include <vector>    // for vector
// IWYU pragma: no_include "cmocka.h"

extern "C" 
{
#include <cmocka.h> // IWYU pragma: export
}

/*-- Symbolic Constant Macros (defines) --*/

#define TEST_FUNCTION(test_func, setup_func, teardown_func) \
    void test_func(void**); \
    static CmockaWrapperTest _##test_func(#test_func, test_func, setup_func, teardown_func); \
    void test_func(void**)

#define expect_error(statement, expected_error) \
    {                                           \
        bool ___expectError = false;            \
        try                                     \
        {                                       \
            statement;                          \
        }                                       \
        catch (const expected_error& e)         \
        {                                       \
            ___expectError = true;              \
        }                                       \
        assert_true(___expectError);            \
    }

/*-------------- Typedefs ----------------*/

class CmockaWrapperTestManager
{
public:
    std::vector<CMUnitTest> s_tests;
    void RegisterTest(CMUnitTest);
};

class CmockaWrapperTest
{
public:
    static CmockaWrapperTestManager Manager;
    CmockaWrapperTest(const char* name, CMUnitTestFunction test_func, CMFixtureFunction setup_func, CMFixtureFunction teardown_func);
};

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

int main(int argc, char** argv);
