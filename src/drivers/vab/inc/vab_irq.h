//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * VAB interrupt structures and routines
 */

#pragma once

/*----------- Nested includes ------------*/
#include <health_monitor.h>
#include <stdint.h>
#include <vab_intu.h>
#include <vab_irq_common.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
typedef struct _vab_isr_ctx_t
{
    uint32_t vab_irq_num;
    SUBSYSTEM_WITH_VAB_ID vab_id;
    uintptr_t vab_base;
    vabss_int_probe_t probe;
    void (*process_probe)(struct _vab_isr_ctx_t* ctx);
    bool unmap_required; /* Note: Currently the maps are static and resident, but this tracks if this context owns the mapping */
} vab_isr_ctx_t;

typedef enum
{
    VAB_ERROR_TARGET_FABRIC = 0,
    VAB_ERROR_TARGET_RAS_NODE = 1,
    VAB_ERROR_TARGET_INVALID = 2
} VAB_ERROR_TARGET;

#pragma pack(push, 1)

/* Note: This structure maps to ras_einj_info_t.param_type.error_parameters[0] */
typedef union
{
    struct
    {
        uint64_t target : 4;     /* VAB_ERROR_TARGET */
        uint64_t component : 4;  /* VAB_FABRIC_INTERFACE  | VAB_RAS_NODE_COMPONENT */
        uint64_t error_type : 8; /* VAB_FABRIC_ERROR_TYPE | VAB_RAS_NODE_ERROR_TYPE */
        uint64_t reserved : 48;
    };
    uint64_t as_uint64;
} vab_error_inj_param_t;

static_assert(sizeof(vab_error_inj_param_t) == sizeof(uint64_t), "'vab_error_inj_param_t' is not the expected size!");

#pragma pack(pop)

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 *  @brief VAB interrupt service routine.
 *
 *  @param[in] param
 *             Interrupt context passed in to handle the ISR.
 *             Is of type vab_isr_ctx_t
 *
 *  @return    None
 */
void vab_isr(void* param);

/**
 *  @brief Process the probe returned after probing RPSS VAB intus
 *
 *  @param[in] ctx VAB_ISR_CTX_T context for the respective VAB
 *
 *  @return    None
 */
void process_vab_rpss_probe(vab_isr_ctx_t* ctx);

/**
 *  @brief Process the probe returned after probing SDMSS VAB intus
 *
 *  @param[in] ctx VAB_ISR_CTX_T context for the respective VAB
 *
 *  @return    None
 */
void process_vab_sdmss_probe(vab_isr_ctx_t* ctx);

/**
 *  @brief Process the probe returned after probing CDEDSS VAB intus
 *
 *  @param[in] ctx VAB_ISR_CTX_T context for the respective VAB
 *
 *  @return    None
 */
void process_vab_cdedss_probe(vab_isr_ctx_t* ctx);

/**
 *  @brief VAB Error Injection Callback
 *
 *  @param[in] einj_payload
 *             Injection payload
 *
 *  @param[in] ctx
 *             Unused
 *
 *  @return    acpi_einj_cmd_status_t
 */
acpi_einj_cmd_status_t vab_error_injection_cb(ras_einj_info_t* einj_payload, void* ctx);
