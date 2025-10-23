# UART AFM CLI Command Reference

This document provides detailed usage, background, and examples for all UART-related CLI commands available in the firmware. It is intended to answer common questions and eliminate confusion about AFM, die assignment, and ownership APIs.

There are some new APIs added to support check for invalid configs and get config, set single UART etc. but exisitng uart_afm and uart_die support basic AFM select and die switching.

All commands are to be run on MCP Die 0. MCP Die 1 also supports all commands as backup.

Value x, y, z as per valid combinations list (at the end of this page)
`afm uart_afm 0 1 x y z` to switch from HSP Die 0 to MCP Die 0 on UART 0
`afm uart_afm 1 1 x y z` to switch from HSP Die 1 to MCP Die 1 on UART 5
`afm uart_die 1 1` to route UART1 and UART2 to Die 1

---

## Overview

These CLI commands allow you to query and configure the mapping and ownership of UARTs on the silicon for all UARTs or single UART. All changes are **temporary** (Reset to knobs value on Reset) unless made persistent via the configuration manager (`cfg_mgr_set`).

- **AFM (Alternate Function Mapping):** Controls which core/function is mapped to each UART.
- **Ownership** Controls which die owns a shared UART.

Possible Configurations :

### UART AFM Mapping Table

| AFM Key | Meaning           |
|---------|-------------------|
|    -    | Not mapped        |
|    0    | Default mapping   |
|   1-2   | Alternate mapping |

#### Die 0

| UART  | COM Name | SCP | HSP | MCTP | SDM | SDM_CDED | MCP | AP-Sec |
|-------|----------|-----|-----|------|-----|----------|-----|--------|
| UART0 |   COM4   |  -  |  0  |   1  |  -  |    2     |  -  |   -    |
| UART1 |   COM5   |  0  |  -  |   -  |  1  |    2     |  -  |   -    |
| UART2 |   COM6   |  1  |  -  |   0  |  2  |    -     |  -  |   -    |
| UART3 |   COM7   |  -  |  -  |   -  |  -  |    -     |  0  |   1    |
| UART4 |   COM8   |  -  |  -  |   -  |  -  |    -     |  -  |   -    |

#### Die 1

| UART  | COM Name | SCP | HSP | MCP | SDM | SDM_CDED |
|-------|----------|-----|-----|-----|-----|----------|
| UART5 |   COM9   |  -  |  0  |  1  |  -  |    2     |
| UART1 |   COM5   |  0  |  -  |  -  |  1  |    2     |
| UART2 |   COM6   |  1  |  -  |  0  |  2  |    -     |

---

## Command Summary

| Command            | Description                                 | Usage Example          |
|--------------------|---------------------------------------------|------------------------|
| uart_afm           | Set UART AFM (all UARTs)                    | `uart_afm 0 0 2 x x`   |
| uart_die           | Set UART die configuration                  | `uart_die 0 1`         |
| get_uart_ownership | Get UART ownership                          | `get_uart_ownership`   |
| get_uart_afm       | Get UART AFM                                | `get_uart_afm`         |
| get_mux_table      | Print the UART Mux Table                    | `get_mux_table`        |

---

## Command Details and Usage

### uart_afm

Set the AFM (Alternate Function Mapping) for all UARTs. This determines which core/function is mapped to each UART. **This update is temporary and is lost on reset**

- **Usage:** `uart_afm <die_num> <afm_u0> <afm_u1> <afm_u2> <afm_u3>`
- **Arguments:** Each afm argument is a mux value (0-3), mapping to a core. Refer the MUX table for reference.
- **Example:** `uart_afm 0 0 2 1 x`
- **Notes:**
  - Invalid or conflicting mappings will be rejected. This is to ensure 2 UARTs are not mapped to the same Core, which will cause CLI input to freeze.
  - This API takes `x` as a positional input if you wish to leave a mux value unchanged. However, there is a mandatory value for `die_num`, either `0` or `1`.
  - Use `get_uart_afm` to verify the new mapping.

### uart_die

Set the die assignment for UART1 and UART2. This controls which die owns each shared UART. **This update is temporary and is lost on reset**

- **Usage:** `uart_die <die_id_u1> <die_id_u2>`
- **Arguments:** Each argument is a die ID (0 or 1 for KNG).
- **Example:** `uart_die 0 1` (UART1 routed to die 0, UART2 to die 1)
- **Notes:**
  - This API takes `x` as a positional input if you wish to leave a die routing value unchanged. For example, `uart_die 0 x` will update UART1 ownership to Die 0 and keep UART2 as-is.
  - Only shared UARTs can have their die assignment changed (UART 1 and UART 2).
  - Use `get_uart_ownership` to verify ownership after setting, if needed.

