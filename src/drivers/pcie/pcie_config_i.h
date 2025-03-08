//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_config_i.h
 * Apply and setup pciess configuration settings retrieved from the
 * configuration manager.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <pcie_config_variable.h>
#include <pcie_ss_common.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/*
 * @brief Retrieve updated PCIe configuration from the configuration manager
 *        and override it for the given RPSS.
 *
 * @param rpss_id The RPSS ID to override the configuration for.
 *
 * @retval None
 */
void override_default_pcie_cfg(uint8_t rpss_id);

/*
 * @brief Populate the root bridge configurations to be shared with UEFI from
 *        the RPSS entity.
 *
 * @param rpss The RPSS entity to populate the configurations from.
 * @param rb_configs The root bridge configurations to populate.
 *
 * @retval None
 */
void populate_rb_configs_from_rpss_entity(pcie_ss_entity_t* rpss, pcie_root_bridge_config* rb_configs);

/*
 * @brief Get the PCIe configuration currently set for the given RPSS ID.
 *
 * @param rpss_id The RPSS ID to get the configuration for.
 *
 * @retval Pointer to the PCIe configuration structure.
 */
pcie_cfg_t* get_configuration_for_rpss(uint8_t rpss_id);
