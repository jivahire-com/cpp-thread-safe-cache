/*
 * Copyright (c) Microsoft Corporation.
 */

/**
 * @file ap_core_test.h
 *
 */

#pragma once

/*----------- Includes ------------*/
#include <CMockaWrapper.h>

extern "C" {
#include <ap_core.h>
#include <ap_core_i.h>
}

/*-- Symbolic Constant Macros (defines) --*/
#define AP_CORE_TEST(fn, setup, teardown) \
    TEST_FUNCTION(ap_core_svc__##fn, setup, teardown)

#define UNUSED(a) ((void)a)

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
extern bool should_mock_ap_core_ppu_init;

/*--------- Function Prototypes ----------*/
extern "C" {
void __real_ap_core_ppu_clusters_on(ap_core_service_context_t* p_context, uint32_t timeout_ms);
void __real_ap_core_ppu_clusters_on_off(ap_core_service_context_t* p_context, uint32_t timeout_ms);
}
