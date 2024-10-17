# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Test that checks for Accelerator Fatal interrupts from SCP core.
"""
import copy
import json
import sys, os
from pathlib import Path
try:
    import prettytable
    print_table = True
except ModuleNotFoundError:
    print_table = False
import re
import time

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))

from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup

from pythia.tdk.common.connections.serial_connection import EmptySerialLinesError, SerialConnection

from pythia.tdk.echofalls.constants.dut_types import DeviceType

from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

class IntrExecCmd:
    def __init__(self, level1_cmd_info: dict = {}, level2_cmd_info: dict = {}, aux_cmd_info: list[dict] = [{}], pre_reset = False, post_reset = False) -> None:
        self.level1_cmd_info = level1_cmd_info
        self.level2_cmd_info = level2_cmd_info
        self.aux_cmd_info = aux_cmd_info
        self.pre_reset = pre_reset
        self.post_reset = post_reset

    def __str__(self) -> str:
        return f"level1_info: {self.level1_cmd_info}\nlevel2_info: {self.level2_cmd_info}\naux_info: {self.aux_cmd_info}"
    
class accel_interrupt_test(EchoFallsBaseTest):

    """
    :param name:                Name of the test case
    :param number:              ADO Number of the test case
    :param workspace_config:    Path to the workspace config file
    :param default_log_home:    Path for log storage
    :param fw_payload_path:     Path to the recipe payload
    :param dut_platform:        Platform used during the test case execution
    :param host_config:         Path to the host config file/directory
    :param host_name:           Name of the host to find the host config file, if host_config is a directory. Defaults to None
    """
    def __init__(
        self,
        name: str = None,
        number: str = "NaN",
        workspace_config: Path | str = None,
        default_log_home: str = None,
        fw_payload_path: str = None,
        dut_platform: str = "KingsgateSVP",
        host_config: Path | str = None,
        host_name: str | None = None,
        interrupt_config_path: Path = None,
        test_name: str = ""
    ):
        
        # Call parent class init
        super().__init__(
            name,
            number,
            workspace_config,
            default_log_home,
            fw_payload_path,
            dut_platform,
            host_config,
            host_name,
        )
        
        self.interrupt_config_path = interrupt_config_path
        self._test_results = []
        self._log_home = default_log_home
        self._test_name = name
        self.__test_config_path()

    def __test_config_path(self):
        """
        Load the interrupt configuration from the provided path.
        """
        try:
            with open(self.interrupt_config_path, 'r') as json_file:
                self.loaded_interrupts = json.load(json_file)
        except (FileNotFoundError, json.JSONDecodeError) as e:
            self.log.error(f"Failed to load interrupt configuration: {e}")
            raise

    # Parse the json values and create unqiue cmd elements for each level-1+level-2 per bit
    def __generate_exec_cmds(self):
        interrupt_exec_dict = {}
        
        for interrupt in self.loaded_interrupts["interrupts"] :

            if(interrupt["enabled"] != 1):
                continue

            level1_info = interrupt["level_1"]

            if("level_2" in interrupt):
                level2_info = interrupt["level_2"]
            else:
                level2_info = None
            
            interrupt_name = interrupt["name"]
            interrupt_exec_dict[interrupt_name] = []

            self.log.info(f"Generating tests for interrupt {interrupt_name}")
            
            level1_cmd_info = {"core": "", "mod": "", "cmd": "", "logs": {}}
            level1_post_reset = level1_pre_reset = False
            level2_cmd_info = {}
            level2_post_reset = level2_pre_reset = False

            if("bits" in level1_info):
                level1_start_bit, level1_end_bit = self.get_start_end_bits(level1_info["bits"])
                level1_fin_val = 0
                if("skip_bits" in level1_info):
                    for bit in range(level1_start_bit, level1_end_bit):
                        level1_fin_val |= self.set_bit(bit)
                    level1_fin_val = hex(level1_fin_val) # Merge all set bits into one value
                    level1_start_bit = 0
                    level1_end_bit = 1
            else:
                # Invalid scenario shouldn't come here even if only one bit is valid
                level1_start_bit = 0
                level1_end_bit = 1

            for level1_bit in range(level1_start_bit, level1_end_bit):

                curr_cmd_obj = IntrExecCmd()

                if(level1_fin_val == 0):
                    level1_reg_value = hex(self.set_bit(level1_bit))
                else:
                    level1_reg_value = level1_bit = level1_fin_val
                    
                lvl1_final_cmd = level1_info["cmd"] + " " + level1_reg_value

                level1_post_reset = level1_info["post_reset"] if("post_reset" in level1_info) else False
                level1_pre_reset = level1_info["pre_reset"] if("pre_reset" in level1_info) else False
                
                level1_cmd_info["core"] = level1_info["core"]
                level1_cmd_info["mod"] = level1_info["mod"]
                level1_cmd_info["cmd"] = lvl1_final_cmd
                

                for log_str in ["wait_logs", "pass_logs", "fail_logs"]:
                    if(log_str in level1_info):
                        level1_cmd_info["logs"][log_str] = list(level1_info[log_str])
                        for i in range(len(level1_info[log_str])):
                            log = level1_info[log_str][i]
                            if('{bits}' in log):
                                level1_info[log_str][i] = log.format(bits=level1_reg_value)
                
                curr_cmd_obj.level1_cmd_info = copy.deepcopy(level1_cmd_info)
                curr_cmd_obj.pre_reset = level1_pre_reset
                curr_cmd_obj.post_reset = level1_post_reset

                if(not level2_info):
                    interrupt_exec_dict[interrupt_name].append(curr_cmd_obj)
                    continue

                # level-2 can have multiple interrupts each with multiple bits and an aux command
                for level2_cmd in level2_info:

                    curr_cmd_obj = IntrExecCmd()
                    aux_cmd_info = []
                    
                    if("bits" in level2_cmd):
                        level2_start_bit, level2_end_bit = self.get_start_end_bits(level2_cmd["bits"])
                        level2_fin_val = 0
                        
                        if("skip_bits" in level2_cmd):
                            for bit in range(level2_start_bit, level2_end_bit):
                                level2_fin_val |= self.set_bit(bit)
                            level2_fin_val = hex(level2_fin_val) # Merge all set bits into one value
                            level2_start_bit = 0
                            level2_end_bit = 1
                        skip_exec = False
                    else:
                        level2_start_bit = 0
                        level2_end_bit = 1
                        level2_fin_val = 0
                        skip_exec = True
                            
                    for level2_bit in range(level2_start_bit, level2_end_bit):
                        if(not skip_exec):
                            if(level2_fin_val == 0):
                                level2_reg_value = hex(self.set_bit(level2_bit))
                            else:
                                level2_reg_value = level2_bit = level2_fin_val
                                
                            lvl2_final_cmd = level2_cmd["cmd"] + " " + level2_reg_value
                        else:
                            lvl2_final_cmd = ""

                        level2_cmd_info["logs"] = {}
                        for log_str in ["wait_logs", "pass_logs", "fail_logs"]:
                            if(log_str in level2_cmd):
                                level2_cmd_info["logs"][log_str] = list(level2_cmd[log_str])
                                for i in range(len(level2_cmd[log_str])):
                                    log = level2_cmd[log_str][i]
                                    if('{bits}' in log):
                                        level2_cmd_info["logs"][log_str][i] = log.format(bits=level2_reg_value)

                        level2_cmd_info["core"] = level2_cmd["core"]
                        level2_cmd_info["mod"] = level2_cmd["mod"]
                        level2_cmd_info["cmd"] = lvl2_final_cmd
                        level2_post_reset = level2_cmd["post_reset"] if("post_reset" in level2_cmd) else False
                        level2_pre_reset = level2_cmd["pre_reset"] if("pre_reset" in level2_cmd) else False

                        if("aux_cmds" in level2_cmd):
                            aux_cmd_info = list(level2_cmd["aux_cmds"])

                        curr_cmd_obj.level1_cmd_info = copy.deepcopy(level1_cmd_info)
                        curr_cmd_obj.level2_cmd_info = copy.deepcopy(level2_cmd_info)
                        curr_cmd_obj.aux_cmd_info = copy.deepcopy(aux_cmd_info)
                        curr_cmd_obj.post_reset = (level1_post_reset | level2_post_reset)
                        curr_cmd_obj.pre_reset = (level1_pre_reset | level2_pre_reset)
                        
                        interrupt_exec_dict[interrupt_name].append(copy.deepcopy(curr_cmd_obj))

        return interrupt_exec_dict

    def __test_notify_v2(self, *args):
        print(f"Test notification: {[val for val in args]}")
        # Test result is a list of format: [Interrupt name, level-1 bit, level-1 result, level-2 bit, level-2 result]
        self._test_results.append(args[0])
        
    def accel_interrupt_test(self):
        """ 
        Test function: 
            1. Setup the Test. 
            2. Wait for SCP, SDM and AP to be up.
            3. Execute the different interrupt test cases one by one.
            4. Parse logs to identify if test passed or failed.
            5. Teardown Test. 
        """
        try:
            test_result = True
            dut_setup = False
            
            if (self.dut.get_dut_type() != DeviceType.BIGFPGA):
                self.log.error("This test is supported only on BIGFPGA")
                # Returning true since we don't want to block SVP pipelines
                time.sleep(5) # Adding a sleep to avoid error during log folder creation by Pythia
                return True

            self.log.info("Generating test commands from JSON")
            interrupt_exec_dicts = self.__generate_exec_cmds()
            
            self.log.info("Executing SDM interrupt test")
            self.dut.setup()
            dut_setup = True

            proc_boot_info = {"scp": ("SCP_MAIN::ScpHeartBeat:", 100), "sdm": ("SDM_MAIN::SdmHeartBeat:", 100), 
                              "sdm_cded": ("SDM_MAIN::SdmHeartBeat:", 100), "apns": ("CLI Initialized & Started", 100)}
            
            self.log.info("Waiting for boot sequence to complete")
            KngPythiaTestSetup.reset_fpga_sideload_custom_seq(self.dut, proc_boot_info, self.log)

            for interrupt_name, interrupts_info in interrupt_exec_dicts.items():

                self.log.info(f"Executing test for interrrupt: {interrupt_name}")
                exec_level1 = True

                for interrupt_info in interrupts_info:
                    
                    read_op = ""
                    
                    if(interrupt_info.pre_reset):
                        self.log.info("Executing reset+boot sequence before level-1")
                        KngPythiaTestSetup.reset_fpga_sideload_custom_seq(self.dut, proc_boot_info, self.log)
                        exec_level1 = True

                    # Level - 1 will exist in all commands
                    # Execute level - 1 for first time for this list of interrupts or after a pre/post reset
                    if(exec_level1):
                        read_op += self.accel_interrupt_execute_cmd(interrupt_info.level1_cmd_info)
                        exec_level1 = False
                        
                        pass_logs_level1 = interrupt_info.level1_cmd_info["logs"]["pass_logs"] if("pass_logs" in interrupt_info.level1_cmd_info["logs"]) else []
                        fail_logs_level1 = interrupt_info.level1_cmd_info["logs"]["fail_logs"] if("fail_logs" in interrupt_info.level1_cmd_info["logs"]) else []
                        self.log.info("Parsing level1 command output logs")
                        level1_res = KngPythiaTestIF.parse_logs_v2(self.log, read_op, pass_logs_level1, fail_logs_level1)
                        test_result &= level1_res

                        if(("post_reset" in interrupt_info.level1_cmd_info) and (interrupt_info.level1_cmd_info["post_reset"])):
                            self.log.info("Executing reset+boot sequence after level-1")
                            KngPythiaTestSetup.reset_fpga_sideload_custom_seq(self.dut, proc_boot_info, self.log)

                    # Level - 2 is optional 
                    if(interrupt_info.level2_cmd_info != {}):
                        read_op += self.accel_interrupt_execute_cmd(interrupt_info.level2_cmd_info)
                    else:
                        exec_level1 = True # IF we're only executing level-1 we reset it
                        self.log.info(f"RESULTS: {interrupt_name} - Level-1 Result: {'PASS' if(level1_res) else 'FAIL'}")
                        self.__test_notify_v2([interrupt_name, interrupt_info.level1_cmd_info["cmd"], "PASS" if(level1_res) else "FAIL", "_", "_", "_"])
                        continue
                        
                    # Aux commands are optional
                    if(interrupt_info.aux_cmd_info != []):
                        # aux commands don't have log checks
                        for aux_cmd in interrupt_info.aux_cmd_info:
                            op = self.accel_interrupt_execute_cmd(aux_cmd)
                            if(aux_cmd["core"] == interrupt_info.level2_cmd_info["core"]):
                                read_op += op
                        
                        # read buffer for level2 core if aux commands are present
                        read_op += self.dut.mb.node_0.soc.primary_die.get_core(interrupt_info.level2_cmd_info["core"]).channel_manager.get_current_channel().clear_buffer()

                    pass_logs_level2 = interrupt_info.level2_cmd_info["logs"]["pass_logs"] if("pass_logs" in interrupt_info.level2_cmd_info["logs"]) else []
                    fail_logs_level2 = interrupt_info.level2_cmd_info["logs"]["fail_logs"] if("fail_logs" in interrupt_info.level2_cmd_info["logs"]) else []
                    self.log.info("Parsing level2 command output logs")
                    level2_res = KngPythiaTestIF.parse_logs_v2(self.log, read_op, pass_logs_level2, fail_logs_level2)
                    test_result &= level2_res

                    self.log.info(f"RESULTS: {interrupt_name} - Level-1 Result: {'PASS' if(level1_res) else 'FAIL'} - Level-2 Results:{'PASS' if(level2_res) else 'FAIL'}")
                    self.__test_notify_v2([interrupt_name, interrupt_info.level1_cmd_info["cmd"], "PASS" if(level1_res) else "FAIL", interrupt_info.level2_cmd_info["cmd"], "PASS" if(level2_res) else "FAIL", interrupt_info.aux_cmd_info])

                    if(interrupt_info.post_reset):
                        self.log.info("Executing reset+boot sequence after level-2")
                        exec_level1 = True
                        KngPythiaTestSetup.reset_fpga_sideload_custom_seq(self.dut, proc_boot_info, self.log)    
                        
            return test_result
        except:
            self.log.error("SDM_INT_EXEC")
            raise
        finally:
            if(print_table):
                self.log.info("Saving test results to pretty table")
                with open(os.path.join(self._log_home, f"{self._test_name}_test_res.txt"), "w") as f:
                    pt = prettytable.PrettyTable(["Interrupt Name", "Level-1 CMD", "Level-1 Result", "Level-2 CMD", "Level-2 Result", "Aux CMD"])
                    for row in self._test_results:
                        pt.add_row(row)
                    f.write(f"{pt}")
            if(dut_setup):
                self.dut.teardown()

    def get_start_end_bits(self, bits):
        """
        Parse bit range string and return start and end bits.

        :param bits: A string representing a bit or a range of bits.
        :return: A tuple (start, end) representing the bit range.
        """
        if "-" not in bits:
            return int(bits), int(bits) + 1
        else:
            start, end = re.findall(r"\d+", bits)
            return int(start), int(end) + 1

    def set_bit(self, bit):
        """
        Create a bitmask for a specific bit.

        :param bit: The bit position to set.
        :return: Bitmask with the specified bit set.
        """
        return (0x1 << bit)

    def accel_interrupt_execute_cmd(self, cmd_info: dict, skip_buff_clear=False) :
        """
        Run command on chosen channel

        :param core: Channel to run on
        :param cmd: Command to execute
        :return: Log after command is run
        """

        result = ""

        # For aux commands we may also need to do a t32 based mem read
        # We will halt and resume proc execution on our own
        if(("intf" in cmd_info) and (cmd_info["intf"] == "t32")):

            debug_obj = self.dut.mb.node_0.soc.primary_die.get_core(cmd_info["core"]).debugger
            
            debug_obj.execute_command("break")
            debug_obj.execute_command(cmd_info["cmd"])
            debug_obj.execute_command("go")
            
            time.sleep(2)
            
        else:
            
            proc = self.dut.mb.node_0.soc.primary_die.get_core(cmd_info["core"])
            proc_uart = proc.channel_manager.get_current_channel()

            self.log.info((f"accel_interrupt_execute_cmd: Running {cmd_info['cmd']}"))

            proc_uart.write_line(write_string=cmd_info["mod"])
            
            time.sleep(1)

            proc_uart.write_line(write_string=cmd_info["cmd"])

            if(("logs" in cmd_info) and ("wait_logs" in cmd_info["logs"])):
                for reg_str in cmd_info["logs"]["wait_logs"]:
                    self.log.info(f"Looking for {reg_str} after command request")
                    found, res = KngPythiaTestIF.read_until_regex(proc_uart, reg_str)
                    result += res
                    if(not found):
                        self.log.warning("Write regex string not found in cli logs, command may have failed")
                        self.log.warning(f"{result}")
                time.sleep(2) # Wait a couple of seconds in case
            else:
                time.sleep(10)
            
            proc_uart.write_line(write_string="..")

            if(not skip_buff_clear):
                result += proc_uart.clear_buffer()

        return result
