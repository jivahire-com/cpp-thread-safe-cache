# define target for Accel Interrupt Lib
set(TARGET ms_accel_intr_lib)

add_library(${TARGET}
    STATIC
        accel_intr_data.c
        accel_intr_dfwk_handlers.c
        accel_intr_events.c
        accel_intr_irq_init.c
        accel_intr_irq_isr.c
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/inc_priv
)

target_link_libraries(${TARGET}
    PUBLIC
        ms::lib::utils
        ms::lib::accelerator_ip
        silibs::idsw_kng
        silibs::kng_silibs_platform
        silibs::sdm_em_ext_interrupts
        ms::service::accel_intr_service
        ms::service::accel_intr_client
    PRIVATE
        fwlib.dfwk
        fwlib.lib.interface.platform.errorhandler
        fwlib.lib.interface.platform.threadx
        fwlib.lib.interface.platform.timers
        fwlib.lib.platform.impl.timers.threadx
        fwlib.lib.status
        ms::lib::accel_mbox_icc_transport
        ms::lib::cortex_m7::atomics
        ms::lib::cortex_m7::nvic
        ms::lib::error_codes
        ms::lib::event_trace::trace
        ms::lib::interrupts
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::accel_intr ALIAS ${TARGET})
