# Data Collection

## Table of Contents

[[_TOC_]]

## Introduction

### Description

This document covers the design of the In-Band Data Collection for the SoC, including interfacing with the host and firmware clients who generate In-Band Telemetry.

### Terms

| Term | Description |
| - | - |
| AP | Application Processor |
| ATU | Address Translation Unit |
| CLI | Command Line Interface |
| DCC | Data Collection Client |
| DCP | Data Collection Protocol |
| DCS | Data Collection Service |
| HOST | Software on the AP handling the DCP |
| HW Ver | Service supporting DIE \ Core settings |
| ICC | Inter-core Communications |
| MCP | Management Control Processor |
| RTOS| Real Time Operating System |

### Reference Documents

| Document | Link |
| - | - |
| Data Collection Protocol Specification | [Link](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Firmware/Telemetry/Data_Collection_Protocol/Data_Collection_Protocol_Specification_v1_0_1.docx?d=wef353ce0601646ef96821de15938c002&csf=1&web=1&e=gKyXFQ) |
| 1PFW Diagnostic Decoder | [Link](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Firmware/Telemetry/Diagnostic_Decoder/1pfw_diagnostic_decoder_wip.docx?d=w5aeef42528484419aba5568011b30279&csf=1&web=1&e=c2GiIB) |
| ICC Base Service | [Link](https://azurecsi.visualstudio.com/DuvallFw/_git/1pfw.fwlibs?path=%2Fdoc%2FModules%2Fservices%2FIccBase.md&_a=contents&version=GBmain) |

## Components

A collection of firmware components make up the entire In-band Telemetry path:

- Data Collection Service
  - Interfaces with the host and firmware data collection clients
- Data Collection Clients
  - Firmware clients that interact with the Data Collection Service

## Data Collection Service

The Data Collection Service facilitates the firmware side of the Data Collection Protocol, supporting multiple clients through the single MCP <-> AP Die 0 channel.

### Requirements

- Shall only be initialized on the MCP on DIE 0.
- Shall manage all DCP ICC interactions between the MCP and AP.
- Shall validate that DCP messages received from the AP adhere to the specification.
- Shall provide a method enabling clients to register a DCP request handler callback.
- Shall provide a method enabling clients to register a DCP response handler callback.
- Shall provide a method enabling clients to send a DCP request to the AP.
- Shall provide a method enabling clients to send a DCP response to the AP.
- Shall be usable by clients after the runtime init phase.

### Dependencies

- ATU Service
- CLI
- HW Ver
- ICC

  **NOTE**: Any dependencies of the above are inherited dependencies for this.

### Design

The Data Collection Service serves as a middle man between clients who want to communicate data in-band with the host. This includes protocol level validation of incoming messages, managing requests or responses sent to the AP, and routing incoming requests or responses from the host to the appropriate client.

To support handling of DCP messages, the DCS will support one ICC request of each type (sending and receiving) per DCP Message, handling reuse of the requests in their completion routines. All ICC messages from the MCP to the AP will be synchronous sends, and all ICC transactions from the AP to the MCP will be asynchronous receives. When handling a DCP Message from the AP the DCS will copy the message internally, queueing it to be processed as part of its main thread processing. The DCS's thread processing handles the DCP messages in a first come first serve order.

To support handling multiple clients and enabling clients to offload their processing of DCP messages clients must provide:

- Client ID
- Client DCP Message Handler
- Client DCP Message Byte Pool

DCP messages are copied from the DCS's pool to the targeted client's pool, enabling clients to offload message processing to their own threads.

### Initialization

Upon initialization the DCS will reset all of its resources (memory for messages, client lists, RTOS resources, etc... ) and setup the MC <-> AP interaction to receive messages from the AP.

:::mermaid
sequenceDiagram
    autonumber

    participant User
    participant DCS
    participant ICC

    User ->> DCS: Initialize(p_ctx, p_config)
    DCS ->> DCS: Initialize RTOS Components
    DCS ->> DCS: Initialize Client List
    
    loop For Every DCP Message
        DCS ->> ICC: Receive Async for Message
        ICC -->> DCS: ACK
    end

    DCS -->> User: Ack
:::

### DCP Message Processing

#### Incoming Messages from the AP

The DCS handles all ICC messages from the AP and enqueues the messages to its thread processing queue.

:::mermaid
sequenceDiagram
    autonumber
    participant DCS
    participant DCS_Mem as DCS Memory Pool
    participant ICC
    participant ICC_Mem as ICC Memory
    participant AP

    note over DCS, AP: DCS Initialized and Client Registered

    %% DCS offloading of DCP Messages to its thread
    critical ICC Interrupt

      AP ->> +ICC: DCP Request to Client
      ICC ->> +DCS: DCP Request Received
      DCS ->> DCS_Mem: Get memory block
      DCS_Mem -->> DCS: Get memory Block
      DCS ->> ICC_Mem: Copy ICC Payload
      ICC_Mem -->> DCS: Copy ICC Payload
      DCS ->> DCS: Enqueue Message for Thread Processing
      DCS -->> -ICC: Request Handled
      ICC -->> -AP: Request Handled

    end
:::

#### Outgoing Messages to the AP

Outgoing messages from the DCS to the AP all all synchronous sends.

:::mermaid
sequenceDiagram
    autonumber
    participant Client
    participant DCS
    participant ICC
    participant AP

    note over Client, AP: DCS Initialized and Client Registered

    Client ->> +DCS: Send DCP Message

    note over Client, AP: Sending messages to the AP is blocking

    critical Sending DCP Message
      DCS ->> +ICC: Send DCP Message
      ICC ->> AP: DCP Message
      AP ->> AP: Handle Message
      AP -->> ICC: Clear Doorbell
      ICC -->> -DCS: DCP Message Sent
    end

    DCS -->> -Client: DCP Message Sent
:::

#### DCP Thread Processing

The thread processing iterates over the queue of messages, validates each message, and offloads the message to the appropriate client.

:::mermaid
sequenceDiagram
    autonumber

    participant Client
    participant Client_Mem as Client Memory Pool
    participant DCS
    participant DCS_Mem as DCS Memory Pool
    participant ICC
    participant ICC_Mem as ICC Memory
    participant AP

    note over Client, AP: DCS Initialized and Client Registered
    note over Client, AP: DCP Message Received and Queued

    %% DCS thread processing
    critical DCS Thread

      DCS ->> DCS: Dequeue message
      DCS ->> DCS: Validate message protocol adhesion
      DCS ->> DCS: Find Client

      alt Message Valid AND Client Found

        DCS ->> Client_Mem: Get memory block
        Client_Mem -->> DCS: Get memory block
        DCS ->> DCS: Copy message into client block
        DCS ->> Client: message_handler(client block)
        Client -->> DCS: message handled

      else Message Invalid OR Client Not Found

        DCS ->> +ICC: Send response with error code
        ICC ->> ICC_Mem: Fill with message
        ICC_Mem -->> ICC: Fill with message
        ICC ->> AP: Response with error code
        AP -->> ICC: Response handled
        ICC -->> -DCS: Response handled

      end

      DCS ->> DCS_Mem: Free Block
      DCS_Mem -->> DCS: Free block

    end
:::

## API

- [DCP](../../../src/services/data_collection/inc/data_collection_protocol.h)
- [DCS](../../../src/services/data_collection/inc/data_collection_service.h)
