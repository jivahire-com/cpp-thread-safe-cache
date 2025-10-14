# Power Telemetry Library Design Document

This document describes the Power Telemetry Library (`pwr_tlm_lib.py`) that provides utilities for collecting and managing power telemetry data from Kingsgate silicon through the Data Collection Protocol (DCP).

## Table of Contents

[[_TOC_]]

## Introduction

### Description

The Power Telemetry Library provides a high-level Python interface for collecting real-time power and thermal telemetry data from Kingsgate silicon. The library abstracts the complexity of the Data Collection Protocol (DCP) and Transfer Relay Protocol (TRP) to enable automated collection, configuration, and decoding of telemetry data for power analysis, thermal monitoring, and system validation.

Key capabilities include:

- **Power Package Telemetry**: Core power states, voltage rails, temperature sensors, throttling events
- **Instantaneous Telemetry**: High-frequency real-time power and thermal measurements
- **Selective Data Collection**: Enable/disable specific telemetry elements to reduce data volume
- **Automated File Management**: Organized storage and retrieval of telemetry packages
- **Binary Decoding**: Integration with diagnostic decoder tools for data analysis

### Terms

| Term   | Description                                           |
| ------ | ----------------------------------------------------- |
| DCP    | Data Collection Protocol - protocol for telemetry data exchange |
| TRP    | Transfer Relay Protocol - message routing between processor cores |
| MCP    | Management Control Processor - handles power/thermal management |
| SCP    | System Control Processor - system-level control functions |
| AP     | Application Processor - main compute cores |
| VR/VRD | Voltage Regulator/Voltage Regulator Device |
| P-State| Performance State - CPU frequency/voltage operating point |
| C-State| CPU idle/sleep power states |
| MPAM   | Memory System Resource Partitioning and Monitoring |
| HNF    | Home Node with Cache - cache coherency component |
| DIMM   | Dual In-line Memory Module |

### Reference Documents

| Document                                  | Description                         |
| ----------------------------------------- | ----------------------------------- |
| MTS Library Design Document              | MTS/DCP/TRP protocol implementations |
| ICC MHU Library Design Document          | Inter-core communication interface   |
| Data Collection Protocol Specification   | DCP message formats and workflows    |

## Requirements

The Power Telemetry Library shall meet the following functional and non-functional requirements:

- The library shall support enabling and disabling specific power telemetry elements individually or collectively
- The library shall support enabling and disabling specific instantaneous telemetry elements individually or collectively
- The library shall provide context manager support (`with` statement) for automatic resource cleanup
- The library shall automatically manage file directory creation and cleanup for telemetry data storage
- The library shall support reading a specified number of telemetry packages with timeout protection
- The library shall support reading telemetry packages for a specified duration without raising timeout exceptions
- The library shall provide client state management (start, stop, reset, get state) for DCP telemetry clients
- The library shall support retrieval and local storage of telemetry manifest files for data decoding
- The library shall provide integration with diagnostic decoder tools for binary telemetry decoding
- The library shall support both power package telemetry (1-second intervals) and instantaneous telemetry (20ms intervals)
- The library shall automatically adjust polling intervals based on enabled telemetry types
- The library shall validate input parameters and provide meaningful error messages for invalid usage
- The library shall support graceful cleanup and resource release even when exceptions occur during operation

## Dependencies

The Power Telemetry Library depends on the following components and external libraries:

- **MTS Library Components**:
  - [dcp_protocol](../mts_tests/mts_lib_design.md) - Data Collection Protocol message definitions and structures
  - [dcp_commands](../mts_tests/mts_lib_design.md) - High-level DCP command implementations
  - [trp_protocol](../mts_tests/mts_lib_design.md) - Transfer Relay Protocol for inter-core communication
  - [trp_lib](../mts_tests/mts_lib_design.md) - TRP endpoint abstractions and implementations

