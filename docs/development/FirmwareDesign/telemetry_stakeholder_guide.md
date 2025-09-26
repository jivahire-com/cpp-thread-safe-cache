# Power Telemetry Stakeholder User Document

## Table of Contents

[[_TOC_]]

## Introduction

This document provides usage details for power telemetry consumers.

Data definitions of telemetry produced is defined in the telemetry specification linked below.

Command detail to enable and retrieve telemetry is defined in the Data Collection Protocol specification linked below.

This document will not duplicate that which those documents have already defined, but will provide usage sequencing and any implementation detail required to use.

Those interested in the internal design may find more detail in the design link in the reference documents below.

### Terms

| Term   | Description                     |
| ------ | ------------------------------- |
| CLI                   | Command Line Interface                        |
| DCP                   | Data Collection Protocol                      |
| MCP                   | Management Control Processor                  |
| SCP                   | System Control Processor                      |

### Reference Documents

| Document                                  | Link                                |
| ----------------------------------------- | ----------------------------------- |
| Telemetry Requirements and Specification | [Link](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Firmware/Telemetry/1PFW_Kingsgate_Power_Telemetry_Requirements_And_Specifications%20V0.4%20WIP.docx?d=wfd425643c61b46e09ad093e10faf5d0b&csf=1&web=1&e=Df1DYl)    |
| Data Collection Protocol Specification  | [Link](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/_layouts/15/Doc.aspx?sourcedoc=%7B41EF1114-520C-42F3-B134-80AE7C0741E6%7D&file=Data_Collection_Protocol_Specification_v1_0_6.docx&action=default&mobileredirect=true) |
| Telemetry Service Design| [Link](./telemetry.md) |
| Diagnostic Decoder User Guide| [Link](https://azurecsi.visualstudio.com/DuvallFw/_wiki/wikis/1PFw%20Host%20Libraries/30850/DiagnosticDecoder) |
| Sensor Fifo Debug Package Decoder| [Link](../../../tests/functional/library/power_telemetry_tests/parse_snsr_fifo_pkg.py) |

## Enabling Specific In-band Telemetry Records

The Telemetry spec refers to exported data as records or events.  All records are disabled by default.
The telemetry consumer must enable each record in order for the data to be exported in a telemetry package. The records are arranged in two groups. The first group is for nominal production telemetry.  The second is called
instantaneous and is used mostly for validation. These two groups are defined by a provider ID.

NOTE: These values are pending final specification approval and are subject to change until that is finalized.

| Provider ID | Event ID | Record / Event Name |
|-------------|----------|---------------------|
| 0x201    | 0x00   | CORE_PSTATE              |
|          | 0x01   | CORE_CSTATE              |
|          | 0x02   | CORE_THROTTLE            |
|          | 0x03   | CORE_RACK_PRIORITIES     |
|          | 0x04   | CORE_VOLTAGE             |
|          | 0x05   | CORE_CURRENT             |
|          | 0x06   | CORE_TEMPERATURE         |
|          | 0x07   | CORE_POWER               |
|          | 0x08   | CORE_HISTOGRAM           |
|          | 0x09   | CORE_AGING               |
|          | 0x0A   | CORE_DROOPS              |
|          | 0x0B   | SOC_PKG_MON              |
|          | 0x0C   | SOC_VR_RAILS             |
|          | 0x0D   | SOC_DIMM_TEMPERATURE     |
|          | 0x0E   | SOC_DIMM_POWER           |
|          | 0x0F   | SOC_HNF_TEMP             |
|          | 0x10   | SOC_SENSOR_TEMP          |
|          | 0x11   | SOC_PER_DIE_MESH         |
|          | 0x12   | SOC_DIE_TO_DIE_LINK_STATE|
|          | 0x13   | SOC_MAX_TEMPERATURE      |
|          | 0x14   | SOC_VM_MPAM_CORE_POWER   |
|          | 0x15   | SOC_VM_MPAM_THROTTLE     |
|          | 0x16   | SOC_VM_MPAM_MEMORY_POWER |
|          | 0x17   | SOC_SOC_MEMORY_THROTTLING|

| Provider ID | Event ID | Record / Event Name |
|-------------|----------|---------------------|
| 0x0202   | 0x00   | INST_CORE_SUMMARY        |
|          | 0x01   | INST_SOC_VOLTAGE_RAILS   |
|          | 0x02   | INST_SOC_DIMM_RT         |
|          | 0x03   | INST_SOC_DIE_TEMP        |
|          | 0x04   | INST_SOC_MAX_TEMP        |

## Sensor Fifo Debug Mode

This mode is for debug purposes and is used to export raw data from the Sensor FIFOs.
Nominal telemetry packages are disabled in this mode.
This mode will auto stop if the entire Telemetry DDR region is used prior to user stopping this mode.

NOTE: plimit ('SCP Msg Fifo') messages are not output in this mode as they are required for the power control loop on the SCP.

The intended use case of this mode is:

- Set up any system pre-conditions
- Enable / Disable sensor fifos for collection.
- Start debug mode collection.
- Initiate event of interest.
- Stop debug mode collection.
- Read debug package using the same interface as nominal telemetry packages.
- Decode binary package into json file.
- Post process

Details:

1. Enable / Disable Fifos

    via SCP CLI - 'snsrfifo lprop'    This command list Fifo names and their ID's.
    Use these ID's for following commands

    enable =>  'snsrfifo fifoen <ID> 1'

    disable =>  'snsrfifo fifoen <ID> 0'

    - Leave 'SCP Msg Fifo' enabled as it is required for power telemetry control loop.
    - For dual die setups, this will need to be done for each Die.

1. Start Debug Mode

   via Die 0 MCP CLI - 'pwrtlm mode snsr_fifo_debug'

   - For dual die setups, this will only need to be done on Die 0 as the mode will be forwarded to Die 1

1. Create system input

1. Stop Debug Mode

   via Die 0 MCP CLI - 'pwrtlm mode disabled'

1. Retrieve the debug packages(1 per die) only using the DCP Read and ReadComplete commands.
   (NOTE: do not send any other DCP commands, such as State Start as that will cause the package to be discarded)

1. Decode the package using the script listed in the reference section above - 'python parse_snsr_fifo_pkg.py package.bin decoded.json'

NOTE: To prepare for another run, empty the sensor fifo's first:
   via Die 0 MCP CLI - 'pwrtlm mode collect'
   Wait briefly and then repeat the steps listed above.
