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

#define TEST_COMPLETION_ROUTINE ((DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE)0x123456)
#define TEST_COMPLETION_CONTEXT ((void*)0x65430)

/*--------- Function Prototypes ----------*/
