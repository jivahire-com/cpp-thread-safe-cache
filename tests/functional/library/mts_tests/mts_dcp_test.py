# Copyright (c) Microsoft Corporation. All rights reserved.
import logging
from pathlib import Path
from typing import Optional
import os
import sys
from typing import Union, List, Dict, Optional

sys.path.append(str(Path(__file__).parent.parent / 'kng_pythia_libs'))

from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup
from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest


# Add paths for both package and direct imports
current_dir = os.path.dirname(os.path.abspath(__file__))
pylibs_dir = os.path.dirname(current_dir)
sys.path.extend([pylibs_dir, current_dir])

try:
    from .dcp_protocol import data_collection_protocol
    from .dcp_commands import dcp_commands
    from .trp_lib import mts_cli_trp_endpoint
    from .trp_protocol import transfer_relay_protocol

except ImportError:
    from dcp_protocol import data_collection_protocol
    from dcp_commands import dcp_commands
    from trp_lib import mts_cli_trp_endpoint
    from trp_protocol import transfer_relay_protocol

logger = logging.getLogger(__name__)

class MtsDcpTest(EchoFallsBaseTest):
    # Initialize class-level flag
    _scp_mcp_heartbeat_received = False

    def __init__(
        self,
        workspace_config: Path | str = None,
        default_log_home: str = None,
        fw_payload_path: str = None,
        host_config: Path | str = None,
        name: str = "MtsDcpTest",
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
        self.dcp = None
        self.die0_scp_channel = None
        self.die0_mcp_channel = None

    def wait_for_scp_mcp_heartbeat(self) -> bool:
        """Wait for both SCP and MCP heartbeat messages"""
        try:
            # Setup channels
            self.die0_scp_channel = self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()
            self.die0_mcp_channel = self.dut.mb.node_0.soc.primary_die.mcp.channel_manager.get_current_channel()

            # Open channels
            self.die0_scp_channel.open()
            self.die0_mcp_channel.open()

            # Wait for SCP heartbeat
            self.log.info("Waiting for initial SCP Heartbeat Message...")
            try:
                self.die0_scp_channel.read_until(key="ScpHeartBeat", timeout_seconds=1800)
                self.log.info("SCP Heartbeat received successfully")
            except Exception as e:
                self.log.error(f"Failed to receive SCP heartbeat: {str(e)}")
                return False

            # Wait for MCP heartbeat
            self.log.info("Waiting for initial MCP Heartbeat Message...")
            try:
                self.die0_mcp_channel.read_until(key="McpHeartBeat", timeout_seconds=3600)
                self.log.info("MCP Heartbeat received successfully")
            except Exception as e:
                self.log.error(f"Failed to receive MCP heartbeat: {str(e)}")
                return False

            self.__class__._scp_mcp_heartbeat_received = True
            return True

        except Exception as e:
            self.log.error(f"Error during heartbeat check: {str(e)}")
            return False

    def setup(self) -> bool:
        """Setup test environment"""
        try:
            self.log.info("Setting up DUT...")
            self.dut.setup()

            # Wait for SCP and MCP heartbeat during initial setup
            if not self.wait_for_scp_mcp_heartbeat():
                self.log.error("Failed to receive initial SCP-MCP heartbeat during setup")
                return False

            self.die0_scp_trp_endpoint = mts_cli_trp_endpoint(self.die0_scp_channel, 0, transfer_relay_protocol.cpu_type.CPU_SCP)

            # Exception raised if command fails
            dcp_commands.client_reset(
                    src_endpoint = self.die0_scp_trp_endpoint,
                    dest_die=0,
                    dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                    client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM)

            return True

        except Exception as e:
            self.log.error(f"Error in setup: {e}")
            return False

    def teardown(self):
        """Cleanup test environment"""
        self.log.info("Tearing down DUT...")
        try:
            if self.die0_scp_channel.is_open():
                self.die0_scp_channel.close()

            if self.die0_mcp_channel.is_open():
                self.die0_mcp_channel.close()

            self.dut.teardown()
            # Reset the heartbeat flag since SVP is being terminated
            self.__class__._scp_mcp_heartbeat_received = False
        except Exception as e:
            self.log.error(f"Error in teardown: {e}")

    def test_client_read_data_polling(self, command=None):
        """A request to a client to provide the memory descriptor for where the telemetry manifest"""

        try:

            self.log.debug("Provider id 0x202, event id 0x4   (SOC MAX TEMP).  Send to power telemetry client.")
            event_tuple = [
            (
                0x0202,
                4,
                data_collection_protocol.client_events_enable_disable_msg.dcp_events_enable_state_t.DCP_EVENTS_ENABLE_STATE_ENABLE
            )
            ]

            self.log.debug("Send DCP_EVENTS_ENABLE_STATE_ENABLE to enable an instantaneous event")
            dcp_commands.client_enable_disable_events(
                            src_endpoint = self.die0_scp_trp_endpoint,
                            dest_die=0,
                            dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                            client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
                            events=event_tuple)

            self.log.debug("Send a client start message to Power Telemetry")
            dcp_commands.client_start_stop(
                            src_endpoint = self.die0_scp_trp_endpoint,
                            dest_die=0,
                            dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                            client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
                            state=data_collection_protocol.client_start_stop_msg.dcp_start_stop_state_t.DCP_START_STOP_STATE_START)

            self.log.debug("Loop for 30 seconds")
            start_time = time.time()
            package_count = 0
            loop_counter = 0
            while True:
                if time.time() - start_time > 30:
                    self.log.debug("Loop timeout reached")
                    break
                while True:
                    self.log.debug(f"Loop counter is {loop_counter}")
                    self.log.debug("Send a read command to power telemetry client.")
                    status, response = dcp_commands.client_read_data(
                        src_endpoint= self.die0_scp_trp_endpoint,
                        dest_die=0,
                        dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                        client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM
                        )

                    self.log.debug(f"Client_read_data status : {status} and client_read_data response : {response}")
                    if status not in [
                        data_collection_protocol.dcp_status_t.DATA_COLLECTION_RD_DATA_VALID_LAST,
                        data_collection_protocol.dcp_status_t.DATA_COLLECTION_RD_DATA_VALID_MORE
                        ]:
                        self.log.debug(f"No Package data available for {loop_counter} iteration")
                        loop_counter = loop_counter + 1
                        break
                    else:
                        package_count = package_count + 1
                        self.log.debug(f"Package data available for {loop_counter} iteration, Total package count received till now is {package_count}")

                        self.log.debug(f"phys: {response.physical_start_addr:x}, size: {response.physical_buffer_size:x}, offset: {response.rd_data_addr_offset:x}, size: {response.rd_data_size:x}, crc: {response.crc:x}\n")
                        self.log.debug("Construct read complete message and send back to power telemetry client.")
                        dcp_commands.client_send_read_data_complete(
                            src_endpoint=self.die0_scp_trp_endpoint,
                            dest_die=0,
                            dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                            client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
                            rd_data_addr_offset=response.rd_data_addr_offset,
                            rd_data_size=response.rd_data_size
                            )
                        loop_counter = loop_counter + 1

            self.log.debug("Send a stop message to power telemetry client")
            dcp_commands.client_start_stop(
                            src_endpoint = self.die0_scp_trp_endpoint,
                            dest_die=0,
                            dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                            client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
                            state=data_collection_protocol.client_start_stop_msg.dcp_start_stop_state_t.DCP_START_STOP_STATE_STOP)

            if package_count > 0:
                self.log.info(f"PASS : Test CLIENT_READ_DATA and CLIENT_READ_DATA_COMPLETE ,Total Package data received {package_count}")
                return True
            else:
                self.log.info(f"FAIL : Test CLIENT_READ_DATA and CLIENT_READ_DATA_COMPLETE ,Total Package data received {package_count}")
                return False

        except Exception as e:
            self.log.error(f"❌ Error in test_client_read_data_polling: {e}")
            self.log.debug("Send a stop message to power telemetry client")
            dcp_commands.client_start_stop(
                            src_endpoint = self.die0_scp_trp_endpoint,
                            dest_die=0,
                            dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                            client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
                            state=data_collection_protocol.client_start_stop_msg.dcp_start_stop_state_t.DCP_START_STOP_STATE_STOP)

            return False


    def test_client_get_manifest(self, command=None):
        """A request to a client to provide the memory descriptor for where the telemetry manifest"""

        try:
            status, response = dcp_commands.client_get_manifest(
                src_endpoint = self.die0_scp_trp_endpoint,
                dest_die=0,
                dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_DCP_SVC
                )

            self.log.info(f"Client_get_manifest status : {status} and client_get_manifest response : {response}")

            return True
        except Exception as e:
            self.log.error(f"❌ Error in test_client_get_manifest: {e}")
            return False

    def test_client_get_platform_information(self, command=None):
        """A request to a client to determine platform details: DCP version, IFWI version, Platform ID, etc"""

        try:
            status, response = dcp_commands.client_get_platform_information(
                src_endpoint = self.die0_scp_trp_endpoint,
                dest_die=0,
                dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_DCP_SVC
                )

            self.log.info(f"client_get_platform_information status : {status} and client_get_platform_information response : {response}")
            self.log.info(f"DCP Version - Major : {response.dcp_ver_major}")
            self.log.info(f"DCP Version - Minor : {response.dcp_ver_minor}")
            self.log.info(f"DCP Version - Patch : {response.dcp_ver_patch}")
            self.log.info(f"IFWI Version - Major : {response.ifwi_ver_major}")
            self.log.info(f"IFWI Version - Minor : {response.ifwi_ver_minor}")
            self.log.info(f"IFWI Version - Patch : {response.ifwi_ver_patch}")
            self.log.info(f"IFWI Version - Revision : {response.ifwi_ver_rev}")
            self.log.info(f"Platform ID : {response.plat_id}")


            if response.dcp_ver_major == 1:
                self.log.info(f"PASS :DCP Version Major is as expected: {response.dcp_ver_major}")
            else:
                self.log.error(f"❌ FAIL :DCP Version Major is NOT as expected: {response.dcp_ver_major}")
                return False

            if response.dcp_ver_minor == 0:
                self.log.info(f"PASS :DCP Version Minor is as expected: {response.dcp_ver_minor}")
            else:
                self.log.error(f"❌ FAIL :DCP Version Minor is NOT as expected: {response.dcp_ver_minor}")
                return False

            if response.plat_id == data_collection_protocol.dcp_msg_get_plat_info_t.COBALT_200:
                self.log.info(f"PASS : Fetched Platform ID {response.plat_id} is matching with DCP_PLATFORM_COBALT_200 {data_collection_protocol.dcp_msg_get_plat_info_t.COBALT_200}")
            else:
                self.log.info(f"FAIL : Fetched Platform ID {response.plat_id} is NOT matching with DCP_PLATFORM_COBALT_200 {data_collection_protocol.dcp_msg_get_plat_info_t.COBALT_200}")
                return False

            return True
        except Exception as e:
            self.log.error(f"❌ Error in test_client_get_platform_information: {e}")
            return False

    def test_client_get_capabilities(self, command=None):
        """A request to a client to determine supported DCP capabilities"""

        try:
            err_flag = True
            self.log.info("Send CLIENT_GET_CAPABILITIES to MCP DCP Service Client.")
            status, response = dcp_commands.client_get_capabilities(
                src_endpoint= self.die0_scp_trp_endpoint,
                dest_die=0,
                dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_DCP_SVC
            )
            self.log.info(f"client_get_capabilities status : {status} and response : {response}")
            self.log.info("Verify the following bitfields are returned per spec")
            if all(val == 1 for val in [response.DCP_MSG_ID_GET_CAPABILITIES, response.DCP_MSG_ID_GET_MANIFEST,response.DCP_MSG_ID_RESET,response.DCP_MSG_ID_GET_PLAT_INFO]):
                self.log.info("PASS : All Below bitfields are set as expected")
            else:
                self.log.info("FAIL : All Below bitfields are NOT set as expected")
                err_flag = False

            self.log.info(f"DCP_MSG_ID_GET_CAPABILITIES     : {response.DCP_MSG_ID_GET_CAPABILITIES}")
            self.log.info(f"DCP_MSG_ID_GET_MANIFEST         : {response.DCP_MSG_ID_GET_MANIFEST}")
            self.log.info(f"DCP_MSG_ID_RESET                : {response.DCP_MSG_ID_RESET}")
            self.log.info(f"DCP_MSG_ID_GET_PLAT_INFO        : {response.DCP_MSG_ID_GET_PLAT_INFO}")

            self.log.info("Send CLIENT_GET_CAPABILITIES to power telemetry client.")
            status, response = dcp_commands.client_get_capabilities(
                src_endpoint= self.die0_scp_trp_endpoint,
                dest_die=0,
                dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM
            )
            self.log.info(f"client_get_capabilities status : {status} and response : {response}")
            self.log.info("Verify the following bitfields are returned per spec")
            if all(val == 1 for val in [response.DCP_MSG_ID_GET_CAPABILITIES, response.DCP_MSG_ID_GET_STATE,response.DCP_MSG_ID_EVENTS_ENABLE_DISABLE,response.DCP_MSG_ID_START_STOP,response.DCP_MSG_ID_READ_DATA,response.DCP_MSG_ID_READ_DATA_COMPLETE,response.DCP_MSG_ID_RESET]):
                self.log.info("PASS : All Below bitfields are set as expected")
            else:
                self.log.info("FAIL : All Below bitfields are NOT set as expected")
                err_flag = False

            self.log.info(f"DCP_MSG_ID_GET_CAPABILITIES     : {response.DCP_MSG_ID_GET_CAPABILITIES}")
            self.log.info(f"DCP_MSG_ID_GET_STATE            : {response.DCP_MSG_ID_GET_STATE}")
            self.log.info(f"DCP_MSG_ID_EVENTS_ENABLE_DISABLE: {response.DCP_MSG_ID_EVENTS_ENABLE_DISABLE}")
            self.log.info(f"DCP_MSG_ID_START_STOP           : {response.DCP_MSG_ID_START_STOP}")
            self.log.info(f"DCP_MSG_ID_READ_DATA            : {response.DCP_MSG_ID_READ_DATA}")
            self.log.info(f"DCP_MSG_ID_READ_DATA_COMPLETE   : {response.DCP_MSG_ID_READ_DATA_COMPLETE}")
            self.log.info(f"DCP_MSG_ID_RESET                : {response.DCP_MSG_ID_RESET}")
            return err_flag
        except Exception as e:
            self.log.error(f"❌ Error in test_client_get_capabilities: {e}")
            return False


    def test_client_start_stop(self, command=None):
        """Test MTS client start/stop functionality"""

        try:

            client_state = dcp_commands.client_get_state(
                        src_endpoint = self.die0_scp_trp_endpoint,
                        dest_die=0,
                        dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                        client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM)

            # the reset command in the setup should have put the client in stopped state
            if client_state != data_collection_protocol.client_get_state_msg.dcp_client_state_t.DCP_CLIENT_STATE_STOPPED:
                self.log.error(f"❌ Client is not in stopped state: {client_state}")
                return False

            # Exception raised if command fails
            dcp_commands.client_start_stop(
                    src_endpoint = self.die0_scp_trp_endpoint,
                    dest_die=0,
                    dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                    client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
                    state=data_collection_protocol.client_start_stop_msg.dcp_start_stop_state_t.DCP_START_STOP_STATE_START)

            client_state = dcp_commands.client_get_state(
                        src_endpoint = self.die0_scp_trp_endpoint,
                        dest_die=0,
                        dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                        client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM)

            if client_state != data_collection_protocol.client_get_state_msg.dcp_client_state_t.DCP_CLIENT_STATE_RUNNING:
                self.log.error(f"❌ Client is not in running state: {client_state}")
                return False

            # Exception raised if command fails
            dcp_commands.client_start_stop(
                    src_endpoint = self.die0_scp_trp_endpoint,
                    dest_die=0,
                    dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                    client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
                    state=data_collection_protocol.client_start_stop_msg.dcp_start_stop_state_t.DCP_START_STOP_STATE_STOP)

            client_state = dcp_commands.client_get_state(
                        src_endpoint = self.die0_scp_trp_endpoint,
                        dest_die=0,
                        dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                        client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM)

            if client_state != data_collection_protocol.client_get_state_msg.dcp_client_state_t.DCP_CLIENT_STATE_STOPPED:
                self.log.error(f"❌ Client is not in stopped state: {client_state}")
                return False

            dcp_commands.client_start_stop(
                    src_endpoint = self.die0_scp_trp_endpoint,
                    dest_die=0,
                    dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                    client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
                    state=data_collection_protocol.client_start_stop_msg.dcp_start_stop_state_t.DCP_START_STOP_STATE_START)

            client_state = dcp_commands.client_get_state(
                        src_endpoint = self.die0_scp_trp_endpoint,
                        dest_die=0,
                        dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                        client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM)

            if client_state != data_collection_protocol.client_get_state_msg.dcp_client_state_t.DCP_CLIENT_STATE_RUNNING:
                self.log.error(f"❌ Client is not in running state: {client_state}")
                return False

            return True

        except Exception as e:
            self.log.error(f"❌ Error in test_client_start_stop: {e}")
            return False

    def test_client_reset(self, command=None):
        """Test MTS client reset functionality"""

        try:
            #Send CLIENT_START_STOP Start to MCP Telemetry Service Client
            dcp_commands.client_start_stop(
                    src_endpoint = self.die0_scp_trp_endpoint,
                    dest_die=0,
                    dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                    client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
                    state=data_collection_protocol.client_start_stop_msg.dcp_start_stop_state_t.DCP_START_STOP_STATE_START)

            #Send CLIENT_GET_STATE to MCP Telemetry Service Client.
            client_state = dcp_commands.client_get_state(
                        src_endpoint = self.die0_scp_trp_endpoint,
                        dest_die=0,
                        dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                        client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM)

            #Verify state is running.
            if client_state == data_collection_protocol.client_get_state_msg.dcp_client_state_t.DCP_CLIENT_STATE_RUNNING:
                self.log.info(f"PASS : Client is in running state: {client_state}")
            else:
                self.log.error(f"❌ Client is not in running state: {client_state}")
                return False

            #Send CLIENT_RESET to MCP Telemetry Service Client.
            dcp_commands.client_reset(
                    src_endpoint = self.die0_scp_trp_endpoint,
                    dest_die=0,
                    dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                    client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM)

            #Send CLIENT_GET_STATE to MCP Telemetry Service Client.
            client_state = dcp_commands.client_get_state(
                        src_endpoint = self.die0_scp_trp_endpoint,
                        dest_die=0,
                        dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                        client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM)

            #Verify State is stopped.
            if client_state == data_collection_protocol.client_get_state_msg.dcp_client_state_t.DCP_CLIENT_STATE_STOPPED:
                self.log.info(f"PASS : Client is in stopped state: {client_state}")
            else:
                self.log.error(f"❌ Client is not in stopped state: {client_state}")
                return False

            #Send CLIENT_START_STOP Start to MCP Telemetry Service Client.
            dcp_commands.client_start_stop(
                    src_endpoint = self.die0_scp_trp_endpoint,
                    dest_die=0,
                    dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                    client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
                    state=data_collection_protocol.client_start_stop_msg.dcp_start_stop_state_t.DCP_START_STOP_STATE_START)

            #Send CLIENT_GET_STATE to MCP Telemetry Service Client.
            client_state = dcp_commands.client_get_state(
                        src_endpoint = self.die0_scp_trp_endpoint,
                        dest_die=0,
                        dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                        client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM)

            #Verify state is running.
            if client_state == data_collection_protocol.client_get_state_msg.dcp_client_state_t.DCP_CLIENT_STATE_RUNNING:
                self.log.info(f"PASS : Client is in running state: {client_state}")
            else:
                self.log.error(f"❌ Client is not in running state: {client_state}")
                return False

            #Uncomment once bug https://azurecsi.visualstudio.com/Dev/_workitems/edit/2619307 is resolved
            # #Send CLIENT_RESET to MCP DCP Service client.
            # dcp_commands.client_reset(
            #         src_endpoint = self.die0_scp_trp_endpoint,
            #         dest_die=0,
            #         dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
            #         client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_DCP_SVC)

            # #Send CLIENT_GET_STATE to MCP DCP Service client.
            # client_state = dcp_commands.client_get_state(
            #             src_endpoint = self.die0_scp_trp_endpoint,
            #             dest_die=0,
            #             dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
            #             client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_DCP_SVC)

            # #Verify State is stopped.
            # if client_state == data_collection_protocol.client_get_state_msg.dcp_client_state_t.DCP_CLIENT_STATE_STOPPED:
            #     self.log.error(f"PASS : Client is in stopped state: {client_state}")
            # else:
            #     self.log.error(f"❌ Client is not in stopped state: {client_state}")
            #     return False

            # #Send CLIENT_RESET to MCP Telemetry Service Client.
            # dcp_commands.client_reset(
            #         src_endpoint = self.die0_scp_trp_endpoint,
            #         dest_die=0,
            #         dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
            #         client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM)

            # #Send CLIENT_GET_STATE to MCP Telemetry Service Client.
            # client_state = dcp_commands.client_get_state(
            #             src_endpoint = self.die0_scp_trp_endpoint,
            #             dest_die=0,
            #             dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
            #             client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM)

            # #Verify State is stopped.
            # if client_state == data_collection_protocol.client_get_state_msg.dcp_client_state_t.DCP_CLIENT_STATE_STOPPED:
            #     self.log.error(f"PASS : Client is in stopped state: {client_state}")
            # else:
            #     self.log.error(f"❌ Client is not in stopped state: {client_state}")
            #     return False

            return True

        except Exception as e:
            self.log.error(f"❌ Error in test_client_reset: {e}")
            return False


import time
from fpfw_automation_primitives.serial.telnet import (
    Telnet_,
)
from trp_lib import trp_endpoint, mts_cli_trp_endpoint
from dcp_protocol import data_collection_protocol
from trp_protocol import transfer_relay_protocol, trp_node_t
import ctypes

# NOTE: running with the SCP UART window in the SVP gui will not work, as there will be data corruption due to multiple telnet connections
# can either run SVP in headless mode:  launch SVP via 'runsvp -SimConfig chie_bins_single_die_dat' or dual die
# option 2, launch SVP with the GUI and close the SCP uart window. 'runsvpgui -SimConfig chie_bins_single_die_dat' or dual die
# The SCP UART window may re-opened for manual testing, but will need to be closed again for this script to work
# In the SVP Design Hierarchy navigate to /KingsgateSVP/DIE_0/RMSS/SCP/SCP_UART,  right click, and select 'Connect Terminal to Uart'
#
# to run the main functionality below from a repo environment
# & (Join-Path ([System.Environment]::GetEnvironmentVariable("REPO_APP_PATH_python.win64", "Process")) "/tools/python.exe") tests/functional/pythia/pylibs/mts_tests/mts_dcp_test.py
#

def main():
    print("This is the main entry point of mts_dcp_test.py")

    # for manual testing redirect loggers to stdout
    logging.basicConfig(
        level=logging.DEBUG,  # Set the logging level (DEBUG, INFO, etc.)
        handlers=[logging.StreamHandler(sys.stdout)]  # Redirect to stdout
        )

    # die 0, SCP
    telnet_port = Telnet_(host="localhost", port="4257", encoding="UTF-8")
    telnet_port.open()

    die0_scp_trp_endpoint = mts_cli_trp_endpoint(telnet_port, 0, transfer_relay_protocol.cpu_type.CPU_SCP)

    # msg_status = dcp_commands.client_reset(
    #                 src_endpoint = die0_scp_trp_endpoint,
    #                 dest_die=0,
    #                 dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
    #                 client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM)

    # client_state = dcp_commands.client_get_state(
    #                 src_endpoint = die0_scp_trp_endpoint,
    #                 dest_die=0,
    #                 dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
    #                 client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM)

    # print(client_state)

    # event_tuple = [(
    #     0x0201,
    #     20,
    #     data_collection_protocol.client_events_enable_disable_msg.dcp_events_enable_state_t.DCP_EVENTS_ENABLE_STATE_ENABLE
    # ),
    # (
    #     0x0202,
    #     4,
    #     data_collection_protocol.client_events_enable_disable_msg.dcp_events_enable_state_t.DCP_EVENTS_ENABLE_STATE_ENABLE
    # )
    # ]

    # dcp_commands.client_enable_disable_events(
    #                 src_endpoint = die0_scp_trp_endpoint,
    #                 dest_die=0,
    #                 dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
    #                 client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
    #                 events=event_tuple)

    # dcp_commands.client_start_stop(
    #                 src_endpoint = die0_scp_trp_endpoint,
    #                 dest_die=0,
    #                 dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
    #                 client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
    #                 state=data_collection_protocol.client_start_stop_msg.dcp_start_stop_state_t.DCP_START_STOP_STATE_START)

    # client_state = dcp_commands.client_get_state(
    #                 src_endpoint = die0_scp_trp_endpoint,
    #                 dest_die=0,
    #                 dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
    #                 client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM)

    # print(client_state)

    # uncomment to manually stress test telemetry data collection
    start_time = time.time()
    while True:
        if time.time() - start_time > 10:
            print("Loop timeout reached")
            break

        while True:
            status, response = dcp_commands.client_read_data(
                src_endpoint=die0_scp_trp_endpoint,
                dest_die=0,
                dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM
                )


            if status not in [
                data_collection_protocol.dcp_status_t.DATA_COLLECTION_RD_DATA_VALID_LAST,
                data_collection_protocol.dcp_status_t.DATA_COLLECTION_RD_DATA_VALID_MORE
                ]:
                break

            print(f"phys: {response.physical_start_addr:x}, size: {response.physical_buffer_size:x}, offset: {response.rd_data_addr_offset:x}, phys offset start: {response.physical_start_addr + response.rd_data_addr_offset:x}, size: {response.rd_data_size:x}, crc: {response.crc:x}\n")

            dcp_commands.client_send_read_data_complete(
                src_endpoint=die0_scp_trp_endpoint,
                dest_die=0,
                dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
                rd_data_addr_offset=response.rd_data_addr_offset,
                rd_data_size=response.rd_data_size
                )

    #     time.sleep(60)

    # dcp_commands.client_start_stop(
    #                 src_endpoint = die0_scp_trp_endpoint,
    #                 dest_die=0,
    #                 dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
    #                 client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
    #                 state=data_collection_protocol.client_start_stop_msg.dcp_start_stop_state_t.DCP_START_STOP_STATE_STOP)

    telnet_port.close()

if __name__ == "__main__":
    main()