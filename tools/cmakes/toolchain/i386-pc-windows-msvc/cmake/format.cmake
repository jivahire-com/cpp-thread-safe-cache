function(format_target TARGET)
    if (DEFINED REPO_CLANG_FORMAT)
        get_target_property(TARGET_SOURCES ${TARGET} SOURCES)
        foreach(TARGET_SOURCE ${TARGET_SOURCES})
        get_filename_component(TARGET_SOURCE ${TARGET_SOURCE} ABSOLUTE)
        list(APPEND TARGET_SOURCE_ABS ${TARGET_SOURCE})
        endforeach()

        add_custom_command(
            TARGET 
                ${TARGET} PRE_BUILD
            COMMAND
                ${REPO_CLANG_FORMAT}
                -style=file
                -i
                ${TARGET_SOURCE_ABS}
            DEPENDS 
                ${TARGET_SOURCE_ABS}
            COMMENT 
                "Formatting: ${TARGET}"
        )

    endif()
endfunction()
