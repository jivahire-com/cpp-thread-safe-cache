function(lint_target TARGET)
    # Increased warning level
    target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Werror)

    # Clang-Tidy linter
    if (DEFINED REPO_CLANG_TIDY)
        set_target_properties(${TARGET} PROPERTIES CXX_CLANG_TIDY "${REPO_CLANG_TIDY}")
        set_target_properties(${TARGET} PROPERTIES C_CLANG_TIDY "${REPO_CLANG_TIDY}")
    endif()

    # Include-What-You-Use checks
    if (DEFINED REPO_IWYU)
        set_target_properties(${TARGET} PROPERTIES CXX_INCLUDE_WHAT_YOU_USE "${REPO_IWYU}")
        set_target_properties(${TARGET} PROPERTIES C_INCLUDE_WHAT_YOU_USE "${REPO_IWYU}")
    endif()
endfunction()
