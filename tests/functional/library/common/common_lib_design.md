# Serial Communication Library Design Document

This document describes the serial communication libraries (`serial_utils.py` and `uefi_shell.py`) that provide utilities for managing serial communication with SCP, MCP, and UEFI channels in the functional test environment.

## Table of Contents

[[_TOC_]]

## Introduction

### Description

The Serial Communication Library consists of two main Python modules that provide utilities for managing serial communication with different firmware components:

- **`serial_utils.py`**: Core serial communication utilities for SCP and MCP channels
- **`uefi_shell.py`**: UEFI-specific shell communication and command execution utilities

These libraries enable automated testing and interaction with firmware components through serial interfaces, supporting both telnet and direct serial connections.

### Terms

| Term   | Description                                           |
| ------ | ----------------------------------------------------- |
| SCP    | System Control Processor                              |
| MCP    | Management Control Processor                          |
| UEFI   | Unified Extensible Firmware Interface                |
| UART   | Universal Asynchronous Receiver-Transmitter          |
| PSCI   | Power State Coordination Interface                    |
| DUT    | Device Under Test                                     |
| SVP    | System Validation Platform                            |
| FPGA   | Field-Programmable Gate Array                        |

### Reference Documents

| Document                                  | Description                                           |
| ----------------------------------------- | ----------------------------------------------------- |
| fpfw_automation_primitives               | Core serial communication primitives library         |
| pythia.tdk.echofalls.constants           | Test framework constants and device type definitions |

## Class Diagram

The following Mermaid class diagram illustrates the relationship between the serial communication classes:

### UML Notation Key

For readers unfamiliar with UML class diagram notation:

