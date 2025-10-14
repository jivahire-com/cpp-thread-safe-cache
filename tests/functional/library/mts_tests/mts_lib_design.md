# MTS Library Design Document

This document describes the Message Transfer Service (MTS) library design, public API, and architectural patterns for enabling telemetry data collection and inter-core communication through the Data Collection Protocol (DCP) and Transfer Relay Protocol (TRP) on Kingsgate silicon.

## Table of Contents

[[_TOC_]]

## Introduction

### Description

The MTS (Message Transfer Service) library provides a comprehensive framework for telemetry data collection and inter-core communication on Kingsgate silicon. The library implements both the Data Collection Protocol (DCP) for telemetry services and the Transfer Relay Protocol (TRP) for message routing between processor cores (SCP, MCP, AP).

The library consists of:
- **DCP Protocol Layer**: `data_collection_protocol` defining message structures and constants
- **DCP Commands Layer**: `dcp_commands` implementing high-level DCP operations
- **TRP Protocol Layer**: `transfer_relay_protocol` defining message routing structures
- **TRP Endpoint Abstractions**: `trp_endpoint` providing pluggable communication interfaces
- **Communication Implementations**: CLI-based and ICC MHU-based endpoint implementations
- **Integration Testing**: `MtsDcpTest` class for comprehensive functional testing

### Terms

| Term   | Description                                           |
| ------ | ----------------------------------------------------- |
| MTS    | Message Transfer Service                              |
| DCP    | Data Collection Protocol                              |
| TRP    | Transfer Relay Protocol                               |
| AP     | Application Processor                                 |
| SCP    | System Control Processor                              |
| MCP    | Management Control Processor                          |
| HSP    | Hardware Security Processor                           |
| TBP    | Trusted Boot Processor                                |
| SDM    | Security and Debug Manager                            |
| SVP    | Software Verification Platform                        |
| FPGA   | Field-Programmable Gate Array                         |
| ICC    | Inter-Core Communication                              |
| MHU    | Message Handling Unit                                 |

### Reference Documents

| Document                                  | Link                                |
| ----------------------------------------- | ----------------------------------- |
| Data Collection Protocol Specification   | DCP protocol implementation constants |
| Transfer Relay Protocol Specification    | TRP message routing definitions     |
| Kingsgate ICC Shared Header              | silibs/libraries/kng_silibs_platform/include/kng_icc_shared.h |
| Pythia 2.0 Test Framework               | pythia/tdk/echofalls/echofalls_base_test.py |

## Requirements

The MTS library design requirements:

- The library shall provide comprehensive DCP message handling for all supported client types
- The library shall support pluggable TRP endpoint implementations for different communication channels
- The library shall implement timeout-based message operations to prevent system deadlocks
- The library shall validate protocol message integrity and provide detailed error reporting
- The library shall support telemetry data collection with manifest and platform information retrieval
- The library shall provide event-driven telemetry with configurable provider and event filtering
- The library shall implement proper sequence number management for message correlation
- The library shall support both CLI-based and ICC MHU-based communication endpoints
- The library shall provide comprehensive logging and debugging capabilities for protocol analysis
- The library shall support file-based data extraction for telemetry analysis workflows

## Dependencies

The MTS library dependencies:

- **ICC MHU Library**: icc_mhu_ap_lib for hardware-based inter-core communication
- **Serial Communication Primitives**: fpfw_automation_primitives.serial for telnet/direct connections
- **Pythia TDK**: pythia.tdk.echofalls for test infrastructure and DUT management
- **ctypes Library**: Python ctypes for C structure manipulation and protocol implementation
- **Standard Library**: logging, time, pathlib, typing, enum, abc for core functionality
- **Test Framework**: EchoFallsBaseTest for integration testing infrastructure

## Design

### Architecture Overview

The MTS library implements a layered protocol stack with pluggable transport mechanisms:

