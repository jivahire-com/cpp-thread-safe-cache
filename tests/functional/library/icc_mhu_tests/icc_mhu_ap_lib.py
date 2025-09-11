# Copyright (c) Microsoft Corporation. All rights reserved.
"""
icc_mhu_ap_lib.py - Python interface to AP ICC MHU interface
"""

import struct
import time
import logging
import os
import sys
from datetime import datetime
from typing import Optional, Tuple, List
from abc import ABC, abstractmethod

# Add paths for both package and direct imports
current_dir = os.path.dirname(os.path.abspath(__file__))
pylibs_dir = os.path.dirname(current_dir)
sys.path.extend([
    pylibs_dir,
    current_dir,
])

logger = logging.getLogger(__name__)


class CoreMemoryMapInterface(ABC):
    """Abstract base class for memory interface implementations"""

    @abstractmethod
    def read(self, address: int, size: int = 4) -> int:
        """
        Read memory at specified address.

        Args:
            address: Memory address to read
            size: Size in bytes (1, 2, 4, 8)

        Returns:
            Value read from memory
        """
        pass

    @abstractmethod
    def write(self, address: int, value: int, size: int = 4) -> None:
        """
        Write value to memory at specified address.

        Args:
            address: Memory address to write
            value: Value to write
            size: Size in bytes (1, 2, 4, 8)
        """
        pass

    def dump(self, address: int, size: int) -> bytearray:
        """
        Dump a block of memory efficiently.
        Default implementation using individual reads - can be overridden for efficiency.

        Args:
            address: Starting memory address
            size: Number of bytes to read

        Returns:
            Bytearray of memory values
        """
        return bytearray([self.read(address + i, 1) for i in range(size)])


class ApCoreMemoryMapInterface(CoreMemoryMapInterface):
    """AP-specific implementation of CoreMemoryMapInterface - stubbed for now"""

    def read(self, address: int, size: int = 4) -> int:
        """
        Read memory at specified address.

        Args:
            address: Memory address to read
            size: Size in bytes (1, 2, 4, 8)

        Returns:
            Value read from memory
        """
        # TODO: Implement actual AP memory read
        return 0

    def write(self, address: int, value: int, size: int = 4) -> None:
        """
        Write value to memory at specified address.

        Args:
            address: Memory address to write
            value: Value to write
            size: Size in bytes (1, 2, 4, 8)
        """
        # TODO: Implement actual AP memory write
        return

    def dump(self, address: int, size: int) -> bytearray:
        """
        Dump a block of memory efficiently.
        Default implementation using individual reads - can be overridden for efficiency.

        Args:
            address: Starting memory address
            size: Number of bytes to read

        Returns:
            Bytearray of memory values
        """
        return bytearray([self.read(address + i, 1) for i in range(size)])


