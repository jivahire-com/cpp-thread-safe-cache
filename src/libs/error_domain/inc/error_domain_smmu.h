//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file error_domain_smmu.h
 * Header file of error domain AP smmu
 */
#pragma once

/*----------- Nested includes ------------*/
/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

#if defined (SCP_RUNTIME_INIT)

void register_smmu_error_domain(void);

#endif