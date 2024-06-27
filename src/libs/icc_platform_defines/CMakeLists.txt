
set(TARGET icc_platform_defines)

add_library(${TARGET} INTERFACE)
target_include_directories(${TARGET} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(ms::lib::icc_platform_defines ALIAS ${TARGET})
