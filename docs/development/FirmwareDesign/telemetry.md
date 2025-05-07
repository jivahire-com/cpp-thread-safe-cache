# Power Telemetry Design Document

## Table of Contents

[[_TOC_]]

## Introduction

### Description

This document describes the design for the Telemetry Service, which enables data collection within the firmware along
with providing In and Out of Band paths for that data. There are HW data producers and FW data producers which store the
data in shared sensor fifos. Production is controlled by the SCP.  The MCP reads the raw data from the sensor fifo's,
processes it and then sends the data out.

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
| Telemetry Requirements and Specification | [Link](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Firmware/Telemetry/1PFW_Kingsgate_Power_Telemetry_Requirements_And_Specifications%20V0.4%20WIP.docx?d=wfd425643c61b46e09ad093e10faf5d0b&csf=1&web=1&e=Df1DYl)    |
| Data Collection Protocol Specification  | [Link](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/_layouts/15/Doc.aspx?sourcedoc=%7B41EF1114-520C-42F3-B134-80AE7C0741E6%7D&file=Data_Collection_Protocol_Specification_v1_0_6.docx&action=default&mobileredirect=true) |
| Message Transfer Service  | [Link](https://azurecsi.visualstudio.com/DuvallFw/_git/1pfw.fwlibs?path=/doc/Modules/services/message_transfer.md) |
| Sensor Fifo Service | [Link](./SensorFifo.md) |

## Requirements

See the Requirements and Specification document in the References section for more detail.

- Shall collect data from the Sensor Fifo's at a specified periodic rate.
- Shall package Telemetry Events at a periodic rate into DDR. The space in DDR is freed when a package is read by the consumer.
- Shall support Enabling / Disabling of Telemetry Event generation into DDR via ICC.

## Dependencies

- Sensor Fifo
- Mesh Manager
- ATU
- ICC
- MHU
- CLI

## Design

The functionality encompassed by the telemetry service is large enough that it has been further partitioned to facilitate ease of design and maintenance.

Summary descriptions for the components are provided below.  Each component has a minimal interface provided for internal access by the other components. (Not shown in diagram below)

:::mermaid
graph TD
    subgraph OuterBlock [Telemetry Service]
        ex[Executive Component]
         subgraph SubBlock [ ]
            dp[Data Processing Component]
            ib[In Band Telemetry Component]
            ob[Out of Band Telemetry Component]
        end
    end
    %% Connections between components
    ex <--> dp
    ex <--> ib
    ex <--> ob

:::

### Executive Component

The executive primarily contains an RTOS thread and dispatches calls to the other components. The executive has 4 timers that initiate the majority of the processing.

- Power Data Aggregation Timer
  - Expiration calls into the Data Processing component
  - Fastest sampling time
  - Data is used for in-band power and instantaneous packages.
  - Data is also used for out of band packages.

- Instantaneous Package Timer
  - Expiration calls into the Data Processing component to retrieve data not obtained from power timer handler.
  - Expiration calls into the In Band Telemetry component

- Power Telemetry Package Timer
  - Expiration calls into the In Band Telemetry component

- 24 Hour Timer
  - Expiration calls into the Data Processing component

The timer expiration callbacks just wake up the telemetry service thread and all processing is performed there.

Any other signaling from external entry points, ISR's, communication stack callbacks, driver framework callbacks etc,  will follow the same pattern, store context data (if any), and wake up the thread to complete the processing.

Using asynchronous management of the external interfaces ensures all telemetry processing is thread safe.  No internal locks are required.

The executive component controls high level functionality of the service via tlm_operating_mode_t.  A high level
state diagram shows the main functionality of the different modes.

NOTE: In DataCollectMode, raw data is collected and processed, but the data is not packaged for the host. In publishing mode, data is also collected and published to the host.  ie the Data Aggregation timer is active in both states.

:::mermaid
stateDiagram-v2
    [*] --> DataCollectMode

    state DataCollectMode {
        [*] --> DCModeActive : entry / Enable Aggr Tmr
        DCModeActive --> [*] : exit / None
    }

    state PublishingMode {
        [*] --> PublishModeActive : entry / Enable Publish Tmr's
        PublishModeActive --> [*] : exit/Dis Pub Tmr's,Free Rsrcs
    }

    state SensorFifoMode {
        [*] --> SensorFifoModeActive : entry / Create Dbg Pkg
        SensorFifoModeActive --> [*] : exit / Cmpl Pkg, Notify Host
    }

    state DisabledMode {
        [*] --> DisabledModeActive : entry / Disable All Tmr's
        DisabledModeActive --> [*] : exit / Enable Aggr Tmr
    }

    DataCollectMode --> PublishingMode : To Publish
    DataCollectMode --> DisabledMode : To Disabled
    DataCollectMode --> SensorFifoMode : To SnsrFifo

    PublishingMode --> DataCollectMode : To DataColl
    PublishingMode --> SensorFifoMode : To SnsrFifo
    PublishingMode --> DisabledMode : To Disabled

    DisabledMode --> DataCollectMode : To DataColl
    DisabledMode --> SensorFifoMode : To SnsrFifo
    DisabledMode --> PublishingMode : To Publish

    SensorFifoMode --> DisabledMode : To Disabled

:::

### Data Processing Component

- Polls data from Sensor Fifo.
- Sensor fifo poll API's are synchronous driver framework calls, resulting in no additional run time latency.
- Update telemetry summary usage data from raw data

### In Band Telemetry Component

- Power telemetry
  - Generates the primary package for monitoring power
  - Consumes data from the data processing component and packages it in events.
  - Notify host to consume package.

- Instantaneous telemetry
  - Similar operation to power telemetry but with instantaneous data instead.
  - Faster rate than power

#### DDR Memory Management

There is 111MB of DDR reserved for power telemetry. The firmware delivers telemetry packages to the host. There
are two primary packages, Production Telemetry and Validation instantaneous.
Production telemetry packages are large in size as well as a long notification period(on the order of minutes).  Validation
instantaneous packages are smaller in size as well as a shorter notification period(on the order of milliseconds).

Therefore the power telemetry DDR memory range is split into two pools, one for each package type. There is also a
24 HR package, but it uses the validation pool as well.

Packages sizes are variable in length and depend on whether package records are enabled for recording as well as some
run-time filtering that removes zero data elements. The max size of each package type is bounded and is calculated at
compile time.  This size is rounded up to an even boundary and referred to as the pool block size. Each of the pools
is then divided into blocks as show below.

```c
      Reserved DDR for Power Telemetry (Mapped via ATU)
    *--------------------------------------------------*
    |             Power Packages DDR Pool              |
    |   *------------------------------------------*   | POWER_POOL_MEM_START
    |   |                Block 1                   |   |
    |   *------------------------------------------*   | POWER_POOL_MEM_START + POWER_POOL_BLOCK_SIZE
    |   |                Block ...                 |   |
    |   *------------------------------------------*   |
    |   |         Block NUM_POWER_POOL_BLOCKS      |   |
    |   *------------------------------------------*   | POWER_POOL_MEM_START + POWER_POOL_TOTAL_SIZE - 1
    |                                                  | POWER_POOL_MEM_START + POWER_POOL_TOTAL_SIZE
    |             Inst Packages DDR Pool               |
    |   *------------------------------------------*   | INST_POOL_MEM_START
    |   |                Block 1                   |   |
    |   *------------------------------------------*   | INST_POOL_MEM_START + INST_POOL_BLOCK_SIZE
    |   |                Block ...                 |   |
    |   *------------------------------------------*   |
    |   |         Block NUM_INST_POOL_BLOCKS       |   |
    |   *------------------------------------------*   | INST_POOL_MEM_START + INST_POOL_TOTAL_SIZE - 1
    |                                                  | INST_POOL_MEM_START + INST_POOL_TOTAL_SIZE
    *--------------------------------------------------*
```

A queue of free blocks is created for both package types utilizing a threadx queue. The free queue is completely filled at initialization time.
Each entry in the queue contains the mapped DDR starting address of the block.
Allocating memory then is just Dequeuing a block address from the queue.
De-allocating memory is simply queuing the block address to the free queue after the data is read.

Since a block allocation pattern is used instead of a heap, there is no occurrence of memory fragmentation.

Memory leaks are still possible, however care is taken to avoid this.

- If there is any failure to create a package, the block is freed via ddr_manager_deallocate_mem()
- If the block cannot be queued in the MTS manager, the block is also freed.
- If the host does not respond to the controller with a CLIENT_READ_DATA_COMPLETE, the data collection service will timeout and issue a notification for the controller to also free the block.
- If the production rate of packages is greater than the consumption rate and the MTS Manger pending queue is full, the controller will pop the oldest pending block, free it, and then add the new block to the pending queue.
- In all cases, event traces are logged if any of these events occur.

NOTES:

- Another possible implementation is utilizing threadx block pools. However, that implementation reserves 4 bytes per block, as internally it manages the block as a linked list. That isn't desirable as it offsets the start address and limits the use of the complete block. This implementation does not have that limitation and overall memory consumption is equivalent, although the queue memory is internal.
- Current model with two memory pools with fixed blocks sized for max usage is the simplest memory model. This shouldn't be a limitation with the large amount of available DDR.  However it is optional to configure the block sizes at run time init utilizing knob values to reduce blocks sizes if deemed necessary at a later date.

#### Message Transfer Service Management

The message transfer service manager is an object within the In band Telemetry Component that handles all interactions
with the message transfer service, which is the proxy for the host.

It routes incoming TRP/DCP messages and queues all pending packages.

#### Telemetry Power Package Example Sequence

NOTE: Instantaneous package generation works in a similar fashion

:::mermaid
sequenceDiagram
participant Exec as Executive Cmpnt
participant Dataproc as Data Processing Cmpnt
participant InBand as In Band Tlm Cmpnt
participant PkgCr as In Band Tlm Cmpnt.Package Creation
participant DdrMgr as In Band Tlm Cmpnt.DDR Manager
participant MtsMgr as In Band Tlm Cmpnt.MTS Manager
participant MTS as Message Transfer Service
Exec->>Exec: Power Package Timer Expired
Exec->>InBand:in_band_tlm_cmpnt_generate_pwr_pkg()
InBand->>DdrMgr: ddr_manager_allocate_mem_for_pwr_pkg()
InBand->>PkgCr :package_create_power_pkg()
PkgCr->>PkgCr: Loop through all Power Records
loop All Power Records
    alt [Pwr Event is enabled]
        PkgCr->>Dataproc: Get Event Data for Record
        Dataproc->>PkgCr: Data
        PkgCr->>PkgCr: Add record to Package
    else [Pwr Event is not enabled]
        PkgCr->>PkgCr: Skip record
    end
end
PkgCr->>MtsMgr: mts_manager_queue_tlm_package()
MtsMgr->>MtsMgr: Queue ackage info in mts_pkg_pending_queue
MtsMgr->>MTS: CLIENT_NOTIFICATION
MTS->>MtsMgr: CLIENT_READ_DATA
MtsMgr->>MtsMgr: Dequeue package info from mts_pkg_pending_queue
MtsMgr->>MTS: CLIENT_READ_DATA response
MTS->>MtsMgr: CLIENT_READ_DATA_COMPLETE
MtsMgr->>DdrMgr: ddr_manager_deallocate_mem()
:::

### Out of Band Telemetry Component

- Manages out of band communication.  (PLDM stack)
- Consumes data from the data processing component and packages it for PLDM service query.

## API

Primarily telemetry work is triggered by timer expirations and external communication notifications.
Therefore the current public interface is just to initialize at startup.

| API           | Description                                           |
| -----------   | ----------------------------------------------------- |
|  telemetry_service_init()    | Initialize the CLI component                          |

Public Interface Implementation [Link](../../../src/services/telemetry/inc/telemetry_service.h)
