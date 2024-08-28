# RMSS DMA Design

## Table of Contents

[[_TOC_]]

## Introduction

### Description

This document is intended to describe the design detail for the module implementing a driver framework compliant DMA driver for use by MCP and SCP.

### Terms

| Term                  | Description                                                            |
| ------                | -------------------------------                                        |
| SCP                   | System Control Processor                                               |
| MCP                   | Management Control Processor                                           |
| DMAC | DMA Controller |
| Source peripheral | Device on an AXI layer from which the DMAD reads data to store in the channel FIFO |
| Destination peripheral | Device to which the DMAC writes the stored data from the FIFO |
| Memory | Source or destination that is always ready for a DMA transfer and does not require a handshaking interface |
| Channel | Read/write data path between a source peripheral and a destination peripheral.  If the destination peripheral is not memory, then a destination handshaking interface is assigned to the channel.  Source/destination handshaking interfaces can be assigned dynamically via channel registers |
| Transaction | Basic unit of a DMAC transfer |

### Reference Documents

| Document                                  | Link                                |
| ----------------------------------------- | ----------------------------------- |
| Firmware Architecture Document | [Link](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Firmware/working/KG%20FW%20Architecture.docx?d=wf8844b94ffcc4b4680437d75085aec0b&csf=1&web=1&e=98HLVt)    |
| Kingsgate RMSS HAS | [Link](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Architecture/HAS%201.0/RMSS/KingsGateRMSS%20HAS%20v0p1.4.docx?d=w16370ee610d64064b14bb49a93125509&csf=1&web=1&e=yIEbOL)
| DMAC Specification                        | [Link](https://microsoft.sharepoint.com/teams/PioneerSoCNon-implementing/Shared%20Documents/General/Third-party%20IP/Synopsys/Documentation/USB/DW_axi_dmac_databook.pdf?isSPOFile=1&OR=Teams-HL&CT=1658185623299&clickparams=eyJBcHBOYW1lIjoiVGVhbXMtRGVza3RvcCIsIkFwcFZlcnNpb24iOiIyNy8yMjA3MDMwMDgxMSIsIkhhc0ZlZGVyYXRlZFVzZXIiOmZhbHNlfQ%3D%3D)   |
| ATU Specification               | [Missing?]() |
| Kingsgate ODCM MMA MAS | [Link](https://microsoft.sharepoint.com/:w:/r/teams/Kingsgate/Shared%20Documents/MicroArchitecture%20Specs/MAS/Pwr%20Mgmt%20%26%20Sensors%20MASes/PEX%20MASes/ODCM/Kingsgate%20ODCM_MMA_MAS.docx?d=wc843cd542aeb4824bcff53e2bf22d72d&csf=1&web=1&e=NWyFvn)
| Kingsgate FGPLL Wrapper MAS | [Link](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Clock/MAS/MAS%20working/Kingsgate_fgpll_wrapper_MAS.docx?d=w7fc72527b83b4429b73f2fc8d1e090bb&csf=1&web=1&e=1IwvFO)

## Requirements

- Shall initialize the DMA engine
- Shall support running on SCP and MCP
- Shall support transfers from DTCM to RMSS RAM
- Shall support transfers of raw/aggregated telemetry data through in-band path to system DDR
- Shall support APIs for single block and multi-block DMA transfers
- Shall support utilization of local RMSS ATU as needed

## Dependencies

The DMA driver will have dependencies on the following libraries/components:

- Driver Framework (Dfwk)
- Silicon Libs (Dmac)
- CLI (Functional Testing)
- ATU

## Clients & Use Cases

Known clients are the following:

- Sensor Fifo Driver/Service
- DMA of aggregated telemetry data from MCP/SCP DTCM to RMSS RAM 0/1
- DMA of aggregated telemetry data through in-band path to DRAM
- Power module?

## Design

### **Overview**

The RMSS DMA service will present a compliant driver framework interface for components wishing to efficiently transfer data to system DDR memory.  The DMA engine will be accessible by SCP, MCP, and SDM/CDED.

### **Hardware Capabilities**

- Up to 32 channels, one per source and destination pair
- Memory-to-memory, memory-to-peripheral, peripheral-to-memory, and peripheral-to-peripheral DMA transfers
- Channel Buffering: Single FIFO per channel
- Channel Control: Single or multiple DMA transactions, programmable multiple transaction size for each channel,
dynamic extension of linked list, programmable block length, error status register to ease debugging
- Multi-block transfers are supported through block chaining (by using a linked list), auto reloading of channel registers, shadow registers, or contiguous blocks.
- Interrupt Outputs: Combined & separate interrupt outputs generated on DMA transfer completion, block transfer completion, single or multiple transaction completion, error condition, channel suspend or disable

### **General FW Architecture**

- The RMSS DMA driver will allow clients to make asynchronous transfers through the driver framework.  Clients will be able to choose between a multi-block scatter/gather or single contiguous transfer types.
- The driver will be interrupt driven.
- The driver will wait for a given DMA channel to be free before initiating the next transfer request.

### **Configuration**

Each of the 2 dies has its own DMA controller in the RMSS expansion region.  Each DMA controller has 2 channels that can be assigned dynamically by the driver or have a static assignment to a particular client.  TBD.

## CLI Interface

### CLI Requirements

- The DMA driver CLI interface for the RMSS DMA driver shall support basic commands to exercise and test the DMA driver.
- The DMA driver CLI shall be able to handle asynchronous commands.
- The CLI interface on SCP or MCP shall interact with both the DMA engines in the RMSS expansion area.

### CLI Design

The dma_cli module piggybacks on the FpFWCLI module from shared services. Since the 1pfw CLI service supports 2 levels of menu/sub-menu, the CLI commands are architected into 2 layers:

- All commands fall under the `dma` menu.
  - `dma start_async <source address> <dest address> <size>` This will execute an asynchronous single-block transfer.
  - `dma atu_query <dest address>` This will return any configuration that the DMA's ATU will require to reach the destination address.
  - `dma status` Shows status of DMA driver including any outstanding requests
  - `dma cfg_get` Shows any configurable parameters for the driver
  - `dma cfg_set` Accepts configuration overrides/settings for the driver
  - `dma log_on` May be implemented to enable verbose messaging from the driver
  - `dma log_off` Turns off verbose messaging

## Opens / In-progress

- If all channels are pre-assigned to clients, and are potentially in use, which channel should a transaction from CLI be scheduled for?
- Should the 2 channels be assigned dynamically by the driver or have a static assignment to a particular client
- SiLibs interrupt support pending
