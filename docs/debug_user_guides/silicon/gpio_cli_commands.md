# UART CLI Command Reference

This document provides detailed usage, background, and examples for all UART-related CLI commands available in the firmware. It is intended to answer common questions and eliminate confusion about AFM, die assignment, and ownership APIs.

There are some new APIs added to support check for invalid configs and get config, set single UART etc. but exisitng uart_afm and uart_die support basic AFM select and die switching.

Switch to MCP from HSP for both Die (UART0), use MCP CLI to switch Die for UART1 and UART2 to avoid losing access to SCP shared UART CLI.

```
uart_afm 1 0 0 0 in Die 0 to switch from HSP to MCP
uart_afm 1 1 1 0 in Die 1
uart_die 1 0 in Die 0 CLI
uart_die 1 0 in Die 1 CLI
```


This will enable UART2 to Die 

---

## Overview

These CLI commands allow you to query and configure the mapping and ownership of UARTs on the silicon for all UARTs or single UART. All changes are **temporary** (Reset to knobs value on Reset) unless made persistent via the configuration manager (`cfg_mgr_set`).

- **AFM (Alternate Function Mapping):** Controls which core/function is mapped to each UART.
- **Ownership** Controls which die owns a shared UART.

Possible Configurations :


### UART AFM Mapping Table

#### Die 0

| UART   | COM Name         | SCP | HSP | MCP | SDM | SDM_CDED | CDED(KMP) | AP-NS | AP-Sec |
|--------|------------------|-----|-----|-----|-----|----------|-----------|-------|--------|
| UART0  | COM4 - HSP       |  x  |  0  |  1  |  x  |    2     |     3     |   x   |        |
| UART1  | COM5 - SCP       |  0  |  x  |  x  |  1  |    2     |     3     |   x   |        |
| UART2  | COM6 - MCP       |  1  |  x  |  0  |  2  |    x     |     x     |   x   |        |
| UART3  | COM7 - AP-NS     |     |     |     |     |          |           |   0   |   1    |
| UART4  | COM8 - AP-NS IOSS|  -  |  -  |  -  |  -  |    -     |     -     |   -   |   -    |

#### Die 1

| UART   | COM Name     | SCP | HSP | MCP | SDM | SDM_CDED | CDED(KMP) |
|--------|--------------|-----|-----|-----|-----|----------|-----------|
| UART5  | COM9         |  x  |  0  |  1  |  x  |    2     |    3      |
| UART1  | COM5         |  0  |  x  |  x  |  1  |    2     |    3      |
| UART2  | COM6         |  1  |  x  |  0  |  2  |    x     |    x      |


---

## Command Summary

| Command            | Description                                 | Usage Example |
|--------------------|---------------------------------------------|--------------|
| uart_afm           | Set UART AFM (all UARTs)                    | `uart_afm 0 1 2 3` |
| uart_afm_update    | Update UART AFM (preserves other settings)  | `uart_afm_update 0 1 2 3` |
| uart_die           | Set UART die configuration                  | `uart_die 0 1` |
| uart_single_afm    | Set single UART AFM                         | `uart_single_afm 2 1` |
| uart_single_die    | Set single UART die/ownership               | `uart_single_die 2 1` |
| uart_ownership     | Set UART ownership                          | `uart_ownership 1 0` |
| uart_get_ownership | Get UART ownership                          | `uart_get_ownership` |
| uart_get_afm       | Get UART AFM                                | `uart_get_afm` |
| uart_list_valid    | Not Working - List valid UART settings      | `uart_list_valid` |

---

## Command Details & Tester Guidance

### uart_afm
Set the AFM (Alternate Function Mapping) for all UARTs at once. This determines which core/function is mapped to each UART. **This is a temporary change.**
- **Usage:** `uart_afm <afm_u0> <afm_u1> <afm_u2> <afm_u3>`
- **Arguments:** Each argument is a value 0-3, mapping to a core
- **Example:** `uart_afm 0 1 2 3`
- **Notes:**
  - Invalid or conflicting mappings will be rejected.
  - Use `uart_get_afm` to verify the new mapping.
  
### uart_afm_update
Update the AFM for all UARTs, but preserves other settings (such as overrides from other cores or debug tools). **This is a temporary change.**
- **Usage:** `uart_afm_update <afm_u0> <afm_u1> <afm_u2> <afm_u3>`
- **Arguments:** Each argument is a value 0-3, mapping to a core
- **Example:** `uart_afm_update x x 2 x`
- **When to use:** If you want to change only some UARTs and keep others as-is, read the current AFM with `uart_get_afm`, then use this command with new values for only the UARTs you want to change.

