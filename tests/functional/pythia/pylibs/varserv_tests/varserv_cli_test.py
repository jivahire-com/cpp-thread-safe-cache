# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Test that checks for Variable Services SET GET Command.
"""

import sys, os
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))

from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup

from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

class varserv_cli_test(EchoFallsBaseTest):

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
        name: str = "Variable_Services_SET_GET",
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

    def varserv_cli_test(self):
        """ 
        Test function: 
            1. Setup the Test. 
            2. Wait for SCP to be up. Send 'SET' and 'GET' commands 
            3. Read response and check if test passed or failed based on response
            4. Teardown Test. 
        """
        self.log.info("Running Variable Services CLI command Test. . .")
        self.dut.setup()

        scp_channel=self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()
        # Open SCP channel
        scp_channel.open()
        if not scp_channel.is_open():
            scp_channel.close()
            self.test_notify(step="Open_Channels", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            return False
        
        try:
            # Wait for SOS Boot Completion message and enter commands
            self.log.info("Waiting for SOS BOOT  Msg")
            scp_channel.read_until(key="SOS boot completed", timeout_seconds=900)
        except Exception as e:
            self.log.error(f"Error reading self.dut.mb.node_0.soc.primary_die.scp.channel_manager UART: {e}")
            self.test_notify(step="ScpHeartBeat", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            return False

        # Submit Variable Services commands
        self.log.info("Submitting Variable Services command . . .") 
        command="variable_serv"
        scp_channel.write_line(write_string=command)

        # Run it twice for validation
        for attempt in range(2):
            command="set_var"
            self.log.info(f"Submitting {command}\n")
            scp_channel.write_line(write_string=command)

            try:
                guid = scp_channel.read_until(key="Async Set Variable Complete Notify", timeout_seconds=500)
                self.log.info("SET Command Complete Notify . . .")
                # Get GUID and store it for later validation
                set_guid = self.vendor_guid(guid)
                self.log.info(f"Submitting {set_guid}\n")
                if set_guid is None:
                    self.log.info("GUID for Set is NOT SUCCESSFUL")
                    scp_channel.close()
                    self.test_notify(step="Variable Services SET GET Command", msg="Test Fail", _is_error=True)
                    self.dut.teardown()
                    return False
                scp_channel.read_until(key="Async Set Variable Done", timeout_seconds=500)
                self.log.info("SET command executed Successfully . . .")
            except Exception as e:
                self.log.error(f"Error reading SCP UART: {e}")
                scp_channel.close()
                self.test_notify(step="Variable Services SET GET Command", msg="Test Fail", _is_error=True)
                self.dut.teardown()
                return False
            
            # Submit GET command
            command="get_var"
            self.log.info(f"Submitting {command}\n")
            scp_channel.write_line(write_string=command)

            try:
                guid = scp_channel.read_until(key="Async Get Variable Complete Notify", timeout_seconds=500)
                self.log.info("GET Command Complete Notify . . .")
                # Store GUID after issuing GET command and validate it with GUID on SET
                get_guid = self.vendor_guid(guid)
                self.log.info(f"Submitting {get_guid}\n")
                if get_guid is None:
                    self.log.info("GUID for Get is NOT SUCCESSFUL")
                    scp_channel.close()
                    self.test_notify(step="Variable Services SET GET Command", msg="Test Fail", _is_error=True)
                    self.dut.teardown()
                    return False
                if get_guid != set_guid:
                    self.log.info("GUID for Set and GET is NOT SUCCESSFUL")
                    scp_channel.close()
                    self.test_notify(step="Variable Services SET GET Command", msg="Test Fail", _is_error=True)
                    self.dut.teardown()
                    return False
                self.log.info("SET-GET GUID validation successful . . .")
                scp_channel.read_until(key="Async Get Variable Done", timeout_seconds=500)
                self.log.info("GET command executed Successfully . . .")
            except Exception as e:
                self.log.error(f"Error reading SCP UART: {e}")
                scp_channel.close()
                self.test_notify(step="Variable Services SET GET Command", msg="Test Fail", _is_error=True)
                self.dut.teardown()
                return False
        
        # Close connection to SCP
        scp_channel.close()
        self.test_notify(step="Variable Services SET GET Command", msg="Test Done", _is_error=False)
        self.dut.teardown()

        return True
    
    #Parse GUID information from SCP messages
    def vendor_guid(self, guid):
        for line in guid.splitlines():
            if "Vendor Guid" in line:
                value = line.split("Vendor Guid")[-1].strip()
                return value
        self.log.info("Vendor GUID NOT found . . .")
        return None
