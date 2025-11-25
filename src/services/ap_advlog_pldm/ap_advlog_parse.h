//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file advlog_parse.h
 *       Defines structures that can help in parsing the advanced logger
 *       buffer.
 *
 */
#pragma once

/*----------- Nested includes ------------*/
#include <fpfw_status.h>
#include <stddef.h>
#include <stdint.h>
#include <DfwkCommon.h>


/*-- Symbolic Constant Macros (defines) --*/
/*
 * Define the UEFI signature macro which will be used to verify whether a
 * valid advanced log chunk is sitting in the memory reserved for it.
 */
#define SIGNATURE_16(A, B)        ((A) | (B << 8))
#define SIGNATURE_32(A, B, C, D)  (SIGNATURE_16(A, B) | (SIGNATURE_16(C, D) << 16))
#define ADVANCED_LOGGER_SIGNATURE SIGNATURE_32('A', 'L', 'O', 'G')

/*------------- Typedefs -----------------*/
/*
 * Advanced logger structures - not all are necessarily required but to
 * make sure that mcp is parsing advanced logger data correctly, define
 * all fields and match them to the exact widths as defined by the
 * advanced logger package in UEFI.
 */
typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t pad1;
    uint32_t nanosecond;
    int16_t time_zone;
    uint8_t daylight;
    uint8_t pad2;
} efi_time;

typedef struct {
    uint32_t signature;        /* Signature 'ALOG' */
    uint16_t version;          /* Current Version */
    uint16_t reserved[3];      /* Reserved for future */
    uint32_t log_buffer;       /* Fixed pointer to start of log */
    uint32_t reserved4;        /* Reserved for future */
    uint32_t log_current;      /* Where to store next log entry. */
    uint32_t discarded_size;   /* Number of bytes of messages missed */
    uint32_t log_buffer_size;  /* Size of allocated buffer */
    bool in_permanent_ram;     /* Log in permanent RAM */
    bool at_runtime;           /* After ExitBootServices */
    bool gone_virtual;         /* After VirtualAddressChange */
    bool hdw_port_initialized; /* HdwPort initialized */
    bool hdw_port_disabled;    /* HdwPort is Disabled */
    bool reserved2[3];         /**/
    uint64_t timer_frequency;  /* Ticks per second for log timing */
    uint64_t ticks_at_time;    /* Ticks when Time Acquired */
    efi_time time;             /* Uefi Time Field */
    uint32_t hw_print_level;   /* Logging level to be printed at hw port */
    uint32_t reserved3;        /**/
} advanced_logger_info;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 * @brief - Reads in the advanced_logger_info structure which will be located at
 *          AP_ADVANCED_LOGGER_BASE in the DDR. This structure contains
 *          important information about the logger such as the total size and
 *          the memory offset till which the log should be read.
 *
 * @param None
 *
 * @return - true if advanced_logger_info was read in and validated successfully
 *           false if advanced_logger_info failed validation.
 */
bool populate_advanced_logger_info();

/**
 * @brief - Retrieves the current size of the AP advanced logger log in memory.
 *
 * @param None
 *
 * @return - Current size of the advanced log in bytes.
 */
uint64_t get_advanced_logger_size();

/**
 * @brief - Retrieves the base address the AP advanced logger log in memory.
 *
 * @param None
 *
 * @return - Current base address of the advanced log in bytes.
 */
uint64_t get_advanced_logger_base();

/**
 * @brief - Retrieves the base address of the compressed AP advanced logger log in memory.
 *
 * @param None
 *
 * @return - Current base address of the compressed advanced log in bytes.
 */
uint64_t get_advanced_logger_compressed_base();

/**
 * @brief - Compresses the advanced logger data into the compressed buffer.
 *
 * @param dst_size - Pointer to size of the destination buffer. On return, it will hold the size of compressed data.
 *
 * @return - FPFW_STATUS_SUCCESS on success, appropriate error code on failure.
 */
fpfw_status_t adv_logger_compress(size_t* dst_size);