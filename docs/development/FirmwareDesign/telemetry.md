# Power Telemetry Design Document

## Table of Contents

[[_TOC_]]

## Document Revision Notes

**Latest Update**: Comprehensive revision to reflect actual implementation structure

## Introduction

### Description

This document describes the design for the Power Telemetry Service, which enables comprehensive telemetry data collection within the firmware along with providing In-Band and Out-of-Band paths for that data. The service collects data from both hardware producers (via sensor FIFOs) and firmware producers. Production is controlled by the SCP (System Control Processor). The MCP (Management Control Processor) reads the raw data from the sensor FIFOs, processes it, computes metrics, and sends the data out through multiple delivery mechanisms.

The telemetry service is split into two main implementations:

- **Power Telemetry MCP Service** (`src/services/power_tlm_mcp`): Runs on the MCP and handles the majority of telemetry processing, packaging, and delivery
- **Power Telemetry SCP Service** (`src/services/power_tlm_scp`): Runs on the SCP and provides specific telemetry data (e.g., droop counts) to the MCP upon request

### Terms

| Term   | Description                     |
| ------ | ------------------------------- |
| ATU                   | Address Translation Unit                      |
| CLI                   | Command Line Interface                        |
| ICC                   | Inter Core Communication                      |
| MCP                   | Management Control Processor                  |
| MHU                   | Message Handling Unit                         |
| NSSM                  | Non-Secure Shared Memory                      |
| SCP                   | System Control Processor                      |

### Reference Documents

