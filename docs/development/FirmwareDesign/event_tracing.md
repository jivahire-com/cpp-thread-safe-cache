# Event Tracing

## Table of Contents

[[_TOC_]]

## Introduction

### Description

This document details firmware event tracing, per core and for offloading via the in-band telemetry path.

### Terms

| Term | Description |
| - | - |
| ET | Event Trace |
| STDIO | Standard input output |
| ETC | Event Trace Collector |
| ETR | Event Trace Relay |
| ETDCC | Event Trace Data Collection Client |

### Reference Documents

| Document | Link |
| - | - |
| Event Tracing Shared Module | [Link](https://azurecsi.visualstudio.com/DuvallFw/_wiki/wikis/1PFw%20Firmware%20Libs/10100/EventTracing) |
| Shared Event Trace Collector | [Link](https://azurecsi.visualstudio.com/DuvallFw/_wiki/wikis/1PFw%20Firmware%20Libs/35781/event_trace_collector) |

## Firmware Components

The full trace system requires multiple components, spread across the platform.

- Per Core Event Providers + Events
- Per Core Event Trace Collector (ETC)
- Per Die Event Trace Relay (ETR)
- Per SoC Event Trace Data Collection Client (ETDCC)

### Per Core Event Providers + Events

Each core will maintain a list of core unique Event Providers, and each provider will maintain their set of events. Providers can be common across cores, however per core they are required to be unique.

### Per Core Event Trace Collector (ETC)

Each core will implement the Shared Event Trace Collector, to manage events and trace buffers. Cores may configure their ETC with core specific needs, like core specific ICC needs for instance.

### Per Die Event Trace Relay (ETR)

Each core has an Event Trace Collector that manages core specific buffers, which will fill up overtime and need to be recycled. Before they are recycled they need to be copied into DDR, to eventually be read by the in-band telemetry path. From the cores' point of view as soon as a core specific buffer is copied into DDR it can recycled and reused. The Event Trace Relay (ETR) will manage receiving requests from the individual cores (per die) for recycling and respond once copied into DDR (or with an error code if not).

Along with copying core specific buffers into DDR, the ETR will notify the Event Trace Data Collection Client (ETDCC) of what buffers in DDR are waiting to be read, along with listening for the response of those buffers being read by the Host. When a buffer in DDR has been read by the Host the ETR will reuse that buffer in DDR.

The Buffers in DDR contain sub buffers from all of the cores, and are referred to as ASIC Buffers. For example, an ASIC buffer may contain core buffers from the SDM, CDED, MCP, or the SCP and may contain more than one from any of the cores. See the Shared Event Tracing Module for more detail.

**NOTE**: The DIE's ETR will reside on the MCP of the DIE.

### Per SoC Event Trace Data Collection Client (ETDCC)

Once Event Trace Buffers are copied into DDR they need to be read by the host (which also frees up the DDR again). To keep the number of dies obscured from the host a single Event Trace Data Collection Client will manage all host interactions for Event Trace. Each die's ETR will notify the ETDCC of what is in DDR. The ETDCC will send a message back to the ETR once the host has read the data, enabling the ETR to reuse that space in DDR.

**NOTE**: The SoC's ETDCC will reside on the MCP of DIE 0.

## Design

### Component Flow Chart

:::mermaid
flowchart LR
    %% Core Components
    fw_comp(Firmware Component)
    base_et(Base Event Trace)
    ETC
    subgraph SoC
        subgraph Die
            subgraph Per Core
                fw_comp --> base_et
                base_et --> ETC
            end
            subgraph Per MCP
                ETC --> ETR
            end
        end
        ETR --> DDR
        subgraph MCP DIE 0
            ETR --> ETDCC
        end
        ETDCC --> Host
        Host --> DDR
    end
:::

### ETC and ETR Flows

The ETC on each core is responsible for managing the Core Event Buffers, which fill up over time, and notifying the ETR on the DIE of when a buffer gets full. This can range from copying buffers into an intermediate location, like ARSM or DDR, to direct access to where the buffer is stored for the core. The ETR on the DIE will reside on the MCP and will work with each core's ETC to copy their buffers into DDR, in preparation for them to read by the Host. The ETR will notify a core's ETC once it's been copied, enabling the ETC to recycle the buffer.

Event Trace Collectors are event driven, where cores can define how they want the ETC Thread to handle the events.

The main events being:

- A buffer completion callback event, indicating that a local core buffer has filled up
- A buffer recycle event, indicating that the ETR has copied a local core buffer into DDR

:::mermaid
sequenceDiagram
    autonumber
    participant ETC
    participant ETR

    ETC -->> ETC: Add Buffer Complete Callback to Event Queue

    note over ETC: Sometime later

    loop Event Queue Empty == False
        alt Event == Buffer Complete CB
            note over ETC: Core Specific Intermediate Buffer Management <br> (intermediate copy for example)
            ETC -->> ETR: Buffer Location and Description
            ETR -->> ETC: Ack
        end
    end

    note over ETC,ETR: Sometime later

    ETR -->> ETC: Buffer Copied
    ETC -->> ETC: Add Buffer Recycle to Event Queue
    ETC -->> ETR: Ack

    note over ETC: Sometime later

    loop Event Queue Empty == False
        alt Event == Buffer Recycle
            note over ETC: Core Specific Intermediate Buffer Management <br> (clearing intermediate / etc)
            ETC -->> ETC: Recycle Buffer
        end
    end
:::

The ETR processing is very similar to the ETC, however it manages messages from multiple ETCs and the single ETDCC.

The main events being:

- An ETC core buffer is ready to be copied into DDR
- A SoC ASIC buffer has been read by the host and can be recycled

:::mermaid
sequenceDiagram
    autonumber
    participant ETC
    participant ETR
    participant ETDCC
    participant DDR

    ETC -->> ETR: Buffer Location and Description
    ETR -->> ETR: Add Pending Core Buffer Event to Event Queue
    ETR -->> ETC: Ack

    note over ETR: Sometime later

    loop Event Queue Empty == False
        alt Event == Pending Buffer
            ETR -->> DDR: Copy Core Buffer to ASIC Buffer
            DDR -->> ETR: Copy Core Buffer to ASIC Buffer
            ETR -->> ETC: Buffer Copied
            ETC -->> ETR: Ack
            alt ASIC Buffer Full == True
                ETR -->> ETDCC: Buffer ready for host
                ETDCC -->> ETR: Ack
            end
        end
    end

    note over ETR,ETDCC: Sometime later

    ETDCC -->> ETR: Buffer read by host
    ETR -->> ETR: Add buffer read event to event queue
    ETR -->> ETDCC: Ack

    note over ETR: Sometime Later

    loop Event Queue Empty == False
        alt Event == Buffer Read by Host
            ETR -->> ETR: Recycle DDR Buffer
        end
    end
:::

### High Level Flows

#### Event Flow Through ETR

:::mermaid
sequenceDiagram
    autonumber
    participant Firmware Component
    participant Base Event Trace
    participant ETC
    participant ETR
    participant DDR

    Firmware Component -->> Base Event Trace: Log Event

    alt Core Buffer Full == False
        Base Event Trace -->> Base Event Trace: Update Lost Event Count
        Base Event Trace -->> Firmware Component: Log Event
    end

    Base Event Trace -->> ETC: Buffer Complete Callback
    ETC -->> ETC: Queue Buffer Complete Event
    ETC -->> Base Event Trace : Buffer Complete Callback
    Base Event Trace -->> Firmware Component: Log Event

    note over Firmware Component,ETC: Sometime later

    ETC -->> ETC: Dequeue Buffer Complete Event
    ETC -->> ETR: Copy into DDR
    ETR -->> DDR: DMA into DDR
    DDR -->> ETR: DMA into DDR

    note over ETR: Notify ETDCC

    ETR -->> ETC: Copy into DDR
    ETC -->> ETC: Recycle buffer
:::

#### Event Flow ETC Through ETDCC

:::mermaid
sequenceDiagram
    autonumber
    participant ETC
    participant ETR
    participant ETDCC
    participant Host
    participant DDR

    %% Nominal ETC and ETR interactions
    ETC -->> ETC: Dequeue Buffer Complete Event
    ETC -->> ETR: Copy into DDR
    ETR -->> DDR: DMA into DDR
    DDR -->> ETR: DMA into DDR
    ETR -->> ETDCC: Notify Host
    ETDCC -->> ETDCC: Add notification to pending notification queue
    ETDCC -->> ETR: Notify host
    ETR -->> ETC: Copy into DDR
    ETC -->> ETC: Recycle buffer

    note over ETDCC: Sometime Later

    %% Nominal ETR, ETDCC, and Host interactions
    Host -->> ETDCC: Start Client DCS Message
    ETDCC -->> Host: Start Client DCS Message

    loop Pending Queue Empty == False
        ETDCC -->> ETDCC: Dequeue notification from pending queue
        ETDCC -->> Host: Data available notification
        Host -->> ETDCC: Data available Notification
        Host -->> ETDCC: Read Data DCS Message
        ETDCC -->> Host: Read Data DCS Message
        Host -->> DDR: Read Data
        DDR -->> Host: Read Data
        Host -->> ETDCC: Read Data Complete DCS Message
        ETDCC -->> Host: Read Data Complete DCS Message
        ETDCC -->> ETR: Buffer Read
        ETR -->> DDR: Recycle DDR Buffer
        DDR -->> ETR: Recycle DDR Buffer
        ETR -->> ETDCC: Buffer Read
    end

    note over ETDCC,Host: ETDCC <-> Host messages subject to <br>change based on Data Collection Specification
:::
