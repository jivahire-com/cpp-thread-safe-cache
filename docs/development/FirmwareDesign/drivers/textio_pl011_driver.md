# Uart Pl011 Text IO Driver

## Table of Contents

[[_TOC_]]

## Introduction

### Description

This document describes the design of the pl011 text io driver. This driver implements the text io interface from the 1pfw.fwlibs repo.

### Terms

| Term | Description |
| -    | - |
| DFWK | [Driver Framework](../DriversAndDriverFramework.md) |
| Driver | An entity that uses Driver Framework |
| Text IO Driver | A driver that processes the requests specified in FpFwTextIoInterface.h |
| Asynchronous | An operation that yields execution until it gets completed and doesn't block current execution context |
| Synchronous - non blocking | An operation that will return immediately and doesn't yield to another execution context |
| Synchronous - blocking | An operation that waits for its completion in the same runtime context (but it might yield to other contexts) |

### Dependencies

| Dependency | Description | Link |
| - | - | - |
| FpFwTextIoInterface | Contains data structures and helper functions for sending TextIo requests. This driver implements support for the requests outlined in this interface. | [Link](https://azurecsi.visualstudio.com/DuvallFw/_git/1pfw.fwlibs?path=/doc/Modules/drivers/Interfaces/TextIoInterface.md) |
| UART PL011 Silibs Library | This library serves as a bare metal library for interfacing to the pl011 hardware, and has been updated with APIs needed for interrupt based operation. | [Link](https://dev.azure.com/ms-tsd/SiInfra/_git/silibs_common?path=/uart_pl011) |
| DFWK | Driver framework provides the runtime and queuing mechanisms for drivers to operate. | [Link](https://azurecsi.visualstudio.com/DuvallFw/_git/1pfw.fwlibs?path=/doc/Modules/DriversAndDriverFramework.md) |

## Goals

- Provide a way to read and write characters for applications such as logging, command line interface, etc...
- Provide different modes for read/write operations.
    1. Synchronous non-blocking.
    2. Asynchronous read/writes.
- Provide queueing for async writes and reads.

## Design

### Request handling

The below requests defined by the FpFwTextIoInterface are implemented here. See below diagrams for how each is handled.

#### TEXT_DRIVER_READ_SYNC_REQUEST_ID

The synchronous request will read from the UART until the number of desired bytes has been received.

```mermaid
sequenceDiagram
    participant Client;
    participant DriverFwk;
    participant TextIOUart;
    participant Uart;

    Note over Client: After Device/Interface initialization
    Client ->> DriverFwk: DfwkInterfaceSendSync(interface, req)
    DriverFwk ->> TextIOUart: textio_pl011_request_dispatch_sync(req)
    loop Read for N Bytes
        loop RX Not Available
            TextIOUart ->> TextIOUart: Break if RX available
        end
        TextIOUart ->> Uart: Read Byte
        Uart ->> TextIOUart: Read Byte into req.input.buffer
    end
    TextIOUart -->> TextIOUart: req.out.readbytes = bytes read
    TextIOUart -->> DriverFwk: DFWK_SUCCESS
    DriverFwk -->> Client: DFWK_SUCCESS
```

#### TEXT_DRIVER_WRITE_SYNC_REQUEST_ID

```mermaid
sequenceDiagram
    participant Client;
    participant DriverFwk;
    participant TextIOUart;
    participant Uart;

    Note over Client: After Device/Interface initialization
    Client ->> DriverFwk: DfwkInterfaceSendSync(interface, req)
    DriverFwk ->> TextIOUart: textio_pl011_request_dispatch_sync(req)
    loop Write for N Bytes
        TextIOUart ->> Uart: Write byte
    end
    TextIOUart -->> TextIOUart: req.out.writtenbytes = bytes written
    TextIOUart -->> DriverFwk: DFWK_SUCCESS
    DriverFwk -->> Client: DFWK_SUCCESS
```

#### TEXT_DRIVER_READ_ASYNC_REQUEST_ID \ TEXT_DRIVER_WRITE_ASYNC_REQUEST_ID

The below diagram is for the read request, however the logic is the same for a write request. Simply substitute read bytes for bytes written / etc.

```mermaid
sequenceDiagram
    participant Client;
    participant DriverFwk;
    participant TextIOUart;
    participant Uart;
    participant TxTimer;

    Note over Client: After Device/Interface initialization
    Client ->> DriverFwk: DfwkInterfaceSendAsync(interface, req)
    DriverFwk -->> Client: Ok
    DriverFwk --x TextIOUart: textio_pl011_default_dispatch_async(req, context)
    TextIOUart ->> TextIOUart: Enqueue request to read queue

    DriverFwk --X TextIOUart: textio_pl011_rx_dispatch_async
    TextIOUart ->> Uart: Read Bytes

    alt read_bytes == bytes_requested
        TextIOUart ->> DriverFwk: DfwkAsyncRequestComplete(req)
        DriverFwk -->> TextIOUart: Ok
        DriverFwk -x Client: OnCompletion(req)
        Client --> DriverFwk: Ok
        TextIOUart -->> DriverFwk: Ok

    else read_bytes != bytes_requested
        TextIOUart ->> TextIOUart: Set pending read request
        TextIOUart ->> TxTimer: Start Timer

        TxTimer -->> TextIOUart: textio_pl011_isr
        TextIOUart ->> Uart: Read Bytes
        TextIOUart ->> TextIOUart: Update pending request

        alt read_bytes == bytes_requested
            TextIOUart ->> DriverFwk: DfwkAsyncRequestComplete(req)
            DriverFwk -->> TextIOUart: Ok
            DriverFwk -x Client: OnCompletion(req)
            Client --> DriverFwk: Ok
            TextIOUart -->> DriverFwk: Ok

        else read_bytes != bytes_requested
            TextIOUart ->> TextIOUart: Wait for next timer
        end
    end

```

## API

[Link](../../../../src/drivers/textio_pl011/inc/textio_pl011.h)

## Limitations

Due to the current lack of Interrupt support in the Kingsgate Repo, this lib makes use of a ThreadX Timer to poll the HW and to perform interrupt like operations (filling tx fifos, reading from rx fifos). See the ADO task [here](https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1741558).
