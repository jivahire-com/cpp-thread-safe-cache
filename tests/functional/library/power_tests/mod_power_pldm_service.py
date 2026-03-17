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
        self.current_scp_die = 0  # Track which die the SCP UART is currently routed to
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
            hsp_channel=self.dut.mb.node_0.soc.primary_die.hsp.channel_manager.get_current_channel()
            hsp_channel.open()
            assert hsp_channel.is_open()

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

        time.sleep(60)
        return True

    def teardown(self):
        # Restore SCP UART to die 0 if it was switched
        try:
            if self.current_scp_die != 0:
                self.__switch_scp_uart_to_die(0)
        except Exception as e:
            self.log.warning(f"Failed to restore SCP UART to die 0: {e}")

        # Close SCP channel if open
        try:
            if self.core_scp_channel and self.core_scp_channel.is_open():
                self.core_scp_channel.close()
        except Exception as e:
            self.log.warning(f"Failed to close SCP channel: {e}")
        
        # Close MCP channel if open  
        try:
            if self.core_mcp_channel and self.core_mcp_channel.is_open():
                self.core_mcp_channel.close()
        except Exception as e:
            self.log.warning(f"Failed to close MCP channel: {e}")
        
        # Close BMC CLI if open
        self.__close_bmc_safe()
        
        try:
            self.dut.teardown()
        except Exception as e:
            self.log.warning(f"Failed DUT teardown: {e}")
        
        time.sleep(10)
        return True

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
        Execute a command on BMC with retry logic.
        Keeps the SSH tunnel alive across commands to avoid exhausting the rack
        manager's SSH session limit. Reconnects only when the channel is dead.

        :param command: Command to execute
        :param timeout: Timeout per attempt in seconds (default: 180s)
        :param sudo_mode: Whether to run command with sudo
        :param max_retries: Maximum retry attempts (default: 10)
        :param retry_delay: Delay between retries in seconds (default: 10s)
        """
        cmd_str = command

        if sudo_mode:
            try:
                sudo_password = self.creds["BMC_PASSWORD"]
                cmd_str = f"echo {sudo_password} | sudo -S su -c '{command}'"
            except (TypeError, KeyError) as e:
                self.log.info(f"Failed to get BMC sudo password: {e}")
                raise AssertionError("Missing or invalid BMC password") from e

        attempt = 0
        while attempt < max_retries:
            try:
                # Ensure BMC connection is alive before each attempt
                self.__ensure_bmc_connection()

                self.log.info(
                    f"Executing BMC command (Attempt {attempt+1}/{max_retries}): {cmd_str}"
                )
                result, stdout, stderr = self.bmc_cli.execute_command(
                    command=cmd_str, command_args=[], timeout_secs=timeout
                )

                # result == -1 with stderr containing "Password:" means the SSH
                # channel is stale (sudo prompt couldn't be satisfied). Treat as
                # retriable by forcing a reconnect.
                if result == -1 and stderr and "Password" in stderr:
                    raise RuntimeError(f"Stale SSH session (sudo prompt failed): stderr={stderr}")

                if result == 0:
                    self.log.info("Command executed successfully.")
                    break
                else:
                    self.log.warning(
                        f"Command failed with result={result}. stderr={stderr}"
                    )
                    break

            except (TimeoutError, Exception) as e:
                attempt += 1
                # Close the stale connection so next retry gets a fresh one
                self.__close_bmc_safe()
                self.log.warning(
                    f"Error on attempt {attempt}/{max_retries} ({type(e).__name__}): {e}"
                )
                if attempt == max_retries:
                    self.log.error("Max retries reached. Command failed permanently.")
                    raise
                self.log.info(f"Retrying after {retry_delay} seconds...")
                time.sleep(retry_delay)

        # Keep the connection open — reuse across subsequent commands.
        # Only close on error (above) or in teardown().
        return result, stdout, stderr

    def __ensure_bmc_connection(self):
        """
        Ensure the BMC CLI SSH connection is alive and usable.
        If the connection is closed or stale, close it cleanly and reopen.
        This prevents cascading failures from a dead SSH tunnel poisoning
        subsequent tests.
        """
        try:
            if self.bmc_cli.is_open():
                # Connection appears open — verify it's actually alive
                # by checking if the underlying transport is active
                transport = getattr(self.bmc_cli, '_ssh_client', None)
                if transport:
                    inner = getattr(transport, '_transport', None) or getattr(transport, 'get_transport', lambda: None)()
                    if inner and not inner.is_active():
                        self.log.warning("BMC CLI transport is dead. Reconnecting...")
                        self.__close_bmc_safe()
                        self.bmc_cli.open()
                        assert self.bmc_cli.is_open()
                        self.log.info("BMC CLI reconnected successfully.")
                # else: transport looks fine, no action needed
            else:
                # Connection is closed — open it
                self.log.info("BMC CLI is closed. Opening connection...")
                self.bmc_cli.open()
                assert self.bmc_cli.is_open()
                self.log.info("BMC CLI opened successfully.")
        except Exception as e:
            # If anything goes wrong during the check, force close and reopen
            self.log.warning(f"BMC connection health check failed ({type(e).__name__}): {e}. Force reconnecting...")
            self.__close_bmc_safe()
            try:
                self.bmc_cli.open()
                assert self.bmc_cli.is_open()
                self.log.info("BMC CLI force-reconnected successfully.")
            except Exception as e2:
                self.log.error(f"BMC CLI force-reconnect failed: {e2}")
                raise

    def __close_bmc_safe(self):
        """
        Safely close the BMC CLI connection, suppressing any errors.
        """
        try:
            if self.bmc_cli and self.bmc_cli.is_open():
                self.bmc_cli.close()
        except Exception:
            pass
    
    # Verify MCP enumeration on BMC using mctptool
    def prerequisite_get_mctp_id(self):
        self.log.info("Getting MCTP EID from BMC")

        # MCP MCTP enumeration can take additional time after boot completes.
        # Retry mctptool list with a delay to allow MCP to register on the BMC.
        max_attempts = 6
        retry_delay = 30  # seconds between retries

        for attempt in range(max_attempts):
            result, stdout, _ = self.__bmc_execute_command(command="mctptool list")
            if result != 0:
                self.log.info(f"mctptool list failed on attempt {attempt + 1}/{max_attempts}")
                if attempt < max_attempts - 1:
                    self.log.info(f"Retrying in {retry_delay} seconds...")
                    time.sleep(retry_delay)
                continue

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

            if self.mctp_eid:
                self.log.info(f"Found MCTP EID: {self.mctp_eid} (attempt {attempt + 1}/{max_attempts})")
                return True

            self.log.info(f"MCP not found in MCTP enumeration (attempt {attempt + 1}/{max_attempts})")
            if attempt < max_attempts - 1:
                self.log.info(f"MCP may still be registering. Retrying in {retry_delay} seconds...")
                time.sleep(retry_delay)

        self.log.info(f"Fail : MCP MCTP ID not found after {max_attempts} attempts ({max_attempts * retry_delay}s)")
        return False

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
            return True
        else:
            self.log.info("Neither MCP_PWRCAP_LIMIT nor MCP_THROTTLING_STATE found.")
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

    def __run_cli_cmd_on_scp_until_comp(self, channel, command, timeout=10, die_label="SCP0"):
        """
        Runs a command on a given SCP channel and returns output until 'pwr_cli_comp' is found.
        :param channel: The SCP channel object to use
        :param command: CLI command to send
        :param timeout: Timeout in seconds for read_until
        :param die_label: Label for log messages (e.g., "SCP0" or "SCP1")
        :return: Output string if 'pwr_cli_comp' found, False otherwise
        """
        channel_was_open = channel.is_open()
        if not channel_was_open:
            channel.open()
            assert channel.is_open()

        channel.write_line(write_string=command)
        try:
            output = channel.read_until(key="pwr_cli_comp", timeout_seconds=timeout)
            self.log.info(f"'pwr_cli_comp' found in {die_label} output for command: {command}")
            success = True
        except TimeoutError as e:
            self.log.error(f"'pwr_cli_comp' NOT found in {die_label} output for command: {command} {e}")
            output = ""
            success = False

        if not channel_was_open:
            channel.close()

        return output if success else False

    def run_cli_cmd_on_scp0_until_comp(self, command, timeout=10):
        """
        Runs a command on SCP0 (primary die) and returns output until 'pwr_cli_comp' is found.
        If 'pwr_cli_comp' is not found, logs error and returns False.
        """
        if self.current_scp_die != 0:
            self.__switch_scp_uart_to_die(0)
        return self.__run_cli_cmd_on_scp_until_comp(self.core_scp_channel, command, timeout, die_label="SCP0")

    def run_cli_cmd_on_scp1_until_comp(self, command, timeout=10):
        """
        Runs a command on SCP1 (secondary die) via UART mux switching.
        Switches UART to die 1, runs the command, then switches back to die 0.
        If 'pwr_cli_comp' is not found, logs error and returns False.
        """
        if self.current_scp_die != 1:
            self.__switch_scp_uart_to_die(1)
        result = self.__run_cli_cmd_on_scp_until_comp(self.core_scp_channel, command, timeout, die_label="SCP1")
        # Switch back to die 0 so other tests/commands default to SCP0
        self.__switch_scp_uart_to_die(0)
        return result

    def __switch_scp_uart_to_die(self, die_id):
        """
        Switch the SCP UART to the specified die by toggling via MCP.
        Uses BMC GPIO to mux between SCP/MCP, and MCP 'afm uart_die' to select the die.

        Steps:
          1. BMC: gpioset gpiochip3 5=1  (route UART to MCP)
          2. Verify 'MCP_MAIN' appears on channel
          3. MCP CLI: afm uart_die <die_id> 0, wait for 'Ok'
          4. BMC: gpioset gpiochip3 5=0  (route UART back to SCP)
          5. Verify 'SCP_MAIN' appears on channel
          6. SCP CLI: build_info whoami, verify 'SoC Die ID: <die_id>'

        :param die_id: 0 for primary die, 1 for secondary die
        :raises RuntimeError: If any step fails or die ID doesn't match
        """
        if die_id == self.current_scp_die:
            self.log.info(f"SCP UART already on die {die_id}, no switch needed.")
            return

        self.log.info(f"Switching SCP UART from die {self.current_scp_die} to die {die_id}...")

        # Step 1: Switch UART mux to MCP
        self.log.info("Setting BMC GPIO to route UART to MCP (gpioset gpiochip3 5=1)")
        result, _, _ = self.__bmc_execute_command(command="gpioset gpiochip3 5=1")
        if result != 0:
            raise RuntimeError("Failed to set BMC GPIO for MCP UART mux")

        # Step 2: Verify MCP_MAIN appears on the channel
        try:
            self.core_scp_channel.read_until(key="MCP_MAIN", timeout_seconds=15)
            self.log.info("MCP_MAIN detected on UART - MCP is now active on channel.")
        except TimeoutError as e:
            raise RuntimeError(f"MCP_MAIN not detected after GPIO switch: {e}")

        # Step 3: Send afm uart_die command via MCP to select the target die
        self.log.info(f"Sending 'afm uart_die {die_id} 0' on MCP CLI")
        self.core_scp_channel.write_line(write_string=f"afm uart_die {die_id} 0")
        try:
            self.core_scp_channel.read_until(key="Ok", timeout_seconds=15)
            self.log.info(f"afm uart_die {die_id} 0 acknowledged with 'Ok'.")
        except TimeoutError as e:
            raise RuntimeError(f"'Ok' not received for 'afm uart_die {die_id} 0': {e}")

        # Step 4: Switch UART mux back to SCP
        self.log.info("Setting BMC GPIO to route UART to SCP (gpioset gpiochip3 5=0)")
        result, _, _ = self.__bmc_execute_command(command="gpioset gpiochip3 5=0")
        if result != 0:
            raise RuntimeError("Failed to set BMC GPIO for SCP UART mux")

        # Step 5: Verify SCP_MAIN appears on the channel
        try:
            self.core_scp_channel.read_until(key="SCP_MAIN", timeout_seconds=15)
            self.log.info("SCP_MAIN detected on UART - SCP is now active on channel.")
        except TimeoutError as e:
            raise RuntimeError(f"SCP_MAIN not detected after GPIO switch: {e}")

        # Step 6: Verify correct die with build_info whoami
        self.log.info("Verifying die ID with 'build_info whoami'")
        self.core_scp_channel.write_line(write_string="build_info whoami")
        try:
            output = self.core_scp_channel.read_until(key="Ok", timeout_seconds=15)
        except TimeoutError as e:
            raise RuntimeError(f"'Ok' not received for 'build_info whoami': {e}")

        expected_die_str = f"SoC Die ID: {die_id}"
        if expected_die_str not in output:
            raise RuntimeError(f"Die ID mismatch: expected '{expected_die_str}' in whoami output but not found")

        self.current_scp_die = die_id
        self.log.info(f"SCP UART successfully switched to die {die_id}.")

    def __get_current_soc_power(self):
        """
        Queries the current SoC power consumption from SCP0 CLI via 'pwr status power'.
        :return: Current SoC power in watts (int), or None if unable to read.
        """
        result = self.run_cli_cmd_on_scp0_until_comp("pwr status power", timeout=5)
        if not result:
            self.log.info("Failed to get power status from SCP0")
            return None
        # Parse output for SoC power line, e.g.: "SoC Power       : 185 W"
        for line in result.splitlines():
            if "SoC Power" in line or "Soc Power" in line or "SOC Power" in line:
                # Skip lines about cap/limit
                if "Cap" in line or "Limit" in line:
                    continue
                match = re.search(r':\s*(\d+)', line)
                if match:
                    power = int(match.group(1))
                    self.log.info(f"Current SoC Power: {power} W")
                    return power
        self.log.info("Could not parse SoC power from 'pwr status power' output")
        return None

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
    
    def __check_prerequisites(self):
        """
        Verify that prerequisites (Test 1) have been completed.
        Also ensures the BMC CLI connection is alive.
        :return: True if prerequisites are met, False otherwise
        """
        if not self.mctp_eid:
            self.log.info("Fail : MCTP EID not available. Test 1 prerequisite must pass first.")
            return False
        if not self.bmc_cli:
            self.log.info("Fail : BMC CLI not available. Suite setup must pass first.")
            return False
        # Ensure BMC connection is alive before starting test
        self.__ensure_bmc_connection()
        return True

    def __poll_sensor_state(self, sensor_id, expected_state, max_retries=6, retry_delay=10):
        """
        Poll a state sensor until it reaches the expected state or retries are exhausted.
        :param sensor_id: Sensor ID to query
        :param expected_state: Expected event state string (e.g., "Sensor Normal", "Sensor Critical")
        :param max_retries: Maximum number of polling attempts (default: 6)
        :param retry_delay: Delay between retries in seconds (default: 10)
        :return: The final event state string
        """
        for attempt in range(max_retries):
            event_state = self.get_state_sensor_event_state(sensor_id)
            if event_state == expected_state:
                self.log.info(f"Sensor reached expected state '{expected_state}' on attempt {attempt + 1}/{max_retries}")
                return event_state
            self.log.info(f"Polling attempt {attempt + 1}/{max_retries}: eventState='{event_state}', expected='{expected_state}'")
            if attempt < max_retries - 1:
                time.sleep(retry_delay)
        self.log.info(f"Sensor did not reach expected state '{expected_state}' after {max_retries} attempts. Last state: '{event_state}'")
        return event_state

    def test_mcp_throttle_state_sensor(self):
        self.log.info("***** Verifying MCP_THROTTLING_STATE sensor functionality on BMC *****")

        # Guard: verify prerequisites
        if not self.__check_prerequisites():
            return False

        try:
            # Step 1: Get initial sensor reading
            event_state = self.get_state_sensor_event_state(self.mcp_throttle_state_sensor_id)
            if event_state == "Sensor Critical":
                self.log.info(f"System state Degraded, skipping current test. Event state: {event_state} Expected: Sensor Normal")
                return True
            elif event_state != "Sensor Normal":
                self.log.info(f"Initial eventState is not 'Sensor Normal': {event_state}")
                return False
            self.log.info(f"Initial eventState is 'Sensor Normal': {event_state}")

            # Step 2: Disable power loop on SCP1 to induce degraded state
            result = self.run_cli_cmd_on_scp1_until_comp("pwr set loopdis 1", timeout=5)
            if not result:
                self.log.info("Fail : Failed to disable power loop on SCP1")
                return False
            time.sleep(10)

            # Step 3: Get updated sensor reading with retry polling
            event_state = self.__poll_sensor_state(
                self.mcp_throttle_state_sensor_id, "Sensor Critical", max_retries=6, retry_delay=10
            )
            if event_state != "Sensor Critical":
                self.log.info(f"Fail : After inducing failure, eventState is not 'Sensor Critical': {event_state}")
                return False
            self.log.info(f"After inducing failure, eventState is 'Sensor Critical': {event_state}")

            # Step 4: Re-enable power loop on SCP1 to restore normal state
            result = self.run_cli_cmd_on_scp1_until_comp("pwr set loopdis 0", timeout=5)
            if not result:
                self.log.info("Fail : Failed to re-enable power loop on SCP1")
                return False
            time.sleep(10)

            # Step 5: Get final sensor reading with retry polling
            event_state = self.__poll_sensor_state(
                self.mcp_throttle_state_sensor_id, "Sensor Normal", max_retries=6, retry_delay=10
            )
            if event_state != "Sensor Normal":
                self.log.info(f"Fail : After restoring, eventState is not 'Sensor Normal': {event_state}")
                return False
            self.log.info(f"After restoring, eventState is 'Sensor Normal': {event_state}")
            return True

        finally:
            # Always restore power loop on SCP1 so next test starts clean
            for attempt in range(3):
                try:
                    if self.current_scp_die != 1:
                        self.__switch_scp_uart_to_die(1)
                    self.__run_cli_cmd_on_scp_until_comp(self.core_scp_channel, "pwr set loopdis 0", timeout=5, die_label="SCP1")
                    self.log.info("Power loop re-enabled on SCP1 (cleanup).")
                    break
                except Exception as e:
                    self.log.warning(f"Failed to re-enable power loop on SCP1 (attempt {attempt + 1}/3): {e}")
            # Always restore UART back to die 0
            try:
                self.__switch_scp_uart_to_die(0)
            except Exception as e:
                self.log.warning(f"Failed to restore SCP UART to die 0: {e}")
    
    def test_mcp_power_cap_effector(self):
        self.log.info("***** Verifying MCP_PWRCAP_LIMIT effector functionality on BMC *****")

        # Guard: verify prerequisites
        if not self.__check_prerequisites():
            return False

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
        time.sleep(15)

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
        time.sleep(15)

        # Step 5: Verify restored power cap on SCP0
        final_vcpu_power_cap, final_soc_power_cap = self.get_power_cap_values()
        if final_soc_power_cap != soc_power_cap:
            self.log.info(f"Fail : Restored SOC Power Cap is not as expected: Current: {final_soc_power_cap}, Expected: {soc_power_cap}")
            return False
        self.log.info(f"Restored Soc Power Cap is as expected: {final_soc_power_cap}")

        return True
    
    def test_induce_pwr_throttle_via_bmc_pwr_capping(self):
        self.log.info("***** Inducing power throttle via BMC power capping *****")

        # Guard: verify prerequisites
        if not self.__check_prerequisites():
            return False

        # Step 1: Get initial power cap values so we can restore later
        _, soc_power_cap = self.get_power_cap_values()
        self.log.info(f"Initial SOC Power Cap for restore: {soc_power_cap}")

        # Step 2: Check the state sensor status to event state is normal
        event_state = self.get_state_sensor_event_state(self.mcp_throttle_state_sensor_id)
        if event_state == "Sensor Critical":
            self.log.info(f"System state Degraded, skipping current test. Event state: {event_state} Expected: Sensor Normal")
            return True
        elif event_state != "Sensor Normal":
            self.log.info(f"Fail : Initial eventState is not 'Sensor Normal': {event_state}")
            return False
        self.log.info(f"Initial eventState is as expected : {event_state}")
        time.sleep(5)

        # Step 3: Set power cap to 50W (POWER_CAP_MIN) to induce throttling
        # This should cause the power control loop to throttle cores below nominal,
        # resulting in Sensor Warning (PLDM_PERFORMANCE_THROTTLED).
        # Sensor Critical (PLDM_PERFORMANCE_DEGRADED) only occurs on loop failure (error state 12).
        low_soc_power_cap_bmc = 50  # POWER_CAP_MIN from firmware power_i.h
        self.log.info(f"Setting power cap to {low_soc_power_cap_bmc} W to induce throttling")
        success = self.set_numeric_effector_value(self.mcp_pwrcap_effector_id, low_soc_power_cap_bmc)
        if not success:
            self.log.info("Fail : Failed to set low power cap via PLDM effector.")
            return False
        self.log.info(f"Successfully set low power cap via PLDM effector: {low_soc_power_cap_bmc}")
        time.sleep(10)  # Wait for system to react to new power cap

        # Step 4: Poll sensor until Sensor Warning (throttled) is detected
        event_state = self.__poll_sensor_state(
            self.mcp_throttle_state_sensor_id, "Sensor Warning", max_retries=6, retry_delay=5
        )
        if event_state != "Sensor Warning":
            self.log.info(f"Fail : Power throttle state sensor did not report 'Sensor Warning': {event_state}")
            return False
        self.log.info(f"Power throttle state sensor reported throttling as expected: {event_state}")
        time.sleep(5)

        # Step 5: Restore original power cap via PLDM effector
        success = self.set_numeric_effector_value(self.mcp_pwrcap_effector_id, soc_power_cap)
        if not success:
            self.log.info("Fail : Failed to restore original power cap via PLDM effector.")
            return False
        self.log.info(f"Successfully restored original power cap via PLDM effector: {soc_power_cap}")
        time.sleep(10)

        # Step 6: Verify power throttle state sensor returns to normal with retry polling
        event_state = self.__poll_sensor_state(
            self.mcp_throttle_state_sensor_id, "Sensor Normal", max_retries=6, retry_delay=10
        )
        if event_state == "Sensor Critical":
            self.log.info(f"System state Degraded, skipping current test. Event state: {event_state} Expected: Sensor Normal")
            return True
        elif event_state != "Sensor Normal":
            self.log.info(f"Fail : Power throttle state sensor did not return to normal as expected: {event_state}")
            return False
        self.log.info(f"Power throttle state sensor returned to normal as expected: {event_state}")
        return True