class IccMhu:
    """
    Base class for ICC MHU (Message Handling Unit) interface.
    Provides common methods to send/receive packets and check status.
    """

    # Generic MHU Frame Addresses - to be overridden in subclasses
    SND_MHU_ADDRESS: int = 0  # Send Frame Address
    REC_MHU_ADDRESS: int = 0  # Recv Frame Address

    # Generic Payload Addresses - to be overridden in subclasses
    SEND_PAYLOAD_ADDR: int = 0  # Send payload address
    RECV_PAYLOAD_ADDR: int = 0  # Recv payload address
    PAYLOAD_SIZE = 0x200

    # Channel Offsets
    CHANNELS_OFFSET = 0x1000
    CHANNEL_STATUS_OFFSET = 0x0
    CHANNEL_SET_OFFSET = 0xC
    CHANNEL_CLEAR_OFFSET = 0x8

    # ICC MHU Packet Header Offsets
    VERSION_OFFSET = 0x0
    TOKEN_OFFSET = 0x4
    ID_OFFSET = 0x6
    COMMAND_OFFSET = 0x8
    PAYLOAD_SIZE_OFFSET = 0xC
    STATUS_OFFSET = 0xE
    PAYLOAD_OFFSET = 0x10

    def __init__(self, memory_interface: CoreMemoryMapInterface):
        """
        Initialize the ICC MHU interface.

        Args:
            memory_interface: Memory access interface implementation (required)
        """
        self.memory_interface = memory_interface

        # Validate that subclass has set required addresses
        if self.SND_MHU_ADDRESS == 0 or self.REC_MHU_ADDRESS == 0:
            raise ValueError("Subclass must define SND_MHU_ADDRESS and REC_MHU_ADDRESS")
        if self.SEND_PAYLOAD_ADDR == 0 or self.RECV_PAYLOAD_ADDR == 0:
            raise ValueError("Subclass must define SEND_PAYLOAD_ADDR and RECV_PAYLOAD_ADDR")

    def _read_memory(self, address: int, size: int = 4) -> int:
        """
        Read memory at specified address.

        Args:
            address: Memory address to read
            size: Size in bytes (1, 2, 4, 8)

        Returns:
            Value read from memory
        """
        return self.memory_interface.read(address, size)

    def _write_memory(self, address: int, value: int, size: int = 4) -> None:
        """
        Write value to memory at specified address.

        Args:
            address: Memory address to write
            value: Value to write
            size: Size in bytes (1, 2, 4, 8)
        """
        self.memory_interface.write(address, value, size)

    def _dump_memory(self, address: int, size: int) -> bytearray:
        """
        Dump a block of memory efficiently.

        Args:
            address: Starting memory address
            size: Number of bytes to read

        Returns:
            Bytearray of memory values
        """
        return self.memory_interface.dump(address, size)

    def check_packet_pending(self, channel_direction: str = "recv") -> bool:
        """
        Check if a packet is pending on the specified channel.

        Args:
            channel_direction: "recv" or "send"

        Returns:
            True if packet pending, False if not

        Raises:
            ValueError: If invalid channel direction
            RuntimeError: If memory access fails
        """
        if channel_direction not in ["recv", "send"]:
            raise ValueError("Invalid channel direction. Use 'recv' or 'send'")

        # Select MHU address based on direction
        if channel_direction == "recv":
            mhu_address = self.REC_MHU_ADDRESS
        else:  # send
            mhu_address = self.SND_MHU_ADDRESS

        # Calculate channel 0 flag 0 post box address
        ch_0_flag_0_pb_address = mhu_address + self.CHANNELS_OFFSET + self.CHANNEL_STATUS_OFFSET

        try:
            # Read the status register
            status = self._read_memory(ch_0_flag_0_pb_address, 4)

            logger.debug(f"Checking for Pending Packet - Address [0x{ch_0_flag_0_pb_address:08X}] Status [0x{status:X}]")

            return bool(status & 0x1)  # Return bit 0 status as boolean

        except Exception as e:
            raise RuntimeError(f"Failed to check packet pending: {e}")

    def get_packet(self, timeout_sec: int = 5) -> Optional[bytearray]:
        """
        Get a packet from the receive channel and return its contents.

        Args:
            timeout_sec: Timeout in seconds to wait for packet

        Returns:
            bytearray containing the complete packet (header + payload), or None on failure

        Raises:
            TimeoutError: If no packet is received within timeout_sec
        """
        # Wait for packet with timeout
        start_time = time.time()
        while True:
            try:
                pending_status = self.check_packet_pending("recv")
                if pending_status:
                    break
            except (ValueError, RuntimeError) as e:
                logger.error(f"Failed to check packet pending status: {e}")
                return None

            # Check timeout
            if time.time() - start_time > timeout_sec:
                raise TimeoutError(f"Timeout waiting for packet after {timeout_sec} seconds")

            # Small delay before checking again
            time.sleep(0.01)

        try:
            # Read packet header
            version = self._read_memory(self.RECV_PAYLOAD_ADDR + self.VERSION_OFFSET, 4)
            token = self._read_memory(self.RECV_PAYLOAD_ADDR + self.TOKEN_OFFSET, 2)
            packet_id = self._read_memory(self.RECV_PAYLOAD_ADDR + self.ID_OFFSET, 2)
            command = self._read_memory(self.RECV_PAYLOAD_ADDR + self.COMMAND_OFFSET, 4)
            payload_size = self._read_memory(self.RECV_PAYLOAD_ADDR + self.PAYLOAD_SIZE_OFFSET, 2)
            status = self._read_memory(self.RECV_PAYLOAD_ADDR + self.STATUS_OFFSET, 2)

            logger.debug("Getting ICC Packet.")
            logger.debug(f"ICC RECV Payload Addr [0x{self.RECV_PAYLOAD_ADDR:08X}]")
            logger.debug(f"    Version [0x{version:08X}]")
            logger.debug(f"    Token   [0x{token:04X}]")
            logger.debug(f"    Id      [0x{packet_id:04X}]")
            logger.debug(f"    Command [0x{command:08X}]")
            logger.debug(f"    Size    [0x{payload_size:04X}]")
            logger.debug(f"    STATUS  [0x{status:04X}]")

            # Read payload data efficiently
            payload_data = self._dump_memory(self.RECV_PAYLOAD_ADDR + self.PAYLOAD_OFFSET, payload_size)

            # payload_data is already a bytearray from _dump_memory
            packet_data = payload_data

            # Clear the pending status
            self._clear_pending_recv()

            # Verify pending flag is cleared
            try:
                pending_after_clear = self.check_packet_pending("recv")
                if pending_after_clear:
                    logger.error("Failed to clear pending bit!")
                    return None
            except (ValueError, RuntimeError) as e:
                logger.error(f"Failed to verify pending bit cleared: {e}")
                return None

            return packet_data

        except Exception as e:
            logger.error(f"Failed to get packet: {e}")
            return None

    def send_packet(self, packet: bytearray, timeout_sec: int = 5) -> None:
        """
        Send a packet with the specified data.

        Args:
            packet: Packet data as bytearray
            timeout_sec: Timeout in seconds to wait for send channel availability

        Raises:
            TimeoutError: If send channel doesn't become available within timeout_sec
            RuntimeError: If packet sending fails
        """

        # Wait for send channel to be available with timeout
        start_time = time.time()
        while True:
            try:
                pending_status = self.check_packet_pending("send")
                if not pending_status:  # Channel is available when NOT pending
                    break
            except (ValueError, RuntimeError) as e:
                raise RuntimeError(f"Failed to check packet pending status: {e}")

            # Check timeout
            if time.time() - start_time > timeout_sec:
                raise TimeoutError(f"Timeout waiting for send channel to be available after {timeout_sec} seconds")

            # Small delay before checking again
            time.sleep(0.01)

        packet_size = len(packet)

        logger.debug("Sending ICC Packet.")
        logger.debug(f"Packet Size [{packet_size}]")
        logger.debug(f"Packet {[hex(b) for b in packet]}")

        try:
            # Write packet size
            self._write_memory(self.SEND_PAYLOAD_ADDR + self.PAYLOAD_SIZE_OFFSET, packet_size, 2)

            # Write packet data
            for i, byte_val in enumerate(packet):
                self._write_memory(self.SEND_PAYLOAD_ADDR + self.PAYLOAD_OFFSET + i, byte_val, 1)

            # Trigger the send by setting the MHU channel
            mhu_set_address = self.SND_MHU_ADDRESS + self.CHANNELS_OFFSET + self.CHANNEL_SET_OFFSET
            self._write_memory(mhu_set_address, 0x1, 4)

            logger.debug("Packet sent successfully")

        except Exception as e:
            raise RuntimeError(f"Failed to send packet: {e}")

    def _clear_pending_recv(self) -> None:
        """Clear the pending status on the receive mailbox."""
        ch_0_flag_0_clr_address = (self.REC_MHU_ADDRESS +
                                  self.CHANNELS_OFFSET +
                                  self.CHANNEL_CLEAR_OFFSET)
        self._write_memory(ch_0_flag_0_clr_address, 0x1, 4)


