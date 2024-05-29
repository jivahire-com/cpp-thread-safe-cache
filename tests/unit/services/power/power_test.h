/*
 * Copyright (c) Microsoft Corporation.
 */

/**
 * @file power_test.h
 *
 */

#pragma once

/*----------- Includes ------------*/
#include <CMockaWrapper.h>

/*-- Symbolic Constant Macros (defines) --*/
#define POWER_TEST(fn, setup, teardown) \
    TEST_FUNCTION(power_svc__##fn, setup, teardown)

#define UNUSED(a) ((void)a)

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
