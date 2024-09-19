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
| ATU                   | Address Translation Unit
| CLI                   | Command Line Interface                        |
| ICC                   | Inter Core Communication                      |
| MCP                   | Management Control Processor                  |
| MHU                   |  Message Handling Unit                        |
| NSSM                  | Non-Secure Shared Memory                      |
| SCP                   | System Control Processor                      |

### Reference Documents

| Document                                  | Link                                |
| ----------------------------------------- | ----------------------------------- |
| Telemetry Requirements and Specification | [Link](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/_layouts/15/Doc.aspx?sourcedoc=%7B5A49A2ED-ECF6-4733-BACF-053E12198DAE%7D&file=1PFW_Kingsgate_Power_Telemetry_Requirements_And_Specifications%20-%20WIP.docx&wdOrigin=TEAMS-MAGLEV.p2p_ns.rwc&action=default&mobileredirect=true)    |
| Data Collection Service  | [Link](./  TBD) |
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

### Executive Component

The executive primarily contains an RTOS thread and dispatches calls to the other components. The executive has 4 timers that initiate the majority of the processing.

- Power Data Aggregation Timer
  - Expiration calls into the Data Processing component
  - Fastest sampling time
  - Data is used for in-band power and performance reports.
  - Data is also used for out of band reports.

- Performance Report Timer
  - Expiration calls into the Data Processing component to retrieve data not obtained from power timer handler.
  - Expiration calls into the In Band Telemetry component

- Power Telemetry Timer
  - Expiration calls into the In Band Telemetry component

- 24 Hour Timer
  - Expiration calls into the Data Processing component

The timer expiration callbacks just wake up the telemetry service thread and all processing is performed there.

Any other signaling from external entry points, ISR's, communication stack callbacks, driver framework callbacks etc,  will follow the same pattern, store context data (if any), and wake up the thread to complete the processing.

Using asynchronous management of the external interfaces ensures all telemetry processing is thread safe.  No internal locks are required.

### Data Processing Component

- Polls data from Sensor Fifo.
- Sensor fifo poll API's are synchronous driver framework calls, resulting in no additional run time latency.
- Update telemetry summary usage data from raw data

### In Band Telemetry Component

- Power telemetry
  - Generates the primary report for monitoring power
  - Consumes data from the data processing component and packages it in events.
  - Notify host to consume report.

- Performance telemetry
  - Similar operation to power telemetry but with performance data instead.
  - Faster rate than power

#### Telemetry Power Package Scenario

NOTE: Performance report generation works in a similar fashion

:::mermaid
sequenceDiagram
    participant Exec as Executive Cmpnt
    participant Dataproc as Data Processing Cmpnt
    participant InBand as In Band Tlm Cmpnt
    participant PkgCr as In Band Tlm Cmpnt.Package Creation
    participant DdrMgr as In Band Tlm Cmpnt.DDR Manager
    participant DcsMgr as In Band Tlm Cmpnt.DCS Manager

    Exec->>Exec: Power Report Timer Expired
    Exec->>InBand:in_band_tlm_cmpnt_generate_pwr_report()
    InBand->>DdrMgr: ddr_manager_allocate_mem_for_pwr_report()
    InBand->>PkgCr :package_create_power_report()
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
    PkgCr->>DcsMgr: Notify Host about Package
:::

### Out of Band Telemetry Component

- Manages out of band communication.  (PLDM stack)
- Consumes data from the data processing component and packages it for PLDM service query.

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

## API

Primarily telemetry work is triggered by timer expirations and external communication notifications.
Therefore the current public interface is just to initialize at startup.

| API           | Description                                           |
| -----------   | ----------------------------------------------------- |
|  telemetry_service_init()    | Initialize the CLI component                          |

Public Interface Implementation [Link](../../../src/services/telemetry/inc/telemetry_service.h)