**UML Notation Key:**
- `<|--` : **Inheritance** (solid line) - "extends" or "is-a" relationship
- `<|..` : **Interface Implementation** (dotted line) - "implements" relationship
- `*--` : **Composition** (filled diamond) - full lifecycle ownership (creates, owns, destroys)
- `o--` : **Aggregation** (empty diamond) - shared/injected ownership (manages but doesn't create)
- `-->` : **Uses** (solid arrow) - direct usage or strong dependency relationship
- `..>` : **Depends On** (dotted arrow) - loose dependency or occasional usage

:::mermaid
classDiagram
    %% Protocol definition layer
    class data_collection_protocol {
        <<protocol>>
        +mts_client_id_t: IntEnum
        +dcp_msg_id_t: IntEnum
        +dcp_status_t: IntEnum
        +dcp_msg_hdr_t: ctypes.Structure
        +client_events_enable_disable_msg: class
        +client_start_stop_msg: class
        +client_get_state_msg: class
        +client_read_data_msg: class
        +client_get_manifest_msg: class
    }

    class transfer_relay_protocol {
        <<protocol>>
        +cpu_type: IntEnum
        +trp_status_t: IntEnum
        +trp_msg_id_t: IntEnum
        +trp_msg_hdr_t: ctypes.Structure
        +trp_block_read_req_t: ctypes.Structure
        +trp_msg_read_block_rsp_t: ctypes.Structure
    }

    %% Command implementation layer
    class dcp_commands {
        <<static>>
        -_sequence_number: int
        +create_header(msg_id: int, payload_size: int, client_id) List[int]
        +client_enable_disable_events(...) None
        +client_start_stop(...) None
        +client_get_state(...) dcp_client_state_t
        +client_get_manifest(...) Tuple
        +client_get_platform_information(...) Tuple
        +client_get_capabilities(...) Tuple
        +client_read_data(...) Tuple
        +client_send_read_data_complete(...) dcp_status_t
        +client_reset(...) dcp_status_t
        +validate_response(response, logger) bool
    }

    %% Abstract endpoint layer
    class trp_endpoint {
        <<abstract>>
        #source_die: ctypes.c_uint8
        #source_cpu: cpu_type
        #sequence_number: int
        #default_timeout_sec: int
        +send_dcp_message(...) bytearray
        +send_trp_message() str
        +get_next_sequence_number() int
        +create_dcp_forward_trp_header(...) trp_msg_hdr_t
        +log_trp_response_headers(...) None
        +log_dcp_response_header(...) None
        +close() void
    }

    class CoreMemoryMapAccess {
        <<interface>>
        +read_to_file(address: int, size: int, filename: str) None
    }

    %% Concrete endpoint implementations
    class mts_cli_trp_endpoint {
        -source_comm_channel
        +_read_channel_response(timeout_sec: float) bytearray
        +send_trp_message() str
        +send_dcp_message(...) bytearray
    }

    class mts_icc_mhu_trp_endpoint {
        -icc_mhu: IccMhu
        +send_trp_message() str
        +send_dcp_message(...) bytearray
        +read_to_file(address: int, size: int, filename: str) None
    }

    class mts_ap_endpoint {
        +send_dcp_message(...) bytearray
    }

    %% ICC MHU integration layer (from icc_mhu_tests)
    class IccMhuAp {
        <<external>>
        +send_packet(packet: bytearray, timeout_sec: int) None
        +get_packet(timeout_sec: int) bytearray
        +close() None
    }

    class ApCoreMemoryInterface {
        <<external>>
        +read(address: int, size: int) int
        +write(address: int, value: int, size: int) void
        +dump(address: int, size: int) bytearray
        +close() void
    }

    %% Test infrastructure layer
    class MtsDcpTest {
        -_scp_mcp_heartbeat_received: bool
        -die0_scp_channel
        -die0_mcp_channel
        -die0_scp_trp_endpoint: mts_cli_trp_endpoint
        +wait_for_scp_mcp_heartbeat() bool
        +setup() bool
        +teardown() None
        +test_client_read_data_polling() bool
        +test_client_get_manifest() bool
        +test_client_get_platform_information() bool
        +test_client_get_capabilities() bool
        +test_client_start_stop() bool
        +test_client_reset() bool
    }

    %% External dependencies
    class IccMhu {
        <<external>>
        +send_packet(packet: bytearray, timeout_sec: int) None
        +get_packet(timeout_sec: int) bytearray
        +close() None
    }

    class EchoFallsBaseTest {
        <<external>>
        +dut
        +log
        +setup() None
        +teardown() None
    }

    %% Protocol relationships
    dcp_commands --> data_collection_protocol : uses
    trp_endpoint --> transfer_relay_protocol : uses
    trp_endpoint --> data_collection_protocol : uses

    %% Inheritance relationships
    trp_endpoint <|-- mts_cli_trp_endpoint : extends
    trp_endpoint <|-- mts_icc_mhu_trp_endpoint : extends
    mts_icc_mhu_trp_endpoint <|-- mts_ap_endpoint : extends
    EchoFallsBaseTest <|-- MtsDcpTest : extends

    %% Interface implementations
    CoreMemoryMapAccess <|.. mts_icc_mhu_trp_endpoint : implements

    %% Aggregation relationships (dependency injection)
    mts_icc_mhu_trp_endpoint o-- IccMhu : manages injected MHU instance
    mts_ap_endpoint o-- IccMhuAp : manages injected MHU AP instance
    IccMhuAp o-- ApCoreMemoryInterface : manages injected memory interface
    MtsDcpTest o-- mts_cli_trp_endpoint : manages injected endpoint

    %% Dependency relationships
    dcp_commands --> trp_endpoint : uses
    MtsDcpTest --> dcp_commands : uses

    %% Documentation notes
    note for data_collection_protocol "Protocol constants and message structures"
    note for trp_endpoint "Abstract base for pluggable communication"
    note for mts_icc_mhu_trp_endpoint "TRP endpoint with MHU transport.<br/>Uses aggregation: IccMhu instance injected via constructor,<br/>endpoint manages but doesn't create MHU hardware interface"
    note for mts_ap_endpoint "AP-specific endpoint without TRP wrapper.<br/>Uses aggregation: creates IccMhuAp with injected ApCoreMemoryInterface memory interface,<br/>provides direct DCP-to-hardware communication for AP cores"
    note for IccMhuAp "ICC MHU AP implementation from icc_mhu_tests library.<br/>Provides MHU packet send/receive with memory interface abstraction"
    note for ApCoreMemoryInterface "T32-based memory interface from icc_mhu_tests library.<br/>Provides hardware memory access through Trace32 debugger"
    note for MtsDcpTest "Integration test infrastructure.<br/>Uses aggregation: endpoints created externally and injected,<br/>test class manages lifecycle but doesn't create transport channels"
:::

### Communication Flow

The typical DCP message exchange follows this pattern:

:::mermaid
sequenceDiagram
    participant Test as Test Client
    participant Commands as dcp_commands
    participant Endpoint as trp_endpoint
    participant TRP as TRP Layer
    participant Target as Target CPU

    Note over Test, Target: DCP Message Send Flow
    Test->>Commands: client_get_state(src_endpoint, dest_die, dest_cpu, client_id)
    Commands->>Commands: create_header(msg_id, payload_size, client_id)
    Commands->>Commands: _get_next_sequence_number()
    Commands->>Endpoint: send_dcp_message(dest_die, dest_cpu, client_id, dcp_msg)
    Endpoint->>Endpoint: create_dcp_forward_trp_header(...)
    Endpoint->>TRP: TRP header + DCP message
    TRP->>Target: Forwarded DCP message

    Note over Test, Target: DCP Response Flow
    Target->>TRP: DCP response
    TRP->>Endpoint: TRP header + DCP response
    Endpoint->>Endpoint: log_trp_response_headers(...)
    Endpoint->>Commands: DCP response bytes
    Commands->>Commands: validate_response(msg_status)
    Commands->>Commands: parse response structure
    Commands->>Test: Parsed response object

    Note over Test, Target: Error Handling
    alt DCP Error Status
        Commands->>Commands: validate_response() returns False
        Commands->>Test: ValueError with detailed error
    end
:::

### Telemetry Data Collection Flow

The telemetry collection process demonstrates the complete MTS workflow:

:::mermaid
sequenceDiagram
    participant Test as MtsDcpTest
    participant DCP as dcp_commands
    participant Endpoint as mts_cli_trp_endpoint
    participant MCP as MCP Telemetry Client

    Note over Test, MCP: Setup Phase
    Test->>DCP: client_reset(MTS_CLIENT_ID_PWR_INST_TELEM)
    DCP->>MCP: Reset telemetry client
    Test->>DCP: client_enable_disable_events(events=[(provider_id, event_id, ENABLE)])
    DCP->>MCP: Enable specific telemetry events

    Note over Test, MCP: Start Collection
    Test->>DCP: client_start_stop(state=START)
    DCP->>MCP: Start telemetry collection
    Test->>DCP: client_get_state()
    DCP->>MCP: Query client state
    MCP-->>Test: STATE_RUNNING

    Note over Test, MCP: Data Collection Loop
    loop For 30 seconds
        Test->>DCP: client_read_data()
        DCP->>MCP: Request telemetry data
        alt Data Available
            MCP-->>Test: DATA_VALID_MORE/LAST + data descriptor
            Test->>DCP: client_send_read_data_complete(offset, size)
            DCP->>MCP: Acknowledge data received
        else No Data
            MCP-->>Test: E_BUSY
            Test->>Test: Continue polling
        end
    end

    Note over Test, MCP: Cleanup Phase
    Test->>DCP: client_start_stop(state=STOP)
    DCP->>MCP: Stop telemetry collection
:::

### Key Design Patterns

#### 1. Abstract Factory Pattern (Endpoint Creation)
The `trp_endpoint` abstraction allows different communication implementations:
```python
# CLI-based endpoint for SVP testing
cli_endpoint = mts_cli_trp_endpoint(telnet_channel, die_id, cpu_type)

# ICC MHU-based endpoint for hardware access
mhu_endpoint = mts_icc_mhu_trp_endpoint(icc_mhu_instance, die_id, cpu_type)

# Specialized AP endpoint without TRP wrapper
ap_endpoint = mts_ap_endpoint()
```

#### 2. Command Pattern (DCP Operations)
Each DCP operation is encapsulated as a static method with consistent interface:
```python
@staticmethod
def client_get_state(*,
    src_endpoint: trp_endpoint,
    dest_die: int,
    dest_cpu: cpu_type,
    client_id: mts_client_id_t,
) -> dcp_client_state_t:
    # Encapsulated command implementation
```

#### 3. Template Method Pattern (Message Processing)
The base `trp_endpoint` defines common message processing while allowing customization:
```python
def send_dcp_message(self, ...):
    # Template method defines algorithm:
    trp_header = self.create_dcp_forward_trp_header(...)  # Step 1
    self._send_message(trp_header + dcp_payload)          # Step 2 (customizable)
    response = self._receive_response(timeout)            # Step 3 (customizable)
    self.log_trp_response_headers(trp_header, response)   # Step 4
    return self._extract_dcp_response(response)           # Step 5
```

#### 4. Strategy Pattern (Communication Channels)
Different communication strategies can be plugged into endpoints:
- **Telnet Strategy**: For SVP and FPGA testing via network connections
- **Direct Serial Strategy**: For silicon hardware testing via UART
- **ICC MHU Strategy**: For high-performance inter-core communication

#### 5. Observer Pattern (Asynchronous Notifications)
The library handles asynchronous notification messages:
```python
# Check if response is a notification, retry if needed
if dcp_hdr.msg_id == DCP_MSG_ID_NOTIFICATION:
    logger.info("Received asynchronous notification, retrying for command response")
    response_bytes = self._read_channel_response(timeout_sec)
```

## API Reference

### DCP Commands Interface

| API | Description | Parameters | Returns |
|-----|-------------|------------|---------|
| `client_enable_disable_events(...)` | Enable/disable telemetry events | `src_endpoint`, `dest_die`, `dest_cpu`, `client_id`, `events` | `None` |
| `client_start_stop(...)` | Start/stop data collection | `src_endpoint`, `dest_die`, `dest_cpu`, `client_id`, `state` | `None` |
| `client_get_state(...)` | Get client running state | `src_endpoint`, `dest_die`, `dest_cpu`, `client_id` | `dcp_client_state_t` |
| `client_get_manifest(...)` | Get telemetry manifest descriptor | `src_endpoint`, `dest_die`, `dest_cpu`, `client_id`, `output_file?` | `Tuple[dcp_status_t, manifest_response]` |
| `client_get_platform_information(...)` | Get platform and version info | `src_endpoint`, `dest_die`, `dest_cpu`, `client_id` | `Tuple[dcp_status_t, platform_response]` |
| `client_get_capabilities(...)` | Get supported DCP capabilities | `src_endpoint`, `dest_die`, `dest_cpu`, `client_id` | `Tuple[dcp_status_t, capabilities_response]` |
| `client_read_data(...)` | Read telemetry data | `src_endpoint`, `dest_die`, `dest_cpu`, `client_id`, `output_file?` | `Tuple[dcp_status_t, read_data_response]` |
| `client_send_read_data_complete(...)` | Acknowledge data read completion | `src_endpoint`, `dest_die`, `dest_cpu`, `client_id`, `offset`, `size` | `dcp_status_t` |
| `client_reset(...)` | Reset client to initial state | `src_endpoint`, `dest_die`, `dest_cpu`, `client_id` | `dcp_status_t` |

### TRP Endpoint Interface

| API | Description | Parameters | Returns |
|-----|-------------|------------|---------|
| `send_dcp_message(...)` | Send DCP message with TRP wrapper | `dest_die`, `dest_cpu`, `client_id`, `dcp_msg`, `timeout_sec?` | `bytearray` |
| `send_trp_message()` | Send simple TRP message | None | `str` |
| `get_next_sequence_number()` | Get incremented sequence number | None | `int` |
| `create_dcp_forward_trp_header(...)` | Create TRP header for DCP forwarding | `dest_die`, `dest_cpu`, `client_id`, `payload_size` | `trp_msg_hdr_t` |
| `close()` | Clean up endpoint resources | None | `None` |

### Memory Access Interface (ICC MHU Endpoints)

| API | Description | Parameters | Returns |
|-----|-------------|------------|---------|
| `read_to_file(address, size, filename)` | Read memory region to file | `address: int`, `size: int`, `filename: str` | `None` |

### Test Infrastructure Interface

| API | Description | Parameters | Returns |
|-----|-------------|------------|---------|
| `setup()` | Initialize test environment and DUT | None | `bool` |
| `teardown()` | Clean up test resources | None | `None` |
| `wait_for_scp_mcp_heartbeat()` | Wait for initial system heartbeat | None | `bool` |
| `test_client_read_data_polling()` | Test telemetry data collection polling | None | `bool` |
| `test_client_get_manifest()` | Test manifest retrieval | None | `bool` |
| `test_client_get_platform_information()` | Test platform information query | None | `bool` |
| `test_client_get_capabilities()` | Test capability bitfield query | None | `bool` |
| `test_client_start_stop()` | Test client state transitions | None | `bool` |
| `test_client_reset()` | Test client reset functionality | None | `bool` |

## Usage Examples

### Basic DCP Communication

```python
from trp_lib import mts_cli_trp_endpoint
from dcp_commands import dcp_commands
from dcp_protocol import data_collection_protocol
from trp_protocol import transfer_relay_protocol
from fpfw_automation_primitives.serial.telnet import Telnet_

# Create communication channel
telnet_channel = Telnet_(host="localhost", port="4257", encoding="UTF-8")
telnet_channel.open()

# Create TRP endpoint
endpoint = mts_cli_trp_endpoint(
    telnet_channel,
    source_die=0,
    source_cpu=transfer_relay_protocol.cpu_type.CPU_SCP
)

# Get platform information
status, platform_info = dcp_commands.client_get_platform_information(
    src_endpoint=endpoint,
    dest_die=0,
    dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
    client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_DCP_SVC
)

print(f"DCP Version: {platform_info.dcp_ver_major}.{platform_info.dcp_ver_minor}")
print(f"Platform ID: {platform_info.plat_id}")

# Clean up
telnet_channel.close()
```

### Telemetry Data Collection

```python
# Enable specific telemetry events
event_list = [
    (0x0202, 4, data_collection_protocol.client_events_enable_disable_msg.dcp_events_enable_state_t.DCP_EVENTS_ENABLE_STATE_ENABLE),  # SOC MAX TEMP
]

dcp_commands.client_enable_disable_events(
    src_endpoint=endpoint,
    dest_die=0,
    dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
    client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
    events=event_list
)

# Start telemetry collection
dcp_commands.client_start_stop(
    src_endpoint=endpoint,
    dest_die=0,
    dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
    client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
    state=data_collection_protocol.client_start_stop_msg.dcp_start_stop_state_t.DCP_START_STOP_STATE_START
)

# Poll for telemetry data
import time
start_time = time.time()
package_count = 0

while time.time() - start_time < 30:  # 30 second collection window
    status, response = dcp_commands.client_read_data(
        src_endpoint=endpoint,
        dest_die=0,
        dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
        client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
        output_file="telemetry_data.bin"  # Optional file output
    )

    if status in [data_collection_protocol.dcp_status_t.DATA_COLLECTION_RD_DATA_VALID_LAST,
                  data_collection_protocol.dcp_status_t.DATA_COLLECTION_RD_DATA_VALID_MORE]:
        package_count += 1
        print(f"Received telemetry package {package_count}")

        # Acknowledge data received
        dcp_commands.client_send_read_data_complete(
            src_endpoint=endpoint,
            dest_die=0,
            dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
            client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
            rd_data_addr_offset=response.rd_data_addr_offset,
            rd_data_size=response.rd_data_size
        )
    elif status == data_collection_protocol.dcp_status_t.DATA_COLLECTION_E_BUSY:
        time.sleep(0.1)  # Brief pause before retry
    else:
        break

# Stop telemetry collection
dcp_commands.client_start_stop(
    src_endpoint=endpoint,
    dest_die=0,
    dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
    client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
    state=data_collection_protocol.client_start_stop_msg.dcp_start_stop_state_t.DCP_START_STOP_STATE_STOP
)

print(f"Collection complete: {package_count} packages received")
```

### ICC MHU Endpoint Usage

```python
from trp_lib import mts_icc_mhu_trp_endpoint, mts_ap_endpoint
from icc_mhu_ap_lib import IccMhuAp, ApCoreMemoryInterface

# For general inter-core communication with TRP wrapper
mhu_instance = IccMhuAp(memory_interface=ApCoreMemoryInterface())
mhu_endpoint = mts_icc_mhu_trp_endpoint(
    icc_mhu=mhu_instance,
    source_die=0,
    source_cpu=transfer_relay_protocol.cpu_type.CPU_AP
)

# For AP-specific communication without TRP wrapper
ap_endpoint = mts_ap_endpoint()

# Use endpoint for DCP communication
status, capabilities = dcp_commands.client_get_capabilities(
    src_endpoint=ap_endpoint,
    dest_die=0,
    dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
    client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_DCP_SVC
)

# File-based data extraction (only available with ICC MHU endpoints)
if hasattr(ap_endpoint, 'read_to_file'):
    ap_endpoint.read_to_file(
        address=0x1000000,
        size=4096,
        filename="memory_dump.bin"
    )

# Clean up
ap_endpoint.close()
```

### Integration Testing with Pythia

```python
class MyMtsTest(MtsDcpTest):
    def __init__(self):
        super().__init__(
            name="CustomMtsTest",
            dut_platform="KingsgateSVP"
        )

    def test_custom_telemetry_scenario(self):
        """Custom test combining multiple DCP operations"""
        try:
            # Test platform information
            status, platform_info = dcp_commands.client_get_platform_information(
                src_endpoint=self.die0_scp_trp_endpoint,
                dest_die=0,
                dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_DCP_SVC
            )

            self.log.info(f"Platform ID: {platform_info.plat_id}")

            # Reset and configure telemetry client
            dcp_commands.client_reset(
                src_endpoint=self.die0_scp_trp_endpoint,
                dest_die=0,
                dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
                client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM
            )

            # Run telemetry collection
            return self.test_client_read_data_polling()

        except Exception as e:
            self.log.error(f"Custom test failed: {e}")
            return False

# Usage
test = MyMtsTest()
if test.setup():
    result = test.test_custom_telemetry_scenario()
    test.teardown()
    print(f"Test result: {'PASS' if result else 'FAIL'}")
```

## Design Principles

The MTS Library demonstrates several key object-oriented design principles that make the code maintainable, testable, and flexible:

### ✅ Single Responsibility Principle (SRP)

**What it means:** Each class should have only one reason to change and focus on a single responsibility.

**Why it's good:** When code has a single, clear purpose, it's easier to understand, test, and modify. Changes to one feature don't accidentally break unrelated functionality.

**How we apply it:**

- **`data_collection_protocol`**: Responsible only for defining DCP protocol constants and message structures
- **`dcp_commands`**: Responsible only for implementing high-level DCP command operations
- **`transfer_relay_protocol`**: Responsible only for defining TRP message routing structures
- **`trp_endpoint`**: Responsible only for abstract message transport functionality
- **`mts_cli_trp_endpoint`**: Responsible only for CLI-based message transport
- **`mts_icc_mhu_trp_endpoint`**: Responsible only for ICC MHU-based message transport
- **`MtsDcpTest`**: Responsible only for integration testing of DCP operations

### ✅ Open/Closed Principle (OCP)

**What it means:** Classes should be open for extension but closed for modification.

**Why it's good:** You can add new functionality without changing existing, tested code. This reduces the risk of introducing bugs when adding features.

**How we apply it:**

- New TRP endpoint implementations can be added by extending `trp_endpoint` without modifying existing code
- New DCP client types can be supported by adding enum values without changing command implementations
- New communication channels can be integrated without modifying the core protocol logic
- Additional telemetry analysis features can be added through composition rather than modification

### ✅ Liskov Substitution Principle (LSP)

**What it means:** Objects of a parent class should be replaceable with objects of a child class without breaking functionality.

**Why it's good:** This ensures that inheritance hierarchies are logical and that polymorphism works correctly.

**How we apply it:**

- Any `trp_endpoint` implementation can be used interchangeably in `dcp_commands` operations
- `mts_cli_trp_endpoint`, `mts_icc_mhu_trp_endpoint`, and `mts_ap_endpoint` all fulfill the endpoint contract identically
- Test infrastructure can use any endpoint implementation without behavior changes
- Protocol layers maintain consistent interfaces regardless of underlying transport mechanism

### ✅ Interface Segregation Principle (ISP)

**What it means:** Interfaces should be focused and not force clients to depend on methods they don't use.

**Why it's good:** Smaller, focused interfaces are easier to implement, test, and understand. Clients only depend on what they actually need.

**How we apply it:**

- `trp_endpoint` only exposes essential message transport operations
- `CoreMemoryMapAccess` interface is separate and only implemented by endpoints that support memory operations
- DCP command methods have focused, single-purpose APIs
- Protocol definition classes separate constants from operational logic

### ✅ Dependency Inversion Principle (DIP)

**What it means:** High-level modules should not depend on low-level modules. Both should depend on abstractions.

**Why it's good:** This reduces coupling between components and makes the system more flexible and testable.

**How we apply it:**

- `dcp_commands` depends on `trp_endpoint` abstraction, not concrete communication implementations
- `MtsDcpTest` depends on the DCP command interface, not specific transport mechanisms
- Protocol operations are abstracted away from specific hardware or communication details
- Test infrastructure can use mock or real endpoints interchangeably

### ✅ Dependency Injection Pattern

**What it means:** Dependencies are provided to a class from the outside rather than created internally.

**Why it's good:**

- **Flexibility**: Same class can work with different implementations
- **Testability**: Easy to inject mock objects for testing
- **Loose Coupling**: Classes don't need to know how to create their dependencies
- **Configuration**: Dependencies can be configured externally

**How we apply it:**

```python
# Dependencies are injected through method parameters
@staticmethod
def client_get_state(*,
    src_endpoint: trp_endpoint,  # Endpoint dependency is injected
    dest_die: int,
    dest_cpu: cpu_type,
    client_id: mts_client_id_t,
) -> dcp_client_state_t:

# Usage - different endpoint types can be injected
cli_result = dcp_commands.client_get_state(src_endpoint=cli_endpoint, ...)
mhu_result = dcp_commands.client_get_state(src_endpoint=mhu_endpoint, ...)
```

### ✅ Composition Over Inheritance

**What it means:** Favor object composition (has-a relationships) over class inheritance (is-a relationships).

**Why it's good:**

- **Runtime Flexibility**: You can change behavior by swapping out composed objects at runtime
- **Avoid Inheritance Problems**: Deep inheritance hierarchies become hard to understand and maintain
- **Multiple Behaviors**: A class can compose multiple different objects to get various capabilities
- **Easier Testing**: You can easily mock or stub individual composed components
- **No Diamond Problem**: Avoids complex multiple inheritance issues
- **Portability**: Components can be reused across different projects and platforms
- **Separation of Concerns**: Clean separation between project-independent behavior and project-dependent implementations

**How we apply it:**

- `mts_icc_mhu_trp_endpoint` **has-a** `IccMhu` instance rather than inheriting ICC functionality
- `MtsDcpTest` **contains** `mts_cli_trp_endpoint` rather than inheriting transport capabilities
- Protocol classes **compose** ctypes structures rather than inheriting from them

**Benefits:**

- **Different communication channels** can be composed with the same protocol logic
- **Test infrastructure** can compose different endpoint types without inheritance complexity
- **Protocol definitions** remain independent of transport implementations
- **Project Portability**: The next project may have different IP blocks (different ICC implementations, new communication hardware, alternative memory interfaces), but we've prevented that coupling from leaking into the project-independent code
- **Common Behavior Separation**: Project-independent logic (DCP protocol, message validation, sequence numbering) is cleanly separated from project-dependent transports (Kingsgate-specific ICC MHU, SVP-specific CLI channels)
- **Technology Migration**: When moving from Kingsgate to the next generation silicon, the DCP/TRP protocol logic can be reused while only the transport layer needs updating

### ✅ Strategy Pattern

**What it means:** Define a family of algorithms (strategies) and make them interchangeable.

**Why it's good:** Allows behavior to be selected at runtime and makes adding new strategies easy.

**How we apply it:**

- Different endpoint implementations (`mts_cli_trp_endpoint`, `mts_icc_mhu_trp_endpoint`) represent different communication strategies
- Same `trp_endpoint` interface, different implementations for different environments
- Can switch between CLI, ICC MHU, or future communication methods without changing DCP logic

### ✅ Command Pattern

**What it means:** Encapsulate requests as objects, allowing you to parameterize clients with different requests and support undoable operations.

**Why it's good:** Provides a consistent interface for operations and enables features like logging, queuing, and validation.

**How we apply it:**

```python
# Each DCP operation is encapsulated as a command with consistent interface
@staticmethod
def client_start_stop(*,
    src_endpoint: trp_endpoint,
    dest_die: int,
    dest_cpu: cpu_type,
    client_id: mts_client_id_t,
    state: dcp_start_stop_state_t,
) -> None:
    # Encapsulated command implementation with validation
    # Consistent error handling and logging
    # Structured response processing
```

**Benefits:**
- All DCP operations have consistent parameter patterns and error handling
- Easy to add logging, validation, and retry logic to all commands
- Commands can be easily tested in isolation

### ✅ Template Method Pattern

**What it means:** Define the skeleton of an algorithm in a base class, letting subclasses override specific steps.

**Why it's good:** Promotes code reuse while allowing customization of specific behaviors.

**How we apply it:**

```python
class trp_endpoint:
    def send_dcp_message(self, ...):
        # Template method defines algorithm skeleton:
        trp_header = self.create_dcp_forward_trp_header(...)  # Step 1 - common
        response = self._send_and_receive(...)               # Step 2 - customizable
        self.log_trp_response_headers(trp_header, response)  # Step 3 - common
        return self._extract_dcp_response(response)          # Step 4 - common

    @abstractmethod
    def _send_and_receive(self, ...):
        # Subclasses implement transport-specific logic
        pass
```

- `trp_endpoint` provides common message processing algorithm
- Subclasses customize transport-specific operations while reusing validation and logging
- `MtsDcpTest` provides common test setup while allowing test-specific operations

### ✅ DRY Principle (Don't Repeat Yourself)

**What it means:** Every piece of knowledge or logic should have a single, authoritative representation in the system.

**Why it's good:**

- **Easier Maintenance**: Fix bugs or make changes in only one place
- **Consistency**: All code uses the same logic, reducing inconsistencies
- **Reduced Errors**: Less duplication means fewer places for bugs to hide
- **Cleaner Code**: Less code to read and understand

**How we apply it:**

```python
# GOOD: Common header creation logic in one place
@staticmethod
def create_header(msg_id: int, payload_size: int, client_id: mts_client_id_t) -> List[int]:
    # Single implementation used by all DCP commands
    header = []
    header.extend([client_id.value & 0xFF, (client_id.value >> 8) & 0xFF])
    header.extend([msg_id & 0xFF, (msg_id >> 8) & 0xFF])
    header.extend(dcp_commands._get_next_sequence_number())
    # ... rest of header construction
    return header

# GOOD: Common response validation logic
@staticmethod
def validate_response(response: dcp_status_t, logger: Optional[logging.Logger] = None) -> bool:
    # Single validation implementation used by all commands
    if response < 0:
        if logger:
            logger.error(f"Command returned error status: {response.name}")
        return False
    return True
```

**What we avoid:**
- Copying and pasting the same header construction logic in multiple commands
- Duplicating response validation patterns across different DCP operations
- Repeating the same logging and error handling code in multiple places

### ✅ Observer Pattern (Event Handling)

**What it means:** Define a one-to-many dependency between objects so that when one object changes state, all dependents are notified.

**Why it's good:** Enables loose coupling between components and supports event-driven architectures.

**How we apply it:**

```python
# Handle asynchronous notifications from the system
if dcp_hdr.msg_id == data_collection_protocol.dcp_msg_id_t.DCP_MSG_ID_NOTIFICATION.value:
    logger.info("Received asynchronous notification message, retrying for command response")
    # System notified us of an event, continue processing
    response_bytes = self._read_channel_response(cmd_timeout_sec)
```

- **Asynchronous Notifications**: System can send notification messages that are handled separately from command responses
- **Heartbeat Monitoring**: `wait_for_scp_mcp_heartbeat()` observes system ready events
- **Status Polling**: Telemetry collection observes data availability events

### ✅ Factory Method Pattern (Endpoint Creation)

**What it means:** Create objects without specifying their exact classes, using a common interface.

**Why it's good:** Provides flexibility in object creation and allows runtime selection of implementations.

**How we apply it:**

```python
# Factory-like endpoint creation based on communication type
def create_endpoint(communication_type: str, **kwargs):
    if communication_type == "cli":
        return mts_cli_trp_endpoint(
            source_comm_channel=kwargs['channel'],
            source_die=kwargs['die'],
            source_cpu=kwargs['cpu']
        )
    elif communication_type == "icc_mhu":
        return mts_icc_mhu_trp_endpoint(
            icc_mhu=kwargs['mhu_instance'],
            source_die=kwargs['die'],
            source_cpu=kwargs['cpu']
        )
    elif communication_type == "ap":
        return mts_ap_endpoint()
```

**Benefits:**
- Test infrastructure can create appropriate endpoints based on test configuration
- Different deployment environments can use different communication strategies
- New endpoint types can be added without changing client code

These patterns work together to create a maintainable, testable, and extensible message transfer service that properly separates protocol definition, command implementation, transport abstraction, and testing concerns.

## Variants

| Variant | Description |
| --- | --- |
| **mts_cli_trp_endpoint** | CLI-based TRP endpoint for SVP and FPGA testing with telnet/direct serial communication channels |
| **mts_icc_mhu_trp_endpoint** | ICC MHU-based TRP endpoint with hardware memory access and file extraction capabilities |
| **mts_ap_endpoint** | Specialized AP endpoint for direct DCP communication without TRP wrapper overhead |
| **MtsDcpTest** | Comprehensive integration test suite with Pythia framework integration and multiple test scenarios |

The library's modular design allows for easy extension to support additional communication channels (native hardware, simulation interfaces), new DCP client types (event trace, custom telemetry), and enhanced testing scenarios without modifying existing protocol implementations.
