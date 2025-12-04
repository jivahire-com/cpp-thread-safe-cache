# ICC MHU Library Design Document

This document describes the Inter-Core Communication Message Handling Unit (ICC MHU) library design, public API, and architectural patterns for enabling efficient message passing between processor cores on Kingsgate silicon.

## Table of Contents

[[_TOC_]]

## Introduction

### Description

The ICC MHU (Inter-Core Communication Message Handling Unit) library provides a robust, hardware-abstracted interface for message passing between different processor cores (SCP, MCP, AP) on Kingsgate silicon. The library implements both single-die and die-to-die (D2D) communication patterns using the hardware MHU mailbox interface.

The library consists of:
- **Memory Interface Abstraction**: `CoreMemoryMapInterface` providing hardware-agnostic memory access
- **Hardware Implementations**: `ApCoreMemoryInterface` and `Trace32ApCore` for T32 debugger-based memory access
- **MHU Protocol Layer**: `IccMhu` base class with `IccMhuAp` AP-specific implementation
- **Test Infrastructure**: `MockMemoryInterface` and comprehensive test functions
- **Integration Testing**: `IccMhuTest` class for Pythia-based functional testing

### Terms

| Term   | Description                                    |
| ------ | ---------------------------------------------- |
| ICC    | Inter-Core Communication                       |
| MHU    | Message Handling Unit                          |
| AP     | Application Processor                          |
| SCP    | System Control Processor                       |
| MCP    | Management Control Processor                   |
| D2D    | Die-to-Die communication                       |
| T32    | Trace32 debugger interface                     |
| SVP    | Software Verification Platform                 |
| FPGA   | Field-Programmable Gate Array                  |

### Reference Documents

| Document                                  | Link                                |
| ----------------------------------------- | ----------------------------------- |
| Kingsgate ICC Shared Header               | silibs/libraries/kng_silibs_platform/include/kng_icc_shared.h |
| Pythia 2.0 Test Framework                | pythia/tdk/echofalls/echofalls_base_test.py |
| T32 Automation Framework                 | fpfw_automation_trace32/trace32.py |

## Requirements

The ICC MHU library design requirements:

- The library shall provide hardware-abstracted memory access through pluggable interfaces
- The library shall support both T32 debugger and native hardware memory access modes
- The library shall implement timeout-based packet send/receive operations to prevent deadlocks
- The library shall validate packet integrity including payload size and content verification
- The library shall support both single-die and die-to-die communication patterns
- The library shall provide comprehensive error handling with detailed logging and diagnostics
- The library shall implement lazy initialization to minimize resource usage
- The library shall support concurrent testing scenarios with proper resource cleanup
- The library shall provide mock interfaces for unit testing without hardware dependencies
- The library shall validate MHU channel status before attempting operations

## Dependencies

The ICC MHU library dependencies:

- **T32 Debugger Framework**: fpfw_automation_trace32.trace32 for hardware memory access
- **Pythia TDK**: pythia.tdk.common.util.debugger.trace32 for test infrastructure
- **Serial Utilities**: serial_utils.py for SCP/MCP channel management
- **Thread Utilities**: thread_utils.py for asynchronous message reception
- **Standard Library**: logging, time, tempfile, pathlib, typing, enum, abc
- **Test Framework**: EchoFallsBaseTest for integration testing

## Design

### Architecture Overview

The ICC MHU library implements a layered architecture following the Strategy and Abstract Factory patterns:

