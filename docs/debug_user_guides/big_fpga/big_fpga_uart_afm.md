## FPGA UART AFM User Guide


### Introduction

This page contains commands and Limitations of UART AFM Sel supported in firmware on the BigFPGA machines.

### Commands

1. For Run time change
    - From SCP and MCP CLI, to select AFM Sel 1 for UART0,1,2 and 3
    ```pwsh
    gpio
    uart_afm 1 1 1 1
    ```
1. For Updating in Knobs to take effect in next boot
    - From Die 0 SCP CLI, to select Die0 AFM Sel value 1 for UART0,1,2 and 3
    ```pwsh
    cfg_mgr
    cfg_mgr_set uart_afm_cfg_die0 1 1 1 1
    ```
    - From Die 0 SCP CLI, to select Die1 AFM Sel value 1 for UART0,1,2 and 3
    ```pwsh
    cfg_mgr
    cfg_mgr_set uart_afm_cfg_die1 1 1 1 1
    ```

### Limitations

1. If user updates UART AFM via knobs such that no UART is assigned with SCP and loses SCP UART on boot, the only way to get around this would be to reflash the knobs partition to get access to the SCP UART. For Run time config, at least SCP or MCP UART should be configured to any COM port for further switching UART AFM.
1. This debug print *"Updating UART config temporarily. Use cfg_mgr_set for persistent update"* comes half as UART AFM switches in between print, add delay or flush UART before updating AFM to resolve this.
