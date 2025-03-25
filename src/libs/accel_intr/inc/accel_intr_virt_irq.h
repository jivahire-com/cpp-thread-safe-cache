//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_intr_virt_irq.h
 * Header file for Accel Interrupt Library code
 */

 #pragma once

 /*----------------------------- Nested includes -----------------------------*/

 #include <accelip_id.h>
 #include <virt_irq.h>

 #include <nvic.h>
 #include <sdm_ext_interrupts.h>
 #include <stdint.h>

 /*------------------- Symbolic Constant Macros (defines) --------------------*/

 /*-------------------------------- Typedefs ---------------------------------*/

 typedef enum
 {
     MCP_NVIC_DOMAIN         = 0,
     MCP_CDED_SDM_DOMAIN     = 1,
     MCP_SDM_DOMAIN          = 2,
 } MCP_VIRT_IRQ_DOMAIN_T;

 typedef enum
 {
     SCP_NVIC_DOMAIN         = 0,
     SCP_CDED_SDM_DOMAIN     = 1,
     SCP_SDM_DOMAIN          = 2,
 } SCP_VIRT_IRQ_DOMAIN_T;

 /*------------------- Declarations (Statics and globals) --------------------*/

 /*--------------------------- Function Prototypes ---------------------------*/

 /**
  * @brief API to return pointer to the accel interrupt nvic callback
  * NOTE: This function will be defined only in x86 compilation for unit tests
  *
  * @param cb_fun Variable to hold address of callback function
  */
 void accel_intr_get_virt_isr_cb(isr_callback_fn_with_params_t *p_cb_fun);

 /**
  * @brief Functions gets pointer to global array of type virt_irq_plat_cb_t
  * for accelerator interrupts and the size of the array
  *
  * @return size of array virt_irq_plat_cb_t
  */
 uint32_t accel_intr_get_virt_irq_plat_info(virt_irq_plat_cb_t **p_plat_info);

 /**
  * @brief Function to register the accelerator NVIC interrupts with the wrapper callback
  *
  * @param sdm_nvic_int NVIC interrupt number for the SDM accelerator
  * @param cded_nvic_int NVIC interrupt number for CDED-SDM accelerator
  */
 void accel_intr_virt_irq_register_isr(uint32_t sdm_nvic_int, uint32_t cded_nvic_int);
