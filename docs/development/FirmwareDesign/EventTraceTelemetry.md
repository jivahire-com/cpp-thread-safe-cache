# Event Trace Telemetry

## Table of Contents

[[_TOC_]]

## Introduction

### Description

This document details how event trave data is collected per core how it is transmitted via the in-band telemetry path.

### Terms

| Term | Description |
| - | - |
| ET | Event Trace |
| STDIO | Standard input output |
| ETC | Event Trace Collector |
| ETR | Event Trace Relay |
| MTS | Message Transfer Service |

### Reference Documents

| Document | Link |
| - | - |
| Event Tracing Shared Module | [Link](https://azurecsi.visualstudio.com/DuvallFw/_wiki/wikis/1PFw%20Firmware%20Libs/10100/EventTracing) |
| Shared Event Trace Collector | [Link](https://azurecsi.visualstudio.com/DuvallFw/_wiki/wikis/1PFw%20Firmware%20Libs/35781/event_trace_collector) |

## Firmware Components

The full trace system requires multiple components, spread across the platform.

- Per Core Event Providers + Events
- Per Core Event Trace Collector (ETC)
- Per Core Event Trace Decoder (ETD)
- Per Die Event Trace Relay (ETR)

### Per Core Event Providers + Events

Each core will maintain a list of core unique Event Providers, and each provider will maintain their set of events. Providers can be common across cores, however per core they are required to be unique.

### Per Core Event Trace Collector (ETC)

Each core will implement the Shared Event Trace Collector, to manage events and trace buffers. Cores may configure their ETC with core specific needs, like core specific ICC needs for instance.

### Per Core Event Trace Decoder (ETD)

Each core has its own event trace decoder that can decode individual events or a buffer of events and output it on STDIO.

### Per Die Event Trace Relay (ETR)

Each core has an Event Trace Collector that manages core specific buffers, which will fill up overtime and need to be recycled. Before they are recycled they need to be copied into DDR, to eventually be read by the in-band telemetry path. From the cores' point of view as soon as a core specific buffer is copied into DDR it can recycled and reused. The Event Trace Relay (ETR) will manage receiving requests from the individual cores (per die) for recycling and respond once copied into DDR (or with an error code if not).

Along with copying core specific buffers into DDR, the ETR will notify the Primary ETR of what buffers in DDR are waiting to be read, along with listening for the response of those buffers being read from the primary ETR. When a buffer in DDR has been read, the ETR will reuse that buffer in DDR.

The Buffers in DDR contain sub buffers from all of the cores, and are referred to as ASIC Buffers. For example, an ASIC buffer may contain core buffers from the SDM, CDED, MCP, or the SCP and may contain more than one from any of the cores. See the Shared Event Tracing Module for more detail.

**NOTE**: The DIE's ETR will reside on the MCP of the DIE.

### Extended role of the ETR on the Primary Die

Once Event Trace Buffers are copied into DDR they need to be read by the host (which also frees up the DDR again). To keep the number of dies obscured from the host a the Primary Die ETR will act as a single point of contact and will manage all host interactions for Event Trace. Each die's ETR will notify the Primary ETR of what is in DDR. The Primary ETR will send a message back to the individual die's ETRs once the host has read the data, enabling the ETR to reuse that space in DDR.

**NOTE**: For KNG, the Primary Die is Die 0. Hence the primary ETR will reside on MCP Die 0.

## Interface Components

Event Tracing Telemetry shall use MTS as the interface to communicate between cores. The communication within all the cores and all the dies shall be via TRP, while interface between the Primary ETR (on MCP D0) and the host shall be via DCP. Below is a diagram of all the interfaces that the Event Trace Telemetry will use.

For HSP Event Trace, we will interface with ICC messages instead of using MTS.
## Design

### Component Flow Chart

:::mermaid
flowchart LR
    %% Core Components
    fw_comp(Firmware Component)
    base_et(Base Event Trace)
    ETC
    ETD
    ETC-MTS-Client(ETC MTS Client)
    ETR-MTS-Client(ETR MTS Client)
    subgraph SoC
        subgraph Die
            subgraph Per Core
                fw_comp --> base_et
                base_et --> ETC
                base_et --> ETD
                ETC --> ETC-MTS-Client
            end
            subgraph Per MCP
                ETC-MTS-Client --> ETR-MTS-Client
                ETR-MTS-Client <--> ETR
            end
        end
        ETR --> DDR
        ETR-MTS-Client -->|Die0| Host
        Host --> DDR
    end
:::

### Data Transfer Interfaces in Kingsgate Event Trace Telemetry

:::mermaid
flowchart LR
    subgraph Die 0 Control Cores
        MCP0[MCP0] <-->|MTS-TRP| MCP0-ETR
        SDM0[SDM0] <-->|MTS-TRP| MCP0-ETR
        SCP0[SCP0] <-->|MTS-TRP| MCP0-ETR
        CDED0[CDED0] <-->|MTS-TRP| MCP0-ETR
        HSP0[HSP0] <--> |ICC| MCP0-ETR
    end
    subgraph Die 1 Control Cores
        MCP1-ETR[MCP1-ETR] <-->|MTS-TRP| MCP1[MCP1]
        MCP1-ETR <-->|MTS-TRP| SDM1[SDM1]
        MCP1-ETR <-->|MTS-TRP| CDED1[CDED1]
        MCP1-ETR <-->|MTS-TRP| SCP1[SCP1]
        MCP1-ETR <--> |ICC| HSP1[HSP1]
    end
    AP0[AP0-Host] <-->|MTS-DCP| MCP0-ETR[MCP0-ETR]
    MCP0-ETR <-->|MTS-TRP| MCP1-ETR
:::

### ETC and ETR Flows

The ETC on each core is responsible for managing the Core Event Buffers, which fill up over time, and notifying the ETR on the DIE of when a buffer gets full. For the HSP, this will be copying buffers into an intermediate location, like DDR. For the other cores, there is access to where the buffer is stored for the core. The ETR on the DIE will reside on the MCP and will work with each core's ETC to copy their buffers into DDR, in preparation for them to read by the Host. The ETR will notify a core's ETC once it's been copied, enabling the ETC to recycle the buffer.

#### Event Trace telemetry flow on each core

Event Trace Collectors are event driven, where cores can define how they want the ETC Thread to handle the events.

The main events being:

- An event logged callback, indicating that an event has been logged into the buffer
- A buffer completion callback event, indicating that a local core buffer has filled up

:::mermaid
sequenceDiagram
    participant FW-Component
    participant ETC
    participant ETD
    participant Core MTS-Client
    participant ETR

    loop
        loop Event Logged
            FW-Component -->> ETC: Event Logged
            ETC -->> ETC: Add Event to ET Buffer
            ETC -->> ETD: On Event Callback (optional) -> Print Event on STDIO
        end

        alt Event Buffer Complete
            ETC -->> ETC: Mark ET Buffer as full
            ETC -->> ETC: Swap Buffers
            ETC -->> Core MTS-Client: Buffer Complete Callback
            Core MTS-Client -->> ETR: TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION
            Core MTS-Client -->> ETR: Buffer Location and Description
            ETR -->> ETR: Process Buffer
            ETR -->> Core MTS-Client: TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE
            Core MTS-Client -->> ETC: FPFwETControllerRecycleBuffer
        end
    end
:::

#### Event Trace telemetry from each core's MTS Client to the ETR

The ETR processing is very similar to the ETC, however it manages messages from multiple ETCs via their respective MTS-Clients.

The main events being:

- An ETC core buffer is ready to be copied into DDR
- An SoC ASIC buffer is complete and needs to be uploaded to the Host

:::mermaid
sequenceDiagram
    participant Core MTS-Client
    participant ETR
    participant Host
    participant Primary ETR
    participant DDR

    loop Event Buffer Complete
        Core MTS-Client -->> ETR: TRP Msg (TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION)
        Core MTS-Client -->> ETR: Buffer Location and Description
        ETR -->> ETR: Validate TRP Message
        alt Event ASIC Buffer Full
            ETR -->> ETR: Complete ASIC buffer, mark as pending
            ETR -->> ETR: Switch to new ASIC Buffer
            alt On Die 0
                ETR -->> Host: DCP Message (DCP_NOTIFICATION_TYPE_DATA_AVAILABLE)
                ETR -->> Host: Buffer Location and Description
            end
            alt On Die 1
                ETR -->> Primary ETR: TRP Message (TRP_MSG_ID_PACKAGE_NOTIFICATION)
                ETR -->> Primary ETR: Buffer Location and Description
            end
        end
        ETR -->> DDR: Copy Core's ET Buffer to DDR ASIC Buffer
        ETR -->> Core MTS-Client: TRP Msg (TRP_MSG_ID_READ_PACKAGE_COMPLETE)
    end
:::

#### Event Trace telemetry from the ETR to the host

The ETR can store much more trace data than individual cores. The ETR communicates with the Host to handle the following events. 

- An SoC ASIC buffer is complete and needs to be uploaded to the Host
- The Host has completed the processing of the Event Trace Buffer and the buffer can be freed up by the ETR

:::mermaid
sequenceDiagram
    participant ETR
    participant Host
    participant DDR

    loop Collect ETC Buffers
        ETR -->> DDR: Copy ETC Buffer into ASIC Buffer
    end
    
    alt SoC ASIC Buffer Full
        ETR -->> ETR: Mark ASIC Buffer as PENDING
        ETR -->> Host: DCP Message DCP_NOTIFICATION_TYPE_DATA_AVAILABLE
        ETR -->> Host: Buffer Location and Description
        ETR -->> ETR: Fetch free ASIC Buffer
        ETR -->> ETR: Start populating new ASIC Buffer
    end
        
    note over ETR, DDR: Sometime later
    Host -->> DDR: Read ASIC Buffer
    Host -->> ETR: DCP Message DCP_MSG_ID_READ_DATA_COMPLETE
    ETR -->> ETR: Free ASIC Buffer
:::

#### Complete Data Flow for Event Trace Telemetry

:::mermaid
sequenceDiagram
    participant FW-Component
    participant ETC
    participant ETD
    participant Core MTS-Client
    participant ETR
    participant Primary ETR
    participant DDR
    participant Host

    loop
        loop Event Logged
            FW-Component -->> ETC: Event Logged
            ETC -->> ETC: Add Event to ET Buffer
            ETC -->> ETD: On Event Callback (optional) -> Print Event on STDIO
        end

        loop Event Buffer Complete
            ETC -->> ETC: Mark ET Buffer as full
            ETC -->> ETC: Swap Buffers
            ETC -->> Core MTS-Client: Buffer Complete Callback
            Core MTS-Client -->> ETR: TRP Msg (TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION)
            Core MTS-Client -->> ETR: Buffer Location and Description
            ETR -->> ETR: Validate TRP Message
            alt Event ASIC Buffer Full
                ETR -->> ETR: Complete ASIC buffer, mark as pending
                ETR -->> ETR: Switch to new ASIC Buffer
                alt On Die 0
                    ETR -->> Host: DCP Message (DCP_NOTIFICATION_TYPE_DATA_AVAILABLE)
                end
                alt On Die 1
                    ETR -->> Primary ETR: TRP Message (TRP_MSG_ID_PACKAGE_NOTIFICATION)
                end
            end
            ETR -->> DDR: Copy Core's ET Buffer to DDR ASIC Buffer
            ETR -->> Core MTS-Client: TRP Msg (TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE)
            Core MTS-Client -->> ETC: FPFwETControllerRecycleBuffer
        end
    end
        
    note over ETR, DDR: When DCP_NOTIFICATION_TYPE_DATA_AVAILABLE received
    Host -->> DDR: Read ASIC Buffer
    Host -->> ETR: DCP Message DCP_MSG_ID_READ_DATA_COMPLETE
    ETR -->> ETR: Free ASIC Buffer
:::