class IccMhuAp(IccMhu):
    """
    Python interface to AP ICC MHU (Message Handling Unit) interface.
    Provides methods to send/receive packets and check status.
    """

    # AP-specific MHU Frame Addresses
    AP2MCP_MHU_SND_NS_ADDRESS = 0x00002AA00000  # AP2MCP Send Frame
    MCP2AP_MHU_REC_NS_ADDRESS = 0x00002AA10000  # MCP2AP Recv Frame

    # Map to base class generic names
    SND_MHU_ADDRESS = AP2MCP_MHU_SND_NS_ADDRESS
    REC_MHU_ADDRESS = MCP2AP_MHU_REC_NS_ADDRESS

    # AP-specific Payload Addresses (from DDR region mapping)
    SEND_PAYLOAD_ADDR = 0x20128000E00  # AP Send payload (MCP's Recv payload)
    RECV_PAYLOAD_ADDR = 0x20128000C00  # AP Recv payload (MCP's Send payload)

    def __init__(self, memory_interface: Optional[CoreMemoryMapInterface] = None):
        """
        Initialize the ICC MHU AP interface.

        Args:
            memory_interface: Memory access interface implementation (optional, defaults to ApCoreMemoryMapInterface)
        """
        if memory_interface is None:
            memory_interface = ApCoreMemoryMapInterface()
        super().__init__(memory_interface)


