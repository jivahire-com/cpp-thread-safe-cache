# GTimer Design

## Table of Contents

[[_TOC_]]

## Introduction

### Description

This document is intended to describe the design of a GTimer module, responsible for initializing and managing the system counter

### Terms

| Term                  | Description                                                            |
| ------                | -------------------------------                                        |
| SCP                   | System Control Processor                                               |

### Reference Documents

| Document                                  | Link                                |
| ----------------------------------------- | ----------------------------------- |
| Global Clocks | [Link](https://woodinvillewiki.com/display/EF/Global+clocks)    |
| ARM Counters, Timers, Watchdogs | [Link](https://microsoft.sharepoint.com/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/3P%20IP/Arm/Voyager/ARM%20RD-Counters,%20Timers%20and%20Watchdogs%20Training%20Slides.pdf)    |
| Server Base System Architecture 7.0 | [Link](https://documentation-service.arm.com/static/60250c7395978b529036da86?token=)    |

## Requirements 

- Shall allow consumers to initialize the system counter to a particular frequency
- Shall ensure that the system counter appears to be counting at 1GHz to be compliant with Server Base System Architecture v7.0 (section 1.5.4.1)
- Shall allow for synchronizing counter values across dies

## Design

### 1GHz tick rate
Because the refclk clock rate is 250MHz, it is necessary to set the increment rate to 4 in order for the counter to appear to count at 1GHz.

### Multi-die counter sync

Timestamp values on multiple chips tend to drift from one another over time.
To keep these values in sync, we will employ a counter synchronization process
DIE 0 SCP is responsible for synchronizing DIE 1
Signals for performing synchronization
* GCNTSYNC_OUT[NUM_SUB_CHIPS-1:0]
  * Synchronization request (SYNCREQ) from manager. Drives subordinate chip GCNTSYNC_IN[0]
* GCNTSYNC_IN[NUM_SUB_CHIPS-1:0]
  * Synchronization acknowledge (SYNCACK) from subordinate. Driven by subordinate chip GCNTSYNC_OUT[0]

Synchronization process [TBD](https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1869234)

## Public APIs

```c
/**
 *
 *    The function initializes the system counter
 *    @param counter_control_base - base address of the counter control register
 *    @param frequency_hz - frequency of the counter
 *    @param scaling_factor - scaling factor for the counter
 * 
 *    @retval none
 */
void gtimer_init(uint32_t counter_control_base, uint32_t frequency_hz, uint8_t scaling_factor);
```

## Unit Testing

Unit tests will be written against each module's public APIs.

## Functional Testing
Functional tests will ensure system boots and AP cores come up successfully on SVP, FPGA, and silicon systems.