"""
Python-based Pythia 2.0 Test Setup Utilities

This module provides setup, programming, and teardown utility functions for Pythia functional tests.

Features:
- FPGA reset and sideloading test firmware
- UART handling for multiple processors
- SDM AFM programming execution
"""

import os
import time
import json
from pathlib import Path

from pythia.tdk.common.util.debugger.trace32 import Trace32
from pythia.tdk.echofalls.duts.bigfpga_dut import BigFpgaDut

# Constants
FPGA_REVISION = "R22"
SDM_UART_MUX = 2
SDM_CDED_UART_MUX = 0
SDM_UART_INDEX = 2

class TestSetupHelper:
    """
    Helper class for setting up and configuring FPGA-based test environments.
    Provides functionalities to reset FPGA, sideload firmware, and execute AFM sequences.
    """

    @staticmethod
    def reset_fpga_sideload_custom_seq(dut: BigFpgaDut, proc_info: dict, log):
        """
        Resets FPGA and sideloads test firmware using a predefined HSP CMM script.

        Args:
            dut (BigFpgaDut): The FPGA DUT instance.
            proc_info (dict): Dictionary mapping processor names to expected UART logs and timeouts.
            log (Logger): Logger instance for logging information.
        """
        script_path = os.path.join(os.getcwd().split('.testlogs')[0], "tests/functional/cmm/")
        script_name = "PythiaLaunchFPGA.cmm"
        script = Path(os.path.join(script_path, script_name))

        # Open all UART channels before resetting FPGA
        for proc_name, info in proc_info.items():
            proc_uart = dut.mb.node_0.soc.primary_die.get_core(proc_name).channel_manager.get_current_channel()
            proc_uart.open()
            proc_uart.clear_buffer()

        # Sleep to avoid conflicts from multiple UART openings
        time.sleep(5)
        log.info("Resetting FPGA and sideloading Test Firmware . . .")
        log.info(f"Calling HSP CMM Script to reset and load ProdFW: {script}")

        hsp = dut.mb.node_0.soc.primary_die.get_core("hsp")
        hsp.debugger.execute_script(script)

        boot_time = 90
        log.info(f"Waiting {boot_time} seconds for FPGA to boot . . .")
        time.sleep(boot_time)

        for proc_name, info in proc_info.items():
            proc_uart = dut.mb.node_0.soc.primary_die.get_core(proc_name).channel_manager.get_current_channel()
            log.info(f'Waiting for boot string {info[0]} from proc {proc_name}')
            proc_uart.read_until(key=info[0], timeout_seconds=info[1])
            log.info("Received boot-up string")

            # Execute AFM programming for SCP if applicable
            if "scp" in proc_name:
                log.info("Executing SDM AFM programming")
                TestSetupHelper.execute_accel_afm_seq(
                    dut.mb.node_0.soc.primary_die.get_core(proc_name).debugger,
                    SDM_UART_MUX, SDM_UART_INDEX
                )

                log.info("Executing SDM CDED AFM programming")
                TestSetupHelper.execute_accel_afm_seq(
                    dut.mb.node_0.soc.primary_die.get_core(proc_name).debugger,
                    SDM_CDED_UART_MUX, SDM_UART_INDEX
                )

    @staticmethod
    def execute_accel_afm_seq(scp_debugger: Trace32, uart_mux: int, proc_index: int):
        """
        Executes the AFM programming sequence for a given processor debugger.

        Args:
            scp_debugger (Trace32): Debugger instance for the target processor.
            uart_mux (int): UART multiplexer index.
            proc_index (int): Processor index for programming.
        """
        soc_afm_script = f"R:/Kingsgate/Kingsgate_TRACE32/Tools/UART_AFM/SCPswitchUART{uart_mux}_AFM.cmm"

        scp_debugger.execute_command("break")
        time.sleep(1)
        scp_debugger.execute_script(script_path=soc_afm_script, arg=f"{proc_index}.")
        time.sleep(5)
        scp_debugger.execute_command("go")
        time.sleep(1)

    @staticmethod
    def load_json_file(json_file):
        try:
            with open(json_file, "r") as file:
                config = json.load(file)
                return  config
        except FileNotFoundError:
            raise FileNotFoundError(f"Error: The JSON file '{json_file}' was not found. Please check the path.")
        except json.JSONDecodeError:
            raise ValueError(f"Error: The JSON file '{json_file}' is not a valid JSON format. Please check its contents.")
    
    @staticmethod
    def get_bdf(buss_mapping, vf_pf, die_id):
        """Generates the BDF (Bus, Device, Function)."""

        if die_id not in buss_mapping:
            raise ValueError(f"Invalid die_id: {die_id}. Expected 0 or 1.")
        buss = buss_mapping[die_id]

        if 0 <= vf_pf < 9:
            device, function = f"{0:02x}", f"{vf_pf:02x}"
        elif 9 <= vf_pf < 17:
            device, function = f"{1:02x}", f"{vf_pf - 9:02x}"
        elif 17 <= vf_pf < 25:
            device, function = f"{2:02x}", f"{vf_pf - 17:02x}"
        elif 25 <= vf_pf < 33:
            device, function = f"{3:02x}", f"{vf_pf - 25:02x}"
        else:
            raise ValueError(f"Invalid vf_pf: {vf_pf}. Expected 0-32.")

        return f"{buss}{device}{function}"
