# Copyright (c) Microsoft Corporation. All rights reserved.

enabled_on(arm-eabi-aarch)

set(TARGET debug)

set(SRC
    ./debug.c
)

set(LIBS 
        PRIVATE
)

add_library(${TARGET} STATIC ${SRC})
target_link_libraries(${TARGET} ${LIBS})
target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

lint_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::debug ALIAS ${TARGET})
