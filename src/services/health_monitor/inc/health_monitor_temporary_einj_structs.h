//
// Copyright (c) Microsoft Corporation.
//

/**
 * @file health_monitor_temporary_einj_structs.h
 * Temporary header file for supporting health monitoring until TFA shared header is available
 */
#pragma once

#include <cper.h>

#pragma pack(1)

// we are moving all under TFA shared. leave this here to avoid build break but update once new silib is available
typedef struct {
    uint32_t version;
    uint16_t component_group;
    uint16_t component_type;
    uint16_t component_instance;
    union {
        uint16_t value;
        struct {
            uint8_t status;
            enum operation_t : uint8_t {
                ENUM_GET,
                ENUM_SET,
                ADDRESS_GET,
                ADDRESS_SET
            } operation;
        };
    } status_operation;
    union {
        uint64_t error_parameters[2];
        struct {
            uint16_t error_type;
            uint16_t severity;
            uint32_t reserved;
            union {
                uint64_t *address_64;
                uint32_t *address_32;
            };
        };
    } param_type;
    union {
        uint64_t error_values;
        uint64_t data_64;
        uint32_t data_32;
    } value_type;
} ras_einj_info_t_temp;

#pragma pack()