**UML Notation Key:**
- `<|--` : **Inheritance** (solid line) - "extends" or "is-a" relationship
- `<|..` : **Interface Implementation** (dotted line) - "implements" relationship
- `*--` : **Composition** (filled diamond) - full lifecycle ownership (creates, owns, destroys)
- `o--` : **Aggregation** (empty diamond) - shared/injected ownership (manages but doesn't create)
- `-->` : **Uses** (solid arrow) - direct usage or strong dependency relationship
- `..>` : **Depends On** (dotted arrow) - loose dependency or occasional usage

:::mermaid
classDiagram
    %% Interface layer at the top
    class CoreMemoryMapInterface {
        <<abstract>>
        +read(address: int, size: int) int
        +write(address: int, value: int, size: int) void
        +dump(address: int, size: int) bytearray
        +close() void
    }

    %% Production implementation layer
    class ApCoreMemoryInterface {
        -t32_instance: Trace32
        -_attached: bool
        +_ensure_attached() void
        +_validate_t32_tuple(ret_tuple) void
        +read(address: int, size: int) int
        +write(address: int, value: int, size: int) void
        +dump(address: int, size: int) bytearray
    }

    class Trace32ApCore {
        -processor_type: PROCESSOR
        -debugger_name: str
        -t32_bins_path: Path
        -_closed: bool
        -_initialized: bool
        +_ensure_initialized() void
        +execute_command(command: str) tuple
        +close() void
    }

    %% Test implementation layer
    class MockMemoryInterface {
        -memory: dict
        -operations_log: list
        -debug: bool
        -fail_on_read: bool
        -fail_on_write: bool
        +configure_scenario(scenario: str, **kwargs) void
        +get_operation_count(operation_type: str) int
    }

    %% MHU protocol layer
    class IccMhu {
        <<abstract>>
        #SND_MHU_ADDRESS: int
        #REC_MHU_ADDRESS: int
        #SEND_PAYLOAD_ADDR: int
        #RECV_PAYLOAD_ADDR: int
        -memory_interface: CoreMemoryMapInterface
        +check_packet_pending(channel_direction: str) bool
        +get_packet(timeout_sec: int) bytearray
        +send_packet(command: int, packet: bytearray, timeout_sec: int) void
        +_clear_pending_recv() void
    }

    class IccMhuAp {
        +AP2MCP_MHU_SND_NS_ADDRESS: int
        +MCP2AP_MHU_REC_NS_ADDRESS: int
        +SEND_PAYLOAD_ADDR: int
        +RECV_PAYLOAD_ADDR: int
    }

    %% Test infrastructure layer
    class IccMhuTest {
        -core_com_channel_scp_die0: Channel
        -core_com_channel_mcp_die0: Channel
        -serial_util_die0: SerialUtility
        +test_icc_cli_mscp_mhu(sender_is_mcp: bool, ...) bool
        +test_icc_cli_d2d_mhu(sender_is_mcp: bool, sender_is_die0: bool, ...) bool
        +validate_mhu_message_payload(messages: List[str], ...) bool
    }

    %% External dependency
    class Trace32 {
        <<external>>
        +execute_command(command: str) tuple
        +initialize() void
        +launch() void
        +quit() void
    }

    %% Interface implementations (clear separation)
    CoreMemoryMapInterface <|.. ApCoreMemoryInterface : implements
    CoreMemoryMapInterface <|.. MockMemoryInterface : implements

    %% External inheritance
    Trace32 <|-- Trace32ApCore : extends

    %% Aggregation relationships (dependency injection)
    ApCoreMemoryInterface o-- Trace32ApCore : manages injected T32 instance
    IccMhu o-- CoreMemoryMapInterface : manages injected memory interface

    %% Protocol inheritance
    IccMhu <|-- IccMhuAp : extends

    %% Test relationships (separated for clarity)
    IccMhuTest --> IccMhuAp : tests with
    IccMhuTest --> MockMemoryInterface : mocks with

    %% Class positioning hints for better layout
    note for CoreMemoryMapInterface "Abstract interface for\nmemory access strategies"
    note for MockMemoryInterface "Test double for\nunit testing"
    note for IccMhu "Abstract MHU protocol layer.<br/>Uses aggregation: memory interface injected via constructor,<br/>protocol layer manages but doesn't create memory access"
    note for IccMhuTest "Integration test infrastructure"
:::

### Communication Flow

The message passing sequence follows this pattern:

:::mermaid
sequenceDiagram
    participant Sender as Sender Core
    participant MHU as MHU Interface
    participant Receiver as Receiver Core
    participant Memory as Memory Interface

    Note over Sender, Memory: Setup Phase
    Receiver->>MHU: setup receive request (icc_mhu recv)
    MHU->>Memory: configure receive mailbox

    Note over Sender, Memory: Send Phase
    Sender->>MHU: check_packet_pending("send")
    MHU->>Memory: read send channel status
    Memory-->>MHU: channel available
    Sender->>MHU: send_packet(command, payload)
    MHU->>Memory: write payload data
    MHU->>Memory: write command and size
    MHU->>Memory: trigger send (set MHU channel)

    Note over Sender, Memory: Receive Phase
    MHU->>Memory: check_packet_pending("recv")
    Memory-->>MHU: packet available
    MHU->>Memory: read packet header
    MHU->>Memory: dump payload data
    MHU->>Memory: clear pending status
    MHU-->>Receiver: return packet data

    Note over Sender, Memory: Validation Phase
    Receiver->>Receiver: validate payload size
    Receiver->>Receiver: validate payload content
:::

### Key Design Patterns

#### 1. Strategy Pattern (Memory Interface)
The `CoreMemoryMapInterface` abstraction allows pluggable memory access strategies:
- **ApCoreMemoryInterface**: Production T32 debugger-based access
- **MockMemoryInterface**: Test doubles for unit testing
- **Future implementations**: Native hardware access, simulation interfaces

#### 2. Template Method Pattern (MHU Operations)
The `IccMhu` base class defines the algorithm skeleton while allowing subclasses to customize:
```python
class IccMhu:
    def send_packet(self, command: int, packet: bytearray, timeout_sec: int):
        # Template method defines the algorithm
        self._wait_for_send_available(timeout_sec)  # Step 1
        self._write_packet_data(packet)             # Step 2
        self._write_packet_metadata(command)        # Step 3
        self._trigger_send()                        # Step 4
```

#### 3. Dependency Injection
Components accept their dependencies rather than creating them:
```python
class IccMhuAp(IccMhu):
    def __init__(self, memory_interface: Optional[CoreMemoryMapInterface] = None):
        if memory_interface is None:
            memory_interface = ApCoreMemoryInterface()  # Default, but injectable
        super().__init__(memory_interface)
```

#### 4. Lazy Initialization
Resource-intensive operations are deferred until needed (see detailed section below):
```python
class Trace32ApCore:
    def _ensure_initialized(self):
        if not self._initialized:
            self.initialize()
            self.launch(...)
            self._initialized = True
```

#### 5. Resource Management (RAII)
Proper cleanup using context management patterns (see detailed section below):
```python
class IccMhuTest:
    def close_all_channels(self):
        for attr in ["core_com_channel_scp_die0", ...]:
            channel = getattr(self, attr)
            if channel is not None and channel.is_open():
                channel.close()
```

## API Reference

### Core Memory Interface

| API | Description | Parameters | Returns |
|-----|-------------|------------|---------|
| `read(address, size=4)` | Read memory at specified address | `address: int`, `size: int` | `int` value |
| `write(address, value, size=4)` | Write value to memory address | `address: int`, `value: int`, `size: int` | `None` |
| `dump(address, size)` | Efficiently dump memory block | `address: int`, `size: int` | `bytearray` |
| `close()` | Clean up interface resources | None | `None` |

### MHU Communication Interface

| API | Description | Parameters | Returns |
|-----|-------------|------------|---------|
| `check_packet_pending(direction)` | Check if packet is pending | `direction: str` ("recv"/"send") | `bool` |
| `get_packet(timeout_sec=5)` | Receive packet with timeout | `timeout_sec: int` | `Optional[bytearray]` |
| `send_packet(command, packet, timeout_sec=5)` | Send packet with command | `command: int`, `packet: bytearray`, `timeout_sec: int` | `None` |

### Test Infrastructure Interface

| API | Description | Parameters | Returns |
|-----|-------------|------------|---------|
| `test_icc_cli_mscp_mhu(sender_is_mcp, ...)` | Test single-die MCP↔SCP communication | `sender_is_mcp: bool`, channel, message_id, payload params | `bool` |
| `test_icc_cli_d2d_mhu(sender_is_mcp, sender_is_die0, ...)` | Test die-to-die communication | `sender_is_mcp: bool`, `sender_is_die0: bool`, communication params | `bool` |
| `validate_mhu_message_payload(messages, expected_size, expected_bytes)` | Validate received message content | `messages: List[str]`, `expected_size: int`, `expected_bytes: List[int]` | `bool` |

### Mock Testing Interface

| API | Description | Parameters | Returns |
|-----|-------------|------------|---------|
| `configure_scenario(scenario, **kwargs)` | Setup test scenario | `scenario: str`, keyword arguments | `None` |
| `get_operation_count(operation_type=None)` | Get count of memory operations | `operation_type: Optional[str]` | `int` |
| `clear_log()` | Clear operations log | None | `None` |

## Usage Examples

### Basic MHU Communication

```python
# Create MHU interface with default AP core
mhu = IccMhuAp()

# Send a packet
command = IccCommandId.ICC_COMMAND_DCP_MSG.value
payload = bytearray([0x10, 0x11, 0x12, 0x13])
mhu.send_packet(command, payload, timeout_sec=10)

# Receive a packet
received_packet = mhu.get_packet(timeout_sec=10)
if received_packet:
    print(f"Received: {[hex(b) for b in received_packet]}")

# Clean up
mhu.close()
```

### Using Mock Interface for Testing

```python
# Create mock memory interface for testing
mock_memory = MockMemoryInterface(debug=True)
mhu = IccMhuAp(memory_interface=mock_memory)

# Configure test scenario
mock_memory.configure_scenario("recv_packet_ready",
                              payload_data=[0xAA, 0xBB, 0xCC],
                              command=0x1234)

# Test packet reception
packet = mhu.get_packet(timeout_sec=1)
assert packet == bytearray([0xAA, 0xBB, 0xCC])

# Verify memory operations
print(f"Read operations: {mock_memory.get_operation_count('read')}")
print(f"Write operations: {mock_memory.get_operation_count('write')}")
```

### Integration Testing with Pythia

```python
class MyIccTest(IccMhuTest):
    def test_mcp_to_scp_communication(self):
        """Test MCP sending message to SCP on same die."""
        return self.test_icc_cli_mscp_mhu(
            sender_is_mcp=True,
            channel=4,
            message_id=2,
            payload_size=4,
            payload_bytes=[0x10, 0x20, 0x30, 0x40]
        )

    def test_die0_to_die1_communication(self):
        """Test die-to-die communication."""
        return self.test_icc_cli_d2d_mhu(
            sender_is_mcp=True,
            sender_is_die0=True,
            channel=9,
            message_id=3,
            payload_size=3,
            payload_bytes=[0xAA, 0xBB, 0xCC]
        )
```

## Design Principles

The ICC MHU Library demonstrates several key object-oriented design principles that make the code maintainable, testable, and flexible:

### ✅ Single Responsibility Principle (SRP)

**What it means:** Each class should have only one reason to change and focus on a single responsibility.

**Why it's good:** When code has a single, clear purpose, it's easier to understand, test, and modify. Changes to one feature don't accidentally break unrelated functionality.

**How we apply it:**

- **`CoreMemoryMapInterface`**: Responsible only for defining memory access contract
- **`ApCoreMemoryInterface`**: Responsible only for T32-based memory access implementation
- **`IccMhu`**: Responsible only for MHU protocol operations
- **`IccMhuAp`**: Responsible only for AP-specific MHU address mappings
- **`MockMemoryInterface`**: Responsible only for test double memory simulation
- **`IccMhuTest`**: Responsible only for orchestrating integration testing scenarios

### ✅ Open/Closed Principle (OCP)

**What it means:** Classes should be open for extension but closed for modification.

**Why it's good:** You can add new functionality without changing existing, tested code. This reduces the risk of introducing bugs when adding features.

**How we apply it:**

- New memory interfaces can be added by implementing `CoreMemoryMapInterface` without modifying existing code
- New MHU variants can be created by extending `IccMhu` for different processor cores
- Test scenarios can be extended without modifying core test infrastructure
- Additional processor cores can be supported by creating new address mappings

### ✅ Liskov Substitution Principle (LSP)

**What it means:** Objects of a parent class should be replaceable with objects of a child class without breaking functionality.

**Why it's good:** This ensures that inheritance hierarchies are logical and that polymorphism works correctly.

**How we apply it:**

- Any `CoreMemoryMapInterface` implementation can be used interchangeably in `IccMhu`
- `ApCoreMemoryInterface` and `MockMemoryInterface` both fulfill the memory interface contract identically
- `IccMhuAp` can be used wherever `IccMhu` is expected without changing behavior
- Test and production memory interfaces have identical behavioral contracts

### ✅ Interface Segregation Principle (ISP)

**What it means:** Interfaces should be focused and not force clients to depend on methods they don't use.

**Why it's good:** Smaller, focused interfaces are easier to implement, test, and understand. Clients only depend on what they actually need.

**How we apply it:**

- `CoreMemoryMapInterface` only exposes essential memory operations (read, write, dump, close)
- MHU protocol operations are separated from memory access concerns
- Test-specific methods are isolated in test classes rather than mixed with production code
- Optional capabilities have sensible defaults and don't burden simple implementations

### ✅ Dependency Inversion Principle (DIP)

**What it means:** High-level modules should not depend on low-level modules. Both should depend on abstractions.

**Why it's good:** This reduces coupling between components and makes the system more flexible and testable.

**How we apply it:**

- `IccMhu` depends on `CoreMemoryMapInterface` abstraction, not concrete T32 or mock implementations
- `IccMhuTest` depends on the MHU interface contract, not specific hardware implementations
- Memory access is abstracted away from communication protocol logic
- Test infrastructure can use mock or real implementations interchangeably

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
class IccMhuAp(IccMhu):
    def __init__(self, memory_interface: Optional[CoreMemoryMapInterface] = None):
        if memory_interface is None:
            memory_interface = ApCoreMemoryInterface()  # Default, but injectable
        super().__init__(memory_interface)

# Usage - different memory interfaces can be injected
production_mhu = IccMhuAp(ApCoreMemoryInterface())
test_mhu = IccMhuAp(MockMemoryInterface())
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

- `IccMhu` **has-a** `CoreMemoryMapInterface` rather than inheriting memory capabilities
- `ApCoreMemoryInterface` **has-a** `Trace32ApCore` instance rather than inheriting T32 functionality
- `IccMhuTest` **uses** `SerialUtility` and `IccMhuAp` rather than inheriting from them

**Benefits:**

- **Different memory interfaces** can be composed with the same MHU protocol logic
- **Test infrastructure** can compose different memory implementations without inheritance complexity
- **Protocol definitions** remain independent of memory access implementations
- **Project Portability**: The next project may have different IP blocks (different T32 debugger versions, new memory interfaces, alternative hardware access methods), but we've prevented that coupling from leaking into the project-independent code
- **Common Behavior Separation**: Project-independent logic (MHU protocol, packet validation, timeout handling) is cleanly separated from project-dependent memory access (Kingsgate-specific T32 integration, mock interfaces for testing)
- **Technology Migration**: When moving from Kingsgate to the next generation silicon, the ICC MHU protocol logic can be reused while only the memory interface layer needs updating, or the MHU IP can replaced completely

### ✅ Strategy Pattern

**What it means:** Define a family of algorithms (strategies) and make them interchangeable.

**Why it's good:** Allows behavior to be selected at runtime and makes adding new strategies easy.

**How we apply it:**

- Different memory interface implementations (`ApCoreMemoryInterface`, `MockMemoryInterface`) represent different memory access strategies
- Same `CoreMemoryMapInterface` contract, different implementations for different environments
- Can switch between T32 debugger access and mock access without changing MHU protocol code

### ✅ Template Method Pattern

**What it means:** Define the skeleton of an algorithm in a base class, letting subclasses override specific steps.

**Why it's good:** Promotes code reuse while allowing customization of specific behaviors.

**How we apply it:**

```python
class IccMhu:
    def send_packet(self, command: int, packet: bytearray, timeout_sec: int):
        # Template method defines the algorithm
        self._wait_for_send_available(timeout_sec)  # Step 1
        self._write_packet_data(packet)             # Step 2
        self._write_packet_metadata(command)        # Step 3
        self._trigger_send()                        # Step 4
```

- `IccMhu` provides common communication algorithm
- `IccMhuAp` customizes address mappings while reusing base functionality

### ✅ DRY Principle (Don't Repeat Yourself)

**What it means:** Every piece of knowledge or logic should have a single, authoritative representation in the system.

**Why it's good:**

- **Easier Maintenance**: Fix bugs or make changes in only one place
- **Consistency**: All code uses the same logic, reducing inconsistencies
- **Reduced Errors**: Less duplication means fewer places for bugs to hide
- **Cleaner Code**: Less code to read and understand

**How we apply it:**

```python
# GOOD: Common T32 validation logic in one place
def _validate_t32_tuple(self, ret_tuple):
    # Single implementation used by all T32 operations

def read(self, address: int, size: int = 4):
    ret_tuple = self.t32_instance.execute_command(f"data.dump ENAXI:{address:08X}")
    self._validate_t32_tuple(ret_tuple)  # Reuse validation

def write(self, address: int, value: int, size: int = 4):
    ret_tuple = self.t32_instance.execute_command(f"data.set ENAXI:{address:08X} %{size_format} {value}")
    self._validate_t32_tuple(ret_tuple)  # Same validation logic
```

**What we avoid:**

- Copying and pasting the same error handling logic in multiple methods
- Duplicating memory operation patterns across different interfaces
- Repeating the same validation logic in multiple places

#### ✅ Lazy Initialization Pattern

**What it means:** Defer the creation or initialization of expensive resources until they are actually needed.

**Why it's good:**

- **Robustness**: If it is not lazy initialized, then it would either have to be started manually, which places burden
   on all consumers, or it is done in the __init__. If in the init, then the trace32 app wil start and initialize
   hardware, but if the user has a simple exception, then the script will shut down immediately and short cycle the
   trace32 hardware. That can cause it to latch in a bad state. With lazy init, the script exits immediately for all
   new user generate exceptions without ever cycling the trace 32 hardware.
- **Performance**: Avoid expensive operations during object construction
- **Resource Conservation**: Don't allocate resources that might never be used
- **Startup Time**: Faster application startup by deferring heavy initialization
- **Memory Efficiency**: Reduce memory footprint by only creating what's needed
- **Error Handling**: Better error handling since initialization happens when resources are actually used

**How we apply it:**

```python
class Trace32ApCore:
    def __init__(self):
        # Lightweight constructor - no expensive T32 initialization
        self._initialized = False
        self._closed = False
        # Configuration is set up but T32 instance isn't created yet

    def _ensure_initialized(self):
        """Lazy initialization of T32 debugger - only when first needed."""
        if not self._initialized:
            logger.debug("Lazy initializing T32 debugger")
            # Expensive operations happen here, not in constructor
            self.initialize()
            self.launch(
                config=self.t32_core_config_path,
                args=self.t32_start_script_path,
                wait_time=self.t32_launch_delay_seconds,
            )
            self._initialized = True

    def execute_command(self, command):
        """All public methods ensure initialization before use."""
        self._ensure_initialized()  # Initialize only when actually needed
        return super().execute_command(command)
```

**Benefits in our context:**

- T32 debugger connection is expensive and may fail - only attempt when actually needed
- Tests can create objects without triggering heavy initialization
- Better error messages since initialization happens in context of actual usage
- Faster test execution when T32 functionality isn't needed

#### ✅ RAII Pattern (Resource Acquisition Is Initialization)

**What it means:** Resource acquisition and cleanup are tied to object lifetime - resources are acquired during construction and automatically released during destruction.

**Why it's good:**

- **Automatic Cleanup**: Resources are always cleaned up, even if exceptions occur
- **Exception Safety**: No resource leaks when exceptions are thrown
- **Deterministic**: Resources are released at predictable times
- **Simplified Code**: No need to remember to call cleanup methods manually
- **Composability**: RAII objects can be composed together safely

**How we apply it:**

```python
class Trace32ApCore:
    def close(self):
        """RAII-style cleanup with protection against multiple calls."""
        logger.debug(f"Trace32ApCore.close() called, _closed={self._closed}")
        if self._closed:
            return  # Already closed, safe to call multiple times

        self._closed = True  # Set immediately to prevent re-entry

        # Only attempt cleanup if we actually initialized resources
        if self._initialized:
            try:
                # Deterministic cleanup of T32 resources
                self.execute_command("sys.d")
                self.execute_command("quit")
                self.quit()
                logger.debug("T32 resources cleaned up successfully")
            except Exception as e:
                logger.warning(f"Exception during cleanup: {e}")
                # Resource is still marked as closed to prevent re-entry

    def __del__(self):
        """Fallback cleanup in destructor (Python's RAII equivalent)."""
        self.close()

class IccMhuTest:
    def close_all_channels(self):
        """RAII-style cleanup for multiple resources."""
        for attr_name in [
            "core_com_channel_scp_die0",
            "core_com_channel_mcp_die0",
            "core_com_channel_scp_die1",
            "core_com_channel_mcp_die1"
        ]:
            channel = getattr(self, attr_name, None)
            if channel is not None and hasattr(channel, 'is_open') and channel.is_open():
                try:
                    channel.close()
                    logger.debug(f"Closed channel: {attr_name}")
                except Exception as e:
                    logger.warning(f"Error closing {attr_name}: {e}")
                    # Continue cleanup of other resources
```

**Python Context Manager Support:**

```python
class IccMhuAp:
    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()  # Automatic cleanup when leaving 'with' block
        return False  # Don't suppress exceptions

# Usage with automatic cleanup
with IccMhuAp() as mhu:
    mhu.send_packet(command, payload)
    packet = mhu.get_packet()
    # mhu.close() is automatically called here, even if exception occurs
```

**Benefits in our context:**

- T32 debugger connections are properly closed even if tests fail
- Serial channels are automatically cleaned up preventing resource leaks
- Memory interfaces release resources deterministically
- Test infrastructure is robust against partial failures during cleanup
- Exception safety ensures no resources are leaked during error conditions

**RAII Best Practices Applied:**

- **Idempotent Cleanup**: `close()` methods can be called multiple times safely
- **Exception Safety**: Cleanup continues even if individual resource cleanup fails
- **Early Marking**: Resources are marked as closed immediately to prevent re-entry
- **Resource Tracking**: State flags track whether resources need cleanup
- **Graceful Degradation**: Partial cleanup failures don't prevent other cleanup

These patterns work together to create a maintainable, testable, and extensible inter-core communication library that properly separates hardware abstraction, protocol implementation, and testing concerns.

## Variants

| Variant | Description |
| --- | --- |
| **IccMhuAp** | AP (Application Processor) implementation with T32 debugger interface and AP-specific memory mappings |
| **MockMemoryInterface** | Test double implementation for unit testing without hardware dependencies, includes scenario configuration |
| **Trace32ApCore** | T32 debugger wrapper with lazy initialization and proper resource management for AP Die 0 |
| **ApCoreMemoryInterface** | Production T32-based memory interface with byte-level access and bulk memory operations |

The library's modular design allows for easy extension to support additional processor cores (SCP, MCP) and communication interfaces (native hardware, simulation) without modifying existing code.