- **Communication Layer**:
  - [icc_mhu_ap_lib](../icc_mhu_tests/icc_mhu_lib_design.md) - ICC MHU interface for AP core communication
  - fpfw_automation_primitives.serial.telnet - Telnet-based serial communication for SVP/FPGA environments

- **System Dependencies**:
  - Python Standard Library: os, sys, logging, time, subprocess, pathlib, ctypes
  - diagdecoder.exe - External diagnostic decoder tool for binary telemetry decoding
  - REPO_APP_PATH environment variables for tool path resolution

## Design

The Power Telemetry Library provides a high-level abstraction layer over the Data Collection Protocol (DCP) for simplified power telemetry data collection. The library manages the complexity of DCP message construction, TRP endpoint communication, and telemetry data lifecycle management.

### Architecture Overview

The library follows a layered architecture pattern:

1. **Application Layer**: `power_tlm_lib` class providing high-level telemetry operations
2. **Protocol Layer**: DCP commands for client control and data retrieval
3. **Transport Layer**: TRP endpoints for inter-core communication
4. **Physical Layer**: Serial communication channels (Telnet, ICC MHU, Direct Serial)

### Class Diagram

The following Mermaid class diagram illustrates the relationship between the power telemetry classes and their dependencies:

### UML Notation Key

For readers unfamiliar with UML class diagram notation:

