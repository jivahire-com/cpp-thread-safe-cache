# Event Tracing

## Table of Contents

[[_TOC_]]

## Introduction

### Description

This document details firmware event tracing, per core and the in-band telemetry path.

### Terms

| Term | Description |
| - | - |
| ET | Event Trace |
| STDIO | Standard input output |
| ETC | Event Trace Collector |

### Reference Documents

| Document | Link |
| - | - |
| Event Tracing Shared Module | [Link](https://azurecsi.visualstudio.com/DuvallFw/_wiki/wikis/1PFw%20Firmware%20Libs/10100/EventTracing) |
| Shared Event Trace Collector | [Link](https://azurecsi.visualstudio.com/DuvallFw/_wiki/wikis/1PFw%20Firmware%20Libs/35781/event_trace_collector) |

## Firmware Components

### Per Core Event Trace Collector

Each core will implement the Shared Event Trace Collector, to manage events and trace buffers.

### Per Core Event Providers + Events

Each core will maintain a list of core unique Event Providers, and each provider will maintain their set of events. 

## Using Event Tracing

Users may create their own providers and events. Depending on the ETC for each core the events may be redirected to STDIO or some other path. All examples are from the SCP ETC Events.

### 1. Create a Provider ID

The first step in using Event Tracing is defining a Provider ID. This is a core unique identifier used to associate events AND event filter levels together. Events will be associated to an Provider ID as will an Event Level Filter, enabling filtering of event levels to the Provider Id level.

Add your core unique ID [here](./../../../src/libs/event_trace/trace/inc/event_trace_providers.h).

Example: This Enum defines two unique Provider Ids for the SCP. See the full Enum in the header.

```c
typedef enum {
    EVENT_TRACE_PROVIDER_ID_SCP_MAIN = 0x0100,

    EVENT_TRACE_PROVIDER_ID_MAX
} EVENT_TRACE_PROVIDER_ID_SCP;
```

### 2. Define your Provider

Now that we have a core unique Provider Id we can define your provider, which will generate the needed components to associate to events.

1. Add the following to your target's linked libraries list: `ms::lib::event_trace::trace`
2. Create a header file to define your provider and include the following:

    ```c
    #include <event_trace.h>
    #include <event_trace_providers.h>
    ```

3. Create a c file that defines key Event Trace defines, ensuring code generation, and include your new header. Add this to your targets sources. Ex:

    ```c
    // Only do this once per target
    // Instantiates the actual event trace functions
    #define FPFW_ET_IMPLEMENTATION
    #define FPFW_ET_METADATA

    #include "my_header_from_step_2.h"
    ```

4. Define your provider in your new header. Ex:

    ```c
    FPFW_ET_DEFINE_PROVIDER_EX(
        EVENT_TRACE_PROVIDER_ID_SCP_ETC, // Core Uniquer Provider ID
        SCP_ETC,                         // Provider Name
        ET_LEVEL_MASK_ALL                // Logging Level Mask (from event_trace.h, you can use your own)
    )
    ```

### 3. Define your Events

Now that we have defined a provider we can associate events with it.

1. Add an event to the same header you defined your provider in. Ex:

    ```c
    /**
     * Define Event Trace events for the SCP Main Provider
    */
    typedef enum {
        SCP_ETC_EVENT_ID_COLLECTOR_INIT = 0,
    } SCP_ETC_EVENT_ID;

    // This event has no fields
    FPFW_ET_DEFINE_EVENT(
        EVENT_TRACE_PROVIDER_ID_SCP_ETC, // Provider ID
        SCP_ETC_EVENT_ID_COLLECTOR_INIT, // Event ID (per provider)
        ScpEtcInit,                      // Event Name
        FPFW_ET_LEVEL_INFO               // Event Logging Level
    )
    ```

### 4. Use your Events

Now that you have events you can use them, which will be processed by the core's ETC.

1. Include your header and use the shared macros to use your new event. Ex:

    ```c
    ....
    #include "my_header_from_step_2.h"
    ....
    FPFW_ET_LOG(ScpEtcInit); // The event name from the event definition
    ```