# ============================================================================
# Test Infrastructure - Mock Memory Interface and Test Functions
# ============================================================================

class MockMemoryInterface(CoreMemoryMapInterface):
    """Mock memory interface for testing IccMhuAp without real hardware"""

    def __init__(self, debug: bool = False):
        """
        Initialize mock memory interface.

        Args:
            debug: Enable debug logging for memory operations
        """
        self.memory = {}  # address -> value mapping
        self.operations_log = []  # Track all memory operations
        self.debug = debug
        self.fail_on_read = False
        self.fail_on_write = False
        self.read_delay = 0.0
        self.write_delay = 0.0

    def read(self, address: int, size: int = 4) -> int:
        """
        Mock memory read operation.

        Args:
            address: Memory address to read
            size: Size in bytes (1, 2, 4, 8)

        Returns:
            Value read from memory

        Raises:
            RuntimeError: If configured to fail
        """
        if self.read_delay > 0:
            time.sleep(self.read_delay)

        if self.fail_on_read:
            raise RuntimeError("Mock read failure")

        value = self.memory.get(address, 0)
        self.operations_log.append(('read', address, size, value))

        if self.debug:
            logger.debug(f"MockMem READ  [0x{address:08X}] size={size} -> 0x{value:X}")

        return value

    def write(self, address: int, value: int, size: int = 4) -> None:
        """
        Mock memory write operation.

        Args:
            address: Memory address to write
            value: Value to write
            size: Size in bytes (1, 2, 4, 8)

        Raises:
            RuntimeError: If configured to fail
        """
        if self.write_delay > 0:
            time.sleep(self.write_delay)

        if self.fail_on_write:
            raise RuntimeError("Mock write failure")

        self.memory[address] = value
        self.operations_log.append(('write', address, value, size))

        # Simulate hardware behavior: writing to clear register clears status register
        recv_clear_addr = (IccMhuAp.MCP2AP_MHU_REC_NS_ADDRESS +
                          IccMhuAp.CHANNELS_OFFSET +
                          IccMhuAp.CHANNEL_CLEAR_OFFSET)
        if address == recv_clear_addr and value == 0x1:
            # Clear the corresponding status register
            recv_status_addr = (IccMhuAp.MCP2AP_MHU_REC_NS_ADDRESS +
                               IccMhuAp.CHANNELS_OFFSET +
                               IccMhuAp.CHANNEL_STATUS_OFFSET)
            self.memory[recv_status_addr] = 0
            if self.debug:
                logger.debug(f"MockMem: Clearing recv status register due to clear command")

        if self.debug:
            logger.debug(f"MockMem WRITE [0x{address:08X}] size={size} <- 0x{value:X}")

    def dump(self, address: int, size: int) -> bytearray:
        """
        Mock memory dump operation.

        Args:
            address: Starting memory address
            size: Number of bytes to read

        Returns:
            Bytearray of memory values
        """
        data = bytearray([self.memory.get(address + i, i % 256) for i in range(size)])
        self.operations_log.append(('dump', address, size, data))

        if self.debug:
            logger.debug(f"MockMem DUMP  [0x{address:08X}] size={size} -> {[hex(b) for b in data[:8]]}")

        return data

    def configure_scenario(self, scenario: str, **kwargs):
        """
        Configure memory for specific test scenarios.

        Args:
            scenario: Test scenario name
            **kwargs: Additional scenario parameters
        """
        if scenario == "recv_packet_ready":
            # Setup receive channel with pending packet
            recv_status_addr = (IccMhuAp.MCP2AP_MHU_REC_NS_ADDRESS +
                               IccMhuAp.CHANNELS_OFFSET +
                               IccMhuAp.CHANNEL_STATUS_OFFSET)
            self.memory[recv_status_addr] = 1

            # Setup packet data
            payload_data = kwargs.get('payload_data', [0x10, 0x11, 0x12, 0x13])
            command = kwargs.get('command', 0x9ABC)
            token = kwargs.get('token', 0x1234)
            packet_id = kwargs.get('packet_id', 0x5678)

            base_addr = IccMhuAp.RECV_PAYLOAD_ADDR
            self.memory[base_addr + IccMhuAp.VERSION_OFFSET] = 1
            self.memory[base_addr + IccMhuAp.TOKEN_OFFSET] = token
            self.memory[base_addr + IccMhuAp.ID_OFFSET] = packet_id
            self.memory[base_addr + IccMhuAp.COMMAND_OFFSET] = command
            self.memory[base_addr + IccMhuAp.PAYLOAD_SIZE_OFFSET] = len(payload_data)
            self.memory[base_addr + IccMhuAp.STATUS_OFFSET] = 0

            # Setup payload
            for i, byte_val in enumerate(payload_data):
                self.memory[base_addr + IccMhuAp.PAYLOAD_OFFSET + i] = byte_val

        elif scenario == "send_channel_available":
            # Setup send channel as available (not pending)
            send_status_addr = (IccMhuAp.AP2MCP_MHU_SND_NS_ADDRESS +
                               IccMhuAp.CHANNELS_OFFSET +
                               IccMhuAp.CHANNEL_STATUS_OFFSET)
            self.memory[send_status_addr] = 0

        elif scenario == "send_channel_busy":
            # Setup send channel as busy (pending)
            send_status_addr = (IccMhuAp.AP2MCP_MHU_SND_NS_ADDRESS +
                               IccMhuAp.CHANNELS_OFFSET +
                               IccMhuAp.CHANNEL_STATUS_OFFSET)
            self.memory[send_status_addr] = 1

        elif scenario == "no_packets":
            # No packets pending on either channel
            recv_addr = (IccMhuAp.MCP2AP_MHU_REC_NS_ADDRESS +
                        IccMhuAp.CHANNELS_OFFSET +
                        IccMhuAp.CHANNEL_STATUS_OFFSET)
            send_addr = (IccMhuAp.AP2MCP_MHU_SND_NS_ADDRESS +
                        IccMhuAp.CHANNELS_OFFSET +
                        IccMhuAp.CHANNEL_STATUS_OFFSET)
            self.memory[recv_addr] = 0
            self.memory[send_addr] = 0

    def get_operation_count(self, operation_type: Optional[str] = None) -> int:
        """Get count of memory operations performed."""
        if operation_type is None:
            return len(self.operations_log)
        return sum(1 for op in self.operations_log if op[0] == operation_type)

    def clear_log(self):
        """Clear the operations log."""
        self.operations_log.clear()


