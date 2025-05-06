"""
- Python based Pythia 2.0 Test.
- This module contains setup, programming and teardown related utility functions for pythia functional tests.
"""

import os
import time
import socket
from pathlib import Path

from pythia.tdk.common.util.debugger.trace32 import Trace32
from pythia.tdk.echofalls.duts.bigfpga_dut import BigFpgaDut
from pythia.tdk.echofalls.constants.dut_types import DeviceType

FPGA_REVISION = "R22"
SDM_UART_MUX = 2
SDM_CDED_UART_MUX = 0
SDM_UART_INDEX = 2

class KngPythiaTestSetup():

    @staticmethod
    def reset_fpga(dut, log):

        script_path = "R:/Kingsgate/Kingsgate_TRACE32/Prep_R22/"
        script_name = "HSPresetSystem.cmm"
        script = Path(r"{}".format(os.path.join(script_path, script_name)))

        log.info("Resetting FPGA additionally (outside dut.setup()) . . .")
        log.info(f"Calling HSP CMM Script to reset SoC: {script}")
        hsp = dut.mb.node_0.soc.primary_die.get_core("hsp")
        hsp.debugger.execute_script(script)
        
        boot_time=90
        log.info(f"Waiting for FPGA to boot . . .")
        time.sleep(boot_time)

        log.info("Resetting FPGA and launching ProdFW Done!")

    # TODO: ADO: 2109203 Identify and breakup this in a more logical way (separate boot to scp heartbeat into one API)
    @staticmethod
    def reset_fpga_sideload_custom_seq(dut: BigFpgaDut, proc_info: dict, log):
        """
        proc_info is a dictionary of proc_name: ("expected_uart_logs", timeout)
        """
        
        # The reset flow is hardcoded for kingsgate
        script_path = path_to_fpga_launch_scripts = os.path.join(os.getcwd().split('.testlogs')[0],"tests/functional/cmm/")
        script_name = "PythiaLaunchFPGA.cmm"
        script = Path(r"{}".format(os.path.join(script_path, script_name)))

        # Open all uarts before resetting FPGA
        for proc_name, info in proc_info.items():
            proc_uart = dut.mb.node_0.soc.primary_die.get_core(proc_name).channel_manager.get_current_channel()
            proc_uart.open()
            proc_uart.clear_buffer()

        # Sleep added as Pythia is invoking AFM scripts per uart.open() - the sleep will avoid any possible conflicts
        time.sleep(5)
        log.info("Resetting FPGA and sideloading Test Firmware . . .")
        log.info(f"Calling HSP CMM Script to reset and load ProdFW: {script}")
        hsp = dut.mb.node_0.soc.primary_die.get_core("hsp")
        hsp.debugger.execute_script(script)
        
        boot_time=90
        log.info(f"Waiting {boot_time} seconds for FPGA to boot . . .")
        time.sleep(boot_time)

        for proc_name, info in proc_info.items():

            proc_uart = dut.mb.node_0.soc.primary_die.get_core(proc_name).channel_manager.get_current_channel()
            log.info(f'Waiting for boot string {info[0]} from proc {proc_name}')
            proc_uart.read_until(key=info[0], timeout_seconds=info[1])
            log.info("Received boot up string")
            
            # Move AFM programming post scp here
            if("scp" in proc_name):
                
                log.info("Executing SDM AFM programming")
                KngPythiaTestSetup.execute_accel_afm_seq(dut.mb.node_0.soc.primary_die.get_core(proc_name).debugger, 
                                                         SDM_UART_MUX, SDM_UART_INDEX)
                
                log.info("Executing SDM CDED AFM programming")
                KngPythiaTestSetup.execute_accel_afm_seq(dut.mb.node_0.soc.primary_die.get_core(proc_name).debugger, 
                                                         SDM_CDED_UART_MUX, SDM_UART_INDEX)
                
    @staticmethod
    def execute_accel_afm_seq(scp_debugger: Trace32, uart_mux: int, proc_index: int):
        
        soc_afm_script = f"R:/Kingsgate/Kingsgate_TRACE32/Tools/UART_AFM/SCPswitchUART{uart_mux}_AFM.cmm"
        
        scp_debugger.execute_command("break")
        time.sleep(1)
        
        scp_debugger.execute_script(script_path=soc_afm_script, arg=f"{proc_index}.")
        time.sleep(5)
        
        scp_debugger.execute_command("go")
        time.sleep(1)

    @staticmethod
    def fpga_oob_reset(log):
        #Create new file in the shared directory for the reset monitor listener
        log.info("Creating reset monitor listener file to trigger OOB reset...")
        fpga_system_name = socket.gethostname().lower()
        log.info(f"System Name: {fpga_system_name}")
        fpga_system_name = fpga_system_name.split('rdu-120015-')[-1]
        reset_monitor_file_path = Path("R:/haps_runtime/kingsgate_big_fpga/reset_tools/force_reset_" + fpga_system_name)
        reset_signal_file_path = Path("R:/haps_runtime/kingsgate_big_fpga/reset_tools/reset_signal/" + fpga_system_name)

        reset_monitor_file = reset_monitor_file_path / "reset"
        new_monitor_file = reset_monitor_file_path / ".newer"
        reset_signal_file = reset_signal_file_path / ".reset"

        # Check if the reset signal file exists, if yes then delete it
        if reset_signal_file.exists():
            reset_signal_file.unlink()
            log.info(f"Stale Reset signal file deleted at: {reset_signal_file}")

        if not new_monitor_file.exists():
            new_monitor_file.touch()
            log.info(f"Newer file created at: {new_monitor_file}")

        if reset_monitor_file.exists():
            reset_monitor_file.unlink()
            log.info(f"Stale Reset file deleted at: {reset_monitor_file}")
        time.sleep(5)

        reset_monitor_file.touch()
        log.info(f"Reset file created at: {reset_monitor_file}")
        time.sleep(5)
        reset_monitor_file.unlink()
        log.info(f"Reset file deleted at: {reset_monitor_file}")
        
        # Check if reset complete signal file is created with 30 second timeout
        log.info(f"Waiting for reset complete signal file to be created at: {reset_signal_file}")
        start_time = time.time()
        while True:
            if reset_signal_file.exists():
                log.info(f"Reset Completed by listener process.")
                break
            else:
                time.sleep(1)
                if time.time() - start_time > 30:
                    log.error(f"Timeout waiting for reset complete signal file to be created at: {reset_signal_file}")
                    break