- **Solid line with empty arrowhead (`<|--`)**: Inheritance (is-a relationship) - child class extends parent class
- **Dashed line with empty arrowhead (`<..`)**: Interface implementation or realization
- **Solid line with filled arrowhead (`-->`)**: Association/dependency (uses or has-a relationship)
- **Dashed line with filled arrowhead (`..>`)**: Dependency (depends on or references)
- **Solid line with filled diamond (`*--`)**: Composition with full lifecycle ownership (creates, owns, destroys)
- **Solid line with empty diamond (`o--`)**: Aggregation with shared/injected ownership (manages but doesn't create)

:::mermaid
classDiagram
    %% Core power telemetry library
    class power_tlm_lib {
        -src_endpoint: trp_endpoint
        -file_location: str
        -manifest_file: str
        -tlm_packages: list
        -current_read_duration_sec: float
        +__init__(src_endpoint: trp_endpoint, file_location: str)
        +__enter__() power_tlm_lib
        +__exit__(exc_type, exc_val, exc_tb) bool
        +close()
        +reset()
        +read_manifest(filename: str)
        +enable_power_package_records(elements: list)
        +enable_all_power_package_records()
        +disable_power_package_records(elements: list)
        +disable_all_power_package_records()
        +enable_instantaneous_package_records(elements: list)
        +enable_all_instantaneous_package_records()
        +disable_instantaneous_package_records(elements: list)
        +disable_all_instantaneous_package_records()
        +get_client_state() dcp_client_state_t
        +start_dcp_client()
        +stop_dcp_client()
        +read_packages_for_duration(filename_prefix: str, duration_sec: int)
        +read_n_packages(num_packages: int, filename_prefix: str, timeout_sec: int)
        +decode_packages()
        +decode_to_file(manifest_path: str, payload_path: str, output_path: str)
        -_read_dcp_package(output_file: str) bool
        -_read_packages_loop(filename_prefix: str, max_packages: int, duration_sec: int) int
    }

    %% Telemetry element enumerations
    class pwr_telemetry_element_id_t {
        <<enumeration>>
        POWER_TELEMETRY_ELEMENT_CORE_PSTATE
        POWER_TELEMETRY_ELEMENT_CORE_CSTATE
        POWER_TELEMETRY_ELEMENT_CORE_THROTTLE
        POWER_TELEMETRY_ELEMENT_CORE_VOLTAGE
        POWER_TELEMETRY_ELEMENT_CORE_CURRENT
        POWER_TELEMETRY_ELEMENT_CORE_TEMPERATURE
        POWER_TELEMETRY_ELEMENT_CORE_POWER
        POWER_TELEMETRY_ELEMENT_SOC_PKG_MON
        POWER_TELEMETRY_ELEMENT_SOC_VR_RAILS
        POWER_TELEMETRY_ELEMENT_SOC_DIMM_TEMPERATURE
        POWER_TELEMETRY_ELEMENT_SOC_MAX_TEMPERATURE
        +23 total elements
    }

    class instantaneous_telemetry_element_id_t {
        <<enumeration>>
        INST_TELEMETRY_ELEMENT_CORE
        INST_TELEMETRY_ELEMENT_SOC_VOLTAGE_RAILS
        INST_TELEMETRY_ELEMENT_SOC_DIMM_RT
        INST_TELEMETRY_ELEMENT_SOC_DIE_TEMP
        INST_TELEMETRY_ELEMENT_SOC_MAX_TEMP
    }

    %% DCP protocol layer (from mts_tests)
    class dcp_commands {
        <<static>>
        +client_reset(src_endpoint, dest_die, dest_cpu, client_id) dcp_status_t
        +client_get_state(src_endpoint, dest_die, dest_cpu, client_id) dcp_client_state_t
        +client_start_stop(src_endpoint, dest_die, dest_cpu, client_id, state)
        +client_enable_disable_events(src_endpoint, dest_die, dest_cpu, client_id, events)
        +client_get_manifest(src_endpoint, dest_die, dest_cpu, client_id, output_file) Tuple
        +client_read_data(src_endpoint, dest_die, dest_cpu, client_id, output_file) Tuple
        +client_send_read_data_complete(src_endpoint, dest_die, dest_cpu, client_id, offset, size) dcp_status_t
    }

    class data_collection_protocol {
        <<module>>
        +mts_client_id_t: enum
        +dcp_status_t: enum
        +dcp_client_state_t: enum
        +dcp_start_stop_state_t: enum
        +dcp_events_enable_state_t: enum
    }

    %% TRP layer abstractions (from mts_tests)
    class trp_endpoint {
        <<abstract>>
        #source_die: int
        #source_cpu: cpu_type
        #sequence_number: int
        #default_timeout_sec: int
        +__init__(source_die: int, source_cpu: cpu_type)
        +close()
        +send_dcp_message(dest_die, dest_cpu, client_id, dcp_msg, timeout_sec)* bytearray
        +send_trp_message()* str
        +get_next_sequence_number() int
        +create_dcp_forward_trp_header(dest_die, dest_cpu, client_id, payload_size) trp_msg_hdr_t
    }

    class mts_cli_trp_endpoint {
        -telnet_port: Telnet_
        +__init__(telnet_port: Telnet_, source_die: int, source_cpu: cpu_type)
        +send_dcp_message(dest_die, dest_cpu, client_id, dcp_msg, timeout_sec) bytearray
        +send_trp_message() str
        +close()
    }

    class mts_ap_endpoint {
        -ap_core: ApCoreMemoryInterface
        -icc_mhu_ap: IccMhuAp
        +__init__()
        +send_dcp_message(dest_die, dest_cpu, client_id, dcp_msg, timeout_sec) bytearray
        +send_trp_message() str
        +read_to_file(physical_addr: int, size: int, output_file: str)
        +close()
    }

    class CoreMemoryMapAccess {
        <<interface>>
        +read_to_file(physical_addr: int, size: int, output_file: str)*
    }

    %% Communication channels (external dependencies)
    class Telnet_ {
        <<external>>
        +host: str
        +port: str
        +encoding: str
        +open()
        +close()
        +write_line(write_string: str)
        +read_until(key: str, timeout_seconds: int) str
    }

    class transfer_relay_protocol {
        <<module>>
        +cpu_type: enum
        +trp_msg_hdr_t: struct
    }

    %% Relationships
    power_tlm_lib o-- trp_endpoint : manages injected endpoint
    power_tlm_lib ..> dcp_commands : calls DCP operations
    power_tlm_lib ..> pwr_telemetry_element_id_t : configures power elements
    power_tlm_lib ..> instantaneous_telemetry_element_id_t : configures instantaneous elements

    dcp_commands ..> data_collection_protocol : uses protocol definitions
    dcp_commands --> trp_endpoint : requires for message sending
    dcp_commands ..> transfer_relay_protocol : uses for CPU/die addressing

    trp_endpoint <|-- mts_cli_trp_endpoint : implements CLI-based transport
    trp_endpoint <|-- mts_ap_endpoint : implements AP core transport
    mts_ap_endpoint ..|> CoreMemoryMapAccess : implements memory access interface

    mts_cli_trp_endpoint --> Telnet_ : uses for serial communication
    trp_endpoint ..> transfer_relay_protocol : uses for message headers

    %% Documentation notes
    note for power_tlm_lib "Context manager with automatic cleanup and resource management.<br/>Uses aggregation: endpoint created externally, injected via constructor,<br/>but library manages its lifecycle (close() in __exit__)"
    note for trp_endpoint "Abstract base for pluggable communication transports"
    note for dcp_commands "Static utility class providing DCP protocol operations"
    note for CoreMemoryMapAccess "Interface enabling direct memory file operations for decoding"
:::

### Communication Flow

The following sequence diagram illustrates the typical power telemetry collection workflow:

:::mermaid
sequenceDiagram
    participant Test as Test Script
    participant PowerTlm as power_tlm_lib
    participant DcpCmds as dcp_commands
    participant TrpEndpoint as trp_endpoint
    participant MCP as MCP Firmware

    Test->>PowerTlm: __enter__() (context manager)
    PowerTlm->>PowerTlm: reset() (automatic)
    PowerTlm->>DcpCmds: client_reset()
    DcpCmds->>TrpEndpoint: send_dcp_message()
    TrpEndpoint->>MCP: DCP Reset Command
    MCP-->>TrpEndpoint: Reset Response
    TrpEndpoint-->>DcpCmds: Response bytes
    DcpCmds-->>PowerTlm: Status

    Test->>PowerTlm: enable_power_package_records([CORE_VOLTAGE, CORE_POWER])
    PowerTlm->>DcpCmds: client_enable_disable_events()
    DcpCmds->>TrpEndpoint: send_dcp_message()
    TrpEndpoint->>MCP: Enable Events Command
    MCP-->>TrpEndpoint: Enable Response
    TrpEndpoint-->>DcpCmds: Response bytes
    DcpCmds-->>PowerTlm: Status

    Test->>PowerTlm: start_dcp_client()
    PowerTlm->>DcpCmds: client_start_stop(START)
    DcpCmds->>TrpEndpoint: send_dcp_message()
    TrpEndpoint->>MCP: Start Client Command
    MCP-->>TrpEndpoint: Start Response
    TrpEndpoint-->>DcpCmds: Response bytes
    DcpCmds-->>PowerTlm: Status

    Test->>PowerTlm: read_n_packages(5, "telemetry", 30)
    loop For each package (1 to 5)
        PowerTlm->>DcpCmds: client_read_data()
        DcpCmds->>TrpEndpoint: send_dcp_message()
        TrpEndpoint->>MCP: Read Data Command
        MCP-->>TrpEndpoint: Telemetry Package
        TrpEndpoint-->>DcpCmds: Package bytes
        DcpCmds-->>PowerTlm: Package data
        PowerTlm->>PowerTlm: Save to file (telemetry_package_N.bin)
    end

    Test->>PowerTlm: __exit__() (context manager cleanup)
    PowerTlm->>DcpCmds: client_start_stop(STOP)
    DcpCmds->>TrpEndpoint: send_dcp_message()
    TrpEndpoint->>MCP: Stop Client Command
    PowerTlm->>TrpEndpoint: close()
:::

## API

The Power Telemetry Library provides the following public APIs for telemetry data collection and management:

### Core Library Management

| API                                    | Description                                                       |
| -------------------------------------- | ----------------------------------------------------------------- |
| `__init__(src_endpoint, file_location)` | Initialize power telemetry library with TRP endpoint and file directory |
| `__enter__()`                          | Context manager entry - enables `with` statement usage           |
| `__exit__(exc_type, exc_val, exc_tb)`  | Context manager exit - ensures automatic cleanup                  |
| `close()`                              | Close TRP endpoint and release communication resources            |

### DCP Client Control

| API                                    | Description                                                       |
| -------------------------------------- | ----------------------------------------------------------------- |
| `reset()`                              | Reset DCP client to initial state, clearing active sessions      |
| `get_client_state()`                   | Get current running state of DCP client (stopped/running/error)  |
| `start_dcp_client()`                   | Start DCP client to begin telemetry data collection              |
| `stop_dcp_client()`                    | Stop DCP client and halt telemetry data collection               |

### Telemetry Configuration

| API                                    | Description                                                       |
| -------------------------------------- | ----------------------------------------------------------------- |
| `enable_power_package_records(elements)` | Enable specific power telemetry elements for data collection   |
| `enable_all_power_package_records()`   | Enable all available power telemetry elements                    |
| `disable_power_package_records(elements)` | Disable specific power telemetry elements                      |
| `disable_all_power_package_records()`  | Disable all power telemetry elements                             |
| `enable_instantaneous_package_records(elements)` | Enable specific instantaneous telemetry elements         |
| `enable_all_instantaneous_package_records()` | Enable all instantaneous telemetry elements                |
| `disable_instantaneous_package_records(elements)` | Disable specific instantaneous telemetry elements        |
| `disable_all_instantaneous_package_records()` | Disable all instantaneous telemetry elements               |

### Data Collection

| API                                    | Description                                                       |
| -------------------------------------- | ----------------------------------------------------------------- |
| `read_manifest(filename)`              | Retrieve and save telemetry manifest file for data decoding      |
| `read_packages_for_duration(prefix, duration)` | Read telemetry packages for specified time duration        |
| `read_n_packages(num, prefix, timeout)` | Read specific number of telemetry packages with timeout        |
| `decode_packages()`                    | Decode collected telemetry packages using manifest file          |
| `decode_to_file(manifest, payload, output)` | Decode single telemetry file using external decoder tool   |

## Usage Examples

### Basic Power Telemetry Collection

```python
from power_telemetry_tests.pwr_tlm_lib import power_tlm_lib, pwr_telemetry_element_id_t
from mts_tests.trp_lib import mts_cli_trp_endpoint
from fpfw_automation_primitives.serial.telnet import Telnet_

# Setup communication endpoint
telnet_port = Telnet_(host="localhost", port="4257", encoding="UTF-8")
telnet_port.open()
mts_endpoint = mts_cli_trp_endpoint(telnet_port, 0, transfer_relay_protocol.cpu_type.CPU_SCP)

# Use context manager for automatic cleanup
with power_tlm_lib(mts_endpoint, "C:/telemetry_data") as pwr_tlm:
    # Reset and configure client

    # Enable specific power elements
    elements = [
        pwr_telemetry_element_id_t.POWER_TELEMETRY_ELEMENT_CORE_VOLTAGE,
        pwr_telemetry_element_id_t.POWER_TELEMETRY_ELEMENT_CORE_POWER,
        pwr_telemetry_element_id_t.POWER_TELEMETRY_ELEMENT_CORE_TEMPERATURE
    ]
    pwr_tlm.enable_power_package_records(elements)

    # Start data collection
    pwr_tlm.start_dcp_client()

    # Collect 10 packages with 60-second timeout
    pwr_tlm.read_n_packages(10, "power_data", 60)

    # Get manifest and decode packages
    pwr_tlm.read_manifest("manifest.bin")
    pwr_tlm.decode_packages()

# Cleanup happens automatically via context manager
```

### Instantaneous Telemetry Collection

```python
from power_telemetry_tests.pwr_tlm_lib import power_tlm_lib, instantaneous_telemetry_element_id_t

with power_tlm_lib(mts_endpoint, "C:/instant_telemetry") as pwr_tlm:

    # Enable high-frequency instantaneous telemetry
    pwr_tlm.enable_all_instantaneous_package_records()
    pwr_tlm.start_dcp_client()

    # Collect for 30 seconds (high-frequency data)
    pwr_tlm.read_packages_for_duration("instant_data", 30)

    # Process collected data
    pwr_tlm.read_manifest("instant_manifest.bin")
    pwr_tlm.decode_packages()
```

### Client State Management

```python
with power_tlm_lib(mts_endpoint, "C:/telemetry") as pwr_tlm:
    # Check initial state
    state = pwr_tlm.get_client_state()
    print(f"Initial client state: {state}")

    # Reset if needed
    if state != dcp_client_state_t.DCP_CLIENT_STATE_STOPPED:
        pwr_tlm.reset()

    # Configure and start
    pwr_tlm.enable_all_power_package_records()
    pwr_tlm.start_dcp_client()

    # Verify running state
    state = pwr_tlm.get_client_state()
    assert state == dcp_client_state_t.DCP_CLIENT_STATE_RUNNING
```

## Design Principles

The Power Telemetry Library demonstrates several key object-oriented design principles that make the code maintainable, testable, and flexible:

### ✅ Single Responsibility Principle (SRP)

**What it means:** Each class should have only one reason to change and focus on a single responsibility.

**Why it's good:** When code has a single, clear purpose, it's easier to understand, test, and modify. Changes to one feature don't accidentally break unrelated functionality.

**How we apply it:**

- `power_tlm_lib`: Responsible only for power telemetry data collection and management
- `pwr_telemetry_element_id_t`: Responsible only for power telemetry element enumeration
- `instantaneous_telemetry_element_id_t`: Responsible only for instantaneous telemetry element enumeration
- Each class has a focused, well-defined purpose within the telemetry domain

### ✅ Open/Closed Principle (OCP)

**What it means:** Classes should be open for extension but closed for modification.

**Why it's good:** You can add new functionality without changing existing, tested code. This reduces the risk of introducing bugs when adding features.

**How we apply it:**

- The library works with abstract `trp_endpoint` base class, allowing new communication methods without modifying the core library
- New telemetry element types can be added to the enumerations without changing collection logic
- Decoder integration is abstracted, allowing different decoder tools to be plugged in

### ✅ Dependency Inversion Principle (DIP)

**What it means:** High-level modules should not depend on low-level modules. Both should depend on abstractions.

**Why it's good:** This reduces coupling between components and makes the system more flexible and testable.

**How we apply it:**

- `power_tlm_lib` depends on the abstract `trp_endpoint` interface rather than specific implementations
- Works seamlessly with `mts_cli_trp_endpoint` (Telnet), `mts_ap_endpoint` (ICC MHU), or any future endpoint implementations
- DCP commands are abstracted through the `dcp_commands` static class interface

### ✅ Dependency Injection Pattern

**What it means:** Dependencies are provided to a class from the outside rather than created internally.

**Why it's good:**

- **Flexibility**: Same library can work with different communication endpoints
- **Testability**: Easy to inject mock endpoints for testing
- **Loose Coupling**: Library doesn't need to know how to create communication channels
- **Configuration**: Communication setup can be configured externally

**How we apply it:**

```python
# Dependencies are injected through the constructor
def __init__(self, src_endpoint: trp_endpoint, file_location: str):
    self.src_endpoint = src_endpoint  # Communication endpoint is injected

# Usage - different endpoint types can be injected
telnet_lib = power_tlm_lib(mts_cli_trp_endpoint(telnet_port, 0, CPU_SCP), "/data")
ap_lib = power_tlm_lib(mts_ap_endpoint(), "/data")
```

### ✅ Context Manager Pattern

**What it means:** Provide automatic resource management using `with` statements to ensure cleanup always happens.

**Why it's good:**

- **Guaranteed Cleanup**: Resources are always released, even if exceptions occur
- **Exception Safety**: Proper error handling and resource cleanup in all scenarios
- **Simplified Usage**: Users don't need to remember to call cleanup methods manually
- **Robust Testing**: Test failures don't leave resources in inconsistent states

**How we apply it:**

```python
class power_tlm_lib:
    def __enter__(self):
        self.reset()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        # Always attempt cleanup, even if an exception occurred
        try:
            self.reset()  # Stop DCP client safely
        except Exception:
            pass  # Log but don't raise during cleanup

        try:
            self.close()  # Close communication endpoint
        except Exception:
            pass

        return False  # Propagate any original exception

# Usage - automatic cleanup guaranteed
with power_tlm_lib(endpoint, "/data") as pwr_tlm:
    pwr_tlm.enable_all_power_package_records()
    pwr_tlm.start_dcp_client()
    pwr_tlm.read_n_packages(10, "data", 60)
    # Cleanup happens automatically, even if an exception occurs
```

### ✅ Composition Over Inheritance

**What it means:** Favor object composition (has-a relationships) over class inheritance (is-a relationships).

**Why it's good:**

- **Runtime Flexibility**: You can change behavior by swapping out composed objects at runtime
- **Avoid Inheritance Problems**: Deep inheritance hierarchies become hard to understand and maintain
- **Multiple Behaviors**: A class can compose multiple different objects to get various capabilities
- **Easier Testing**: You can easily mock or stub individual composed components
- **No Diamond Problem**: Avoids complex multiple inheritance issues
- **Portability**: Makes it easier to migrate code between different projects and technology stacks by separating project-independent behavior from project-specific dependencies
- **Separation of Common Behavior**: Allows extraction of reusable components that work across different environments and platforms

**Composition vs Aggregation - Ownership Patterns:**

In UML, there are two types of "has-a" relationships that differ in ownership responsibility:

**Composition (filled diamond `*--`)**: Full lifecycle ownership

- The containing class creates, owns, and destroys the contained object
- Contained object cannot exist without the container
- Example: `Car *-- Engine` (car creates and owns the engine)

**Aggregation (empty diamond `o--`)**: Shared/managed ownership

- The contained object is created externally and injected
- The container manages but doesn't create the contained object
- Contained object can exist independently of the container
- Example: `power_tlm_lib o-- trp_endpoint` (endpoint created externally, library manages it)**Dependency Injection and UML Relationships:**

Dependency injection is a **design pattern/technique**, not a UML relationship type. The UML relationship depends on **what the injected class does with the dependency**:

**DI → Aggregation (`--o`)**: When the receiving class manages but doesn't own creation
- Object created externally, injected via constructor/setter
- Receiving class may manage lifecycle (close, cleanup) but didn't create
- Most common DI pattern
- Example: `power_tlm_lib --o trp_endpoint`

**DI → Composition (`*--`)**: When the receiving class takes full ownership after injection
- Object created externally but receiving class takes complete control
- Receiving class becomes responsible for the object's entire remaining lifecycle
- Less common but valid DI pattern
- Example: Database connection transferred to a transaction manager

**DI → Dependency (`..>`)**: When the receiving class only uses without managing
- Object used temporarily or passed through to other methods
- No lifecycle management by the receiving class
- Example: Logger passed to methods for temporary use

The key insight: **Dependency Injection is orthogonal to UML relationships** - it's about *how* dependencies are provided, while UML relationships describe *what* the receiving class does with them.

```python
# Traditional Composition (filled diamond *--) - NO dependency injection
class Car:
    def __init__(self):
        self.engine = Engine()  # Car creates and owns engine

    def __del__(self):
        del self.engine  # Car destroys engine

# DI → Aggregation (empty diamond --o) - MOST COMMON DI pattern
class power_tlm_lib:
    def __init__(self, src_endpoint):  # Dependency injected
        self.src_endpoint = src_endpoint  # Endpoint created externally

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.src_endpoint.close()  # Library manages but didn't create

# DI → Composition (filled diamond *--) - DI with full ownership transfer
class TransactionManager:
    def __init__(self, db_connection):  # Connection injected
        self.connection = db_connection  # Takes full ownership
        self.connection.begin_transaction()  # Modifies state permanently

    def __del__(self):
        self.connection.rollback()  # Full lifecycle responsibility
        self.connection.close()     # Destroys the connection

# DI → Dependency (dashed arrow ..>) - DI with temporary usage
class DataProcessor:
    def __init__(self, logger):  # Logger injected but not stored
        logger.info("DataProcessor initialized")  # Use temporarily
        # No self.logger = logger - doesn't retain reference

    def process(self, data, logger):  # Logger passed per-method
        logger.debug(f"Processing {data}")  # Temporary usage only
```

**Portability Benefits:**

When code needs to work across multiple projects or be migrated to new technology stacks, composition provides clear separation:

```python
# Project-independent telemetry logic (portable across projects)
class power_tlm_lib:
    def __init__(self, transport):
        self.src_endpoint = transport  # Composed dependency

    def read_n_packages(self, num_packages, prefix, timeout):
        # This logic can work with any transport implementation
        for i in range(num_packages):
            data = self.src_endpoint.send_dcp_message(...)  # Abstract interface
            self._save_package_to_file(data, f"{prefix}_{i}.bin")

# Project-specific implementations (easily replaceable)
class SVPTelnetEndpoint: pass       # For SVP project via Telnet
class SiliconICCEndpoint: pass      # For silicon project via ICC MHU
class FPGADirectEndpoint: pass      # For FPGA project via direct serial

# Easy migration between projects
svp_telemetry = power_tlm_lib(SVPTelnetEndpoint())
silicon_telemetry = power_tlm_lib(SiliconICCEndpoint())
```

**How we apply it:**

- `power_tlm_lib` `has-a` `trp_endpoint` rather than `is-a` endpoint
- Different communication implementations can be composed with the same telemetry functionality
- DCP protocol operations are composed through `dcp_commands` rather than inherited
- File management and decoding capabilities are composed into the main class
- Telemetry logic remains project-independent while communication mechanisms can be project-specific

### ✅ Strategy Pattern

**What it means:** Define a family of algorithms (strategies) and make them interchangeable.

**Why it's good:** Allows behavior to be selected at runtime and makes adding new strategies easy.

**How we apply it:**

- Different `trp_endpoint` implementations represent different communication strategies
- Same telemetry interface, different implementations for different environments (SVP, Silicon, FPGA)
- Decoder integration supports different decoder tools as strategies

### ✅ Template Method Pattern

**What it means:** Define the skeleton of an algorithm in a base class, letting subclasses override specific steps.

**Why it's good:** Promotes code reuse while allowing customization of specific behaviors.

**How we apply it:**

- `_read_packages_loop()` provides the template for package reading workflows
- `read_n_packages()` and `read_packages_for_duration()` customize specific termination conditions
- Common package processing logic is reused while allowing different collection strategies

### 🎯 Why These Principles Matter

**For Power Analysis Teams:**

- **Predictable Interface**: Consistent API patterns make telemetry collection predictable across different test scenarios
- **Environment Flexibility**: Same code works across SVP, silicon, and FPGA environments
- **Reliable Data Collection**: Context manager ensures data integrity even when tests fail

**For Maintenance and Development:**

- **Easier Debugging**: Well-structured code makes it easier to isolate telemetry collection issues
- **Extensible Design**: Adding new telemetry elements or communication methods is straightforward
- **Robust Testing**: Dependency injection enables comprehensive unit testing with mock endpoints

**For Cross-Project Usage:**

- **Portable Codebase**: Telemetry logic can be reused across different silicon projects
- **Technology Migration**: Easy adaptation when moving between communication interfaces
- **Consistent Tools**: Same telemetry collection patterns across different development environments
