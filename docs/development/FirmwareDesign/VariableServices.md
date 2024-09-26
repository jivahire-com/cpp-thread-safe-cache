# Variable Service Library Document

## Table of Contents

[[_TOC_]]

## Introduction

### Description

This document describes the Variable Service library, which enables multiple clients on MSCP platform to get/set variables to be published to HSP.

### Revision History

| Revised by   | Date      | Changes           |
| ------------ | --------- | ------------------|
| Shreya Chakraborty   | 09/18/2024| Initial design    |
|   |  |  |

### Terms


| Term   | Description                     |
| ------ | ------------------------------- |
| HSP    | Security Processor |
| ATU    | Address Translation Unit
| CLI    | Command Line Interface                        |
| ICC    | Inter Core Communication                      |
| MCP    | Management Control Processor                  |
| SCP    | System Control Processor                      |

### Reference Documents

| Document                                  | Link                                |
| ----------------------------------------- | ----------------------------------- |
| UEFI Specification | [Link](https://uefi.org/specs/UEFI/2.10/08_Services_Runtime_Services.html)    |
| Variable Services Design (EF_UEFI repo) | [Link](https://azurecsi.visualstudio.com/Woodinville/_git/EF_UEFI?path=%2FDocs%2FFirmwareDesign%2FVariableService%2FOverview.md&version=GBmain&_a=preview) |
| ICC Base | [Link](https://azurecsi.visualstudio.com/DuvallFw/_git/1pfw.fwlibs?path=/doc/Modules/services/IccBase.md) |
| Hsp Mailbox Doc | [Link](https://azurecsi.visualstudio.com/Woodinville/_git/kingsgate.hsp?path=/docs/mailbox.md) |

## Requirements

1. Provides consumers a way to synchronously to send variable service message (Get/Set/Query Variable cmds) from MSCP to HSP & recv resp during initialization.
2. Provides consumers a way to asynchronously send/recv variable sevice message from HSP during runtime.

## Dependencies

- ICC Base (icc_hspmbox)
- Crash Dump (for bug assert)

## Design

The library provides wrapper APIs that make it easy for the consumers to fetch/publish/query variables to/from HSP.
The Sync APIs are recommended to be used during init & Async APIs in runtime.

### Internal Implementation details

This section will provide details on the request response command code pairs used for variable service. Platform will exchange the following command codes with HSP:

```c
/**
 * Definition for various mailbox messages. Each message is
 * a request/response pair.
 */
enum {
    HSP_MAILBOX_MSG_UNKNOWN = 0x0,
    /*...*/
    HSP_MAILBOX_CMD_GET_VARIABLE_REQ = 0x100,                /**< Request to HSP for UEFI variable value. */
    HSP_MAILBOX_CMD_GET_VARIABLE_RSP = 0x101,                /**< Response from HSP with UEFI variable value. */
    HSP_MAILBOX_CMD_SET_VARIABLE_REQ = 0x102,                /**< Request to HSP to set UEFI variable. */
    HSP_MAILBOX_CMD_SET_VARIABLE_RSP = 0x103,                /**< Response from HSP with UEFI variable set status. */
    HSP_MAILBOX_CMD_QUERY_VARIABLE_INFO_REQ = 0x104,         /**< Request to HSP for UEFI variable info. */
    HSP_MAILBOX_CMD_QUERY_VARIABLE_INFO_RSP = 0x105,         /**< Response fom HSP with UEFI variable info. */
    HSP_MAILBOX_CMD_GET_NEXT_VARIABLE_NAME_REQ = 0x106,      /**< Request to HSP to enumerate UEFI variables. */
    HSP_MAILBOX_CMD_GET_NEXT_VARIABLE_NAME_RSP = 0x107,      /**< Response from HSP for enumerating UEFI variables. */
    /*...*/
    HSP_MAILBOX_CMD_MAX,
};

```

#### Variable Service Get Command (HSP_MAILBOX_CMD_GET_VARIABLE_REQ, HSP_MAILBOX_CMD_GET_VARIABLE_RSP)

```c++
/**
 * @brief struct for payload framing for HSP_MAILBOX_CMD_GET_VARIABLE_REQ
*/
struct hsp_mbox_get_variable
{
    uint32_t    variable_name_size;       //! length of the variable name in uint16_t size including null terminator
    union hsp_guid vendor_guid;             //! input vendor guid
    uint32_t    attributes_size;         //! optional. can be 0 or 1
    uint32_t    data_size;               //! optional. can be 0
};

/**
 * @brief Returns the value of a variable.
 *        Initiator: ApSecure, SCP, MCP
 *        Receiver: Hsp
 * 
 * @param  get_variable_address - Pointer to a hsp_mbox_get_variable struct
 * @note   the HSP response to HSP_MAILBOX_CMD_GET_VARIABLE_REQ will use the standard 
 *         struct kng_hsp_mailbox_msg_rsp. The uint32_t status field will represent EFI_STATUS.
 *         If EFI_BUFFER_TOO_SMALL, then DataSize is updated with the needed size
 */
struct kng_hsp_mailbox_cmd_get_variable
{
    struct kng_hsp_mailbox_msg_header header;    /**< msg header containing cmd, seq,contect and flags. */
    uint32_t get_variable_address;                 /**< pointer to hsp_mbox_get_variable struct. */
};
```

**Usage:**

1. Config manager in scp will request config knobs or variables stored in flash (to override the default ones) from hsp during runtime init. (SCP0 OR 1 OR BOTH?)
2. DDR manager in scp requests to read `DDRSystemMemoryRanges` variable from HSP that was previously set by set by UEFI (through set var command).
3. System info in scp requests to read `RESETReason` variable from HSP during the init phase if the hsp boot meta data indicates pior reset reason was HspFwResetReasonUpdate. This is used
   for caching the reset reason. If hsp's response says the variable was set, reason reason is SYSTEM_INFO_RESET_REASON_POST_AP_BOOT_WARM else it's SYSTEM_INFO_RESET_REASON_PORE_AP_BOOT_WARM.
4. SEL logging, SCP & MCP request SEL from variable store HSP

#### Variable Service Set Command (HSP_MAILBOX_CMD_SET_VARIABLE_REQ, HSP_MAILBOX_CMD_SET_VARIABLE_RSP)

```c++
/**
 * @brief struct for payload framing for HSP_MAILBOX_CMD_SET_VARIABLE_REQ
*/
struct hsp_mbox_set_variable
{
    uint32_t    variable_name_size;       //! length of the variable name in uint16_t size including null terminator
    union hsp_guid vendor_guid;             //! input vendor guid
    uint32_t    attributes;
    uint32_t    data_size;
};

/**
 * @brief Sets the value of a variable.This service can be used to create a new variable, modify the value of an existing variable,
 *        or to delete an existing variable.
 *        Initiator: ApSecure, SCP, MCP
 *        Receiver: Hsp
 * 
 * @param  set_variable_address - Pointer to a hsp_mbox_set_variable struct
 * @note   the HSP response to HSP_MAILBOX_CMD_SET_VARIABLE_REQ will use the standard 
 *         struct kng_hsp_mailbox_msg_rsp. The uint32_t status field will represent EFI_STATUS.
 */
struct kng_hsp_mailbox_cmd_get_variable
{
    struct kng_hsp_mailbox_msg_header header;    /**< msg header conataining cmd, seq,contect and flags. */
    uint32_t set_variable_address;                 /**< pointer to hsp_mbox_set_variable struct. */
};
```

**Usage:**

1. DDR mgr creates/updates DDR address map (reserves mem regions of specific range for HSP, MSCP) adds structs & variable names required for HSP & UEFI to the DDR memory etc during init.
   This updated address map is passed to TF-A via HSP by setting `DDRSystemMemoryRanges` variable.
2. SEL logging, SCP/MCP store SEL in variable store via HSP (`MCPLOGSEL` , `SCPLOGSEL`).
3. System Info module on SCP uses HSP Mailbox command to pass version info to set the `MSCPVersion` variable & set`RESETReason` variable for cold boot during init.
4. Config manager sets variables `MissionMode` and `PrSelection` over Hsp mbox.
5. Fuse module sets the variable `FUSEINFO` post fuse distribution to store fuse info & sends it over to HSP to store to SPI.
6. Health monitor on SCP/MCP sets variables `HwErrRec0001` to raise/send UEFI defined error record to HSP.
7. Mesh creates a NUMA payload, sets variable `NUMAINFO` & sends the config to hsp over mbox to be store on SPI

#### Variable Service Query Info Command (HSP_MAILBOX_CMD_QUERY_VARIABLE_INFO_REQ, HSP_MAILBOX_CMD_QUERY_VARIABLE_INFO_RSP)

```c++
/**
 * @brief struct for payload framing for HSP_MAILBOX_CMD_QUERY_VARIABLE_INFO_REQ
*/
struct hsp_mbox_query_variable_info
{
    uint32_t attributes;
    uint64_t max_variable_storage_size;
    uint64_t remaining_variable_storage_size;
    uint64_t max_variable_size;
};

/**
 * @brief Returns information about the EFI variables
 *        Initiator: ApSecure, SCP, MCP
 *        Receiver: Hsp
 * 
 * @param  query_variable_info_address - Pointer to a hsp_mbox_query_variable_info struct
 * @note   the HSP response to HSP_MAILBOX_CMD_QUERY_VARIABLE_INFO_REQ will use the standard 
 *         struct kng_hsp_mailbox_msg_rsp. The uint32_t status field will represent EFI_STATUS.
 */
typedef struct kng_hsp_mailbox_cmd_query_variable_info
{
    struct kng_hsp_mailbox_msg_header header;    /**< msg header containing cmd, seq,contect and flags. */
    uint32_t query_variable_info_address;           /**< pointer to hsp_mbox_query_variable_info struct. */
} hsp_mbox_query_variable_info_cmd, *phsp_mbox_query_variable_info_cmd;
```

**Usage:**

tbd

#### Variable Service Get Next Variable Name Command (HSP_MAILBOX_CMD_GET_NEXT_VARIABLE_NAME_REQ, HSP_MAILBOX_CMD_GET_NEXT_VARIABLE_NAME_RSP)

```c++
/**
 * @brief struct for payload framing for HSP_MAILBOX_CMD_GET_NEXT_VARIABLE_NAME_REQ
*/
struct hsp_mbox_get_next_variable_name
{
    uint32_t variable_name_size;       // size of the variable name output buffer in uint16_t
    union hsp_guid vendor_guid;
    uint32_t attributes;
};

/**
 * @brief Enumerates the current variable names
 *        Initiator: ApSecure, SCP, MCP
 *        Receiver: Hsp
 * 
 * @param  get_next_variable_name_address - Pointer to a hsp_mbox_get_next_variable_name struct
 * @note   the HSP response to HSP_MAILBOX_CMD_SET_VARIABLE_REQ will use the standard 
 *         struct kng_hsp_mailbox_msg_rsp. The uint32_t status field will represent EFI_STATUS.
 *         If EFI_BUFFER_TOO_SMALL, then DataSize is updated with the needed size
 */
typedef struct kng_hsp_mailbox_cmd_get_next_variable_name_cmd
{
    struct kng_hsp_mailbox_msg_header header;    /**< msg header containing cmd, seq,contect and flags. */
    uint32_t get_next_variable_name_address;         /**< pointer to hsp_mbox_get_next_variable_name struct. */
};
```

**Usage:**

tbd

### Warmstart Considerations

tbd

## API

```c++
/**
 * @brief  Sync API to read a variable from HSP over Hsp Mbox. The response can be 
 * fetched from the shared memory region provided by the caller.
 * 
 * @note Blocking call, this API must be invoked only during the initialization. 
 * 
 * @param mem_ctx  Shared memory context for the variable
 * @param req_params Request parameters for the variable
 */
void variable_service_sync_get_variable(fpfw_icc_base_ctx_t* hsp_icc_ctx, var_service_shared_mem_t *mem_ctx, var_service_req_params_t *req_params);

/**
 * @brief  Sync API to write a variable & send to HSP over Hsp Mbox.
 * 
 * @note Blocking call, this API must be invoked only during the initialization.
 * 
 * @param mem_ctx  Shared memory context for the variable
 * @param req_params  Request parameters for the variable
 */
void variable_service_sync_set_variable(fpfw_icc_base_ctx_t* hsp_icc_ctx, var_service_shared_mem_t *mem_ctx, var_service_req_params_t *req_params);
```

## Verification

Implicitly, all APIs have been unit tested. Additionally we intend to have CLI to test it out on SVP & Bigfpga.