### get_uart_ownership

Query the die routing for UART1 and UART2.

- **Usage:** `get_uart_ownership`
- **Output:** Shows which die is currently routed to each shared UART.

### get_uart_afm

Query the current AFM settings for all UARTs.

- **Usage:** `get_uart_afm`
- **Output:** Shows the AFM value for each UART (0-3).

---

## FAQ & Best Practices

- **What is the difference between AFM and die assignment?**
  - AFM controls which core/function is mapped to a UART. Die assignment controls which die is routed to a shared UART.
- **How do I know which AFM value maps to which core?**
  - Use above table to see the mapping for each UART and die. You can also use the command `get_mux_table`
- **What happens if I set an invalid or conflicting mapping?**
  - The command will fail and print an error.
- **Are changes persistent?**
  - No. All changes via afm CLI commands are temporary. Use `cfg_mgr_set` to update knobs for persistent changes.
- **How do I restore defaults?**
  - For changes made via the afm CLI, reset the SoC to restore defaults.
- **How do I debug if a UART is not responding?**
  - Use `get_uart_afm` and `get_uart_ownership` to check current settings. Use `get_mux_table` to fetch the mux table for reference.

---

## Example Scenarios

- **Change all UART AFM mappings on a die:**
  
```
  uart_afm 0 0 2 1 1 // For die 0
  uart_afm 1 0 2 1 1 // For die 1
```

- **Change only UART2 AFM:**
  ```
  uart_afm 0 x x 2 x // For Die 0
  uart_afm 1 x x 2 x // For Die 1
  ```
- **Assign UART1 to die 0, UART2 to die 1:**
  ```
  uart_die 0 1
  get_uart_ownership
  ```

---

## Troubleshooting

- If you see `Failed: Invalid Args`, check your argument count and value ranges.
- If you see `Failed - 0x...`, the configuration is not allowed or there is a firmware error.
- When in doubt, reset the device to restore default UART mappings.

---

## Additional References

- Silicon debug user guide (for core/die/AFM mapping tables)
- Firmware release notes (for any changes to CLI API or mapping rules)
- Contact the firmware team for edge cases or advanced debug scenarios.

---

## Valid UART AFM Select and Die Ownership Combinations

Below are the valid AFMSEL configurations for Die 0 and Die 1. Use these as reference for allowed AFM select values and ownership combinations. '-O' just indicates the die should be owner of that UART to have SCP or MCP CLI access. When '-O' is twice, any one is enough, for example, first entry HSP SCP-O MCP AP_NS or HSP SCP MCP-O AP_NS both are valid. That means either UART1 or UART2 should be owned by Die 0.

### Die 0 UART AFM Combinations

