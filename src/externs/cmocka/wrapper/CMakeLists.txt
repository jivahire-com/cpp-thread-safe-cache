set(TARGET testrunner.cmocka)

set(SOURCES
    CMockaWrapper.cpp
)

add_library(${TARGET} STATIC ${SOURCES})
target_link_libraries(${TARGET} PRIVATE cmocka-static)
target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
lint_target(${TARGET})
format_target(${TARGET})

# Add CMocka include as SYSTEM, so static analysis doesn't complain 
# about this 3rd party code
target_include_directories(${TARGET} SYSTEM PUBLIC ${TARGET_DIR}/include)

# Clang/MSVC error where this file declares directives not supported by clang
target_compile_definitions(${TARGET} PUBLIC -D_XKEYCHECK_H)

add_library(ms::cmocka::testrunner ALIAS ${TARGET})
