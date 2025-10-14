# Copyright (c) Microsoft Corporation. All rights reserved.
"""
icc_mhu_lib_test.py - Python based Pythia 2.0 Test.
Tests that check for icc mhu command output over same and across dies
"""
import time
import sys
import os
from pathlib import Path
from typing import List

sys.path.append(str(Path(__file__).parent.parent / "kng_pythia_libs"))
sys.path.append(str(Path(__file__).parent.parent / "common"))

from serial_utils import SerialUtility
from thread_utils import ReceiverThread
from kng_pythia_test_setup import KngPythiaTestSetup
from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

# Add paths for both package and direct imports
current_dir = os.path.dirname(os.path.abspath(__file__))
pylibs_dir = os.path.dirname(current_dir)
sys.path.extend(
    [
        pylibs_dir,
        current_dir,
    ]
)


class IccMhuTest(EchoFallsBaseTest):
    # Initialize class-level flag
    _scp_mcp_heartbeat_received = False

    def __init__(
        self,
        workspace_config: Path | str = None,
        default_log_home: str = None,
        fw_payload_path: str = None,
        host_config: Path | str = None,
        name: str = "IccMhuTest",
        number: str = "NaN",
        dut_platform: str = "KingsgateSVP",
        host_name: str | None = None,
    ):
        super().__init__(
            name=name,
            number=number,
            workspace_config=workspace_config,
            default_log_home=default_log_home,
            fw_payload_path=fw_payload_path,
            dut_platform=dut_platform,
            host_config=host_config,
            host_name=host_name,
        )
        self.core_com_channel_scp_die0 = None
        self.core_com_channel_mcp_die0 = None
        self.core_com_channel_scp_die1 = None
        self.core_com_channel_mcp_die1 = None

        self.serial_util_die0 = None
        self.serial_util_send = None
        self.serial_util_receive = None

    def setup_dut(self) -> bool:
        """Setup the DUT for testing"""
        try:
            self.log.info("Setting up DUT...")
            self.dut.setup()

            # If on the FPGA perform an Out of Band Reset,
            if self.dut.get_dut_type() == DeviceType.BIGFPGA:
                self.log.warning(
                    "Device type is bigFPGA. Performing an additional OOB reset ..."
                )
                KngPythiaTestSetup.fpga_oob_reset(self.log)

            def get_channels(die):
                return (
                    die.scp.channel_manager.get_current_channel(),
                    die.mcp.channel_manager.get_current_channel(),
                )

            soc = self.dut.mb.node_0.soc
            if soc.secondary_die is not None:
                self.log.info(
                    "Executing on DualDie Config, so secondary die will be used for SCP and MCP core"
                )
                self.core_com_channel_scp_die0, self.core_com_channel_mcp_die0 = (
                    get_channels(soc.primary_die)
                )
                self.core_com_channel_scp_die1, self.core_com_channel_mcp_die1 = (
                    get_channels(soc.secondary_die)
                )
            else:
                self.log.info(
                    "Executing on SingleDie Config, so primary die will be used for SCP and MCP core"
                )
                self.core_com_channel_scp_die0, self.core_com_channel_mcp_die0 = (
                    get_channels(soc.primary_die)
                )

            return True
        except Exception as e:
            self.log.error(f"Error during DUT setup: {str(e)}")
            return False

    def teardown_dut(self) -> bool:
        """Teardown the DUT after testing"""
        try:
            self.log.info("Tearing down DUT...")

            # Reset the heartbeat flag since SVP is being terminated
            self.__class__._scp_mcp_heartbeat_received = False

            self.dut.teardown()
            return True

        except Exception as e:
            self.log.error(f"Error during DUT teardown: {str(e)}")
            return False

    def validate_mhu_message_payload(
        self,
        mhu_messages: List[str],
        expected_payload_size: int,
        expected_payload_bytes: List[int],
    ) -> bool:
        """
        Validate the MHU message payload against expected values.

        Args:
            mhu_messages: List of MHU message strings
            expected_payload_size: Expected payload size
            expected_payload_bytes: List of expected payload bytes

        Returns:
            bool: True if validation passes, False otherwise
        """
        try:
            # Find the message with command ID
            command_message = None
            payload_messages = []

            for msg in mhu_messages:
                if "MHU Recv Complete CB - command:" in msg:
                    command_message = msg
                elif "MHU Recv Complete CB - payload [" in msg:
                    payload_messages.append(msg)

            if not command_message or not payload_messages:
                self.log.error("Could not find required MHU message components")
                return False

            # Log the messages we found
            self.log.info("\nMHU Message Components:")
            self.log.info("-" * 50)
            self.log.info(f"Command Message: {command_message}")
            self.log.info("Payload Messages:")
            for msg in payload_messages:
                self.log.info(f"  {msg}")
            self.log.info("-" * 50 + "\n")

            # Validate command message and payload messages
            try:
                # Extract and validate payload size from command message
                size_part = command_message.split("payload size:")[1].strip()
                received_size = int(size_part)

                # Convert both to strings for comparison
                received_size_str = str(received_size)
                expected_size_str = str(expected_payload_size)

                if received_size_str != expected_size_str:
                    self.log.error(
                        f"Payload size mismatch. Expected: {expected_size_str}, Received: {received_size_str}"
                    )
                    return False
                else:
                    self.log.info(
                        f"Payload size match successful. Expected: {expected_size_str}, Received: {received_size_str}"
                    )

                # Extract and validate payload values
                received_payload = []
                for msg in payload_messages:
                    # Extract number after 0x from payload
                    payload_str = msg.split("==")[1].strip()
                    payload_num = payload_str.split("0x")[1]
                    received_payload.append(payload_num)

                # Convert expected payload bytes to strings
                expected_payload_strs = [str(byte) for byte in expected_payload_bytes]

                # Sort both lists to ensure order doesn't matter
                received_payload.sort()
                expected_payload_strs.sort()

                if received_payload != expected_payload_strs:
                    self.log.error(
                        f"Payload mismatch. Expected: {expected_payload_strs}, Received: {received_payload}"
                    )
                    return False
                else:
                    self.log.info(
                        f"Payload values match! Expected: {expected_payload_strs}, Received: {received_payload}"
                    )

            except (IndexError, ValueError) as e:
                self.log.error(f"Error parsing MHU messages: {str(e)}")
                return False

            self.log.debug("MHU message payload validation successful")
            return True

        except Exception as e:
            self.log.error(f"Error validating MHU message payload: {str(e)}")
            return False

    def generate_key(self, payload_bytes):
        """
        Returns the formatted MHU receive confirmation message for the last byte in the payload.

        Args:
            payload_bytes (list of int): List of byte values (0-255)

        Returns:
            str: Formatted string for the last payload byte
        """
        if not payload_bytes:
            return "No payload bytes provided."

        last_index = len(payload_bytes) - 1
        last_value = payload_bytes[-1]

        return f"MHU Recv Complete CB - payload [{last_index}] == 0x{last_value:x}"

    def setup_serial_utilities(self, sender):
        """Initialize serial utilities based on sender die and log the setup."""
        if "die0" in sender:
            receiver = sender.replace("die0", "die1")
            self.log.debug(
                f"Setting up D2D communication: Sender = {sender}, Receiver = {receiver}"
            )
            self.serial_util_receive = SerialUtility(
                scp_channel=self.core_com_channel_scp_die1,
                mcp_channel=self.core_com_channel_mcp_die1,
                logger=self.log,
                dut=self.dut,
            )
            self.serial_util_send = SerialUtility(
                scp_channel=self.core_com_channel_scp_die0,
                mcp_channel=self.core_com_channel_mcp_die0,
                logger=self.log,
                dut=self.dut,
            )
        else:
            receiver = sender.replace("die1", "die0")
            self.log.debug(
                f"Setting up D2D communication: Sender = {sender}, Receiver = {receiver}"
            )
            self.serial_util_receive = SerialUtility(
                scp_channel=self.core_com_channel_scp_die0,
                mcp_channel=self.core_com_channel_mcp_die0,
                logger=self.log,
                dut=self.dut,
            )
            self.serial_util_send = SerialUtility(
                scp_channel=self.core_com_channel_scp_die1,
                mcp_channel=self.core_com_channel_mcp_die1,
                logger=self.log,
                dut=self.dut,
            )

    def check_heartbeat(self):
        """Check heartbeat for both sender and receiver utilities."""
        return (
            self.serial_util_receive.wait_for_scp_mcp_heartbeat()
            and self.serial_util_send.wait_for_scp_mcp_heartbeat()
        )

    def prepare_payload(self, payload_bytes):
        """Prepare payload bytes from various input formats."""
        if payload_bytes is None:
            return [5, 6, 7]
        elif isinstance(payload_bytes, str):
            return [int(x.strip()) for x in payload_bytes.split()]
        elif not isinstance(payload_bytes, list):
            return list(payload_bytes)
        return payload_bytes

    def log_test_parameters(
        self, direction, channel, message_id, payload_size, payload_bytes
    ):
        """Log the test parameters."""
        self.log.info("\n" + "=" * 80)
        self.log.info("ICC MHU Test Parameters:")
        self.log.info("-" * 80)
        self.log.info(f"Direction: {direction}")
        self.log.info(f"Channel: {channel}")
        self.log.info(f"Message ID: {message_id}")
        self.log.info(f"Payload Size: {payload_size}")
        self.log.info(f"Payload Bytes: {payload_bytes}")
        self.log.info("=" * 80 + "\n")

    def execute_command(self, utility, is_mcp, command, read_key="Ok"):
        """Execute command on MCP or SCP."""
        return (
            utility.run_command_on_mcp(command=command, read_until_key=read_key)
            if is_mcp
            else utility.run_command_on_scp(command=command, read_until_key=read_key)
        )

    def close_all_channels(self):
        """Close all die0 and die1 communication channels if open."""
        for attr in [
            "core_com_channel_scp_die0",
            "core_com_channel_mcp_die0",
            "core_com_channel_scp_die1",
            "core_com_channel_mcp_die1",
        ]:
            channel = getattr(self, attr)
            if channel is not None and channel.is_open():
                channel.close()

    def test_icc_cli_mscp_mhu(
        self,
        sender_is_mcp: bool,
        channel: int = 4,
        message_id: int = 2,
        payload_size: int = 3,
        payload_bytes: List[int] = None,
    ) -> bool:
        """
        Test ICC MHU communication in one direction between MCP and SCP.

        Args:
            sender_is_mcp: True if MCP is sending, False if SCP is sending
            channel: MHU channel number (default: 4)
            message_id: Message ID (default: 2)
            payload_size: Size of payload in bytes (default: 3)
            payload_bytes: List of payload bytes (default: [5, 6, 7])
        Returns:
            bool: True if test passed, False otherwise
        """
        try:
            # Set up sender and receiver based on direction
            sender = "MCP" if sender_is_mcp else "SCP"
            receiver = "SCP" if sender_is_mcp else "MCP"
            direction = f"{sender} to {receiver}"

            self.serial_util_die0 = SerialUtility(
                scp_channel=self.core_com_channel_scp_die0,
                mcp_channel=self.core_com_channel_mcp_die0,
                logger=self.log,
                dut=self.dut,
            )
            # Wait for SCP heartbeat during initial setup
            if not self.serial_util_die0.wait_for_scp_mcp_heartbeat():
                self.log.error(
                    "Failed to receive initial SCP-MCP heartbeat during setup"
                )
                return False

            payload_bytes = self.prepare_payload(payload_bytes)
            self.log_test_parameters(
                direction, channel, message_id, payload_size, payload_bytes
            )

            # Setup receive request on receiver
            receive_cmd = f"icc_mhu recv {channel} {message_id} {payload_size}"
            self.log.debug(f"Setting up receive request on {receiver}: {receive_cmd}")
            receive_result = (
                self.serial_util_die0.run_command_on_scp(
                    command=receive_cmd, read_until_key="Ok"
                )
                if sender_is_mcp
                else self.serial_util_die0.run_command_on_mcp(
                    command=receive_cmd, read_until_key="Ok"
                )
            )
            if not receive_result:
                self.log.error(f"Failed to setup receive request on {receiver}")
                return False

            # Start receiver thread using appropriate serial read method
            read_method = (
                self.serial_util_die0.read_scp_serial_until
                if sender_is_mcp
                else self.serial_util_die0.read_mcp_serial_until
            )

            key = self.generate_key(payload_bytes)

            receiver_thread = ReceiverThread(
                read_method=read_method, key=key, timeout=80
            )
            self.log.debug(f"Starting {receiver} log capture...")
            receiver_thread.start()
            time.sleep(2)
            # Send message from sender
            payload_str = " ".join(str(b) for b in payload_bytes)
            send_cmd = (
                f"icc_mhu send {channel} {message_id} {payload_size} {payload_str}"
            )
            self.log.debug(f"Executing send command on {sender}: {send_cmd}")
            send_result = (
                self.serial_util_die0.run_command_on_mcp(
                    command=send_cmd, read_until_key="Ok"
                )
                if sender_is_mcp
                else self.serial_util_die0.run_command_on_scp(
                    command=send_cmd, read_until_key="Ok"
                )
            )
            if not send_result:
                self.log.error(f"Failed to send message from {sender}")
                return False

            # Wait for receiver thread to complete
            receiver_thread.join(timeout=80)
            if receiver_thread.is_alive():
                self.log.error(f"{receiver} log capture thread timed out.")
                return False

            if not receiver_thread.success or not receiver_thread.log:
                self.log.error(f"No output captured from {receiver}")
                return False

            self.log.info(f"\nComplete captured output from {receiver}:")
            self.log.info("=" * 80)
            self.log.info(receiver_thread.log)
            self.log.info("=" * 80)

            mhu_messages = [
                line
                for line in receiver_thread.log.splitlines()
                if "MHU Recv Complete CB" in line
            ]
            if not mhu_messages:
                self.log.error(
                    f"No MHU messages found in captured output ({direction})"
                )
                return False
            self.log.debug(f"Found MHU Messages ({direction}):")
            for msg in mhu_messages:
                self.log.info(msg)

            if not self.validate_mhu_message_payload(
                mhu_messages, payload_size, payload_bytes
            ):
                self.log.error(f"MHU message payload validation failed ({direction})")
                return False

            self.log.info(
                f"ICC MHU communication test ({direction}) completed successfully"
            )
            return True

        except Exception as e:
            self.log.error(f"Error during ICC MHU test: {str(e)}")
            return False
        finally:
            self.close_all_channels()

    def test_icc_cli_d2d_mhu(
        self,
        sender_is_mcp: bool,
        sender_is_die0: bool,
        channel: int = 9,
        message_id: int = 2,
        payload_size: int = 3,
        payload_bytes: List[int] = None,
    ) -> bool:
        """
        Perform a D2D ICC MHU communication test between MCP and SCP across dies.

        This method sets up a one-directional message transfer from sender to receiver
        using ICC MHU protocol, validates heartbeat, sends a payload, and verifies
        reception and content integrity.

        Args:
            sender_is_mcp (bool): True if MCP is the sender, False if SCP is the sender.
            sender_is_die0 (bool): True if die0 is the sender, False if die1 is the sender.
            channel (int): MHU channel number to use for communication. Default is 9.
            message_id (int): Message ID to tag the communication. Default is 2.
            payload_size (int): Size of the payload in bytes. Default is 3.
            payload_bytes (List[int], optional): List of payload bytes to send.
                If None, defaults to [5, 6, 7]. Can also be a space-separated string.

        Returns:
            bool: True if the test passes (heartbeat, send, receive, and payload validation),
                False if any step fails.
        """
        try:
            sender = f"{'MCP' if sender_is_mcp else 'SCP'}_die{'0' if sender_is_die0 else '1'}"
            receiver = f"{sender.split('_', maxsplit=1)[0]}_die{'1' if 'die0' in sender else '0'}"
            direction = f"{sender} to {receiver}"

            self.setup_serial_utilities(sender)

            if not self.check_heartbeat():
                self.log.error(
                    "Failed to receive initial SCP-MCP heartbeat during setup"
                )
                return False

            payload_bytes = self.prepare_payload(payload_bytes)
            self.log_test_parameters(
                direction, channel, message_id, payload_size, payload_bytes
            )

            receive_cmd = f"icc_mhu recv {channel} {message_id} {payload_size}"
            self.log.debug(f"Setting up receive request on {receiver}: {receive_cmd}")
            if not self.execute_command(
                self.serial_util_receive, sender_is_mcp, receive_cmd
            ):
                self.log.error(f"Failed to setup receive request on {receiver}")
                return False

            key = self.generate_key(payload_bytes)
            read_method = (
                self.serial_util_receive.read_mcp_serial_until
                if sender_is_mcp
                else self.serial_util_receive.read_scp_serial_until
            )
            receiver_thread = ReceiverThread(
                read_method=read_method, key=key, timeout=80
            )
            self.log.debug(f"Starting {receiver} log capture...")
            receiver_thread.start()
            time.sleep(2)
            payload_str = " ".join(str(b) for b in payload_bytes)
            send_cmd = (
                f"icc_mhu send {channel} {message_id} {payload_size} {payload_str}"
            )
            self.log.debug(f"Executing send command on {sender}: {send_cmd}")
            if not self.execute_command(self.serial_util_send, sender_is_mcp, send_cmd):
                self.log.error(f"Failed to send message from {sender}")
                return False

            receiver_thread.join(timeout=80)
            if receiver_thread.is_alive():
                self.log.error(f"{receiver} log capture thread timed out.")
                return False
            if not receiver_thread.success or not receiver_thread.log:
                self.log.error(f"No output captured from {receiver}")
                return False

            self.log.info(f"\nComplete captured output from {receiver}:")
            self.log.info("=" * 80)
            self.log.info(receiver_thread.log)
            self.log.info("=" * 80)

            mhu_messages = [
                line
                for line in receiver_thread.log.splitlines()
                if "MHU Recv Complete CB" in line
            ]
            if not mhu_messages:
                self.log.error(
                    f"No MHU messages found in captured output ({direction})"
                )
                return False

            self.log.debug(f"Found MHU Messages ({direction}):")
            for msg in mhu_messages:
                self.log.info(msg)

            if not self.validate_mhu_message_payload(
                mhu_messages, payload_size, payload_bytes
            ):
                self.log.error(f"MHU message payload validation failed ({direction})")
                return False

            self.log.debug(
                f"ICC MHU communication test ({direction}) completed successfully"
            )
            return True

        except Exception as e:
            self.log.error(f"Error during ICC MHU test: {str(e)}")
            return False

        finally:
            self.close_all_channels()
