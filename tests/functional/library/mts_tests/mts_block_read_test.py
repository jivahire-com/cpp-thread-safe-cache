# Copyright (c) Microsoft Corporation. All rights reserved.
#
import logging
import ctypes
from fpfw_automation_primitives.serial.telnet import Telnet_
from trp_protocol import transfer_relay_protocol
from dcp_protocol import struct_to_hex_string, data_collection_protocol

logger = logging.getLogger(__name__)


class mts_read_intercore_block:
    def __init__(
        self,
        host: str,
        port: int,
        source_die: int,
        source_cpu: transfer_relay_protocol.cpu_type,
    ):
        self.host = host
        self.port = port
        self.source_die = source_die
        self.source_cpu = source_cpu
        self.source_comm_channel = None
        self.sequence_number = 0
        self.default_timeout_sec = 30

    def get_next_sequence_number(self) -> int:
        """Generate the next sequence number."""
        self.sequence_number = (self.sequence_number + 1) & 0xFFFF
        return self.sequence_number

    def open_telnet(self):
        """Open MCP telnet connection."""
        self.source_comm_channel = Telnet_(
            host=self.host, port=self.port, encoding="UTF-8"
        )
        self.source_comm_channel.open()
        logger.info("Telnet connection opened.")

    def send_read_intercore_block(self, core_id: int):
        """Prepare TRP header, send TRP_MSG_ID_READ_INTERCORE_BLOCK, and parse the response."""
        # Prepare TRP header
        trp_msg_hdr = transfer_relay_protocol.trp_msg_hdr_t()
        trp_msg_hdr.src_node.die_id = 0
        trp_msg_hdr.src_node.core_id = transfer_relay_protocol.cpu_type.CPU_MCP
        trp_msg_hdr.dest_node.die_id = 0
        trp_msg_hdr.dest_node.core_id = core_id
        trp_msg_hdr.mts_client_id = (
            data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM
        )
        trp_msg_hdr.trp_msg_id = (
            transfer_relay_protocol.trp_msg_id_t.TRP_MSG_ID_READ_INTERCORE_BLOCK.value
        )
        trp_msg_hdr.source_seq_num = self.get_next_sequence_number()
        trp_msg_hdr.payload_size = ctypes.sizeof(
            transfer_relay_protocol.trp_block_read_req_t
        )

        # Prepare block read complete request
        block_read_req = transfer_relay_protocol.trp_block_read_req_t()
        block_read_req.block_id = 0

        # Send the command
        send_str = (
            "mts trp_send "
            + struct_to_hex_string(trp_msg_hdr)
            + struct_to_hex_string(block_read_req)
        )
        logger.debug(f"Sending TRP_MSG_ID_READ_INTERCORE_BLOCK: {send_str}")
        self.source_comm_channel.write_line(write_string=send_str)

        # Read the response
        response = self.source_comm_channel.read_until(
            key="TrpRx", timeout_seconds=self.default_timeout_sec
        )
        logger.debug("Parsed TRP_MSG_ID_READ_INTERCORE_BLOCK_RESPONSE:")
        rsp_index = response.find("Rsp: ")
        if rsp_index == -1:
            raise ValueError("TRP Response not found in {response}")

        response_str = response[rsp_index + 4 :].strip()
        response_str = response_str.split("\n")[
            0
        ]  # Take only the part before the newline

        # Split the string into individual components and convert to list of integers
        response_list = [int(x) for x in response_str.split()]

        # Convert response_list to byte array
        response_bytes = bytearray(response_list)
        logger.debug(f"TRP response: {' '.join(f'{b:02X}' for b in response_bytes)}\n")

        if len(response_bytes) < ctypes.sizeof(transfer_relay_protocol.trp_msg_hdr_t):
            raise ValueError("Response size is too small to contain a valid header")

        # Extract the header
        header = transfer_relay_protocol.trp_msg_hdr_t.from_buffer_copy(
            response_bytes[: ctypes.sizeof(transfer_relay_protocol.trp_msg_hdr_t)]
        )

        # Validate the header status
        if (
            header.trp_msg_status
            != transfer_relay_protocol.trp_status_t.TRP_STATUS_SUCCESS
        ):
            raise ValueError(
                f"Header validation failed with status: {header.trp_msg_status}"
            )

        # Extract the payload
        payload = transfer_relay_protocol.trp_msg_read_block_rsp_t.from_buffer_copy(
            response_bytes[ctypes.sizeof(transfer_relay_protocol.trp_msg_hdr_t) :]
        )

        # Log the parsed response fields
        logger.debug("Parsed TRP_MSG_ID_READ_INTERCORE_BLOCK_RESPONSE:")
        logger.debug(f"client ID: {header.mts_client_id}")
        logger.debug(f"Message ID: {header.trp_msg_id}")
        logger.debug(f"Address offset: {payload.addr_offset}")
        logger.debug(f"block size: {payload.block_size}")

    def send_read_intercore_block_complete(self, core_id: int):
        """Send TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE."""
        # Prepare TRP header
        trp_msg_hdr = transfer_relay_protocol.trp_msg_hdr_t()
        trp_msg_hdr.src_node.die_id = 0
        trp_msg_hdr.src_node.core_id = transfer_relay_protocol.cpu_type.CPU_MCP
        trp_msg_hdr.dest_node.die_id = 0
        trp_msg_hdr.dest_node.core_id = core_id
        trp_msg_hdr.mts_client_id = (
            data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM
        )
        trp_msg_hdr.trp_msg_id = (
            transfer_relay_protocol.trp_msg_id_t.TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE.value
        )
        trp_msg_hdr.source_seq_num = self.get_next_sequence_number()
        trp_msg_hdr.payload_size = ctypes.sizeof(
            transfer_relay_protocol.trp_msg_read_block_rsp_t
        )

        # Prepare block read complete request
        block_read_req = transfer_relay_protocol.trp_msg_read_block_rsp_t()
        block_read_req.block_id = 42

        # Send the command
        send_str = (
            "mts trp_send "
            + struct_to_hex_string(trp_msg_hdr)
            + struct_to_hex_string(block_read_req)
        )
        logger.debug(f"Sending TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE: {send_str}")
        self.source_comm_channel.write_line(write_string=send_str)

    def close_telnet(self):
        """Close MCP telnet connection."""
        if self.source_comm_channel:
            self.source_comm_channel.close()
            logger.info("Telnet connection closed.")


# to run the main functionality below from a repo environment
# & (Join-Path ([System.Environment]::GetEnvironmentVariable("REPO_APP_PATH_python.win64", "Process")) "/tools/python.exe") tests/functional/library/mts_tests/mts_block_read_test.py # noqa: E501
#
def main():
    logging.basicConfig(level=logging.DEBUG, handlers=[logging.StreamHandler()])
    script = mts_read_intercore_block(
        host="localhost",
        port=4256,
        source_die=0,
        source_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
    )

    try:
        script.open_telnet()
        script.send_read_intercore_block(transfer_relay_protocol.cpu_type.CPU_SDM)
        script.send_read_intercore_block(transfer_relay_protocol.cpu_type.CPU_CDED_SDM)
        script.send_read_intercore_block_complete(
            transfer_relay_protocol.cpu_type.CPU_SDM
        )
        script.send_read_intercore_block_complete(
            transfer_relay_protocol.cpu_type.CPU_CDED_SDM
        )
    finally:
        script.close_telnet()


if __name__ == "__main__":
    main()
