set(TARGET scp_avs)

set(SRC
    ./scp_avs_driver.c)

set(LIBS PUBLIC fwlib.dfwk fwlib.datastructures.linkedlist fwlib.assert)

add_library(${TARGET} STATIC ${SRC})    

target_include_directories(${TARGET}
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    ${TARGET}
        PRIVATE
            silibs::avs
            silibs::ddrss
            scp_avs_cli
        PUBLIC
            ms::lib::debug
            fwlib.assert
            fwlib.dfwk
            fwlib.init
            fwlib.datastructures.linkedlist
            fwlib.utils
            silibs_platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::scp_avs ALIAS ${TARGET})