def test_basic_functionality(verbose: bool = True) -> bool:
    """
    Test basic IccMhuAp functionality with mock memory.

    Args:
        verbose: Enable verbose output

    Returns:
        True if all tests pass, False otherwise
    """
    if verbose:
        print("=== Testing Basic IccMhuAp Functionality ===")

    # Create mock memory and MHU instance
    mock_memory = MockMemoryInterface(debug=verbose)
    mhu = IccMhuAp(memory_interface=mock_memory)

    test_results = []

    # Test 1: Check packet pending (should be False initially)
    try:
        mock_memory.configure_scenario("no_packets")
        result = mhu.check_packet_pending("recv")
        test_results.append(result == False)
        if verbose:
            print(f"✓ Test 1 - Initial packet pending check: {result}")
    except Exception as e:
        test_results.append(False)
        if verbose:
            print(f"✗ Test 1 - Packet pending check failed: {e}")

    # Test 2: Send packet
    try:
        mock_memory.configure_scenario("send_channel_available")
        mhu.send_packet(bytearray([0x34, 0x12, 0x00, 0x00, 0xAA, 0xBB, 0xCC]), timeout_sec=1)
        test_results.append(True)
        if verbose:
            print("✓ Test 2 - Send packet successful")
    except Exception as e:
        test_results.append(False)
        if verbose:
            print(f"✗ Test 2 - Send packet failed: {e}")

    # Test 3: Receive packet
    try:
        mock_memory.configure_scenario("recv_packet_ready",
                                     payload_data=[0x10, 0x11, 0x12, 0x13])
        packet = mhu.get_packet(timeout_sec=1)
        success = packet is not None and len(packet) == 4
        test_results.append(success)
        if verbose:
            if success:
                print(f"✓ Test 3 - Receive packet successful: {[hex(b) for b in packet] if packet else 'None'}")
            else:
                print("✗ Test 3 - Receive packet failed or returned None")
    except Exception as e:
        test_results.append(False)
        if verbose:
            print(f"✗ Test 3 - Receive packet failed: {e}")

    # Test 4: Invalid direction
    try:
        mhu.check_packet_pending("invalid")
        test_results.append(False)  # Should have failed
        if verbose:
            print("✗ Test 4 - Should have failed with invalid direction")
    except ValueError:
        test_results.append(True)
        if verbose:
            print("✓ Test 4 - Correctly rejected invalid direction")
    except Exception as e:
        test_results.append(False)
        if verbose:
            print(f"✗ Test 4 - Unexpected error: {e}")

    passed = sum(test_results)
    total = len(test_results)

    if verbose:
        print(f"Basic functionality tests: {passed}/{total} passed")

    return passed == total


