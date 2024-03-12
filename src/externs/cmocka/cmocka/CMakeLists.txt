# Add pure cmocka library for mocking purposes
set(TARGET extern.cmocka)
add_library(${TARGET} INTERFACE)
target_link_libraries(${TARGET} INTERFACE cmocka-static)
target_include_directories(${TARGET} SYSTEM INTERFACE ${TARGET_DIR}/include)
target_include_directories(${TARGET} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/inc)

add_library(ms::cmocka::mocks ALIAS ${TARGET})