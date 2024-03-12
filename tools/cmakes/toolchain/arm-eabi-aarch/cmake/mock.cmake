function(mock_target TARGET)
    # For each function argument
    math(EXPR INDICES "${ARGC} - 2")
    foreach(INDEX RANGE ${INDICES})
        math(EXPR TARGET_INDEX "${INDEX} + 1")
        list(APPEND WRAP_FUNCTIONS -Wl,--wrap=${ARGV${TARGET_INDEX}})
    endforeach()

    # Add wraps to link line
    target_link_options(${TARGET} PRIVATE ${WRAP_FUNCTIONS})
endfunction()
