# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET newlib_lock)

set(SRC
    ./newlib_lock.c
)

set(LIBS
        PRIVATE
            fwlib.lib.utils
            ms::lib::crash_dump
)

add_library(${TARGET} STATIC ${SRC})
target_link_libraries(${TARGET} ${LIBS})
target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_compile_definitions(${TARGET}
    PUBLIC _RETARGETABLE_LOCKING
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::newlib_lock ALIAS ${TARGET})