| AFMSEL (U0 U1 U2 U3) | Mapping & Ownership |
|----------------------|--------------------|
| 0 0 0 0              | HSP SCP-O MCP-O AP_NS |
| 0 0 0 1              | HSP SCP-O MCP-O AP_SEC |
| 0 0 2 0              | HSP SCP-O SDM AP_NS |
| 0 0 2 1              | HSP SCP-O SDM AP_SEC |
| 0 1 0 0              | HSP SDM MCP-O AP_NS |
| 0 1 0 1              | HSP SDM MCP-O AP_SEC |
| 0 1 1 0              | HSP SDM SCP-O AP_NS |
| 0 1 1 1              | HSP SDM SCP-O AP_SEC |
| 0 2 0 0              | HSP SDM_CDED MCP-O AP_NS |
| 0 2 0 1              | HSP SDM_CDED MCP-O AP_SEC |
| 0 2 1 0              | HSP SDM_CDED SCP-O AP_NS |
| 0 2 1 1              | HSP SDM_CDED SCP-O AP_SEC |
| 0 3 0 0              | HSP CDED MCP-O AP_NS |
| 0 3 0 1              | HSP CDED MCP-O AP_SEC |
| 0 3 1 0              | HSP CDED SCP-O AP_NS |
| 0 3 1 1              | HSP CDED SCP-O AP_SEC |
| 1 0 2 0              | MCP SCP-O SDM AP_NS |
| 1 0 2 1              | MCP SCP-O SDM AP_SEC |
| 1 1 1 0              | MCP SDM SCP-O AP_NS |
| 1 1 1 1              | MCP SDM SCP-O AP_SEC |
| 1 2 1 0              | MCP SDM_CDED SCP-O AP_NS |
| 1 2 1 1              | MCP SDM_CDED SCP-O AP_SEC |
| 1 2 2 0              | MCP SDM_CDED SDM AP_NS |
| 1 2 2 1              | MCP SDM_CDED SDM AP_SEC |
| 1 3 1 0              | MCP CDED SCP-O AP_NS |
| 1 3 1 1              | MCP CDED SCP-O AP_SEC |
| 1 3 2 0              | MCP CDED SDM AP_NS |
| 1 3 2 1              | MCP CDED SDM AP_SEC |
| 2 0 0 0              | SDM_CDED SCP-O MCP-O AP_NS |
| 2 0 0 1              | SDM_CDED SCP-O MCP-O AP_SEC |
| 2 0 2 0              | SDM_CDED SCP-O SDM AP_NS |
| 2 0 2 1              | SDM_CDED SCP-O SDM AP_SEC |
| 2 1 0 0              | SDM_CDED SDM MCP-O AP_NS |
| 2 1 0 1              | SDM_CDED SDM MCP-O AP_SEC |
| 2 1 1 0              | SDM_CDED SDM SCP-O AP_NS |
| 2 1 1 1              | SDM_CDED SDM SCP-O AP_SEC |
| 2 3 0 0              | SDM_CDED CDED MCP-O AP_NS |
| 2 3 0 1              | SDM_CDED CDED MCP-O AP_SEC |
| 2 3 1 0              | SDM_CDED CDED SCP-O AP_NS |
| 2 3 1 1              | SDM_CDED CDED SCP-O AP_SEC |
| 3 0 0 0              | CDED SCP-O MCP-O AP_NS |
| 3 0 0 1              | CDED SCP-O MCP-O AP_SEC |
| 3 0 2 0              | CDED SCP-O SDM AP_NS |
| 3 0 2 1              | CDED SCP-O SDM AP_SEC |
| 3 1 0 0              | CDED SDM MCP-O AP_NS |
| 3 1 0 1              | CDED SDM MCP-O AP_SEC |
| 3 1 1 0              | CDED SDM SCP-O AP_NS |
| 3 1 1 1              | CDED SDM SCP-O AP_SEC |
| 3 2 0 0              | CDED SDM_CDED MCP-O AP_NS |
| 3 2 0 1              | CDED SDM_CDED MCP-O AP_SEC |
| 3 2 1 0              | CDED SDM_CDED SCP-O AP_NS |
| 3 2 1 1              | CDED SDM_CDED SCP-O AP_SEC |

### Die 1 UART AFM Mux Combinations

| AFMSEL (U0 U1 U2 U3) | Mapping & Ownership |
|----------------------|--------------------|
| 0 0 0 x              | HSP SCP-O MCP-O Don't care |
| 0 0 2 x              | HSP SCP-O SDM Don't care |
| 0 1 0 x              | HSP SDM MCP-O Don't care |
| 0 1 1 x              | HSP SDM SCP-O Don't care |
| 0 2 0 x              | HSP SDM_CDED MCP-O Don't care |
| 0 2 1 x              | HSP SDM_CDED SCP-O Don't care |
| 0 3 0 x              | HSP CDED MCP-O Don't care |
| 0 3 1 x              | HSP CDED SCP-O Don't care |
| 1 0 2 x              | MCP SCP-O SDM Don't care |
| 1 1 1 x              | MCP SDM SCP-O Don't care |
| 1 2 1 x              | MCP SDM_CDED SCP-O Don't care |
| 1 2 2 x              | MCP SDM_CDED SDM Don't care |
| 1 3 1 x              | MCP CDED SCP-O Don't care |
| 1 3 2 x              | MCP CDED SDM Don't care |
| 2 0 0 x              | SDM_CDED SCP-O MCP-O Don't care |
| 2 0 2 x              | SDM_CDED SCP-O SDM Don't care |
| 2 1 0 x              | SDM_CDED SDM MCP-O Don't care |
| 2 1 1 x              | SDM_CDED SDM SCP-O Don't care |
| 2 3 0 x              | SDM_CDED CDED MCP-O Don't care |
| 2 3 1 x              | SDM_CDED CDED SCP-O Don't care |
| 3 0 0 x              | CDED SCP-O MCP-O Don't care |
| 3 0 2 x              | CDED SCP-O SDM Don't care |
| 3 1 0 x              | CDED SDM MCP-O Don't care |
| 3 1 1 x              | CDED SDM SCP-O Don't care |
| 3 2 0 x              | CDED SDM_CDED MCP-O Don't care |
| 3 2 1 x              | CDED SDM_CDED SCP-O Don't care |
