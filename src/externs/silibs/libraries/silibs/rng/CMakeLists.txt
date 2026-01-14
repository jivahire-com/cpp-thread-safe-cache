# Copyright (c) Microsoft Corporation. All rights reserved.

message("----------------------------------")
message("---- Adding Silibs Crypto RNG ----")

set(TARGET extern.silibs.crypto.rng)

set(SRCS
    ${TARGET_DIR}/libraries/crypto/libraries/rng/src/rng.c
)

add_library(${TARGET} STATIC ${SRCS})

target_include_directories(
    ${TARGET}
        SYSTEM PUBLIC
            ${TARGET_DIR}/libraries/crypto/libraries/rng/include
        SYSTEM PRIVATE
            ${TARGET_DIR}/libraries/crypto/libraries/rng/src
)

target_link_libraries(${TARGET}
    PUBLIC
        silibs::kng_silibs_platform 
)

add_library(rng ALIAS ${TARGET})
add_library(silibs::crypto::rng ALIAS ${TARGET})
