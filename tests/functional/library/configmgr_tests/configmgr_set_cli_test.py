# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Test that checks for Config Manager CLI set Command.
"""
import time
import re
import sys, os
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))

from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup
from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

class configmgr_set_cli_test(EchoFallsBaseTest):

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
        name: str = "Configmgr_cli_test_set",
        number: str = "NaN",
        workspace_config: Path | str = None,
        default_log_home: str = None,
        fw_payload_path: str = None,
        dut_platform: str = "KingsgateSVP",
        host_config: Path | str = None,
        host_name: str | None = None,
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
    
    def configmgr_set_cli_test(self):
        """ 
        Test function: 
            1. Setup the Test. 
            2. Wait for SCP to be up. Send 'set' command 
            3. Read response and check if test passed or failed based on response
            4. Teardown Test. 
        """
        self.log.info("Running Config Manager CLI command Test. . .")
        self.dut.setup()
        if self.dut.get_dut_type() == DeviceType.BIGFPGA:
            self.log.warning("Device type is bigFPGA. Performing an additional OOB reset ...")
            KngPythiaTestSetup.fpga_oob_reset(self.log)
            
        core_com_channel=self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()
        core_com_channel.open()
        assert core_com_channel.is_open()
        
        try:
            self.log.info("Waiting for Heartbeat Msg")
            core_com_channel.read_until(key="ScpHeartBeat", timeout_seconds=900)
        except Exception as e:
            self.log.error(f"Error reading self.dut.mb.node_0.soc.primary_die.scp.channel_manager UART: {e}")
            self.test_notify(step="ScpHeartBeat", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        self.log.info("Submitting Config Manager set command . . .")
        command="cfg_mgr"
        core_com_channel.write_line(write_string=command)

        command="cfg_mgr_list"
        self.log.info(f"Submitting LIST command {command} . . .") 
        core_com_channel.write_line(write_string=command)

        try:
            list_data = core_com_channel.read_until(key="System Knob Configuration Entities Completed", timeout_seconds=300)
            self.log.info("LIST command executed Successfully . . .")
            knob_name = "uint8_test_knob"
            index_value = self.get_knob_index(list_data,knob_name)
        except Exception as e:
            self.log.error(f"Error reading SCP UART: {e}")
            core_com_channel.close()
            self.test_notify(step="Config Manager LIST Command", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        self.log.info(f"INDEX Value: {index_value}\n")
        command=f"cfg_mgr_set {index_value} 3"
        self.log.info(f"Submitting SET command {command} . . .") 
        core_com_channel.write_line(write_string=command)

        try:
            data = core_com_channel.read_until(key="Knob configuration update completed", timeout_seconds=300)
            self.log.info("SET command executed Successfully . . .")
            set_data = self.validate_data(data)
            self.log.info(f"SET Value: {set_data}\n")
            if set_data is None:
                self.log.info("Data for SET is NOT SUCCESSFUL")
                core_com_channel.close()
                self.test_notify(step="Config Manager SET Command", msg="Test Fail", _is_error=True)
                self.dut.teardown()
                time.sleep(30)
                return False
        except Exception as e:
            self.log.error(f"Error reading SCP UART: {e}")
            core_com_channel.close()
            self.test_notify(step="Config Manager SET Command", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False
        
        command="cfg_mgr_dump uint8_test_knob"
        self.log.info(f"Submitting DUMP command {command} . . .") 
        core_com_channel.write_line(write_string=command)

        try:
            check = core_com_channel.read_until(key="Ok", timeout_seconds=300)
            self.log.info("DUMP command executed successfully . . .")
            # Store GUID after issuing GET command and validate it with GUID on SET
            get_data = self.validate_data(check)
            self.log.info(f"GET Value: {get_data}\n")
            if get_data is None:
                self.log.info("Data for Get is NOT SUCCESSFUL")
                core_com_channel.close()
                self.test_notify(step="Config Manager SET Command", msg="Test Fail", _is_error=True)
                self.dut.teardown()
                time.sleep(30)
                return False
            if get_data != set_data:
                self.log.info("Data byte for Set and GET validation is NOT SUCCESSFUL")
                core_com_channel.close()
                self.test_notify(step="Config Manager SET Command", msg="Test Fail", _is_error=True)
                self.dut.teardown()
                time.sleep(30)
                return False
            self.log.info("SET-DUMP Data byte validation is successful . . .")
        except Exception as e:
            self.log.error(f"Error reading SCP UART: {e}")
            core_com_channel.close()
            self.test_notify(step="Config Manager SET Command", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        core_com_channel.close()
        self.test_notify(step="Config Manager SET Command", msg="Test Done", _is_error=False)
        self.dut.teardown()
        time.sleep(30)

        return True

    #Parse knob index value from knob name    
    def get_knob_index(self, data, knob_name):
        match = re.search(rf"Knob Index\[(\d+)\], Name\[{re.escape(knob_name)}\]", data)
        if match:
            return match.group(1)
        return None

    #Parse data bytes information from SET message
    def validate_data(self, data):
        last_value = None
        lines = data.splitlines()
        for i, line in enumerate(lines):
            if "Current Knob Value" in line:
                # Check the next line for the value
                if i + 1 < len(lines):
                    next_line = lines[i + 1].strip()
                    # Extract digits from the next line
                    last_value = ''.join(filter(str.isdigit, next_line))
                    self.log.info(f"Digit found: {last_value}")
        
        if last_value:
            self.log.info(f"Data Byte found: {last_value}")
            return last_value
        else:
            self.log.info("Data Byte NOT found . . .")
            return None






