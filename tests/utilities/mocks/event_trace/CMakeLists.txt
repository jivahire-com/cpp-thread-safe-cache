# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET kng.mock.event_trace)

add_library(${TARGET} 
    STATIC 
        event_trace_mocks.c
)

target_include_directories(${TARGET}
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(
    ${TARGET}
    PRIVATE
        ms::cmocka::mocks
    PUBLIC
        IFPFW.EventTracing
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

# Call out what mock functions from the event trace mock we need
mock_target(${TARGET} 
    FPFwETDecoderInit 
    FPFwETTraceInitialize 
    FPFwETDecoderPrintEvent
)

add_library(ms::mocks::event_trace ALIAS ${TARGET})
