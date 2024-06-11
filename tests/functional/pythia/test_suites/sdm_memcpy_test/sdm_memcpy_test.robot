*** Settings ***
Documentation    Verifies the sdm memcpy completes from AP core 0.

# Import the python library, class must match filename when filepaths of full files.
# Importing also calls __init__().
Library     ${CURDIR}/../../tests
Library     tests.sdm_memcpy_test.sdm_memcpy_test
...             workspace_config=${WORKSPACE_CONFIG}
...             default_log_home=${LOG_DIR}
...             fw_payload_path=${PAYLOAD_DIR}
...             host_config=${HOST_CONFIG_DIR}/sdm_memcpy_test_config.json
...             WITH NAME    sdm_memcpy_test_lib

*** Test Cases ***
Test Case - SDM memcpy test [FR 1423999] [FR 1424020] [FR 1423800]
    # Get an instance of the test library
    ${test_object}=    Get Library Instance    sdm_memcpy_test_lib
    
    # Call the test method from the test library and store the result
    ${test_pass}=    Call Method    ${test_object}    test_sdm_memcpy

    # Log the status
    Should be True    ${test_pass}
