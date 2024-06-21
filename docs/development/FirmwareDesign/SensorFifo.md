# Sensor Fifo Design Document

## Table of Contents

[[_TOC_]]

## Introduction

### Description

This document is intended to describe the design and API of the components used to implement the sensor fifo functionality.
Sensor Ram Bridge refers to the actual hardware component that controls the underlying RAM
storing the data.  Sensor Fifo refers to the firmware service that abstract details of the underlying hardware.

### Terms

| Term                  | Description                                                                   |
| ------                | -------------------------------                                               |
| SCF                   | Sensor Control Framework    |
| Epoch                 | Time it takes telemetry from all the tiles and cores to be transmitted to the sensor ram |
| Stride                | Amount of data representing a single write pointer increase in a sensor ram fifo |

### Reference Documents

| Document                                  | Link                                |
| ----------------------------------------- | ----------------------------------- |
| RMSS HAS                 |[Link](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Architecture/HAS%201.0/RMSS/KingsGateRMSS%20HAS%20v0p1.4.docx?d=w16370ee610d64064b14bb49a93125509&csf=1&web=1&e=Dc1EiZ) |
| Power Management HAS |[Link](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Architecture/HAS%201.0/Power%20Management,%20Power%20Telemetry%20and%20Sensors/Kingsgate%20Power%20Management%20HAS%20v1.0.docx?d=w8b3ba55b57c440cfaa02d3eda974ce60&csf=1&web=1&e=nLFWeX) |
| MSCP EXP MAS                 |[Link](https://microsoft.sharepoint.com/:w:/r/teams/Kingsgate/Shared%20Documents/MicroArchitecture%20Specs/MAS/Kingsgate%20MSCP_EXP%20MAS_1.0.docx?d=w05de620c51a74eb7a4895f373ea50048&csf=1&web=1&e=RUMQ5z)  |
| Sensors MAS |[Link](https://microsoft.sharepoint.com/:w:/r/teams/Kingsgate/Shared%20Documents/MicroArchitecture%20Specs/MAS/Pwr%20Mgmt%20%26%20Sensors%20MASes/Kingsgate%20Sensors%20MAS.docx?d=wfc54fea30c604d31aa28f4a8922b1218&csf=1&web=1&e=oxcN1e) |
| Silibs Sensor Ram Bridge |[Link](https://dev.azure.com/ms-tsd/Kingsgate/_git/silibs?path=/libraries/sensor_ram_bridge) |
| Driver Framework |[Link](https://azurecsi.visualstudio.com/DuvallFw/_git/1pfw.fwlibs?path=/doc/Modules/DriversAndDriverFramework.md) |

## Requirements

- Shall provide a method for enabling/disabling FIFO insertions.
- Shall provide a method for discarding the elements in an individual FIFO.
- Shall provide a method to retrieve FIFO properties.
- Shall provide a method to add new entries to Firmware FIFOs.
- Shall provide a method to poll for new data in each FIFO.
- Shall provide a method to retrieve FIFO statistics and errors
- Shall provide a command line interface for manual FIFO test commands.
- Shall support multi-threaded access.

### Constraints

- Only a single consumer per fifo is supported

## Dependencies

- Driver Framework
- Sensor Ram Bridge Silicon Library

## Design

### Types of FIFOs

The sensor ram bridge hosts both hardware and software FIFOs. These FIFOs are designed to be circular non-blocking, this means that the producer may overwrite
data that is not yet read by the consumer if the FIFO is full. Error counters are incremented when this occurs which is intended for telemetry reporting.

Hardware FIFOs also have a watermark interrupt that can be used to fetch data when available. Software FIFOs lack of this feature, so the producer will require
to implement a periodic routine that checks the state of the FIFO and process samples based on the positions of the read and write pointers.

Some of the hardware FIFOs have multiple producers that provide data to a single epoch. There can be epoch overlaps if one of the producers sends its data for a new epoch before the other producers finish to send for the previous one. Hardware has implemented a way to handle two epochs at a time, but if a producer is sending
for a third epoch before the previous two are completed an error is signaled.

The SOC is physically implemented as two dies (chiplets). Each die has 34 tiles.  Each tile contains two cores and two HNFs(Hierarchical NOC Framework).  Some of the data is collected per tile, other data is collected per core. The sensor RAM and control processors are replicated on each die.
Therefore there will be two full instances running on a SOC, collecting data for half of the system.

```c
          *---------------------------------------------------------*
          |           PState Telemetry Fifo                         |
          | entries = 16, stride = 16, cores = 68, Total = 17408    |
          +---------------------------------------------------------+
          |           SCP Msg Telemetry Fifo  (req multiple of 128) |
          | entries =  4, stride = 16, cores = 68, Total =  4352    |
          +---------------------------------------------------------+
          |       Temperature Telemetry Buffer     DVFS             |
          | entries =  8, stride = 32, tiles = 34, Total =  8704    |
          +---------------------------------------------------------+
          |         Voltage Telemetry Buffer      DVFS              |
          | entries =   8, stride =  16, 34 tiles, Total =  4352    |
          +---------------------------------------------------------+
          |         Current Telemetry Buffer                        |
          | entries = 16, stride =   16, 68 cores, Total = 17408    |
          +---------------------------------------------------------+
          |     SOC PVT Temperature Telemetry Buffer                |
          | entries =  4, stride =   40,           Total =   160    |
          +---------------------------------------------------------+
          |      SOC PVT Voltage Telemetry Buffer                   |
          | entries =  4, stride =   48,           Total =   192    |
          +---------------------------------------------------------+
          | DIMM Temperature Telemetry Buffer                       |
          | entries =  8, stride =   24, 12 chan   Total =  2304    |
          +---------------------------------------------------------+
          | VR Temperature Telemetry Buffer                         |
          | entries =  24, stride =   24,          Total =   576    |
          +---------------------------------------------------------+
          | VR Current Telemetry Buffer                             |
          | entries =  24, stride =   40,          Total =   960    |
          +---------------------------------------------------------+
          |                                        Total = 56416    |
          |            Unused = 9120                                |
          *---------------------------------------------------------*
```

### Design Overview

#### Primary Design Goals

- Due to the nature of hardware generated telemetry data stored in a hardware controlled RAM Buffer, there are limitations on design portability. However,the design is partitioned to support portability as much as possible.
- Maximize performance. With respect to the consumers utilizing this service, data collection from the fifo's and processing of that data occurs at one of the faster rates in the system.  To minimize thread interactions and overhead, this design provides polling API's to the consumer.  The consumers can then be configured to run on the correct run-time thread priority in the system.  They will poll the data from the fifo's,  perform necessary operations on the data and forward the data to its final consumer. This ensures minimal data copying and minimal RTOS activity.
- Modified API's from V1, but functionally equivalent.  Polling was the implementation for V1 as well. Api's now use explicit data types to enable compile time checking, and reduce casting and/or memory allocation errors.

#### Design Partitioning

Refer to the class diagram below.

Consumers of the fifo data only need call the public API's of sensor_fifo_svc.
Primarily the consumer's will poll the fifo's for incoming data and firmware producers will add entries to the firmware controlled fifos.

The design utilizes Driver Framework.  Due to the timeliness requirements, only synchronous reads and writes are supported.

The class associations are all performed during the init phase, which allows for a platform specific device instantiation.

- scf_mhu_device_t
  - This is the specific class for the Kingsgate SCH MHU which is the hardware controller for the sensor ram bridge silicon. This is the only partition that will call silicon firmware libraries. It is designed to be as minimal as possible.

- sensor_fifo_device_t
  - Abstraction of the platform specific device.  It can contain device attributes required by the service, independent of the actual device. For instance if there is a refresh product that has a slightly different silicon interface, the platform init code can instantiate a different device that inherits this interface. None of the service code would require any changes.

- sensor_fifo_driver_interface_t
  - Provides common accessor functions to the device by utilizing driver framework.

- sensor_fifo_svc
  - Contains the common hardware independent firmware for tracking and manipulation of the hardware fifos. The service is partitioned into common fifo handling and the fifo specific public api handling.

- sensor_fifo_cli_svc
  - Command line interface to the sensor fifo for both manual and automated testing. Note that it has a reference to the same sensor_fifo_driver_interface_t that sensor_fifo_svc does.  This allows it to define debug api's to interface with the fifo's that are not presented to the consumer public interface.  The interface calls driver framework and utilizes its locking capabilities to manage multi-threaded access to the fifos from either the consumer thread or cli thread. Also note, that the cli has access to the sensor_fifo_svc public api's as well.

:::mermaid
classDiagram
    class consumer {
    }
    class sensor_fifo_svc {
    }
    class sensor_fifo_cli_svc {
    }
    class DFWK_INTERFACE_HEADER {
    }
    class sensor_fifo_driver_interface_t {
        +sensor_fifo_device_t* device
    }
    class DFWK_DEVICE_HEADER {
    }
    class sensor_fifo_device_t {
    }
    class scf_mhu_device_t {
    }
    DFWK_INTERFACE_HEADER <|-- sensor_fifo_driver_interface_t
    DFWK_DEVICE_HEADER <|-- sensor_fifo_device_t
    sensor_fifo_device_t <|-- scf_mhu_device_t
    sensor_fifo_driver_interface_t --> sensor_fifo_device_t : has a pointer to
    sensor_fifo_svc --> sensor_fifo_driver_interface_t : has a pointer to
    sensor_fifo_cli_svc --> sensor_fifo_driver_interface_t : has a pointer to
    consumer ..> sensor_fifo_svc : calls public methods
    sensor_fifo_cli_svc ..> sensor_fifo_svc : calls public methods
:::

### Configuration

The scf_mhu_device initialization function will configure the 64k SCF RAM. It will partition the RAM between all of the FIFO's and configure the hardware registers as necessary.  At this time, there are no requirements to support compile or runtime variations. If such requirements are added, then the configuration data can be moved to the platform runtime init.

## API

### Public API

[Link to Public Api](../../../src/services/sensor_fifo/inc/sensor_fifo_service.h)

Example usage of one of the polling API's

NOTE: The API contract requires the consumer read the polling API until status.more_entries is false.

```C
    sensor_ram_poll_status_t status;
    sensor_telem_t temperatureData;
    uint8_t tileIndex;

    do
    {
        status = sensor_fifo_svc_poll_tile_temperature(&temperatureData, &tileIndex);
        if (status.curr_data_is_valid)
        {
            do_something_with_data(&temperatureData, &tileIndex);
        }
    } while (status.more_entries == true);
```
:::mermaid
sequenceDiagram
    participant consumer
    participant sensor_fifo_svc
    participant sensor_fifo_driver_interface
    participant scf_mhu_device
    participant sensor_ram
    consumer->>sensor_fifo_svc: status = sensor_fifo_svc_poll_tile_temperature
    sensor_fifo_svc->>sensor_fifo_driver_interface: get and latch rd/wr pointers
    sensor_fifo_driver_interface->>scf_mhu_device: get and latch rd/wr pointers
    scf_mhu_device-->>sensor_fifo_driver_interface: Action complete
    sensor_fifo_driver_interface-->>sensor_fifo_svc: Action complete
    sensor_fifo_svc->>sensor_ram: read entry
    sensor_ram-->>sensor_fifo_svc: Action complete
    sensor_fifo_svc->>sensor_fifo_svc: read_ptr++
    sensor_fifo_svc-->>consumer:status.more_entries == (read_ptr != write_ptr)
    loop while (status.more_entries == true)
        consumer->>sensor_fifo_svc: status = sensor_fifo_svc_poll_tile_temperature
        sensor_fifo_svc->>sensor_ram: read entry
        sensor_ram-->>sensor_fifo_svc: Action complete
        sensor_fifo_svc->>sensor_fifo_svc: read_ptr++
        alt read_ptr == write_ptr
            sensor_fifo_svc->>sensor_fifo_driver_interface: Update hardware pointers
            sensor_fifo_driver_interface->>scf_mhu_device: Update hardware pointers
            scf_mhu_device-->>sensor_fifo_driver_interface: Action complete
            sensor_fifo_driver_interface-->>sensor_fifo_svc: Action complete
            sensor_fifo_svc->>sensor_fifo_svc: clear latched pointers
        end
        sensor_fifo_svc-->>consumer:status.more_entries == (read_ptr != write_ptr)
    end
:::

## Design Notes / Alternate Considerations

- Analog values do not contain units and are platform specific.  Since the consumers need to perform data calculations on the data anyway, a system partitioning decision was made to let the consumer perform platform specific conversion factors on the data to convert to specific units.  That allows sensor fifo to just manage the data to and from the individual fifos.

- Hardware fifo's do have watermark interrupt capability. The firmware fifo's do not and will always require polling. Also due to the different producer data rates, servicing interrupts for each of the HW fifos can result in more context switching of the RTOS and be a net loss in terms of run-time efficiency.  However, interrupt support could easily be added. Since it is undesirable to do much processing in the interrupt handler, an api would be added to add a simple notification handler, which would wake up the consumer and call the same polling API's.  In this case, due to the high priority of handling the data, it is preferable to not utilize the Driver Framework thread.

- One design alternative would be to have no data definitions in the sensor fifo service.  Keeping sensor fifo generic would help with design re-use, however that would also push design complexity to the consumers. The hardware fifo data format is defined by the underlying hardware for this module but the consumers also require computed data, such as tile index. For HW and FW data, the data format is required for both the producer and consumer.  If the format is not defined here, then either the data format would be defined in one with an unnecessary dependency for the other, or a 4th entity would need to be created. Void pointers provide no compile time checks and create opportunities for cast errors. Quadword alignment is a requirement for this module, not for the producers or consumers. Data defined here can be padded for Quadword alignment to ensure data copied to the fifo's is from valid memory. There is no common data or functionality across the fifos, each one needs to be accessed independently. Re-use in this design can be handled by making the public API's thin wrappers calling generic code and by providing an init time configuration table. Simple cmake variables can be used to swap out FIFO public API variations across platforms.
