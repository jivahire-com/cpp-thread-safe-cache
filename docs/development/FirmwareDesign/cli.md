# Command Line Interface

## Table of Contents

[[_TOC_]]

## Introduction

### Description

Command Line Interface (CLI) refers to interacting with the firmware at runtime through a terminal. The MSCP CLI runs on top of the connected UARTs and reads/writes from there and uses the Shared CLI Service.

### Terms

| Term | Description |
| -| - |
| CLI | Command Line Interface |
| UART | Universal Asynchronous Receiver / Transmitter |

### Reference Documents

| Document | Link |
| -| - |
| Shared CLI Service | [Link](https://azurecsi.visualstudio.com/DuvallFw/_git/1pfw.fwlibs?path=/doc/Modules/services/CLI.md&version=GBmain&_a=contents) |

### Design

Refer to the shared CLI Service for any design.

### Adding new CLI Commands

CLI Commands can be added via separate libraries that use the shared CLI Service + whatever platform specific libraries they want. CLI will not be baked into existing libraries. If an existing library needs to be updated to support some CLI functionality, like querying data from the library, both can be updated: the library to provide the querying function and the CLI to execute that and dump the data to the terminal.

To see an example of how to add CLI commands look here: [CLI Commands](../../../src/libs/cli_power/cli_power.c).
