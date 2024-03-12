message("----------------------------------")
message("- Adding Silibs Common Override --")

set(TARGET extern.silibs.silibs_platform)

set(SRCS
${CMAKE_CURRENT_SOURCE_DIR}/silibs_platform/src/silibs_platform.c
)

add_library(${TARGET} STATIC ${SRCS})

target_include_directories(
    ${TARGET}
        SYSTEM PUBLIC
          ${TARGET_DIR}/libraries/common/silibs_platform/include
)

target_link_libraries(${TARGET}
    PUBLIC
)

lint_target(${TARGET})
format_target(${TARGET})

#package as silibs_platform due to kng_silibs_platform dependency within silibs
add_library(silibs_platform ALIAS ${TARGET})
add_library(silibs::platform ALIAS ${TARGET})