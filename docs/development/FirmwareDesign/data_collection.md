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
| TRP | Telemetry Relay Protocol |

### Reference Documents

| Document | Link |
| - | - |
| Data Collection Protocol Specification | [Link](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Firmware/Telemetry/Data_Collection_Protocol/Data_Collection_Protocol_Specification_v1_0_1.docx?d=wef353ce0601646ef96821de15938c002&csf=1&web=1&e=gKyXFQ) |
| 1PFW Diagnostic Decoder | [Link](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Firmware/Telemetry/Diagnostic_Decoder/1pfw_diagnostic_decoder_wip.docx?d=w5aeef42528484419aba5568011b30279&csf=1&web=1&e=c2GiIB) |
| ICC Base Service | [Link](https://azurecsi.visualstudio.com/DuvallFw/_git/1pfw.fwlibs?path=%2Fdoc%2FModules%2Fservices%2FIccBase.md&_a=contents&version=GBmain) |

## Components

A collection of firmware components make up the entire In-band Telemetry path:

- Data Collection Service
  - Interfaces with the host and firmware data collection clients via DCP
- Data Collection Clients
  - Firmware clients that interact with the Data Collection Service
- Telemetry Relay
  - Bi-directional exchange of TRP messages between CPU's.  Can either be on the same die or on different dies. The available connections are dependent on whether an ICC MHU channel is instantiated. Note the relay does allow multiple hops for exchanging data if a direct CPU to CPU connection does not exist. Available CPU's may be MCP, SCP, AP, SDM/CDED etc.

## Data Collection Service

The Data Collection Service facilitates the firmware side of the Data Collection Protocol, supporting multiple clients.

The Host only interfaces with the DCP clients that reside on the Die 0 MCP. These Die 0 MCP clients are responsible
for collecting and forwarding content to other CPU's which can either reside on Die 0 or remote Die's.

This inter CPU communication is handled by DCS utilizing TRP.  The telemetry relay component handles this communication
routing via MHU ICC transports. Each DCS instantiation per CPU specifies a TRP routing table of supported destinations.
Note: on an incoming TRP message, the telemetry relay component will check if the destination is for the local CPU, if not
it will look up the destination in the routing table and forward the message.

### Requirements

- Shall abstract cpu architecture and distribution from the Host.
- Shall manage all DCP ICC interactions between the MCP and AP.
- Shall validate that DCP messages received from the AP adhere to the specification.
- Shall provide a method enabling clients to receive DCP requests and DCP responses.
- Shall provide a method enabling clients to receive TRP requests and TRP responses..
- Shall provide a method enabling clients to send a DCP request to the AP.
- Shall provide a method enabling clients to send a DCP response to the AP.
- Shall be usable by clients after the runtime init phase.

### Dependencies

- ATU Service
- CLI
- HW Ver
- ICC
- MHU ICC Transport

  **NOTE**: Any dependencies of the above are inherited dependencies for this.

### Design

The Data Collection Service serves as a middle man between clients who want to communicate data in-band with the host. This includes protocol level validation of incoming messages, managing requests or responses sent to the AP, and routing incoming requests or responses from the host to the appropriate client.

Incoming DCP messages from the AP are wrapped with a TRP header.  The message id in the TRP header is TRP_MSG_ID_DCP_FORWARD.
This allows the clients to just handle TRP messages and use the message id for handling. The TRP header contains the same
DCP client id as used in DCP messages. This allows DCS to forward messages to its registered clients easily.

To support handling multiple clients and enabling clients to offload their processing of TRP messages clients must provide:

- Client ID
- Client TRP Message Block Pool
- Client TRP Message Queue
- Client Notification Callback

The Telemetry Relay component will abstract all ICC interactions.  DCS API's abstract the telemetry relay component from the clients.
All outgoing ICC messages from the telemetry relay component are synchronous sends, and all incoming ICC transactions from other CPU's are asynchronous receives.
Incoming messages are called from the Driver framework thread, a block will be allocated, the TRP message will be copied to the block, a pointer to the block will be queued, and then the client will be notified for processing.

### Initialization

Upon initialization the DCS will reset all of its resources (memory for messages, client lists, RTOS resources, etc... ) and subscribe to the appropriate ICC commands for each endpoint.

:::mermaid
sequenceDiagram
    autonumber

    participant RTI as Run Time Init
    participant DCS
    participant ICC

    RTI ->> DCS: Initialize(p_config)
    DCS ->> DCS: Initialize RTOS Components
    DCS ->> DCS: Initialize Client List

    loop For Every ICC Endpoint
        DCS ->> ICC: Receive Async for TRP Icc command (Also DCP ICC command for Die 0 MCP)
        ICC -->> DCS: ACK
    end

    DCS -->> RTI: Done
:::

### DCP Message Processing

#### Incoming Messages from the AP

The DCS handles all ICC messages from the AP and enqueues the messages to its thread processing queue.

DCS is allocating memory from the clients pool and then notifying the client.
The client is responsible for processing the message and freeing the memory block after.

:::mermaid
sequenceDiagram
    autonumber

    participant AP
    participant ICC_Mem as ICC Memory
    participant ICC
    participant DCS
    participant Client as DCS Client

    note over AP, Client: DCS Initialized and Client Registered

    %% DCS offloading of DCP Messages to its thread
    critical ICC Driver Framework Thread Callback

      AP ->> +ICC: DCP Request to Client
      ICC ->> +DCS: DCP Request Received
      alt Valid DCP Message && Client Handler Exists
        DCS ->> Client: Allocate memory block
        Client -->> DCS: Allocate memory Block
        DCS ->> DCS: Fill in TRP Header
        DCS ->> ICC_Mem: Copy ICC Payload
        ICC_Mem -->> DCS: Copy ICC Payload
        DCS ->> Client: Enqueue Message Pointer
        DCS ->> Client: New Message Notification
      else Message Not valid or No Client Handler
        DCS ->> DCS: Allocate Memory Block
        DCS ->> DCS: Copy Message to Memory Block
        DCS ->> DCS: Enqueue Block on DCP Error Queue
        note over DCS: Error response handled from DCS thread
        DCS ->> DCS: Notify DCS thread
      end
      DCS -->> -ICC: Request Handled
      ICC -->> -AP: Request Handled

    end
:::

#### Incoming Messages from another CPU

The DCS handles all ICC messages from the AP and enqueues the messages to its thread processing queue.

DCS is allocating memory from the clients pool and then notifying the client.
The client is responsible for processing the message and freeing the memory block after.

:::mermaid
sequenceDiagram
    autonumber

    participant CPU
    participant ICC_Mem as ICC Memory
    participant ICC
    participant DCS
    participant Client as DCS Client

    note over CPU, Client: DCS Initialized and Client Registered

    critical ICC Driver Framework Thread Callback

      CPU ->> +ICC: TRP Message
      ICC ->> +DCS: TRP Message
      alt Valid TRP Message, destination is this CPU and die, && Client Handler Exists
        DCS ->> Client: Allocate memory block
        Client -->> DCS: Allocate memory Block
        DCS ->> ICC_Mem: Copy ICC Payload
        ICC_Mem -->> DCS: Copy ICC Payload
        DCS ->> Client: Enqueue Message Pointer
        DCS ->> Client: New Message Notification
      else Valid TRP Message, destination is not this CPU
        DCS ->> DCS: Allocate Memory Block
        DCS ->> DCS: Copy Message to Memory Block
        DCS ->> DCS: Enqueue Block on TRP Forwarding Queue
        note over DCS: Forwarding handled from DCS thread
        DCS ->> DCS: Notify DCS thread
      else Invalid TRP Message or no client handler
        DCS ->> DCS: Log Error
      end
      DCS -->> -ICC: Request Handled
      ICC -->> -CPU: Request Handled

    end
:::

#### Outgoing Messages to the AP

Outgoing messages from the DCS to the AP all all synchronous sends.
NOTE: The client's calling thread will block until the message is sent.

:::mermaid
sequenceDiagram
    autonumber
    participant Client as DCS Client
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

:::mermaid
sequenceDiagram
    autonumber

    participant DCS
    participant TlmRly as Telemetry Relay
    participant CPU
    participant AP

    note over DCS, AP: DCS Initialized

    %% DCS thread processing
    critical DCS Thread

        loop While TRP Forwarding Queue Not Empty
            DCS ->> DCS: Dequeue message
            DCS ->> TlmRly: Forward Message
            TlmRly ->> TlmRly: Lookup Destination in routing table
            alt Destination Found
                TlmRly ->> CPU: Forward message
            else Destination Not Found
                TlmRly ->> TlmRly: Log Error
            end
            DCS ->> DCS: Free Message block
        end

        loop While DCP Error Queue Not Empty
            DCS ->> DCS: Dequeue message
            DCS ->> DCS: Update Error Status
            DCS ->> TlmRly: Send Error Response
            TlmRly ->> TlmRly: Lookup Destination in routing table
            alt Destination Found
                TlmRly ->> AP: Send Error Response
            else Destination Not Found
                TlmRly ->> TlmRly: Log Error
            end
            DCS ->> DCS: Free Message block
        end

        loop While DCS Client Queue Not Empty (DCP_CLIENT_ID_DCS msgs)
            DCS ->> DCS: Dequeue message
            DCS ->> DCS: Handle Message
            alt Other CPU's require notification
                DCS ->> TlmRly: Broadcast Message
                TlmRly ->> TlmRly: Lookup Destination in routing table
                TlmRly ->> CPU: Broadcast Message
            end

            DCS ->> TlmRly: Reply To Sender
            TlmRly ->> TlmRly: Lookup Destination in routing table
            alt Destination Found
                TlmRly ->> AP: Reply To Sender
            else Destination Not Found
                TlmRly ->> TlmRly: Log Error
            end
            DCS ->> DCS: Free Message block
        end

    end
:::

## API

- [DCP](../../../src/services/data_collection/inc/data_collection_protocol.h)
- [DCS](../../../src/services/data_collection/inc/data_collection_service.h)
- [TRP](../../../src/services/data_collection/inc/telemetry_relay_protocol.h)