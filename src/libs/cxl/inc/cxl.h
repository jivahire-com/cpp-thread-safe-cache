//
// Copyright (c) Microsoft Corporation. All rights reserved.
//



/**
 * @file cxl.h
 *
 * @brief Contains API's to init CXL and fetch CXL related information
 **/

#pragma once

/*----------- Nested includes ------------*/

#include <assert.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/
#define NUM_CXL_PARAMS 2
#define CXL_MEM_REGION_BASE_ADDR (0x080000000000ULL)
#define CXL_MEM_REGION_SIZE      (0x004000000000ULL) // 256GB

/*-------------- Typedefs ----------------*/

typedef struct _cxl_hdm_decoder_capability_reg
{
    uint32_t decoder_count : 4;
    uint32_t target_count : 4;
    uint32_t a11to8_interleave_capable : 1;
    uint32_t a14to12_interleave_capable : 1;
    uint32_t poison_on_decode_error_capability : 1;
    uint32_t interleave_capable_3_6_12_way : 1;
    uint32_t interleave_capable_16_way : 1;
    uint32_t uio_capable : 1;
    uint32_t reserved : 2;
    uint32_t uio_capable_decoder_count : 4;
    uint32_t memdata_nxm_capable: 1;
    uint32_t supported_coherency_models : 2;
    uint32_t reserved2 : 9;
} cxl_hdm_decoder_capability_reg_t;

typedef struct _cxl_hdm_decoder_global_control_reg
{
    uint32_t poison_on_decode_error_en : 1;
    uint32_t hdm_decoder_en : 1;
    uint32_t reserved : 30;
} cxl_hdm_decoder_global_control_reg_t;

typedef union _cxl_hdm_decoder_base_low_reg
{
    uint32_t as_uint32;
    struct {
        uint32_t reserved : 28;
        uint32_t base_low : 4;
    };
} cxl_hdm_decoder_base_low_reg_t;

typedef struct _cxl_hdm_decoder_base_high_reg
{
    uint32_t base_high;
} cxl_hdm_decoder_base_high_reg_t;
typedef union _cxl_hdm_decoder_size_low_reg
{
    uint32_t as_uint32;
    struct {
        uint32_t reserved : 28;
        uint32_t size_low : 4;
    };
} cxl_hdm_decoder_size_low_reg_t;

typedef struct _cxl_hdm_decoder_size_high_reg
{
    uint32_t size_high;
} cxl_hdm_decoder_size_high_reg_t;

typedef struct _cxl_hdm_decoder_control_reg
{
    uint32_t interleave_granularity : 4;
    uint32_t interleave_ways : 4;
    uint32_t lock_on_commit : 1;
    uint32_t commit : 1;
    uint32_t commmitted : 1;
    uint32_t error_not_committed : 1;
    uint32_t target_range_type : 1;
    uint32_t bi_capable : 1;
    uint32_t uio_capable : 1;
    uint32_t reserved : 1;
    uint32_t upstream_interleave_granularity : 4;
    uint32_t upstream_interleave_ways : 4;
    uint32_t interleave_set_position : 4;
    uint32_t reserved2 : 4;
} cxl_hdm_decoder_control_t;

typedef struct _cxl_hdm_decoder_target_list_reg
{
    uint8_t target_port_id[4];
} cxl_hdm_decoder_target_list_reg_t;

typedef struct _cxl_chbcr_header
{
    cxl_hdm_decoder_capability_reg_t capability_reg;
    cxl_hdm_decoder_global_control_reg_t global_control_reg;
    uint32_t reserved[2];
} cxl_chbcr_header_t;

typedef struct _cxl_hdm_decoder_region
{
    cxl_hdm_decoder_base_low_reg_t base_low;
    cxl_hdm_decoder_base_high_reg_t base_high;
    cxl_hdm_decoder_size_low_reg_t size_low;
    cxl_hdm_decoder_size_high_reg_t size_high;
    cxl_hdm_decoder_control_t control;
    cxl_hdm_decoder_target_list_reg_t target_list;
    uint32_t reserved;
} cxl_hdm_decoder_region_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/** 
 * @brief Initialize CXL CHBCR using config knob values
 * @param chbcr_base_address - base address of CHBCR in DDR memory
 * 
 **/
void cxl_chbcr_init(uint8_t* chbcr_base_address);

/** 
 * @brief Set the base and size of the CXL memory region
 * @param hdm_decoder - pointer to the HDM decoder region where size/base will be stored
 * @param base - pointer to the base address of the memory region
 * @param num_targets - number of targets in the region, should be 1 if no interleaving
 * 
 * @return size of the memory region
 **/
uint64_t set_cxl_mem_region_base_and_size(cxl_hdm_decoder_region_t* hdm_decoder, uint64_t *base, uint8_t num_targets);
