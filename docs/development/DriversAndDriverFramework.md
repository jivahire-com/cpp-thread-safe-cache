# Drivers and Driver Framework

[[_TOC_]]

## Introduction

### Description

Driver Framework is exactly like what it sounds like, a framework drivers can utilize to access certain functionality to assist design and implementation. To stay aligned with FPFW it's recommended to use the framework where possible. Please see the reference documentation for more detail on the framework: [link](https://azurecsi.visualstudio.com/DuvallFw/_wiki/wikis/1PFw%20Firmware%20Libs/32086/DriversAndDriverFramework).

## Definition

| Item | Description |
| - | - |
| Driver | Any unit of software designed to handle a protocol or interface, with no business logic. |
| DFWK | Driver Framework |

## References

| Item | Description |
| - | - |
| DFWK | [Shared Documentation](https://azurecsi.visualstudio.com/DuvallFw/_wiki/wikis/1PFw%20Firmware%20Libs/32086/DriversAndDriverFramework) |

## Initialization

Since the runtime for the supported cores in this repo is ThreadX, the framework can be initialized on a core by calling the [Framework ThreadX Initialization](https://azurecsi.visualstudio.com/DuvallFw/_git/1pfw.fwlibs?path=/modules/libs/DriverFramework/Ports/ThreadX/DfwkThreadXHost.h&version=GBmain&line=46&lineEnd=76&lineStartColumn=1&lineEndColumn=22&lineStyle=plain&_a=contents). Once initialized the rest of the framework can be used. Ex: initializing device objects, initializing drivers, initializing interfaces, sending requests to interfaces, etc.. 
