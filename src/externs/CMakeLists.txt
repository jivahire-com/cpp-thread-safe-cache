if (${REPO_ENABLE_TESTS})
    add_subdirectory(cmocka)
endif()

# Add threadx subdirectory
add_subdirectory(threadx)

# Add silibs subdirectory
add_subdirectory(silibs)

# Add 1pfw fwlibs subdirectory
add_subdirectory(1pfw_fwlibs)

