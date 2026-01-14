# Copyright (c) Microsoft Corporation. All rights reserved.

function(coverage_target TARGET)
    string( TOLOWER "${CMAKE_BUILD_TYPE}" BUILD_TYPE )
    if (${REPO_ENABLE_COVERAGE} AND ${BUILD_TYPE} STREQUAL "debug")
        target_compile_options(${TARGET} PRIVATE -fprofile-instr-generate -fcoverage-mapping)
        target_link_options(${TARGET} PRIVATE -fprofile-instr-generate)
    endif()
endfunction()
