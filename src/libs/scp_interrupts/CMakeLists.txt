
set(TARGET scp_interrupts)

add_library(${TARGET} INTERFACE)

target_include_directories(${TARGET} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/inc)

add_library(ms::lib::scp_interrupts ALIAS ${TARGET})
