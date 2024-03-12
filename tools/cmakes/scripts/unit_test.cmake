function(unit_test_target TARGET)
    configure_file(
        ${CMAKE_SOURCE_DIR}/tests/unit/TestConfig.xml
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET}.xml
        @ONLY
    )
endfunction()
