# Copyright (c) Microsoft Corporation. All rights reserved.

"""BMC utility helpers.
Provides reusable functions for executing BMC CLI commands.
"""
from typing import Iterable, List, Dict, Any

class BmcCommandError(RuntimeError):
    """Raised when a BMC command reports an error on stderr."""
    pass

def ensure_bmc_cli_open(dut) -> Any:
    """Ensure the BMC CLI is open and return it."""
    bmc_cli = dut.mb.node_0.dcscm.bmc.cli
    if not bmc_cli.is_open():
        bmc_cli.open()
    return bmc_cli

def run_bmc_commands(dut, log, commands: Iterable[str], raise_on_error: bool = True) -> List[Dict[str, Any]]:
    """Run a sequence of BMC commands.

    Args:
        dut: DUT object containing mb.node_0.dcscm.bmc.cli
        log: Logger for info/error messages
        commands: Iterable of command strings
        raise_on_error: If True raise BmcCommandError on any stderr output
    Returns:
        List of dicts per command with return_code, stdout, stderr.
    """
    bmc_cli = ensure_bmc_cli_open(dut)
    results = []
    for cmd in commands:
        ret, stdout, stderr = bmc_cli.execute_command(command=cmd, command_args=[])
        if stderr:
            log.error(f"BMC command failed: {cmd} | Error: {stderr}")
            if raise_on_error:
                raise BmcCommandError(f"BMC command failed: {cmd} | Error: {stderr}")
        else:
            log.info(f"BMC command successful: {cmd} | Return: {ret}")
        results.append({
            "command": cmd,
            "return_code": ret,
            "stdout": stdout,
            "stderr": stderr,
        })
    return results

def set_bmc_uart_mux(dut, log, target: str, raise_on_error: bool = True):
    """Set the BMC UART mux to a specific core target (SCP or MCP).

    Args:
        dut: DUT object
        log: Logger
        target: 'SCP' or 'MCP' (default AFM settings), or 'UART1' or 'UART3' etc.
        raise_on_error: Raise if GPIO command errors occur
    """
    target_u = target.upper()
    mux_map = {
        "SCP": ["gpioset gpiochip3 5=0", "gpioset gpiochip6 6=0"],
        "MCP": ["gpioset gpiochip3 5=1", "gpioset gpiochip6 6=0"],
        "UART1": ["gpioset gpiochip3 5=0", "gpioset gpiochip6 6=0"],
        "UART3": ["gpioset gpiochip3 5=1", "gpioset gpiochip6 6=0"],
    }
    if target_u not in mux_map:
        raise ValueError(f"Unknown mux target: {target}")
    log.info(f"Setting BMC UART mux to {target_u}")
    return run_bmc_commands(dut, log, mux_map[target_u], raise_on_error=raise_on_error)

__all__ = ["run_bmc_commands", "ensure_bmc_cli_open", "BmcCommandError", "set_bmc_uart_mux"]