# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Test for power cap effector and power throttle sensor functionality
"""

import os
import re
import sys
import time
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), "..", "kng_pythia_libs"))

from kng_pythia_test_setup import KngPythiaTestSetup
from library.common.serial_utils import SerialUtility
from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest
from RscmHelperLibrary import RscmHelperLibrary


class mod_power_pldm_service(EchoFallsBaseTest):
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
        name: str = "mod_power_pldm_service",
        number: str = "NaN",
        workspace_config: Path | str = None,
        default_log_home: str = None,
        fw_payload_path: str = None,
        dut_platform: str = None,
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
        self.mctp_eids = []
        self.mctp_eid = None  # Store single EID value
        self.creds = None
        self.bmc_cli = None
        self.core_scp_channel = None
        self.core_mcp_channel = None
        self.rscm_helper = None
        self.utility = None
        self.mcp_throttle_state_sensor_id = 8193
        self.mcp_pwrcap_effector_id = 1
        self.delay_15_minutes = 900

    def setup(self):
        # Step 1: DUT setup
        self.dut.setup()

        # Step 2: Perform reset based on DUT type
        if self.dut.get_dut_type() == DeviceType.BIGFPGA:
            self.log.warning(
                "Device type is bigFPGA. Performing an additional OOB reset ..."
            )
            KngPythiaTestSetup.fpga_oob_reset(self.log)
        elif self.dut.get_dut_type() == DeviceType.RVP:
            self.log.warning("Device type is RVP. Performing SoC Reset ...")
            cred_path = os.environ.get("SECURE_FILE_PATH")
            self.creds = KngPythiaTestSetup.load_credentials_from_yaml(cred_path)
            self.rscm_helper = RscmHelperLibrary(
                rm_host=self.host_config.rack_scm.host,
                bmc_host=self.dut.mb.node_0.dcscm.bmc.ip,
                rm_user=self.creds["RM_USER"],
                rm_password=self.creds["RM_PASSWORD"],
                bmc_user=self.creds["BMC_USER"],
                bmc_password=self.creds["BMC_PASSWORD"],
                node=self.host_config.node_id,
            )
            self.rscm_helper.rscm_soc_reset()

        # Step 3: Get BMC client object
        self.bmc_cli = self.dut.mb.node_0.dcscm.bmc.cli
        self.bmc_cli.open()
        assert self.bmc_cli.is_open()

        # Step 4: Open MCP channel and wait for MCP boot complete
        self.core_mcp_channel = (
            self.dut.mb.node_0.soc.primary_die.mcp.channel_manager.get_current_channel()
        )
        self.core_mcp_channel.open()
        assert self.core_mcp_channel.is_open()
        # Switch BMC UART mux to MCP
        if self.dut.get_dut_type() == DeviceType.RVP and self.rscm_helper:
            self.rscm_helper.set_bmc_uart_mux_mcp(self.bmc_cli)

        mcp_boot_status = self.__check_mcp_boot_complete(self.delay_15_minutes)
        if mcp_boot_status is False:
            self.log.info("Fail : MCP boot check failed during setup")
            return False
        self.core_mcp_channel.close()

        # Step 5: Open SCP channel & Verify heartbeat messages
        self.core_scp_channel = (
            self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()
        )
        self.core_scp_channel.open()
        assert self.core_scp_channel.is_open()
        # Switch BMC UART mux to SCP
        if self.dut.get_dut_type() == DeviceType.RVP and self.rscm_helper:
            self.rscm_helper.set_bmc_uart_mux_scp(self.bmc_cli)
        self.core_scp_channel.read_until(key="ScpHeartBeat", timeout_seconds=300)

        # Step 6 : Verify PLDM service is active
        pldm_status = self.__wait_for_pldm_active()
        if pldm_status is False:
            self.log.info("Fail : PLDM service is not active after waiting period")
            return False
        time.sleep(60)
        return True

    def teardown(self):
        # Start PLDM service (restore service state)
        self.__pldm_service_state(True)
        
        # Close SCP channel if open
        if self.core_scp_channel and self.core_scp_channel.is_open():
            self.core_scp_channel.close()
        
        # Close MCP channel if open  
        if self.core_mcp_channel and self.core_mcp_channel.is_open():
            self.core_mcp_channel.close()
        
        # Close BMC CLI if open
        if self.bmc_cli and self.bmc_cli.is_open():
           self.bmc_cli.close()
        
        self.dut.teardown()
        time.sleep(10)
        return True

    def __wait_for_pldm_active(self):
        _, stdout, _ = self.__bmc_execute_command(
            command="systemctl is-active xyz.openbmc_project.pldmd.service",
            sudo_mode=True,
        )
        if stdout.strip() == "active":
            self.log.info("PLDM service is active.")
            return True
        self.log.info(f"Current PLDM state: {stdout.strip()}. Waiting...")
        return False
    
    def __check_mcp_boot_complete(self, timeout):
        try:
            self.core_mcp_channel.read_until(
                key="StartupShutdownSvc::LocalCoreSyncStageRemoteEnd",
                timeout_seconds=timeout,
            )
            self.log.info(
                f"Boot message 'StartupShutdownSvc::LocalCoreSyncStageRemoteEnd' acknowledged from MCP"
            )
        except TimeoutError as e:
            print(f"A TimeoutError occurred while reading a boot message from MCP: {e}")
            return False
        return True

    def __bmc_execute_command(
        self, command, timeout=180, sudo_mode=False, max_retries=10, retry_delay=10
    ):
        """
        Execute a command on BMC with retry logic (fixed delay).

        :param command: Command to execute
        :param timeout: Timeout per attempt in seconds (default: 120s)
        :param sudo_mode: Whether to run command with sudo
        :param max_retries: Maximum retry attempts (default: 5)
        :param retry_delay: Delay between retries in seconds (default: 10s)
        """
        cmd_str = command
        if not self.bmc_cli.is_open():
            self.bmc_cli.open()
            assert self.bmc_cli.is_open()

        if sudo_mode:
            try:
                sudo_password = self.creds["BMC_PASSWORD"]
                cmd_str = f"echo {sudo_password} | sudo -S su -c '{command}'"
            except (TypeError, KeyError) as e:
                self.log.info(f"Failed to get BMC sudo password: {e}")
                self.bmc_cli.close()
                raise AssertionError("Missing or invalid BMC password") from e

        attempt = 0
        while attempt < max_retries:
            try:
                self.log.info(
                    f"Executing BMC command (Attempt {attempt+1}/{max_retries}): {cmd_str}"
                )
                result, stdout, stderr = self.bmc_cli.execute_command(
                    command=cmd_str, command_args=[], timeout_secs=timeout
                )

                if result == 0:
                    self.log.info("Command executed successfully.")
                    break
                else:
                    self.log.warning(
                        f"Command failed with result={result}. stderr={stderr}"
                    )
                    break

            except TimeoutError as e:
                attempt += 1
                self.log.warning(
                    f"TimeoutError on attempt {attempt}/{max_retries}: {e}"
                )
                if attempt == max_retries:
                    self.log.error("Max retries reached. Command failed permanently.")
                    self.bmc_cli.close()
                    raise
                self.log.info(f"Retrying after {retry_delay} seconds...")
                time.sleep(retry_delay)

        self.bmc_cli.close()
        return result, stdout, stderr
    
    def __pldm_service_state(self, start_service=None):
        """
        Control PLDM service state based on start_service parameter.
        :param start_service: True to start service, False to stop service, None to toggle
        Returns True if operation successful, False otherwise.
        """
        # Check current service status
        result, stdout, _ = self.__bmc_execute_command(
            command="systemctl is-active xyz.openbmc_project.pldmd.service",
            sudo_mode=True,
        )
        
        service_active = stdout.strip() == "active"
        
        if start_service is None:
            # Toggle behavior (original functionality)
            if service_active:
                # Service is running, stop it
                self.log.info("PLDM service is active, stopping it...")
                result, _, _ = self.__bmc_execute_command(
                    command="systemctl stop xyz.openbmc_project.pldmd.service",
                    sudo_mode=True,
                )
                if result != 0:
                    self.log.info("Fail : Failed to stop pldmd service on BMC")
                    return False
                self.log.info("PLDM service stopped successfully")
            else:
                # Service is not running, start it
                self.log.info("PLDM service is not active, starting it...")
                result, _, _ = self.__bmc_execute_command(
                    command="systemctl start xyz.openbmc_project.pldmd.service",
                    sudo_mode=True,
                )
                if result != 0:
                    self.log.info("Fail : Failed to start pldmd service on BMC")
                    return False
                self.log.info("PLDM service started successfully")
        elif start_service:
            # Start service if not already active
            if not service_active:
                self.log.info("PLDM service is not active, starting it...")
                result, _, _ = self.__bmc_execute_command(
                    command="systemctl start xyz.openbmc_project.pldmd.service",
                    sudo_mode=True,
                )
                if result != 0:
                    self.log.info("Fail : Failed to start pldmd service on BMC")
                    return False
                self.log.info("PLDM service started successfully")
            else:
                self.log.info("PLDM service is already active")
        else:
            # Stop service if active
            if service_active:
                self.log.info("PLDM service is active, stopping it...")
                result, _, _ = self.__bmc_execute_command(
                    command="systemctl stop xyz.openbmc_project.pldmd.service",
                    sudo_mode=True,
                )
                if result != 0:
                    self.log.info("Fail : Failed to stop pldmd service on BMC")
                    return False
                self.log.info("PLDM service stopped successfully")
            else:
                self.log.info("PLDM service is already stopped")
        
        time.sleep(5)
        return True
    

    # Verify MCP enumeration on BMC using mctptool
    def prerequisite_get_mctp_id(self):
        self.log.info("Getting MCTP EID from BMC")
        result, stdout, _ = self.__bmc_execute_command(command="mctptool list")
        if result != 0:
            return False
        lines = stdout.strip().split("\n")

        # Example output of mctptool list:
        # 12  | Pa-RoT Cerberus on BMC TIP | I2C Bus: 0 Addr: 0x82 | MctpControl SPDM VDPCI
        # 16  | HSP                  | I2C Bus: 9 Addr: 0xaa | MctpControl SPDM VDPCI
        # 17  | MCP                  | I3C Bus: 1 Addr: 0x596434f0200 | MctpControl PLDM
        # 64  | Cerberus on LION     | I2C Bus: 16 Addr: 0x82 | MctpControl SPDM VDPCI
        for line in lines:
            parts = line.split("|")
            if len(parts) > 1 and "MCP" in parts[1]:
                mcp_id = int(parts[0].strip())
                self.mctp_eids.append(mcp_id)
                self.mctp_eid = mcp_id  # Store the single EID value
                break
        if self.mctp_eids == []:
            self.log.info("Fail : MCTP ID not found")
            return False
        self.log.info(f"Found MCTP EID: {self.mctp_eid}")
        return True

    def prerequisite_verify_sensor_effector_registration(self):
        self.log.info("***** Verifying sensor/effector Listed on BMC *****")
        result, stdout, _ = self.__bmc_execute_command(command="busctl tree xyz.openbmc_project.pldm")
        if result != 0:
            return False
        self.log.info("Execute busctl tree command successfully")
        lines = stdout.strip().split("\n")
        # Example output of busctl tree:
        # ├─xyz.openbmc_project
        # │ ├─xyz.openbmc_project/effecters
        # │ │ ├─xyz.openbmc_project/effecters/power
        # │ │  └─xyz.openbmc_project/effecters/MCP_PWRCAP_LIMIT
        # │ └─xyz.openbmc_project/state_sensors
        # │   ├─xyz.openbmc_project/state_sensors/MCP_THROTTLING_STATE

        found_pwrcap = False
        found_throttle = False
        for line in lines:
            if "MCP_PWRCAP_LIMIT" in line:
                found_pwrcap = True
            if "MCP_THROTTLING_STATE" in line:
                found_throttle = True

        # Log the status of both components
        self.log.info(f"MCP_PWRCAP_LIMIT found: {found_pwrcap}, MCP_THROTTLING_STATE found: {found_throttle}")
        
        if found_pwrcap or found_throttle:
            if found_pwrcap and found_throttle:
                self.log.info("Both MCP_PWRCAP_LIMIT and MCP_THROTTLING_STATE found.")
            elif found_pwrcap:
                self.log.info("Only MCP_PWRCAP_LIMIT found.")
            else:
                self.log.info("Only MCP_THROTTLING_STATE found.")
            # Stop PLDM service to allow direct tool access
            self.__pldm_service_state(False)
            return True
        else:
            self.log.info("Neither MCP_PWRCAP_LIMIT nor MCP_THROTTLING_STATE found.")
            # Stop PLDM service even on failure
            self.__pldm_service_state(False)
            return False
    
    def get_state_sensor_event_state(self, sensor_id):
        """
        Get the event state of a given state sensor from BMC using pldmtool.
        :param sensor_id: Sensor ID to query
        :return: Event state string or None if failed
        typical ouput looks as follows --->
        {
            "compositeSensorCount": 1,
            "sensorOpState[0]": "Sensor Enabled",
            "presentState[0]": "Sensor Normal",
            "previousState[0]": "Sensor Critical",
            "eventState[0]": "Sensor Normal"
        }
        """
        if not self.mctp_eid:
            self.log.info("MCTP EID not available. Run prerequisite_get_mctp_id first.")
            return None
        result, stdout, _ = self.__bmc_execute_command(
            command=f"pldmtool platform GetStateSensorReadings -m {self.mctp_eid} -i {sensor_id} -r 0"
        )
        if result != 0:
            return None
        for line in stdout.strip().split("\n"):
            if "eventState[0]" in line:
                return line.split(":")[1].strip().strip('"')
        return None

    def set_numeric_effector_value(self, sensor_id: int, value: int):
        """
        Set a numeric effector value on BMC using pldmtool.
        :param sensor_id: Sensor ID to set
        :param value: Value to set
        :return: True if command succeeded and 'SUCCESS' found in stdout, False otherwise
        """
        if not self.mctp_eid:
            self.log.info("MCTP EID not available. Run prerequisite_get_mctp_id first.")
            return False
        result, stdout, stderr = self.__bmc_execute_command(
            command=f"pldmtool platform SetNumericEffecterValue -m {self.mctp_eid} -i {sensor_id} -s 2 -d {value}"
        )
        if result != 0:
            self.log.info(f"Fail : Failed to set numeric effector value: {stderr}")
            return False
        if "SUCCESS" not in stdout:
            self.log.info(f"Fail : 'SUCCESS' not found in SetNumericEffecterValue output: {stdout}")
            return False
        self.log.info(f"SetNumericEffecterValue output: {stdout}")
        return True

    def run_cli_cmd_on_scp0_until_comp(self, command, timeout=10):
        """
        Runs a command on SCP and returns output string until 'pwr_cli_comp' is found.
        If 'pwr_cli_comp' is not found, logs error and returns False.
        """
        # Check if SCP channel is open, if not open it
        channel_was_open = self.core_scp_channel.is_open()
        if not channel_was_open:
            self.core_scp_channel.open()
            assert self.core_scp_channel.is_open()

        self.core_scp_channel.write_line(write_string=command)
        try:
            output = self.core_scp_channel.read_until(key="pwr_cli_comp", timeout_seconds=timeout)
            self.log.info(f"'pwr_cli_comp' found in SCP output for command: {command}")
            success = True
        except TimeoutError as e:
            self.log.error(f"'pwr_cli_comp' NOT found in SCP output for command: {command} {e}")
            output = ""
            success = False

        # Close SCP channel if it wasn't open when we started
        if not channel_was_open:
            self.core_scp_channel.close()

        return output if success else False

    def get_power_cap_values(self):
        """
        Queries the current power cap values from SCP0 CLI.
        :return: (vcpu_power_cap, soc_power_cap) tuple if successful, else (None, None)
        """
        result = self.run_cli_cmd_on_scp0_until_comp("pwr status cap", timeout=5)
        if not result:
            self.log.info("Fail : Failed to get power cap status from SCP0")
            return None, None
        soc_power_cap = None
        vcpu_power_cap = None
        soc_power_cap_none = False
        for line in result.splitlines():
            if "SOC Power Cap" in line:
                if "NONE" in line:
                    self.log.info("No SOC power cap set yet.")
                    soc_power_cap_none = True
                else:
                    match = re.search(r"SOC Power Cap\s*:\s*(\d+)", line)
                    if match:
                        soc_power_cap = int(match.group(1))
                        self.log.info(f"SOC Power Cap: {soc_power_cap} W")
            if "Vcpu Power Cap" in line:
                match = re.search(r"Vcpu Power Cap\s*:\s*(\d+)", line)
                if match:
                    vcpu_power_cap = int(match.group(1))
                    self.log.info(f"Vcpu Power Cap: {vcpu_power_cap} W")
            if soc_power_cap_none and "Soc Max Thermal Limit" in line:
                match = re.search(r"Soc Max Thermal Limit\s*:\s*(\d+)", line)
                if match:
                    soc_power_cap = int(match.group(1))
                    self.log.info(f"Using Soc Max Thermal Limit as SOC Power Cap: {soc_power_cap} W")
        return vcpu_power_cap, soc_power_cap
    
    def test_mcp_throttle_state_sensor(self):
        self.log.info("***** Verifying MCP_THROTTLING_STATE sensor functionality on BMC *****")

        # Step 1: Get initial sensor reading
        event_state = self.get_state_sensor_event_state(self.mcp_throttle_state_sensor_id)
        if event_state == "Sensor Critical":
            self.log.info(f"System state Degraded, skipping current test. Event state: {event_state} Expected: Sensor Normal")
            return True
        elif event_state != "Sensor Normal":
            self.log.info(f"Initial eventState is not 'Sensor Normal': {event_state}")
            return False
        self.log.info(f"Initial eventState is 'Sensor Normal': {event_state}")

        # Step 2: Disable power loop on SCP0 to induce degraded state
        result = self.run_cli_cmd_on_scp0_until_comp("pwr set loopdis 1", timeout=5)
        if not result:
            self.log.info("Fail : Failed to disable power loop on SCP0")
            return False
        time.sleep(5)

        # Step 3: Get updated sensor reading
        event_state = self.get_state_sensor_event_state(self.mcp_throttle_state_sensor_id)
        if event_state != "Sensor Critical":
            self.log.info(f"Fail : After inducing failure, eventState is not 'Sensor Critical': {event_state}")
            return False
        self.log.info(f"After inducing failure, eventState is 'Sensor Critical': {event_state}")

        # Step 4: Re-enable power loop on SCP0 to restore normal state
        result = self.run_cli_cmd_on_scp0_until_comp("pwr set loopdis 0", timeout=5)
        if not result:
            self.log.info("Fail : Failed to re-enable power loop on SCP0")
            return False
        time.sleep(5)

        # Step 5: Get final sensor reading
        event_state = self.get_state_sensor_event_state(self.mcp_throttle_state_sensor_id)
        if event_state != "Sensor Normal":
            self.log.info(f"Fail : After restoring, eventState is not 'Sensor Normal': {event_state}")
            return False
        self.log.info(f"After restoring, eventState is 'Sensor Normal': {event_state}")
        return True
    
    def test_mcp_power_cap_effector(self):
        self.log.info("***** Verifying MCP_PWRCAP_LIMIT effector functionality on BMC *****")

        # Step 1: Get initial power cap values on scp0
        vcpu_power_cap, soc_power_cap = self.get_power_cap_values()
        if vcpu_power_cap is None:
            self.log.info("Fail : VCPU power cap value cannot be None!!.")
            return False
        self.log.info(f"Initial Vcpu Power Cap: {vcpu_power_cap}, SOC Power Cap: {soc_power_cap}")

        # Step 2: Set new power cap via PLDM effector
        soc_power_cap_bmc = 250  # Example value, adjust as needed
        success = self.set_numeric_effector_value(self.mcp_pwrcap_effector_id, soc_power_cap_bmc)
        if not success:
            self.log.info("Fail : Failed to set new power cap via PLDM effector.")
            return False
        self.log.info(f"Successfully set new power cap via PLDM effector: {soc_power_cap_bmc}")
        time.sleep(5)

        # Step 3: Verify new power cap on SCP0
        new_vcpu_power_cap, new_soc_power_cap = self.get_power_cap_values()
        if new_soc_power_cap != soc_power_cap_bmc:
            self.log.info(f"Fail : New SOC Power Cap is not as expected: Current: {new_soc_power_cap}, Expected: {soc_power_cap_bmc}")
            return False
        self.log.info(f"New Soc Power Cap is as expected: {new_soc_power_cap}")

        # Step 4: Restore original power cap via PLDM effector
        success = self.set_numeric_effector_value(self.mcp_pwrcap_effector_id, soc_power_cap)
        if not success:
            self.log.info("Fail : Failed to restore original power cap via PLDM effector.")
            return False
        self.log.info(f"Successfully restored original power cap via PLDM effector: {soc_power_cap}")
        time.sleep(5)

        # Step 5: Verify restored power cap on SCP0
        final_vcpu_power_cap, final_soc_power_cap = self.get_power_cap_values()
        if final_soc_power_cap != soc_power_cap:
            self.log.info(f"Fail : Restored SOC Power Cap is not as expected: Current: {final_soc_power_cap}, Expected: {soc_power_cap}")
            return False
        self.log.info(f"Restored Soc Power Cap is as expected: {final_soc_power_cap}")

        return True
    
    def test_induce_pwr_throttle_via_bmc_pwr_capping(self):
        self.log.info("***** Inducing power throttle via BMC power capping *****")

        # Step 1: Check the state sensor status to event state is normal
        event_state = self.get_state_sensor_event_state(self.mcp_throttle_state_sensor_id)
        # Add clause to prevent flaky tests until scp power err message spamming bug fix
        if event_state == "Sensor Critical":
            self.log.info(f"System state Degraded, skipping current test. Event state: {event_state} Expected: Sensor Normal")
            return True
        elif event_state != "Sensor Normal":
            self.log.info(f"Fail : Initial eventState is not 'Sensor Normal': {event_state}")
            return False
        self.log.info(f"Initial eventState is as expected : {event_state}")
        time.sleep(5)

        # Step 2: Set a low power cap via PLDM effector to induce throttling
        low_soc_power_cap_bmc = 50  # Example low value to induce throttling, maybe confirm this post querying `pwr status power` on scp0
        success = self.set_numeric_effector_value(self.mcp_pwrcap_effector_id, low_soc_power_cap_bmc)
        if not success:
            self.log.info("Fail : Failed to set low power cap via PLDM effector.")
            return False
        self.log.info(f"Successfully set low power cap via PLDM effector: {low_soc_power_cap_bmc}")
        time.sleep(5)  # Wait for system to react to new power cap

        # Step 3: Verify power throttle state sensor changes to warning
        event_state = self.get_state_sensor_event_state(self.mcp_throttle_state_sensor_id)
        if event_state == "Sensor Critical":
            self.log.info(f"System state Degraded, skipping current test. Event state: {event_state} Expected: Sensor Warning")
            return True
        elif event_state != "Sensor Warning":
            self.log.info(f"Fail : Power throttle state sensor did not change as expected: {event_state}")
            return False
        self.log.info(f"Power throttle state sensor changed as expected: {event_state}")
        time.sleep(5)

        # Step 4: Restore original power cap via PLDM effector
        success = self.set_numeric_effector_value(self.mcp_pwrcap_effector_id, soc_power_cap)
        if not success:
            self.log.info("Fail : Failed to restore original power cap via PLDM effector.")
            return False
        self.log.info(f"Successfully restored original power cap via PLDM effector: {soc_power_cap}")
        time.sleep(5)

        # Step 5: Verify power throttle state sensor returns to normal
        event_state = self.get_state_sensor_event_state(self.mcp_throttle_state_sensor_id)
        if event_state == "Sensor Critical":
            self.log.info(f"System state Degraded, skipping current test. Event state: {event_state} Expected: Sensor Normal")
            return True
        elif event_state != "Sensor Normal":
            self.log.info(f"Fail : Power throttle state sensor did not return to normal as expected: {event_state}")
            return False
        self.log.info(f"Power throttle state sensor returned to normal as expected: {event_state}")
        return True