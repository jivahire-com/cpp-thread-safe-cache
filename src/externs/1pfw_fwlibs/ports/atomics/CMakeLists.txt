set(TARGET kng.fwlib.port.atomics)

add_library(${TARGET} 
    STATIC 
        atomics.c
)

target_link_libraries(${TARGET}
    PUBLIC
        fwlib.platform.atomicOperations
    PRIVATE
        ms::lib::cortex_m7
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::atomics ALIAS ${TARGET})
