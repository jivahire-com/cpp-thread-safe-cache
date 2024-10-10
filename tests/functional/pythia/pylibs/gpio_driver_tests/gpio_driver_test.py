# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Test that checks the GPIO driver functionalities in MSCP.
"""
import sys, os, re, json, time
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))

from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup

from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

class gpio_driver_test(EchoFallsBaseTest):
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
        name: str = "GPIO_Driver_Test",
        number: str = "NaN",
        workspace_config: Path | str = None,
        default_log_home: str = None,
        fw_payload_path: str = None,
        dut_platform: str = "KingsgateSVP",
        host_config: Path | str = None,
        host_name: str | None = None,
        gpio_config_path: Path | str = None,
    ):
        self.gpio_config = None
        self.gpio_config_path = gpio_config_path

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

        if self.gpio_config_path is not None:
            try:
                with open(self.gpio_config_path, 'r') as json_file:
                    self.gpio_config = json.load(json_file)
            except (FileNotFoundError, json.JSONDecodeError) as e:
                self.log.error(f"Failed to load GPIO configuration: {e}")


    def test_gpio_driver(self):
        """
        Test that checks the GPIO driver functionalities in MSCP.
        """
        self.log.info("Starting GPIO driver test")
        # Call the DUT setup
        self.dut.setup()

        # Check if GPIO configuration is loaded
        if self.gpio_config is None:
            self.log.error("GPIO configuration not loaded")
            self.test_notify(step="InvalidTestConfig", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            return False

        #
        # Test GPIO driver
        #
        self.channels = {}

        self.channels["scp"] = self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()
        self.channels["mcp"] = self.dut.mb.node_0.soc.primary_die.mcp.channel_manager.get_current_channel()

        self.channels["scp"].open()
        assert self.channels["scp"].is_open()
        self.channels["mcp"].open()
        assert self.channels["mcp"].is_open()

        # Wait until heartbeat is received
        self.log.info("Reading SCP UART for HeartBeat . . .")
        try:
            self.channels["scp"].read_until(key="ScpHeartBeat", timeout_seconds=900)
        except Exception as e:
            self.log.error(f"Error reading SCP UART: {e}")
            self.test_notify(step="ScpHeartBeat", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            return False

        self.log.info("Reading MCP UART for HeartBeat . . .")
        try:
            self.channels["mcp"].read_until(key="McpHeartBeat", timeout_seconds=900)
        except Exception as e:
            self.log.error(f"Error reading MCP UART: {e}")
            self.test_notify(step="McpHeartBeat", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            return False
        
        # Wait for the system to stabilize
        time.sleep(10)

        for core, channel in self.channels.items():
            # Check GPIO configurations
            for gpio_ctrl in self.gpio_config["GPIO_Controllers"] :
                ctrl_name = gpio_ctrl["Name"]
                ctrl_Id = gpio_ctrl["ID"]
                self.log.info(f"GPIO Controller: Name: {ctrl_name} : ID: {ctrl_Id}")
                for gpio_pin in gpio_ctrl["Pins"] :
                    pin_id = gpio_pin["Pin"]
                    pin_name = gpio_pin["Name"]
                    pin_direction = gpio_pin["Direction"]
                    pin_interrupt = gpio_pin["Interrupt"]
                    self.log.info(f"{core} GPIO {ctrl_name} Pin: Name: {pin_name} : ID: {pin_id} : Direction: {pin_direction} : Interrupt: {pin_interrupt}")
                    try:
                        self.test_pin_configuration(channel, ctrl_Id, pin_id, pin_direction, pin_interrupt)
                    except Exception as e:
                        self.log.error(f"{e}")
                        self.test_notify(step=f"{core}_PinConfig", msg="Test Fail", _is_error=True)
                        self.dut.teardown()
                        return False

            # Test GPIO ISR to test GPIO driver ISR functionality with reserved Pins
            try:
                self.test_GPIO_isr(channel, self.gpio_config["ReservedForIsr"][core])
            except Exception as e:
                self.log.error(f"{e}")
                self.test_notify(step=f"{core}_GPIO_ISR", msg="Test Fail", _is_error=True)
                self.dut.teardown()
                return False

        self.log.info("Test passed")
        self.log.info("GPIO driver test completed successfully")

        self.test_notify(step="GPIO Driver test", msg="Test Done", _is_error=False)
        # Call the DUT teardown
        self.dut.teardown()
        return True

    def test_pin_configuration(self, channel, ctrl_id, pin_id, pin_direction, pin_interrupt):
        channel.write_line(write_string="gpio")

        # Check Direction
        expected = f"Get {ctrl_id} / {pin_id} GPIO Direction: {pin_direction}"
        channel.write_line(write_string=f"get_dir {ctrl_id} {pin_id}")
        result = channel.read_until(key=expected, timeout_seconds=30)
        matched = re.findall(expected, result)
        if matched:
            self.log.info(f"gpio-get_dir {ctrl_id} {pin_id}: Output {matched}")
        else:
            raise Exception(f"Failed to verify GPIO {ctrl_id} {pin_id} Direction")

        # Check Interrupt enable
        expected = f"Get {ctrl_id} / {pin_id} GPIO interrupt: {pin_interrupt}";
        channel.write_line(write_string=f"get_int_enable {ctrl_id} {pin_id}")
        result = channel.read_until(key=expected, timeout_seconds=30)
        matched = re.findall(expected, result)
        if matched:
            self.log.info(f"gpio-get_int_enable {ctrl_id} {pin_id}: Output {matched}")
        else:
            raise Exception(f"Failed to verify GPIO {ctrl_id} {pin_id} Interrupt")

        channel.write_line(write_string="..")
        
    def test_GPIO_isr(self, channel, config):
        channel.write_line(write_string="gpio")
        ctrl = config["Ctrl"]
        pin = config["Pin"]

        # Set direction to output to generate interrupt
        channel.write_line(write_string=f"set_dir {ctrl} {pin} 1")
        time.sleep(1)

        # Enable Interrupt
        channel.write_line(write_string=f"set_int_enable {ctrl} {pin} 1")
        time.sleep(1)

        # Register ISR
        channel.write_line(write_string=f"register_isr {ctrl} {pin}")
        time.sleep(1)

        # Generate Interrupt
        channel.write_line(write_string=f"set_pin {ctrl} {pin} 0")
        time.sleep(1)

        channel.write_line(write_string=f"set_pin {ctrl} {pin} 1")
        time.sleep(1)

        expected = f"GPIO ISR Callback: Status: 0x00000000, CtrlID: {ctrl}, PinID: {pin}"
        try:
            channel.read_until(key=expected, timeout_seconds=3)
            self.log.info(f"ISR Callback for {ctrl} {pin}")
        except Exception as e:
            raise Exception(f"Failed to verify GPIO {ctrl} {pin} ISR Callback") from e

        # Restore configuration
        channel.write_line(write_string="restore")
        time.sleep(1)
        channel.write_line(write_string="..")
