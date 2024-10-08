# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Test that checks the Accel Fatal Interrupts handling in SCP.
"""

import sys, os, re, json, time
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))

from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup

from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

from pythia.tdk.echofalls.constants.dut_types import DeviceType

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
        interrupt_config_path: Path | str = None,
    ):

        self.loaded_interrupts = None
        self.interrupt_config_path = interrupt_config_path

        if self.interrupt_config_path is not None:
            try:
                self.test_config_path()
            except Exception as e:
                self.log.error(f"Error loading interrupt config: {e}")

        # Call parent class init
        super().__init__(
            name,
            number,
            workspace_config,
            default_log_home,
            fw_payload_path,
            dut_platform,
            host_config,
            host_name
        )
    
    def test_config_path(self):
        """
        Load the interrupt configuration from the provided path.
        """
        try:
            with open(self.interrupt_config_path, 'r') as json_file:
                self.loaded_interrupts = json.load(json_file)
        except (FileNotFoundError, json.JSONDecodeError) as e:
            self.log.error(f"Failed to load interrupt configuration: {e}")

    def accel_interrupt_test(self, pass_logs, fail_logs, core="sdm"):
        """ 
        Test function: 
            1. Setup the Test. 
            2. Wait for SCP, AP and SDM to be up.
            3. Trigger each interrupt based on loaded_interrupts
            3. Check expected vs unexpected response and mark Pass / Fail
            4. Teardown Test. 
        """
        self.log.info("Running Accel Fatal Interrupt Test. . .")
        self.dut.setup()

        self.channels = {}

        self.channels["ap"]  = self.dut.mb.node_0.soc.primary_die.apns.channel_manager.get_current_channel()
        self.channels["scp"] = self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()

        if (core == "sdm") :
            self.channels["sdm"] = self.dut.mb.node_0.soc.primary_die.sdm.channel_manager.get_current_channel()
        else :
            self.channels["sdm"] = self.dut.mb.node_0.soc.primary_die.sdm_cded.channel_manager.get_current_channel()

        # Open all needed connections
        if (self.dut.get_dut_type() == DeviceType.BIGFPGA):
            KngPythiaTestSetup.reset_fpga_sideload_testfw(self.dut, self.log)

        elif (self.dut.get_dut_type() == DeviceType.SVP):
            self.channels["ap"].open()
            self.channels["ap"].is_open()
            self.channels["scp"].open()
            self.channels["scp"].is_open()
            self.channels["sdm"].open()
            self.channels["sdm"].is_open()

        else:
            self.log.error("Unsupported DUT type")
            self.dut.teardown()
            return False

        self.log.info("Reading SCP UART for HeartBeat . . .")
        try:
            self.channels["scp"].read_until(key="ScpHeartBeat", timeout_seconds=900)
        except Exception as e:
            self.log.error(f"Error reading SCP UART: {e}")
            self.channels["scp"].close()
            self.test_notify(step="ScpHeartBeat", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            return False

        self.log.info("Reading SDM UART for HeartBeat . . .")
        try:
            self.channels["sdm"].read_until(key="SdmHeartBeat", timeout_seconds=900)
        except Exception as e:
            self.log.error(f"Error reading SDM UART: {e}")
            self.channels["sdm"].close()
            self.test_notify(step="SdmHeartBeat", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            return False


        self.irq_num = self.loaded_interrupts["irq"][core]
        self.test_result = True

        for interrupt in self.loaded_interrupts["interrupts"] :
            
            # Execute an interrupt only when enabled
            if (interrupt["enabled"] == 1) :

                intr_name = interrupt["name"]

                self.log.info(f"Testing: {intr_name}")

                # Execute each trigger
                for trigger in interrupt["triggers"] :

                    common_parse = False

                    self.log.info(f"Trigger: Level {trigger}")

                    # Check for individual commands within a trigger
                    for cmd in trigger["cmds"] :

                        # When each command can be split in bits, results need to be checked after each
                        # Else result will be checked when array of commands are done
                        if "bits" in cmd :

                            start_bit, end_bit = self.get_start_end_bits(cmd["bits"])

                            for bit in range(start_bit, end_bit) :

                                reg_value = hex(self.set_bit(bit))
                                final_cmd = cmd["cmd"] + " " + reg_value
                                test_read = self.accel_interrupt_execute_cmd(core=cmd["core"], mod=cmd["mod"], cmd=final_cmd)

                                cur_test_result = self.accel_interrupt_parse_logs(self.irq_num, trigger, test_read, hex(self.set_bit(bit)))
                                self.test_result &= cur_test_result

                                if (cur_test_result == False) :
                                    self.test_notify(step=intr_name, msg="Test Fail", _is_error=True)
                                else :
                                    self.test_notify(step=intr_name, msg="Test Pass", _is_error=False)

                        else :

                            final_cmd = cmd["cmd"]
                            test_read = self.accel_interrupt_execute_cmd(core=cmd["core"], mod=cmd["mod"], cmd=final_cmd)
                            common_parse =True

                    if common_parse :
                        cur_test_result = self.accel_interrupt_parse_logs(self.irq_num, trigger, test_read, "")
                        self.test_result &= cur_test_result
                        if (cur_test_result == False) :
                            self.test_notify(step=intr_name, msg="Test Fail", _is_error=True)
                        else :
                            self.test_notify(step=intr_name, msg="Test Pass", _is_error=False)

        self.dut.teardown() 
        return self.test_result

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

    def accel_interrupt_execute_cmd(self, core, mod, cmd) :
        """
        Run command on chosen channel

        :param core: Channel to run on
        :param cmd: Command to execute
        :return: Log after command is run
        """
        self.log.info(f"accel_interrupt_execute_cmd: Running {mod} {cmd}")

        self.channels[core].write_line(write_string=mod)
        
        time.sleep(1)
        
        self.channels[core].write_line(write_string=cmd)

        time.sleep(20)

        self.channels[core].write_line(write_string="..")

        self.log.info(f"accel_interrupt_execute_cmd: Output {result}")

        return result

    def accel_interrupt_parse_logs(self, irq_num, trigger, test_read, bit) :

        if "exp_log" in trigger :

            for exp in trigger["exp_log"] :
                
                if '{irq_num}' in exp :
                    exp = exp.format(irq_num=irq_num, hex_bit=bit)

                if '{hex_bit}' in exp :
                    exp = exp.format(hex_bit=bit)

                self.log.info(f"accel_interrupt_parse_logs: Expected {exp}")

                matches = re.findall(exp, test_read)

                self.log.info(f"accel_interrupt_parse_logs: matches {matches}")

                if len(matches) == 0 :
                    self.log.info(f"accel_interrupt_parse_logs: Not found -> FAIL")
                    return False

        if "unexp_log" in trigger :

            unexp = trigger["unexp_log"]
 
            if '{irq_num}' in unexp :
                unexp = unexp.format(irq_num=irq_num, hex_bit=bit)

            if '{hex_bit}' in unexp :
                unexp = unexp.format(hex_bit=bit)

            self.log.info(f"accel_interrupt_parse_logs: Un-expected {unexp}")

            matches = re.findall(unexp, test_read)

            self.log.info(f"accel_interrupt_parse_logs: matches {matches}")

            if len(matches) != 0 :
                self.log.info(f"accel_interrupt_parse_logs: Found -> FAIL")
                return False
        
        self.log.info(f"accel_interrupt_parse_logs: -> PASS")

        return True