# Generate Build Metadata

message("Generating Build Metadata")

# For automation pull in the build number, the Major, Minor, and Patch all come from the Automation pipelines, therefore we don't need to update those.
if (DEFINED ENV{BUILD_BUILDNUMBER})

  string(REPLACE "." ";" VERSION_LIST $ENV{BUILD_BUILDNUMBER})
  list(LENGTH VERSION_LIST VERSION_LIST_COUNT)
  # Silibs pipeline utilizes a build number format denoted by X.Y
  if(VERSION_LIST_COUNT GREATER 2)
    set(BUILD_NUM_INDEX 3)
  else()
    set(BUILD_NUM_INDEX 1)
  endif()

  # message(STATUS "The number of items in VERSION_LIST is: ${BUILD_NUM_INDEX}")
  list(GET VERSION_LIST ${BUILD_NUM_INDEX} BUILD)

  if (NOT DEFINED BUILD)
    set(BUILD ${PROJECT_VERSION_TWEAK})
  endif()

else()
  set(BUILD 0)
endif()

# Version
set(SYS_PRODUCTVERSION_STR "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${BUILD}.${CMAKE_BUILD_TYPE}")

# Get timestamp
string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S")

# Get the Git branch
execute_process(
  COMMAND git rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_BRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the Git commit SHA
execute_process(
  COMMAND git rev-parse --short HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_SHA
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Fetch Build PC Name
execute_process(
  COMMAND hostname 
  OUTPUT_VARIABLE BUILD_PC 
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# SCP Tag
execute_process(
  COMMAND git describe --tags --dirty --always
  OUTPUT_VARIABLE SCP_DESCRIBE
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
message("SCP_DESCRIBE \"${SCP_DESCRIBE}\"")

set(TARGET_DIR ${REPO_ROOT_DIR}/.externs/repo/silibs)

# Silibs Tag
message("WORKING_DIRECTORY \"${TARGET_DIR}/libraries/\"")
execute_process(
    COMMAND git describe --tags --dirty --always
    WORKING_DIRECTORY ${TARGET_DIR}/libraries/
    OUTPUT_VARIABLE SILIBS_DESCRIBE
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message("SILIBS_DESCRIBE \"${SILIBS_DESCRIBE}\"")

# Silibs Last Commit
execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${TARGET_DIR}/libraries/
        OUTPUT_VARIABLE SILIBS_LAST_COMMIT_DESCRIBE
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

message("SILIBS_LAST_COMMIT_DESCRIBE ${SILIBS_LAST_COMMIT_DESCRIBE}")

# SDM Tag
message("WORKING_DIRECTORY \"${TARGET_DIR}/libraries/sdm\"")
execute_process(
  COMMAND git describe --tags --dirty --always
  WORKING_DIRECTORY ${TARGET_DIR}/libraries/sdm
  OUTPUT_VARIABLE SDM_DESCRIBE
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

message("SDM_DESCRIBE \"${SDM_DESCRIBE}\"")

# SDM Last Commit
execute_process(
  COMMAND git rev-parse --short HEAD
  WORKING_DIRECTORY ${TARGET_DIR}/libraries/sdm
  OUTPUT_VARIABLE SDM_LAST_COMMIT_DESCRIBE
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

message("SDM_LAST_COMMIT_DESCRIBE ${SDM_LAST_COMMIT_DESCRIBE}")

string(RANDOM LENGTH 8 ALPHABET "0123456789ABCDEF" SYS_GUID_32BYTES_0)
string(RANDOM LENGTH 8 ALPHABET "0123456789ABCDEF" SYS_GUID_32BYTES_1)
string(RANDOM LENGTH 8 ALPHABET "0123456789ABCDEF" SYS_GUID_32BYTES_2)
string(RANDOM LENGTH 8 ALPHABET "0123456789ABCDEF" SYS_GUID_32BYTES_3)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/build_data.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/build_data.h
  @ONLY
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/build_data.c.in
  ${CMAKE_CURRENT_BINARY_DIR}/build_data.c
  @ONLY
)

set(TARGET kingsgate.lib.build_data)

# Create target
add_library(
  ${TARGET}
    STATIC
      ${CMAKE_CURRENT_BINARY_DIR}/build_data.c
)

# Configured files are put into the binary dir, ensure it gets included
target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

add_library(ms::build_data ALIAS ${TARGET})