- **Solid line with empty arrowhead (`<|--`)**: Inheritance (is-a relationship) - child class extends parent class
- **Dashed line with empty arrowhead (`<..`)**: Interface implementation or realization
- **Solid line with filled arrowhead (`-->`)**: Association/dependency (uses or has-a relationship)
- **Dashed line with filled arrowhead (`..>`)**: Dependency (depends on or references)
- **Solid line with filled diamond (`*--`)**: Composition (full lifecycle ownership - creates, owns, destroys)
- **Solid line with empty diamond (`o--`)**: Aggregation (shared/injected ownership - manages but doesn't create)

:::mermaid
classDiagram
    %% Core communication layer
    class SerialChannelBase {
        #channel
        #name: str
        #_ready_text_received: bool
        #wait_for_ready_text: str
        +__init__(channel, name: str, ready_text: str)
        +is_ready_text_received: bool
        +open_channel()
        +close_channel()
        +run_command(command: str, read_until_key: Optional[str]) Optional[str]
        +read_channel_until(read_until_key: str, timeout_seconds: int) Optional[str]
        +wait_for_ready_text_message() bool
        -_read_serial_until(channel, read_until_key: str, timeout_seconds: int) Optional[str]
    }

    %% Application-specific implementations
    class UefiShell {
        +__init__(channel, name: str, ready_text: str)
        +run_uefi_command(command: str, read_until_key: str) Optional[str]
        +wait_for_shell_ready() bool
        +turn_on_core(cores: str) Optional[str]
        +turn_off_core(cores: str) Optional[str]
        +suspend_core(cores: str, suspend_state: str) Optional[str]
        +resume_core(cores: str) Optional[str]
        -_parse_core_specification(cores: str) str
        -_get_expected_core_list(cores: str) list
        -_validate_core_response(response: str, expected_cores: list, pattern: str) bool
    }

    class SerialUtility {
        -scp_channel
        -mcp_channel
        -log
        -dut
        -_scp_mcp_heartbeat_received: bool
        +__init__(scp_channel, mcp_channel, logger, dut)
        +open_mcp_channel()
        +open_scp_channel()
        +run_command_on_mcp(command: str, read_until_key: str, pass_logs: List[str]) Optional[str]
        +run_command_on_scp(command: str, read_until_key: str) Optional[str]
        +read_scp_serial_until(read_until_key: str, timeout_seconds: int) Optional[str]
        +read_mcp_serial_until(read_until_key: str, timeout_seconds: int) Optional[str]
        +wait_for_scp_mcp_heartbeat() bool
        -_read_serial_until(channel, read_until_key: str, timeout_seconds: int) Optional[str]
    }

    %% External channel implementations
    class Telnet_ {
        <<external>>
        +host: str
        +port: str
        +encoding: str
        +open()
        +close()
        +is_open() bool
        +write_line(write_string: str)
        +read_until(key: str, timeout_seconds: int) str
    }

    class DirectSerial {
        <<external>>
        +id: str
        +baud_rate: str
        +throttle_seconds: float
        +open()
        +close()
        +is_open() bool
        +write_line(write_string: str)
        +read_until(key: str, timeout_seconds: int) str
    }

    %% External constants
    class DeviceType {
        <<enumeration>>
        BIGFPGA
        SVP
    }

    %% Inheritance relationships
    SerialChannelBase <|-- UefiShell : extends

    %% Aggregation relationships - dependency injection patterns
    SerialUtility o-- Telnet_ : manages injected SCP/MCP channels
    SerialUtility o-- DirectSerial : manages injected SCP/MCP channels
    SerialUtility ..> DeviceType : checks device type

    SerialChannelBase o-- Telnet_ : manages injected channel
    SerialChannelBase o-- DirectSerial : manages injected channel

    %% Note: UefiShell gets channel access through inheritance from SerialChannelBase

    %% Documentation notes
    note for SerialChannelBase "Base class providing common serial communication patterns.<br/>Uses aggregation: channels created externally, injected via constructor,<br/>but base class manages lifecycle (close operations)"
    note for UefiShell "UEFI-specific operations with CPU core management"
    note for SerialUtility "SCP/MCP dual-channel communication utility"
:::

## Requirements

The Serial Communication Library shall meet the following requirements:

- The library shall support communication with SCP and MCP channels through serial interfaces
- The library shall support both Telnet and Direct Serial connection types
- The library shall provide heartbeat detection capabilities for SCP and MCP channels
- The library shall support UEFI shell command execution with response validation
- The library shall provide CPU core management operations (turn on/off, suspend/resume)
- The library shall support flexible core specification formats (single, range, all)
- The library shall provide comprehensive error handling and logging capabilities
- The library shall support configurable timeout values for serial operations
- The library shall validate command responses and provide feedback on operation success

## Dependencies

The Serial Communication Library depends on the following external components:

- **fpfw_automation_primitives.serial.telnet**: Provides Telnet-based serial communication
- **fpfw_automation_primitives.serial.direct**: Provides direct serial port communication
- **pythia.tdk.echofalls.constants.dut_types**: Device type constants and enumerations
- **logging**: Python standard library for logging functionality
- **time**: Python standard library for time-related operations
- **typing**: Python standard library for type hints and annotations

## Design

The Serial Communication Library provides a layered architecture for managing serial communication with firmware components:

### Architecture Overview

The library consists of three main layers:

1. **Base Communication Layer** (`SerialChannelBase`): Provides common serial communication functionality
2. **Component-Specific Layer** (`SerialUtility`): Handles SCP/MCP specific operations
3. **Application Layer** (`UefiShell`): Provides high-level UEFI shell operations

### Communication Flow

The following sequence diagram illustrates the typical communication flow for UEFI shell operations:

:::mermaid
sequenceDiagram
    participant Test as Test Script
    participant UefiShell as UefiShell
    participant SerialBase as SerialChannelBase
    participant Channel as Serial Channel
    participant UEFI as UEFI Firmware

    Test->>UefiShell: turn_on_core("5-8")
    UefiShell->>UefiShell: _parse_core_specification("5-8")
    UefiShell->>SerialBase: run_command("FS0:PsciTest.efi CORE_ON 5 - 8", "remove-symbol-file")
    SerialBase->>Channel: write_line("FS0:PsciTest.efi CORE_ON 5 - 8")
    Channel->>UEFI: Command
    UEFI->>Channel: Response with core messages
    Channel->>SerialBase: read_until("remove-symbol-file")
    SerialBase->>UefiShell: Response string
    UefiShell->>UefiShell: _validate_core_response()
    UefiShell->>Test: Response or None
:::

### SCP/MCP Communication Flow

:::mermaid
sequenceDiagram
    participant Test as Test Script
    participant SerialUtil as SerialUtility
    participant SCP as SCP Channel
    participant MCP as MCP Channel

    Test->>SerialUtil: wait_for_scp_mcp_heartbeat()
    SerialUtil->>SCP: open()
    SerialUtil->>MCP: open()
    SerialUtil->>SCP: read_until("ScpHeartBeat", 1800s)
    SCP-->>SerialUtil: ScpHeartBeat message
    SerialUtil->>MCP: read_until("McpHeartBeat", 1800s)
    MCP-->>SerialUtil: McpHeartBeat message
    SerialUtil->>Test: True (success)
:::

## API

### SerialUtility Class

The `SerialUtility` class provides methods for managing communication with SCP and MCP channels.

| API                                    | Description                                                       |
| -------------------------------------- | ----------------------------------------------------------------- |
| `__init__(scp_channel, mcp_channel, logger, dut)` | Initialize SerialUtility with communication channels    |
| `open_mcp_channel()`                   | Opens the MCP communication channel                               |
| `open_scp_channel()`                   | Opens the SCP communication channel                               |
| `run_command_on_mcp(command, read_until_key, pass_logs)` | Execute command on MCP and validate response       |
| `run_command_on_scp(command, read_until_key)` | Execute command on SCP and read response                    |
| `read_mcp_serial_until(read_until_key, timeout)` | Read from MCP channel until key found or timeout        |
| `read_scp_serial_until(read_until_key, timeout)` | Read from SCP channel until key found or timeout        |
| `wait_for_scp_mcp_heartbeat()`         | Wait for both SCP and MCP heartbeat messages                     |

### SerialChannelBase Class

The `SerialChannelBase` class provides base functionality for serial channel management.

| API                                    | Description                                                       |
| -------------------------------------- | ----------------------------------------------------------------- |
| `__init__(channel, name, ready_text)`  | Initialize base serial channel with configuration                |
| `is_ready_text_received`               | Property to check if ready text was received                     |
| `open_channel()`                       | Opens the communication channel                                   |
| `close_channel()`                      | Closes the communication channel                                  |
| `run_command(command, read_until_key)` | Execute command and optionally read response                      |
| `read_channel_until(read_until_key, timeout)` | Read from channel until key found or timeout            |
| `wait_for_ready_text_message()`        | Wait for ready text message indicating channel is ready          |

### UefiShell Class

The `UefiShell` class extends `SerialChannelBase` with UEFI-specific operations.

| API                                    | Description                                                       |
| -------------------------------------- | ----------------------------------------------------------------- |
| `__init__(channel, name, ready_text)`  | Initialize UEFI shell with channel configuration                 |
| `run_uefi_command(command, read_until_key)` | Execute UEFI command and read response                      |
| `wait_for_shell_ready()`               | Wait for UEFI shell to be ready                                  |
| `turn_on_core(cores)`                  | Turn on CPU cores (supports single, range, or "ALL")             |
| `turn_off_core(cores)`                 | Turn off CPU cores (supports single, range, or "ALL")            |
| `suspend_core(cores, suspend_state)`   | Suspend CPU cores to specified state (C1, C2, C3, C4)           |
| `resume_core(cores)`                   | Resume CPU cores from suspended state                            |

## Usage Examples

### Basic SCP/MCP Communication

```python
    from fpfw_automation_primitives.serial.telnet import Telnet_
    from serial_utils import SerialUtility

    # Initialize channels
    scp_channel = Telnet_(host="localhost", port="4257", encoding="UTF-8")
    mcp_channel = Telnet_(host="localhost", port="4256", encoding="UTF-8")

    # Create utility instance
    serial_util = SerialUtility(scp_channel, mcp_channel, logger=logger, dut=dut)

    # Wait for heartbeat
    if serial_util.wait_for_scp_mcp_heartbeat():
        # Execute commands
        response = serial_util.run_command_on_mcp("help", "mcp>")
```

### UEFI Shell Operations

```python
from fpfw_automation_primitives.serial.telnet import Telnet_
from uefi_shell import UefiShell

# Initialize UEFI channel
uefi_channel = Telnet_(host="localhost", port="4258", encoding="UTF-8")
uefi = UefiShell(uefi_channel)

# Open channel and wait for ready
uefi.open_channel()
uefi.wait_for_shell_ready()

# CPU core management
uefi.turn_on_core("5-8")           # Turn on cores 5 through 8
uefi.suspend_core("6", "C2")       # Suspend core 6 to C2 state
uefi.resume_core("6")              # Resume core 6
uefi.turn_off_core("ALL")          # Turn off all cores
```

### Direct Serial Connection

```python
from fpfw_automation_primitives.serial.direct import DirectSerial
from uefi_shell import UefiShell

# Direct serial connection (for silicon testing)
direct_port = DirectSerial(id="COM8", baud_rate="115200", throttle_seconds=0.005)
uefi_shell = UefiShell(direct_port)

uefi_shell.open_channel()
uefi_shell.run_uefi_command("help")
```

## Connection Types

| Connection Type   | Description                                           | Use Case                    |
| ----------------- | ----------------------------------------------------- | --------------------------- |
| Telnet            | Network-based serial communication via telnet        | SVP and FPGA testing        |
| DirectSerial      | Direct serial port communication                     | Silicon hardware testing   |

## Design Principles

The Serial Communication Library demonstrates several key object-oriented design principles that make the code maintainable, testable, and flexible:

### ✅ Single Responsibility Principle (SRP)

**What it means:** Each class should have only one reason to change and focus on a single responsibility.

**Why it's good:** When code has a single, clear purpose, it's easier to understand, test, and modify. Changes to one feature don't accidentally break unrelated functionality.

**How we apply it:**

- `SerialChannelBase`: Responsible only for managing generic serial communication
- `UefiShell`: Responsible only for UEFI-specific operations and CPU management
- Each class has a focused, well-defined purpose

### ✅ Open/Closed Principle (OCP)

**What it means:** Classes should be open for extension but closed for modification.

**Why it's good:** You can add new functionality without changing existing, tested code. This reduces the risk of introducing bugs when adding features.

**How we apply it:**

- `SerialChannelBase` can be extended through inheritance (like `UefiShell`) without modifying the base class
- New channel types can be added without changing existing communication logic

### ✅ Liskov Substitution Principle (LSP)

**What it means:** Objects of a parent class should be replaceable with objects of a child class without breaking functionality.

**Why it's good:** This ensures that inheritance hierarchies are logical and that polymorphism works correctly.

**How we apply it:**

- `UefiShell` can be use the same base class behavior as `SerialChannelBase`
- The child class maintains the contract and behavior expectations of the parent
- `UefiShell` extends functionality without altering the base behavior

### ✅ Dependency Inversion Principle (DIP)

**What it means:** High-level modules should not depend on low-level modules. Both should depend on abstractions.

**Why it's good:** This reduces coupling between components and makes the system more flexible and testable.

**How we apply it:**

- `SerialChannelBase` depends on the abstract concept of a "channel" rather than specific implementations
- Works with any object that has the right methods (`Telnet_`, `DirectSerial`, etc.)

### ✅ Dependency Injection Pattern

**What it means:** Dependencies are provided to a class from the outside rather than created internally.

**Why it's good:**

- **Flexibility**: Same class can work with different implementations
- **Testability**: Easy to inject mock objects for testing
- **Loose Coupling**: Classes don't need to know how to create their dependencies
- **Configuration**: Dependencies can be configured externally

**How we apply it:**

```python
# Dependencies are injected through the constructor
def __init__(self, channel, name: str, ready_text: str):
    self.channel = channel  # Channel dependency is injected

# Usage - different channel types can be injected
telnet_channel = SerialChannelBase(Telnet_("localhost", "4258"), "UEFI", "Shell>")
direct_channel = SerialChannelBase(DirectSerial("COM8", "115200"), "Hardware", "Ready")
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

**Portability Benefits:**

When code needs to work across multiple projects or be migrated to new technology stacks, composition provides clear separation:

```python
# Project-independent logic (portable across projects)
class SerialCommunicationLogic:
    def __init__(self, transport):
        self.transport = transport  # Composed dependency

    def execute_command_sequence(self, commands):
        # This logic can work with any transport implementation
        for cmd in commands:
            response = self.transport.send(cmd)
            if not self._validate_response(response):
                return False
        return True

# Project-specific implementations (easily replaceable)
class SVPTelnetTransport: pass      # For SVP project
class SiliconUARTTransport: pass    # For silicon project
class FPGATransport: pass           # For FPGA project

# Easy migration between projects
svp_comm = SerialCommunicationLogic(SVPTelnetTransport())
silicon_comm = SerialCommunicationLogic(SiliconUARTTransport())
```

**Problems with excessive inheritance:**

```python
# BAD: Deep inheritance hierarchy
class SerialChannel: pass
class TelnetChannel(SerialChannel): pass
class SecureTelnetChannel(TelnetChannel): pass
class LoggingSecureTelnetChannel(SecureTelnetChannel): pass  # Getting complex!
```

**Better with composition:**

```python
# GOOD: Compose behaviors instead
class SerialChannelBase:
    def __init__(self, channel, logger=None, security=None):
        self.channel = channel      # Compose channel behavior
        self.logger = logger        # Compose logging behavior
        self.security = security    # Compose security behavior

# Mix and match at runtime
basic_channel = SerialChannelBase(Telnet_("localhost", "4258"))
secure_channel = SerialChannelBase(Telnet_("localhost", "4258"), security=TLSWrapper())
logged_channel = SerialChannelBase(Telnet_("localhost", "4258"), logger=FileLogger())
```

**How we apply it:**

- `SerialChannelBase` `has-a` channel rather than `is-a` channel
- Different channel implementations can be composed with the same base functionality
- `UefiShell` composes `SerialChannelBase` behavior with UEFI-specific operations
- Communication logic remains project-independent while transport mechanisms can be project-specific

### ✅ Strategy Pattern

**What it means:** Define a family of algorithms (strategies) and make them interchangeable.

**Why it's good:** Allows behavior to be selected at runtime and makes adding new strategies easy.

**How we apply it:**

- Different channel types (`Telnet_`, `DirectSerial`) represent different communication strategies
- Same interface, different implementations for different environments

### ✅ Template Method Pattern

**What it means:** Define the skeleton of an algorithm in a base class, letting subclasses override specific steps.

**Why it's good:** Promotes code reuse while allowing customization of specific behaviors.

**How we apply it:**

- `SerialChannelBase` provides common communication functionality
- `UefiShell` customizes specific behaviors while reusing base functionality

### ✅ DRY Principle (Don't Repeat Yourself)

**What it means:** Every piece of knowledge or logic should have a single, authoritative representation in the system.

**Why it's good:**

- **Easier Maintenance**: Fix bugs or make changes in only one place
- **Consistency**: All code uses the same logic, reducing inconsistencies
- **Reduced Errors**: Less duplication means fewer places for bugs to hide
- **Cleaner Code**: Less code to read and understand

**How we apply it:**

```python
# GOOD: Common serial reading logic in one place
def _read_serial_until(self, channel, read_until_key: str, timeout_seconds: int):
    # Single implementation used by both SCP and MCP reading methods

def read_scp_serial_until(self, read_until_key: str, timeout_seconds: int = 60):
    return self._read_serial_until(self.scp_channel, read_until_key, timeout_seconds)

def read_mcp_serial_until(self, read_until_key: str, timeout_seconds: int = 60):
    return self._read_serial_until(self.mcp_channel, read_until_key, timeout_seconds)
```

**What we avoid:**

- Copying and pasting the same error handling logic in multiple methods
- Duplicating command execution patterns across different channel types
- Repeating the same validation logic in multiple places

### ✅ Builder Pattern (Configuration Builder)

**What it means:** Construct complex objects step by step, allowing different configurations of the same basic structure.

**Why it's good:**

- **Readable Configuration**: Complex setup becomes a series of clear, named steps
- **Flexible Construction**: Same builder can create different variations
- **Validation**: Can validate configuration before building the final object
- **Optional Parameters**: Handle many optional parameters elegantly

**How we could apply it (example enhancement):**

```python
class SerialChannelBuilder:
    """Builder for creating configured serial channels."""

    def __init__(self):
        self._channel_type = None
        self._connection_params = {}
        self._timeouts = {}
        self._logging = None

    def with_telnet(self, host: str, port: str):
        self._channel_type = "telnet"
        self._connection_params = {"host": host, "port": port}
        return self

    def with_direct_serial(self, port: str, baud_rate: str):
        self._channel_type = "direct"
        self._connection_params = {"port": port, "baud_rate": baud_rate}
        return self

    def with_timeouts(self, read_timeout: int = 60, connect_timeout: int = 30):
        self._timeouts = {"read": read_timeout, "connect": connect_timeout}
        return self

    def with_logging(self, logger):
        self._logging = logger
        return self

    def build_uefi_shell(self, name: str, ready_text: str = "UEFI Interactive Shell"):
        """Build a configured UEFI shell."""
        channel = self._create_channel()
        return UefiShell(channel, name, ready_text)

    def _create_channel(self):
        if self._channel_type == "telnet":
            return Telnet_(**self._connection_params)
        elif self._channel_type == "direct":
            return DirectSerial(**self._connection_params)

# Usage - much more readable than long constructor calls
uefi_shell = (SerialChannelBuilder()
              .with_telnet("localhost", "4258")
              .with_timeouts(read_timeout=120)
              .with_logging(custom_logger)
              .build_uefi_shell("Test_UEFI"))
```

**Benefits in our context:**

- Simplifies creating channels with different configurations
- Makes test setup more readable and maintainable
- Allows reusing common configurations across tests

### 🎯 Why These Principles Matter

**For New Developers:**

- **Predictable Code**: Following consistent patterns makes code easier to understand
- **Easier Testing**: Well-designed classes are easier to test in isolation
- **Reduced Bugs**: Loose coupling means changes in one place are less likely to break other parts

**For Maintenance:**

- **Easier Changes**: Adding new features or fixing bugs is more straightforward
- **Better Reliability**: Well-structured code is less prone to unexpected interactions
- **Clearer Intent**: Code that follows good principles is self-documenting

**For Team Development:**

- **Consistent Approach**: Everyone follows the same architectural patterns
- **Easier Code Reviews**: Well-structured code is easier to review and understand
- **Knowledge Sharing**: Good design patterns transfer knowledge between team members
