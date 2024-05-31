/*
 * Copyright (c) Microsoft Corporation.
 */

/**
 * @file sos_test.h
 *
 */

#pragma once

/*----------- Includes ------------*/
#include <CMockaWrapper.h>

/*-- Symbolic Constant Macros (defines) --*/
#define SOS_TEST(fn, setup, teardown) \
    TEST_FUNCTION(sos_svc__##fn, setup, teardown)

#define UNUSED(a) ((void)a)

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