| Document                                  | Link                                |
| ----------------------------------------- | ----------------------------------- |
| Telemetry Requirements and Specification | [Telemetry Requirements](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Firmware/Telemetry/1PFW_Kingsgate_Power_Telemetry_Requirements_And_Specifications%20V0.4%20WIP.docx?d=wfd425643c61b46e09ad093e10faf5d0b&csf=1&web=1&e=Df1DYl)    |
| Data Collection Protocol Specification  | [DCP Specification](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/_layouts/15/Doc.aspx?sourcedoc=%7B41EF1114-520C-42F3-B134-80AE7C0741E6%7D&file=Data_Collection_Protocol_Specification_v1_0_6.docx&action=default&mobileredirect=true) |
| Message Transfer Service  | [MTS Documentation](https://azurecsi.visualstudio.com/DuvallFw/_git/1pfw.fwlibs?path=/doc/Modules/services/message_transfer.md) |
| Sensor Fifo Service | [Sensor FIFO Design](./SensorFifo.md) |

## Requirements

See the Requirements and Specification document in the References section for more detail.

- Shall collect data from Sensor FIFOs at a 10ms periodic rate
- Shall package Power Telemetry Events at a configurable periodic rate (typically 2 minutes) into DDR
- Shall package Instantaneous Telemetry Events at a configurable periodic rate (typically 5-100ms) into DDR
- Shall package 24-Hour Telemetry Events at a configurable periodic rate (default 24 hours) into DDR
- Shall support Enabling/Disabling of Telemetry Event generation via DCP (Data Collection Protocol)
- Shall support individual record enable/disable for power and instantaneous packages
- Shall provide Out-of-Band telemetry access via PLDM numeric sensors
- Shall support single-die and dual-die system configurations
- The DDR space is freed when a package is read by the host consumer
- Shall prevent DDR memory leaks through multiple safeguard mechanisms

## Dependencies

### Hardware

- **Sensor FIFOs**: Hardware FIFOs that buffer telemetry data from various sources
- **DDR Memory**: 111MB reserved region for telemetry packages (mapped via ATU)
- **ATU (Address Translation Unit)**: Maps MCP address space to physical DDR
- **Global Timer**: For microsecond-resolution timestamps

### Firmware Libraries

- **ThreadX RTOS**: Thread, queue, event flag, timer, and block pool primitives
- **Sensor Fifo Service** (`ms::service::sensor_fifo`): Driver framework interface to hardware sensor FIFOs
- **Message Transfer Service** (`ms::service::mts`): In-band communication with host
- **PLDM Service** (`fwlib.service.pldm`): Out-of-band PLDM sensor infrastructure
- **Power Service** (`power_runconfig`): Provides droop count telemetry (SCP only)
- **Core Exchange** (`ms::lib::pwr_tlm_core_ex`): Cross-core data exchange (SCP ↔ MCP)
- **Die-to-Die Exchange** (`silibs::mesh_d2d_telemetry`): Cross-die data exchange (dual-die systems)

### Firmware Utilities

- **Event Trace** (`ms::lib::event_trace::trace`): Diagnostics and debug tracing
- **Error Codes** (`ms::lib::error_codes`): Standard error handling
- **Linked List** (`FpFwLinkedList`): Package queue management
- **Assert/Utils** (`fwlib.lib.assert`, `fwlib.lib.utils`): Runtime checks and utilities

### Silicon Libraries

- **PVT** (`silibs::pvt`): Process, voltage, temperature sensor access
- **DVFS** (`silibs::dvfs`): Dynamic voltage/frequency scaling data
- **Mesh Telemetry** (`silibs::mesh_d2d_telemetry`): Die mesh counters
- **Semaphore** (`silibs::semaphore`): Hardware semaphore for die-to-die synchronization

### Other Services

- **Core Info** (`ms::lib::core_info`): System configuration (core count, die count)
- **Telemetry Fuses** (`ms::lib::telemetry::fuses`): Fuse-based configuration
- **ARM Intrinsics** (`ms::lib::arm_intrinsic`): Compiler intrinsics for performance

## Design

### Architecture Overview

The telemetry service is implemented using a component-based architecture to facilitate modularity, testability, and maintainability. The MCP service is divided into distinct components, each built as a separate CMake library target to enable decoupling and test mocking.

#### Code Organization

The MCP service follows these design patterns:

- **Component Structure**: Each component is a separate directory with its own CMakeLists.txt
- **Class Pattern**: Each `.c` file within a component represents a logical class
- **Interface Pattern**: Each class has an `_i.h` header file that defines its internal interface
- **Public Headers**: The `inc/` directory in each component contains the public interface
- **Test Access**: Some internal data structures are exposed in the `_i.h` headers to facilitate unit testing

#### Component Diagram

The diagram below shows the high-level component organization within the MCP telemetry service:

:::mermaid
graph TD
    subgraph OuterBlock [Power Telemetry MCP Service]
        ex[Executive Component<br/>exec_tlm_cmpnt]
         subgraph SubBlock [Processing & Delivery]
            dp[Data Processing Component<br/>data_proc_tlm_cmpnt]
            ib[In Band Telemetry Component<br/>in_band_tlm_cmpnt]
            ob[Out of Band Telemetry Component<br/>out_of_band_tlm_cmpnt]
        end
        et[Event Trace Component<br/>event_trace_cmpnt]
    end
    %% Connections between components
    ex <--> dp
    ex <--> ib
    ex <--> ob
    ex --> et
    dp --> et
    ib --> et
    ob --> et

:::

### Executive Component

**Location**: `src/services/power_tlm_mcp/exec_tlm_cmpnt/`

**Class**: `exec_tlm_cmpnt.c` / `exec_tlm_cmpnt_i.h`

#### Responsibilities

The executive component is the orchestrator of the telemetry service. It contains an RTOS thread that dispatches calls to the other components and manages the overall telemetry operating mode.

#### Timer Management

The executive manages 6 timers that drive telemetry processing:

- **Data Aggregation Timer** (default: 10ms period)
  - Fastest sampling timer
  - Calls into Data Processing component to poll sensor FIFOs
  - Active in both DataCollectMode and PublishingMode
  - Data used for both in-band and out-of-band telemetry

- **Instantaneous Sample Timer**
  - Calls Data Processing component to finalize instantaneous data
  - Calls In Band Telemetry component to add sample to package
  - Configurable period (typically 5-100ms)

- **Power Package Timer**
  - Calls Data Processing component to prepare power package data
  - Calls In Band Telemetry component to generate and publish package
  - Configurable period (typically minutes)

- **24 Hour Package Timer**
  - Calls Data Processing component for long-term metrics
  - Calls In Band Telemetry component to generate 24-hour package
  - Configurable period (default: 24 hours, can be overridden for testing)

- **Out-of-Band Timer**
  - Services PLDM requests for out-of-band telemetry

- **One Second Timer**
  - Calls Data Processing component for mesh telemetry updates
  - Provides 1-second granularity for specific metrics

#### Event-Driven Architecture

Timer expirations and external events (MTS messages, PLDM requests) use an event flag pattern:

- Event flags: `DATA_AGGR_TMR_EXPIRED`, `PWR_PKG_TMR_EXPIRED`, `NEW_INBAND_MTS_MESSAGE`, etc.
- Timer callbacks and notification functions set event flags
- The telemetry service thread waits on these flags
- All processing occurs on the telemetry thread context

This design ensures thread safety without requiring internal locks.

#### Operating Modes

The executive component manages telemetry operating modes via `tlm_operating_mode_t`:

- **TLM_OP_MODE_PUBLISHING**: Full telemetry collection, processing, and delivery to host
- **TLM_OP_MODE_COLLECTING_DATA**: Sensor FIFOs are polled, data collected and processed, but not packaged/published
- **TLM_OP_MODE_DISABLED**: All telemetry disabled, no data collection (test mode)
- **TLM_OP_MODE_SENSOR_FIFO_RAW_DATA**: Special debug mode for raw sensor FIFO data collection

NOTE: The Data Aggregation timer is active in both COLLECTING_DATA and PUBLISHING modes.

#### State Diagram

:::mermaid
stateDiagram-v2
    [*] --> CollectingData

    state CollectingData {
        [*] --> CollectActive : entry / Enable Aggr Tmr
        CollectActive --> [*] : exit / None
    }

    state Publishing {
        [*] --> PublishActive : entry / Enable All Tmr's
        PublishActive --> [*] : exit / Dis Pub Tmr's, Free Rsrcs
    }

    state SensorFifoDebug {
        [*] --> DebugActive : entry / Create Dbg Pkg
        DebugActive --> [*] : exit / Cmpl Pkg, Notify Host
    }

    state Disabled {
        [*] --> DisabledActive : entry / Disable All Tmr's
        DisabledActive --> [*] : exit / Enable Aggr Tmr
    }

    CollectingData --> Publishing : Mode Change to Publishing
    CollectingData --> Disabled : Mode Change to Disabled
    CollectingData --> SensorFifoDebug : Mode Change to Debug

    Publishing --> CollectingData : Mode Change to Collecting
    Publishing --> SensorFifoDebug : Mode Change to Debug
    Publishing --> Disabled : Mode Change to Disabled

    Disabled --> CollectingData : Mode Change to Collecting
    Disabled --> SensorFifoDebug : Mode Change to Debug
    Disabled --> Publishing : Mode Change to Publishing

    SensorFifoDebug --> Disabled : Mode Change to Disabled

:::

### Data Processing Component

**Location**: `src/services/power_tlm_mcp/data_proc_tlm_cmpnt/`

#### Overview

The Data Processing Component is responsible for collecting raw telemetry data from sensor FIFOs, processing it, computing statistical metrics, and providing formatted data to both the In-Band and Out-of-Band telemetry components.

#### Sub-Modules (Classes)

This component is organized into 8 specialized modules:

##### 1. Data Sampling (`data_sampling.c` / `data_sampling_i.h`)

**Responsibilities:**

- Polls sensor FIFO data using synchronous driver framework calls
- Parses raw sensor data from multiple FIFO types:
  - Tile temperature sensors
  - Tile voltage sensors
  - Core current sensors
  - P-state/C-state telemetry
  - VR (Voltage Regulator) temperature and current
  - SOC PVT temperature sensors
  - DIMM sensors
  - Die mesh telemetry
- Manages runtime state for cores, tiles, SOC, DIMMs, and MPAMs
- Tracks state transitions (P-state, C-state, throttling)
- Maintains residency timestamps and metrics

**Key Data Structures:**

- `core_runtime_info_t`: Per-core runtime state (voltages, currents, power, temperatures, states)
- `tile_runtime_info_t`: Per-tile temperature tracking
- `soc_runtime_info_t`: SOC-level metrics (VR rails, HNF temps, PC states)
- `dimm_runtime_info_t`: DIMM temperature and power
- `mpam_runtime_info_t`: MPAM (Memory Partitioning and Monitoring) state

##### 2. Compute Metrics (`compute_metrics.c` / `compute_metrics_i.h`)

**Responsibilities:**

- Computes statistical metrics (min/max/average) from sampled data
- Implements moving averages for smoothing certain metrics
- Manages three time-based metric collections:
  - **2-minute metrics** (`computed_metrics_2_min_t`): Standard power telemetry
  - **24-hour metrics** (`computed_metrics_24_hrs_t`): Long-term histograms and aging counters
  - **Die-to-die metrics** (`computed_metrics_d2d_2_min_t`): Metrics exchanged between dies
  - **Out-of-band metrics** (`computed_metrics_oob_t`): PLDM sensor data with moving averages

**Metrics Computed:**

- Per-core: P-state/C-state residencies, throttle durations, voltage/current/power/temperature statistics
- Per-DIMM: Temperature and power statistics, throttle tracking
- Per-VR rail: Current, voltage, temperature statistics
- SOC-level: Maximum temperatures, HNF temperatures, mesh telemetry
- MPAM: Core power, memory power, throttle residencies
- Histograms: Voltage vs temperature bins (24-hour package)

##### 3. Data Utilities (`data_utilities.c` / `data_utilities_i.h`)

**Responsibilities:**

- Provides math helper functions for telemetry data processing
- Moving average calculations
- Min/max/average computations
- Unit conversions

##### 4. Aging Counters (`aging_counters.c` / `aging_counters_i.h`)

**Responsibilities:**

- Manages aging counter pair selection and rotation
- Reads aged/unaged counter values from hardware
- Tracks counter validity and timestamps
- Provides data for long-term reliability metrics (24-hour package)

##### 5. Die-to-Die Exchange (`die_2_die_exchange.c` / `die_2_die_exchange_i.h`)

**Responsibilities:**

- Manages data exchange between primary and secondary dies in dual-die systems
- Coordinates preparation of cross-die metrics (e.g., MPAM memory power)
- Handles die ID management and synchronization

##### 6. In-Band Package Interface (`in_band_package_interface.c` / `in_band_package_interface_i.h`)

**Responsibilities:**

- Provides API for In-Band Telemetry component to retrieve packaged data
- Implements all the `data_proc_tlm_cmpnt_get_pwr_*` and `data_proc_tlm_cmpnt_get_inst_*` functions
- Formats computed metrics into package-ready structures
- Handles die-specific ID offsets for multi-die systems

##### 7. Out-of-Band Interface (`out_of_band_interface.c`)

**Responsibilities:**

- Provides API for Out-of-Band Telemetry component
- Implements critical sensor value retrievals for PLDM
- Returns values like max SOC temp, SOC power, DIMM temperatures, VR temps, average frequency

##### 8. Data Processing Main (`data_proc_tlm_cmpnt.c`)

**Responsibilities:**

- Component initialization and coordination
- Entry point for `data_proc_tlm_cmpnt_process_input_data()` - main polling loop
- Mode transition handling
- Coordinates data preparation for package generation

#### Data Flow

    Sensor FIFOs → Data Sampling → Compute Metrics → Package Interfaces → In-Band/Out-of-Band Components

#### Class Diagram

:::mermaid
classDiagram
    class DataProcTlmCmpnt {
        +init()
        +process_input_data()
        +prepare_data_for_pwr_pkg()
        +finalize_data_for_pwr_pkg()
        +tlm_mode_enter_actions()
    }

    class DataSampling {
        -core_runtime_info_t core_rt[]
        -soc_runtime_info_t soc_rt
        -dimm_runtime_info_t dimm_rt
        +process_tile_temperature_sensor_fifo()
        +process_pstate_sensor_fifo()
        +process_core_current_sensor_fifo()
        +process_dimm_sensor_fifo()
        +finalize_pwr_pkg_metrics()
    }

    class ComputeMetrics {
        -computed_metrics_2_min_t metrics_2min
        -computed_metrics_24_hrs_t metrics_24hrs
        -computed_metrics_oob_t metrics_oob
        +comp_metrics_for_single_core_pstate()
        +comp_metrics_for_single_core_cstate()
        +comp_metrics_for_single_core_throttling()
        +reset_local_2_min_metrics()
    }

    class InBandPackageInterface {
        +get_pwr_core_pstate_data()
        +get_pwr_core_cstate_data()
        +get_pwr_soc_vr_rail_data()
        +get_inst_soc_core_summary_data()
    }

    class OutOfBandInterface {
        +get_oob_crit_max_soc_temp_dC()
        +get_oob_soc_pwr_mW()
        +get_oob_dimm_avg_temp_dC()
    }

    class AgingCounters {
        +init()
        +read_aging_counters()
        +select_next_counter_pair()
    }

    class Die2DieExchange {
        +init()
        +copy_data_to_exchange()
        +get_this_die_id()
    }

    class DataUtilities {
        +moving_avg_update()
        +mma_update()
    }

    DataProcTlmCmpnt --> DataSampling
    DataProcTlmCmpnt --> ComputeMetrics
    DataProcTlmCmpnt --> AgingCounters
    DataProcTlmCmpnt --> Die2DieExchange
    DataSampling --> ComputeMetrics : updates
    ComputeMetrics --> DataUtilities : uses
    ComputeMetrics --> InBandPackageInterface : provides data
    ComputeMetrics --> OutOfBandInterface : provides data

:::

### In Band Telemetry Component

**Location**: `src/services/power_tlm_mcp/in_band_tlm_cmpnt/`

#### Overview

The In-Band Telemetry Component is responsible for packaging telemetry data and delivering it to the host through the Message Transfer Service (MTS) over DDR memory.

#### Sub-Modules (Classes)

##### 1. Package Creation (`package_creation.c` / `package_creation_i.h`)

**Responsibilities:**

- Creates telemetry packages by assembling data from the Data Processing Component
- Implements record enable/disable functionality per element type
- Generates three types of packages:
  - **Power Packages**: Comprehensive 2-minute aggregated telemetry
  - **Instantaneous Packages**: High-frequency snapshot telemetry (5-100ms)
  - **24-Hour Packages**: Long-term metrics (histograms, aging counters)

**Package Records Created:**

Power Package Records:

- Core: P-state, C-state, Throttle, Rack Priorities, Voltage, Current, Temperature, Power, Droop Count
- SOC: VR Rails, HNF, DIMM Temperature/Power, Sensor Temperature, Max SOC Temp, Die Mesh, D2D Link
- MPAM: Core Power, Memory Power, Throttle
- Memory: Throttle

24-Hour Package Records:

- Core: Histogram, Aging Counters
- SOC: Package Monitor (PC states)

Instantaneous Package Records:

- Core: Summary (pstate, cstate, voltage, current, temperature, power)
- SOC: Rails, DIMM Runtime, Die Temperature, Max Temperature

**Key Features:**

- Variable-length packages based on enabled records
- Zero-data filtering to reduce package size
- Die ID offsets for multi-die systems
- Record numbering and sequencing

##### 2. DDR Manager (`ddr_manager.c` / `ddr_manager_i.h`)

**Responsibilities:**

- Manages DDR memory allocation for telemetry packages
- Implements block-based memory pool management (no fragmentation)
- Uses ThreadX queues for free block tracking
- Maps ATU (Address Translation Unit) addresses to physical DDR

**Memory Organization:**

The 111MB DDR reserved for telemetry is divided into two pools:

- **Power Pool**: For power and 24-hour packages
  - Block size: ~256KB aligned (accommodates max power package size)
  - Number of blocks: 3 (sufficient for 2-minute generation rate)
  - Location: Start of telemetry DDR region

- **Instantaneous Pool**: For instantaneous packages
  - Block size: ~4KB aligned (accommodates max inst package with multiple samples)
  - Number of blocks: Calculated from remaining DDR space
  - Location: After power pool + 4KB separation

**Memory Allocation Pattern:**

- Allocate: Dequeue block address from free queue
- Deallocate: Queue block address back to free queue
- No fragmentation due to fixed block sizes
- Memory leak prevention through multiple safeguards

**Leak Prevention Mechanisms:**

- Package creation failure → immediate deallocation
- MTS queue failure → immediate deallocation
- Host read timeout → controller frees block after notification
- Overproduction → oldest pending block freed, new block queued
- All cases generate event traces for diagnostics

##### 3. MTS Manager (`mts_manager.c` / `mts_manager_i.h`)

**Responsibilities:**

- Manages all interactions with the Message Transfer Service (host proxy)
- Routes incoming TRP (Transfer Relay Protocol) and DCP (Data Collection Protocol) messages
- Maintains pending package queue
- Handles multi-die telemetry coordination

**Message Handling:**

- `TRP_MSG_ID_PACKAGE_NOTIFICATION`: Notifies host of new package
- `TRP_MSG_ID_READ_PACKAGE`: Host requests package data
- `TRP_MSG_ID_READ_PACKAGE_COMPLETE`: Host confirms package read
- `TRP_MSG_ID_DCP_FORWARD`: Data Collection Protocol commands (mode changes, record enable/disable)
- `TRP_MSG_ID_CLIENT_DEFINED`: Custom telemetry commands (e.g., prepare power package)

**Multi-Die Handling:**

- Primary die (Die 0): Aggregates packages from all dies, manages MTS client
- Secondary die (Die 1): Forwards packages to primary via TRP messages
- Uses linked lists for package tracking (`pkg_free_list`, `pkg_active_list`)

**Package Lifecycle:**

1. Package created in DDR by Package Creation
2. Queued to MTS Manager → Added to `pkg_active_list`
3. MTS Manager sends `CLIENT_NOTIFICATION` to MTS service
4. Host sends `CLIENT_READ_DATA` → MTS Manager responds with package info
5. Host sends `CLIENT_READ_DATA_COMPLETE` → MTS Manager frees DDR block

##### 4. Element Schema (`element_schema.c` / `element_schema.h`)

**Responsibilities:**

- Defines schema information for all telemetry elements
- Provides element metadata (IDs, sizes, types)
- Used by package creation for record formatting

##### 5. Sensor FIFO Debug (`snsr_fifo_debug.c` / `snsr_fifo_debug_i.h`)

**Responsibilities:**

- Special debug mode for collecting raw sensor FIFO data
- Bypasses normal processing to capture FIFO contents directly
- Packages raw data for host analysis
- Activated via `TLM_OP_MODE_SENSOR_FIFO_RAW_DATA`

##### 6. In-Band Component Main (`in_band_tlm_cmpnt.c` / `in_band_tlm_cmpnt_i.h`)

**Responsibilities:**

- Component initialization
- Entry points for package generation (`generate_pwr_pkg()`, `add_inst_sample()`, `generate_24hr_pkg()`)
- Mode transition handling
- MTS message dispatching
- Multi-die coordination (notify secondary MCPs and SCP)

#### Class Diagram

:::mermaid
classDiagram
    class InBandTlmCmpnt {
        +init()
        +generate_pwr_pkg()
        +add_inst_sample()
        +generate_24hr_pkg()
        +handle_incoming_mts_msgs()
        +notify_sec_mcps_prepare_pwr_pkg()
    }

    class PackageCreation {
        -power_pkg_element_enable[]
        -inst_pkg_element_enable[]
        +create_power_pkg()
        +append_to_inst_pkg()
        +create_24hr_pkg()
        +enable_disable_pwr_record()
        +create_pwr_core_pstate_record()
    }

    class DdrManager {
        -pwr_pkg_free_queue
        -inst_pkg_free_queue
        +init()
        +allocate_mem_for_pwr_pkg()
        +allocate_mem_for_inst_pkg()
        +deallocate_mem()
    }

    class MtsManager {
        -pkg_free_list
        -pkg_active_list
        -in_flight_tlm_pkg
        +init()
        +queue_tlm_package()
        +handle_dcp_msg()
        +handle_trp_msg()
        +handle_read_msg()
        +send_trp_pkg_notification()
    }

    class SnsrFifoDebug {
        +begin_sensor_fifo_dbg_collection()
        +end_sensor_fifo_dbg_collection()
    }

    InBandTlmCmpnt --> PackageCreation
    InBandTlmCmpnt --> DdrManager
    InBandTlmCmpnt --> MtsManager
    InBandTlmCmpnt --> SnsrFifoDebug
    PackageCreation --> DdrManager : allocates memory
    MtsManager --> DdrManager : frees memory

:::

#### Telemetry Power Package Generation Sequence

:::mermaid
sequenceDiagram
participant Exec as Executive Cmpnt
participant Dataproc as Data Processing Cmpnt
participant InBand as In Band Tlm Cmpnt
participant PkgCr as Package Creation
participant DdrMgr as DDR Manager
participant MtsMgr as MTS Manager
participant MTS as Message Transfer Service
Exec->>Exec: Power Package Timer Expired
Exec->>Dataproc: prepare_data_for_pwr_pkg()
Note over Dataproc: Notify secondary dies & SCP<br/>Update aging counters
Exec->>Dataproc: finalize_data_for_pwr_pkg()
Note over Dataproc: Process remaining FIFO entries<br/>Update residencies
Exec->>InBand: generate_pwr_pkg()
InBand->>DdrMgr: allocate_mem_for_pwr_pkg()
DdrMgr-->>InBand: DDR address & size
InBand->>PkgCr: create_power_pkg(ddr_addr, size)
loop All Power Records
    alt Power Element Enabled
        PkgCr->>Dataproc: get_pwr_*_data()
        Dataproc-->>PkgCr: Element Data
        PkgCr->>PkgCr: Add record to package
    else Element Disabled
        PkgCr->>PkgCr: Skip record
    end
end
PkgCr-->>InBand: Package size
InBand->>MtsMgr: queue_tlm_package(addr, size)
MtsMgr->>MtsMgr: Add to pkg_active_list
MtsMgr->>MTS: CLIENT_NOTIFICATION
MTS->>MtsMgr: CLIENT_READ_DATA
MtsMgr->>MtsMgr: Remove from pkg_active_list
MtsMgr-->>MTS: READ_DATA response (addr, size)
Note over MTS: Host reads package from DDR
MTS->>MtsMgr: CLIENT_READ_DATA_COMPLETE
MtsMgr->>DdrMgr: deallocate_mem(addr)
DdrMgr->>DdrMgr: Return block to free queue

:::



### Out of Band Telemetry Component

**Location**: `src/services/power_tlm_mcp/out_of_band_tlm_cmpnt/`

**Class**: `out_of_band_tlm_cmpnt.c` / `out_of_band_tlm_cmpnt_i.h`

#### Responsibilities

The Out-of-Band Telemetry Component provides telemetry data through the PLDM (Platform Level Data Model) interface for out-of-band management access.

#### PLDM Sensor Management

- Registers numeric sensors with the PLDM service
- Implements sensor get handlers for critical telemetry values
- Uses sensor value caching to reduce compute overhead
- Cache validity: 100ms (prevents excessive recomputation for multiple sensor reads)

#### Supported PLDM Sensors

1. **Max SOC Temperature** (deci-Celsius)
   - Maximum temperature across all SOC sensors and tiles
2. **SOC Power** (milliwatts)
   - Total SOC power consumption
3. **Max DIMM Temperature** (deci-Celsius)
   - Maximum temperature across all DIMMs
4. **Total DIMM Power** (milliwatts)
   - Aggregate power for all DIMMs
5. **Max VR Temperature** (deci-Celsius)
   - Maximum voltage regulator temperature
6. **Average SOC Frequency** (MHz)
   - Weighted average frequency across cores
7. **Per-DIMM Average Temperature** (deci-Celsius)
   - Individual DIMM channel temperatures
8. **Per-DIMM Maximum Temperature** (deci-Celsius)
9. **Per-DIMM Average Power** (milliwatts)

#### Sensor Value Retrieval

Each sensor has a dedicated handler function (`pwr_tlm_oob_get_*`) that:

1. Checks cache validity
2. If cache invalid, queries Data Processing Component via Out-of-Band Interface
3. Updates cache with moving average values from `computed_metrics_oob`
4. Returns value in PLDM composite format

#### Event-Driven Operation

- PLDM requests trigger `NEW_OUT_OF_BAND_PLDM_REQ` event flag
- Executive component dispatches to `out_of_band_tlm_cmpnt_handle_new_pldm_requests()`
- Processes all pending PLDM sensor reads
- Non-critical sensors logged periodically (every 10th request) to reduce trace volume

#### Class Diagram

:::mermaid
classDiagram
    class OutOfBandTlmCmpnt {
        -pldm_numeric_sensor_context_t pwr_tlm_numeric_sensor_ctxts[]
        -pwr_tlm_numeric_sensor_get_handler_t pwr_tlm_numeric_sensor_get_handlers[]
        -uint32_t pwr_tlm_oob_sensor_cache[]
        -bool cache_is_valid
        -uint64_t cache_timestamp_uS
        -pldm_numeric_sensor_context_t* active_sensor
        -pwr_tlm_numeric_sensor_get_handler_t active_handler
        +init()
        +handle_new_pldm_requests()
        +print_sensors()
        -on_pwr_tlm_numeric_sensor_get_ext_entry()
    }

    class SensorHandlers {
        <<handlers>>
        +pwr_tlm_oob_get_max_soc_temp()
        +pwr_tlm_oob_get_soc_pwr()
        +pwr_tlm_oob_get_max_dimm_temp()
        +pwr_tlm_oob_get_dimm_total_pwr()
        +pwr_tlm_oob_get_max_vr_temp()
        +pwr_tlm_oob_get_soc_avg_freq()
        +pwr_tlm_oob_get_dimm_avg_temp()
        +pwr_tlm_oob_get_dimm_max_temp()
        +pwr_tlm_oob_get_dimm_avg_pwr()
    }

    class DataProcOOBInterface {
        +get_oob_crit_max_soc_temp_dC()
        +get_oob_soc_pwr_mW()
        +get_oob_dimm_total_pwr_mW()
        +get_oob_crit_max_dimm_temp_dC()
        +get_oob_crit_max_vr_temp_dC()
        +get_oob_soc_avg_freq_MHz()
        +get_oob_dimm_avg_temp_dC()
        +get_oob_dimm_max_temp_dC()
        +get_oob_dimm_avg_pwr_mW()
    }

    class PLDMService {
        <<external>>
        +pldm_register_numeric_sensor()
        +fpfw_pldm_service_release()
    }

    OutOfBandTlmCmpnt --> SensorHandlers : uses
    SensorHandlers --> DataProcOOBInterface : queries
    OutOfBandTlmCmpnt --> PLDMService : registers with
    OutOfBandTlmCmpnt --> DataProcOOBInterface : calls

:::

### Power Telemetry SCP Service

**Location**: `src/services/power_tlm_scp/`

**Class**: `pwr_tlm_service_scp.c` / `pwr_tlm_service_scp_i.h`

#### Responsibilities

The SCP telemetry service runs on the System Control Processor and provides specific telemetry data to the MCP upon request. It acts as a data provider rather than a full telemetry processing system.

#### Functionality

- **MTS Client Registration**: Registers with Message Transfer Service as `MTS_CLIENT_ID_PWR_INST_TELEM`
- **Droop Count Collection**: Primary function is to collect and provide droop count telemetry
  - Receives `TLM_CLIENT_CMD_GEN_PWR_PACKAGE_MCP_2_SCP_PUSH` command from MCP
  - Calls `power_get_adclk_telem()` to retrieve droop counts from power service
  - Writes droop counts to SCP telemetry exchange via `pwr_tlm_core_exch_scp_write_droop_counts()`
  - MCP reads from exchange when generating power packages

#### Architecture

    MCP Telemetry → TRP Message → SCP MTS Manager → Power Service → Core Exchange → MCP Reads

#### Event Trace

Provides debug event traces for:

- Incoming TRP messages
- Unexpected DCP messages
- Command processing
- MTS queue operations

### Event Trace Component

**Location**: `src/services/power_tlm_mcp/event_trace_cmpnt/`

**Class**: `telemetry_events.c`

#### Responsibilities

- Defines all telemetry-specific event trace providers and events
- Used throughout all telemetry components for diagnostics
- Separate component to avoid circular dependencies

#### Event Categories

- Telemetry service initialization and mode changes
- Package creation and delivery
- MTS message handling
- DDR memory allocation/deallocation
- PLDM sensor requests
- Error conditions and warnings

## Public API

### MCP Telemetry Service

**Header**: `src/services/power_tlm_mcp/inc/telemetry_service.h`

#### Initialization

```c
void telemetry_service_init(uint8_t die_id,
                            uint32_t pwr_pkg_period_ms,
                            uint32_t inst_pkg_sample_period_ms,
                            uint16_t inst_samples_per_pkg,
                            uint32_t _24_hr_pkg_sample_period_ms,
                            bool is_single_die_system);
```

Initializes the telemetry service with specified timing parameters and die configuration.

**Parameters:**

- `die_id`: The die identifier (0 for primary, 1 for secondary in dual-die systems)
- `pwr_pkg_period_ms`: Power package generation period in milliseconds (typically ~120,000ms / 2 minutes)
- `inst_pkg_sample_period_ms`: Instantaneous sample period in milliseconds (5-100ms)
- `inst_samples_per_pkg`: Number of instantaneous samples per package (1-20)
- `_24_hr_pkg_sample_period_ms`: 24-hour package period in milliseconds (can be overridden for testing)
- `is_single_die_system`: `true` for single-die, `false` for dual-die systems

**Initialization Sequence:**

1. Executive Component initialization (creates thread, timers, event flags)
2. Data Processing Component initialization (all sub-modules)
3. In-Band Telemetry Component initialization (DDR manager, MTS manager, package creation)
4. Out-of-Band Telemetry Component initialization (PLDM sensor registration)

**Default State**: After initialization, service starts in `TLM_OP_MODE_COLLECTING_DATA` mode

### SCP Telemetry Service

**Header**: `src/services/power_tlm_scp/inc/pwr_tlm_service_scp.h`

#### Initialization

```c
void pwr_tlm_svc_scp_init(void);
```

Initializes the SCP telemetry service. Registers MTS client and prepares for droop count requests.

### Component APIs

The following APIs are available for internal component communication (not typically called by application code):

#### Executive Component

**Header**: `src/services/power_tlm_mcp/exec_tlm_cmpnt/inc/exec_tlm_cmpnt.h`

```c
void exec_tlm_cmpnt_change_telemetry_mode(tlm_operating_mode_t new_mode);
void exec_tlm_cmpnt_set_mode_change_pending(tlm_operating_mode_t pending_mode);
bool exec_tlm_cmpnt_is_telemetry_publishing_enabled(void);
bool exec_tlm_cmpnt_is_oob_data_valid(void);
void exec_tlm_cmpnt_get_status(telemetry_executive_status_t *status);
void exec_tlm_cmpnt_notify_new_in_band_mts_message(void);
void exec_tlm_cmpnt_notify_new_out_of_band_pldm_request(void);
uint64_t exec_tlm_cmpnt_get_timestamp_microseconds(void);
void exec_tlm_cmpnt_set_oob_log_enable(bool enable);
```

#### Data Processing Component

**Header**: `src/services/power_tlm_mcp/data_proc_tlm_cmpnt/inc/data_proc_tlm_cmpnt.h`

Provides extensive API for retrieving processed telemetry data. Key functions include:

- `data_proc_tlm_cmpnt_process_input_data()`: Main sensor FIFO polling entry point
- `data_proc_tlm_cmpnt_prepare_data_for_pwr_pkg()`: Pre-package preparation
- `data_proc_tlm_cmpnt_finalize_data_for_pwr_pkg()`: Final metrics update before packaging
- `data_proc_tlm_cmpnt_get_pwr_core_*()`: Get power telemetry core data (pstate, cstate, throttle, voltage, etc.)
- `data_proc_tlm_cmpnt_get_pwr_soc_*()`: Get power telemetry SOC data (VR rails, DIMMs, temperatures, etc.)
- `data_proc_tlm_cmpnt_get_inst_soc_*()`: Get instantaneous telemetry data
- `data_proc_tlm_cmpnt_get_oob_*()`: Get out-of-band sensor values

#### In-Band Telemetry Component

**Header**: `src/services/power_tlm_mcp/in_band_tlm_cmpnt/inc/in_band_tlm_cmpnt.h`

```c
void in_band_tlm_cmpnt_generate_pwr_pkg(void);
void in_band_tlm_cmpnt_add_inst_sample(void);
void in_band_tlm_cmpnt_generate_24hr_pkg(void);
void in_band_tlm_cmpnt_handle_incoming_mts_msgs(void);
bool in_band_tlm_cmpnt_is_any_instantaneous_enabled(void);
bool in_band_tlm_cmpnt_is_power_record_enabled(pwr_telemetry_element_id_t element_id);
bool in_band_tlm_cmpnt_is_inst_record_enabled(instantaneous_telemetry_element_id_t element_id);
```

#### Out-of-Band Telemetry Component

**Header**: `src/services/power_tlm_mcp/out_of_band_tlm_cmpnt/inc/out_of_band_tlm_cmpnt.h`

```c
void out_of_band_tlm_cmpnt_handle_new_pldm_requests(void);
void out_of_band_tlm_cmpnt_print_sensors(void);
```

## Usage

### Typical Startup Sequence

The following sequence diagram shows the initialization flow when the telemetry service starts:

:::mermaid
sequenceDiagram
participant App as Application
participant TlmSvc as Telemetry Service
participant ExecCmpnt as Executive Component
participant DataProcCmpnt as Data Processing Component
participant InBandCmpnt as In-Band Component
participant OOBCmpnt as Out-of-Band Component
participant DdrMgr as DDR Manager
participant MtsMgr as MTS Manager
participant PLDMSvc as PLDM Service

App->>TlmSvc: telemetry_service_init(...)

TlmSvc->>ExecCmpnt: exec_tlm_cmpnt_init()
ExecCmpnt->>ExecCmpnt: Create telemetry thread
ExecCmpnt->>ExecCmpnt: Create event flags
ExecCmpnt->>ExecCmpnt: Create 6 timers
ExecCmpnt->>ExecCmpnt: Set mode = COLLECTING_DATA
ExecCmpnt->>ExecCmpnt: Start Data Aggregation Timer (10ms)

TlmSvc->>DataProcCmpnt: data_proc_tlm_cmpnt_init(die_id, is_single_die)
DataProcCmpnt->>DataProcCmpnt: Initialize all sub-modules
Note over DataProcCmpnt: die_2_die_exch, data_sampling,<br/>compute_metrics, package_interface

TlmSvc->>InBandCmpnt: in_band_tlm_cmpnt_init(die_id, inst_samples_per_pkg)
InBandCmpnt->>DdrMgr: ddr_manager_init()
DdrMgr->>DdrMgr: Create memory pools
Note over DdrMgr: Power & Inst pools with free queues
InBandCmpnt->>MtsMgr: mts_manager_init()
alt Die 0 (Primary)
    MtsMgr->>MtsMgr: Register MTS client
else Die 1 (Secondary)
    Note over MtsMgr: Forward to primary
end
InBandCmpnt->>InBandCmpnt: package_creation_init()

TlmSvc->>OOBCmpnt: out_of_band_tlm_cmpnt_init(die_id)
OOBCmpnt->>PLDMSvc: Register 9 numeric sensors
OOBCmpnt->>OOBCmpnt: Initialize cache

TlmSvc-->>App: Initialization complete
Note over ExecCmpnt: Service in COLLECTING_DATA mode<br/>Data Aggregation Timer running

:::

#### Detailed Steps

1. **Application Initialization**
   - Application calls `telemetry_service_init()` with system configuration
   - Provides die ID, timer periods, and die count

2. **Executive Component Initialization**
   - Creates telemetry RTOS thread
   - Creates event flag group for timer and message notifications
   - Creates all 6 timers (not yet started except Data Aggregation)
   - Sets initial mode to `TLM_OP_MODE_COLLECTING_DATA`
   - Starts Data Aggregation Timer (10ms periodic)
   - Thread begins waiting on event flags

3. **Data Processing Component Initialization**
   - Initializes die-to-die exchange (sets die ID)
   - Initializes data sampling (runtime state structures)
   - Initializes compute metrics (2-min, 24-hr, OOB metric storage)
   - Initializes package interfaces

4. **In-Band Component Initialization**
   - DDR Manager: Creates free queues and initializes all memory blocks as free
   - MTS Manager: Initializes package lists, registers MTS client (primary die only)
   - Package Creation: All telemetry records enabled by default

5. **Out-of-Band Component Initialization**
   - Registers 9 PLDM numeric sensors with handler functions
   - Initializes sensor value cache as invalid

6. **Collecting Data Mode Operation**
   - Data Aggregation Timer fires every 10ms
   - Sensor FIFOs polled, data processed and metrics updated
   - No packages created or published yet

### Runtime Operation: Publishing Mode

The following sequence diagram shows the typical runtime operation after transitioning to publishing mode:

:::mermaid
sequenceDiagram
participant Host as Host
participant MtsMgr as MTS Manager
participant ExecCmpnt as Executive Component
participant DataProcCmpnt as Data Processing Component
participant InBandCmpnt as In-Band Component
participant SnsrFifo as Sensor FIFO Service

Note over Host,SnsrFifo: Host sends DCP command to enable publishing

Host->>MtsMgr: DCP: SET_MODE(PUBLISHING)
MtsMgr->>ExecCmpnt: set_mode_change_pending(PUBLISHING)
ExecCmpnt->>ExecCmpnt: Process mode change
ExecCmpnt->>DataProcCmpnt: tlm_mode_enter_actions(PUBLISHING)
DataProcCmpnt->>DataProcCmpnt: Reset timestamps & metrics
DataProcCmpnt->>DataProcCmpnt: Init aging counters & mesh telemetry
ExecCmpnt->>ExecCmpnt: Start all timers

Note over ExecCmpnt,SnsrFifo: Telemetry now in PUBLISHING mode

loop Every 10ms - Data Aggregation
    ExecCmpnt->>ExecCmpnt: Data Aggr Timer fires
    ExecCmpnt->>DataProcCmpnt: process_input_data()
    DataProcCmpnt->>SnsrFifo: Poll sensor FIFOs
    SnsrFifo-->>DataProcCmpnt: Raw telemetry data
    DataProcCmpnt->>DataProcCmpnt: Parse & update runtime state
    DataProcCmpnt->>DataProcCmpnt: Compute metrics
end

loop Every 5-100ms - Instantaneous Sample
    ExecCmpnt->>ExecCmpnt: Inst Sample Timer fires
    ExecCmpnt->>DataProcCmpnt: prepare_data_for_inst_sample()
    ExecCmpnt->>InBandCmpnt: add_inst_sample()
    Note over InBandCmpnt: Append sample to package
    alt Package Full
        InBandCmpnt->>InBandCmpnt: Finalize & queue package
        InBandCmpnt->>MtsMgr: Notify host
    end
end

loop Every ~2 minutes - Power Package
    ExecCmpnt->>ExecCmpnt: Power Pkg Timer fires (T-100ms)
    ExecCmpnt->>DataProcCmpnt: prepare_data_for_pwr_pkg()
    Note over DataProcCmpnt: Notify secondary dies & SCP<br/>Update aging counters

    ExecCmpnt->>ExecCmpnt: Timer fires (T=0)
    ExecCmpnt->>DataProcCmpnt: finalize_data_for_pwr_pkg()
    Note over DataProcCmpnt: Process remaining FIFOs<br/>Finalize residencies
    ExecCmpnt->>InBandCmpnt: generate_pwr_pkg()
    Note over InBandCmpnt: Package created in DDR<br/>(see detailed sequence)
    InBandCmpnt->>MtsMgr: Queue package
    MtsMgr->>Host: CLIENT_NOTIFICATION
    Host->>MtsMgr: CLIENT_READ_DATA
    MtsMgr-->>Host: Package info
    Note over Host: Read from DDR
    Host->>MtsMgr: CLIENT_READ_DATA_COMPLETE
    MtsMgr->>MtsMgr: Free DDR block
end

loop Every 1 second - Mesh Telemetry
    ExecCmpnt->>ExecCmpnt: One Second Timer fires
    ExecCmpnt->>DataProcCmpnt: process_one_second_input_data()
    DataProcCmpnt->>DataProcCmpnt: Update mesh counter metrics
end

loop Every 24 hours - Long-term Metrics
    ExecCmpnt->>ExecCmpnt: 24hr Package Timer fires
    ExecCmpnt->>DataProcCmpnt: prepare_data_for_24hr_pkg()
    ExecCmpnt->>InBandCmpnt: generate_24hr_pkg()
    Note over InBandCmpnt: Package histograms & aging counters
    InBandCmpnt->>MtsMgr: Queue package
    MtsMgr->>Host: CLIENT_NOTIFICATION
    Host->>MtsMgr: CLIENT_READ_DATA
    MtsMgr-->>Host: Package info
    Note over Host: Read from DDR
    Host->>MtsMgr: CLIENT_READ_DATA_COMPLETE
    MtsMgr->>MtsMgr: Free DDR block
    DataProcCmpnt->>DataProcCmpnt: Reset 24hr metrics
end

:::

### Multi-Die Coordination

In dual-die systems:

1. Primary die (Die 0) runs full telemetry stack with MTS client
2. Secondary die (Die 1) runs telemetry service and forwards packages to primary
3. Before power package generation, primary notifies secondary via TRP message
4. Secondary prepares its data and copies to die-to-die exchange
5. Primary reads exchange and includes secondary die data in package
6. Primary delivers combined package to host

### SCP Integration

1. SCP service registers with MTS as passive data provider
2. Before power package generation, MCP sends TRP message to SCP
3. SCP retrieves droop counts from power service
4. SCP writes droop counts to core exchange
5. MCP reads droop counts from exchange and includes in package