def test_error_scenarios(verbose: bool = True) -> bool:
    """
    Test error handling scenarios.

    Args:
        verbose: Enable verbose output

    Returns:
        True if all tests pass, False otherwise
    """
    if verbose:
        print("\n=== Testing Error Scenarios ===")

    test_results = []

    # Test 1: Memory interface that always fails
    try:
        mock_memory = MockMemoryInterface()
        mock_memory.fail_on_read = True
        mock_memory.fail_on_write = True
        mhu = IccMhuAp(memory_interface=mock_memory)
        mhu.check_packet_pending("recv")
        test_results.append(False)  # Should have failed
        if verbose:
            print("✗ Test 1 - Should have failed with memory interface that always fails")
    except Exception:
        test_results.append(True)
        if verbose:
            print("✓ Test 1 - Correctly handled memory interface failure")

    # Test 2: Memory read failure
    try:
        mock_memory = MockMemoryInterface()
        mock_memory.fail_on_read = True
        mhu = IccMhuAp(memory_interface=mock_memory)
        mhu.check_packet_pending("recv")
        test_results.append(False)  # Should have failed
        if verbose:
            print("✗ Test 2 - Should have failed with memory read error")
    except Exception:
        test_results.append(True)
        if verbose:
            print("✓ Test 2 - Correctly handled memory read failure")

    # Test 3: Timeout scenario
    try:
        mock_memory = MockMemoryInterface()
        mock_memory.configure_scenario("no_packets")
        mhu = IccMhuAp(memory_interface=mock_memory)
        mhu.get_packet(timeout_sec=1)
        test_results.append(False)  # Should have timed out
        if verbose:
            print("✗ Test 3 - Should have timed out")
    except TimeoutError:
        test_results.append(True)
        if verbose:
            print("✓ Test 3 - Correctly timed out waiting for packet")
    except Exception as e:
        test_results.append(False)
        if verbose:
            print(f"✗ Test 3 - Unexpected error: {e}")

    # Test 4: Send channel busy timeout
    try:
        mock_memory = MockMemoryInterface()
        mock_memory.configure_scenario("send_channel_busy")
        mhu = IccMhuAp(memory_interface=mock_memory)
        mhu.send_packet(bytearray([0x34, 0x12, 0x00, 0x00]), timeout_sec=1)
        test_results.append(False)  # Should have timed out
        if verbose:
            print("✗ Test 4 - Should have timed out on busy send channel")
    except TimeoutError:
        test_results.append(True)
        if verbose:
            print("✓ Test 4 - Correctly timed out on busy send channel")
    except Exception as e:
        test_results.append(False)
        if verbose:
            print(f"✗ Test 4 - Unexpected error: {e}")

    passed = sum(test_results)
    total = len(test_results)

    if verbose:
        print(f"Error scenario tests: {passed}/{total} passed")

    return passed == total