### uart_die
Set the die assignment for UART1 and UART2. This controls which die owns each shared UART. **This is a temporary change.**
- **Usage:** `uart_die <die_id_u1> <die_id_u2>`
- **Arguments:** Each argument is a die ID (typically 0 or 1).
- **Example:** `uart_die 0 1` (UART1 owned by die 0, UART2 by die 1)
- **Notes:**
  - Only shared UARTs can have their die assignment changed.
  - Use `uart_get_ownership` to verify ownership after setting, if needed.

### uart_single_afm
Set the AFM for a single UART.
- **Usage:** `uart_single_afm <uart_num> <afm_uart>`
- **Arguments:**
  - `uart_num`: UART index (0-3)
  - `afm_uart`: AFM value (0-3)
- **Example:** `uart_single_afm 2 1`, to update UART2 AFM to 1
- **Notes:**
  - This is useful for fine-grained changes or debugging.
  - Invalid mappings will be rejected.

### uart_single_die
Set the die/ownership for a single UART.
- **Usage:** `uart_single_die <uart_num> <own_uart>`
- **Arguments:**
  - `uart_num`: UART index (0-3)
  - `own_uart`: 0 or 1 
- **Example:** `uart_single_die 2 1`, Current Die will own UART2
- **Notes:**
  - Only shared UARTs can have their die assignment changed.

### uart_ownership
Set the ownership for UART1 and UART2 in one command. **This is a temporary change.**
- **Usage:** `uart_ownership <uart1_own> <uart2_own>`
- **Arguments:** 0 or 1 for each UART (die ID)
- **Example:** `uart_ownership 1 0`, Current Die owns UART1 but not UART2
- **Notes:**
  - Use this in other Die as well to set uart_ownership 0 1, to release the UART

### uart_get_ownership
Query the current ownership of UART1 and UART2.
- **Usage:** `uart_get_ownership`
- **Output:** Shows which die currently owns each shared UART.

### uart_get_afm
Query the current AFM settings for all UARTs.
- **Usage:** `uart_get_afm`
- **Output:** Shows the AFM value for each UART (0-3).

### uart_list_valid
Not yet implemented - This API is wishlist and will be developed, if needed, List all valid UART AFM/ownership/die settings for the current silicon.
- **Usage:** `uart_list_valid`
- **Output:**
  - Shows all valid combinations of AFM, die, and ownership for your silicon.
  - Use this as a reference before making changes to avoid invalid configurations.

---

## FAQ & Best Practices

- **What is the difference between AFM and die assignment?**
  - AFM controls which core/function is mapped to a UART. Die assignment controls which die owns a shared UART.
- **How do I know which AFM value maps to which core?**
  - Use above table to see the mapping for each UART and die.
- **What happens if I set an invalid or conflicting mapping?**
  - The command will fail and print an error. Use only values shown in `uart_list_valid`.
- **Are changes persistent?**
  - No. All changes via these CLI commands are temporary. Use `cfg_mgr_set` for persistent changes.
- **How do I restore defaults?**
  - For UART AFM/die/ownership, reset the device.
- **How do I debug if a UART is not responding?**
  - Use `uart_get_afm` and `uart_get_ownership` to check current settings. Use `uart_list_valid` to verify allowed values.

---

## Example Scenarios

- **Change all UART AFM mappings:**
  ```
  uart_afm 0 1 2 3
  uart_get_afm
  ```
- **Change only UART2 AFM:**
  ```
  uart_get_afm
  uart_single_afm 2 1
  uart_get_afm
  ```
- **Assign UART1 to die 0, UART2 to die 1:**
  ```
  uart_die 0 1
  uart_get_ownership
  ```
- **List all valid settings for your silicon:**
  ```
  uart_list_valid - Not yet implemented 
  ```

---

## Troubleshooting

- If you see `Failed: Invalid Args`, check your argument count and value ranges.
- If you see `Failed - 0x...`, the configuration is not allowed or there is a firmware error.
- Always use `uart_get_afm` before making changes to avoid invalid settings.
- If in doubt, reset the device to restore default UART mappings.

---

## Additional References
- Silicon debug user guide (for core/die/AFM mapping tables)
- Firmware release notes (for any changes to CLI API or mapping rules)
- Contact the firmware team for edge cases or advanced debug scenarios.
