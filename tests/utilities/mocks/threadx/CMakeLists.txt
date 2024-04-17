set(TARGET mock.threadx)

add_library(${TARGET} 
    STATIC 
        thread_x_mocks.c
)

target_include_directories(${TARGET}
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(
    ${TARGET}
    PRIVATE
        fwlib.utils
        ms::cmocka::mocks
        ms::lib::threadx
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
mock_target(${TARGET}
    _tx_thread_interrupt_control
    _txe_queue_create
    _txe_semaphore_create
    _txe_thread_create
    _txe_queue_send
    _txe_queue_receive
    _txe_semaphore_get
    _txe_semaphore_put
    _txe_timer_activate
    _txe_timer_create
    _txe_timer_deactivate
)

add_library(ms::mocks::threadx ALIAS ${TARGET})