def test_performance(verbose: bool = True) -> bool:
    """
    Test performance characteristics.

    Args:
        verbose: Enable verbose output

    Returns:
        True if performance is acceptable, False otherwise
    """
    if verbose:
        print("\n=== Testing Performance ===")

    mock_memory = MockMemoryInterface()
    mhu = IccMhuAp(memory_interface=mock_memory)
    mock_memory.configure_scenario("send_channel_available")

    # Time multiple send operations
    start_time = time.time()

    for i in range(100):
        packet = bytearray([i % 256, (i+1) % 256, (i+2) % 256, (i+3) % 256])
        mhu.send_packet(packet, timeout_sec=1)

    elapsed = time.time() - start_time

    if verbose:
        print(f"✓ Sent 100 packets in {elapsed:.3f} seconds")
        print(f"  Average: {elapsed/100*1000:.2f} ms per packet")
        print(f"  Memory operations: {mock_memory.get_operation_count()}")

    # Performance should be reasonable (less than 5 seconds for 100 operations)
    performance_ok = elapsed < 5.0

    if verbose:
        if performance_ok:
            print("✓ Performance test passed")
        else:
            print("⚠ Performance might be slower than expected")

    return performance_ok


def main():
    """Main function for testing IccMhuAp functionality."""
    import argparse

    parser = argparse.ArgumentParser(description="ICC MHU AP Library Test Suite")
    parser.add_argument('--test', choices=['basic', 'errors', 'performance', 'all'],
                       default='all', help='Test suite to run')
    parser.add_argument('--verbose', action='store_true', default=True,
                       help='Enable verbose output')
    parser.add_argument('--debug', action='store_true',
                       help='Enable debug logging')

    args = parser.parse_args()

    # Setup logging
    if args.debug:
        logging.basicConfig(level=logging.DEBUG,
                          format='%(levelname)s: %(name)s: %(message)s')
    else:
        logging.basicConfig(level=logging.INFO,
                          format='%(levelname)s: %(message)s')

    print("ICC MHU AP Library Test Suite")
    print("=" * 40)

    results = []

    if args.test in ['basic', 'all']:
        results.append(test_basic_functionality(args.verbose))

    if args.test in ['errors', 'all']:
        results.append(test_error_scenarios(args.verbose))

    if args.test in ['performance', 'all']:
        results.append(test_performance(args.verbose))

    print("\n" + "=" * 40)
    passed = sum(results)
    total = len(results)

    if passed == total:
        print(f"🎉 All {total} test suite(s) PASSED!")
        return 0
    else:
        print(f"❌ {total - passed} of {total} test suite(s) FAILED!")
        return 1


if __name__ == "__main__":
    exit(main())