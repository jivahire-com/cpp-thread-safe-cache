enabled_on(arm-eabi-aarch)

set(TARGET vectors.cortex_m7)

# This target defines the initial interrupt vectors for a cortex m7 project
#
#  The library will define objects that must be placed in these sections
#
#  .vectors
#     The interrupt vectors that must be loaded at the start of the program memory
#  .init
#     The initialization code called prior to main, as well as some default interrupt
#     handlers
#
#  An executable that links this project must
#    * define a "main" function
#    * place .vectors at the beginning of memory
#    * place .init in the .text section or an appropriate executable section
#    * define other symbols in the linker map that are used including
#        * __stack_end__        = The location sp will be initialized to
#        * __data_load_start__  = The memory address data will be loaded from
#        * __data_start__ 
#        * __data_end__ 
#        * __bss_start__
#        * __bss_end__
#        * __heap_start__
#        * __heap_end__
#        * __ctors_start__
#        * __ctors_end__
#
#  The early initialization routine does the following:
#    1. Set up the initial stack pointer
#    2. Copy the .data section from __data_load_start__ to __data_start__
#         This assumes that the .data section may originate in a ROM
#    3. Zero the .bss section at __bss_start__
#    4. Stores the heap size at the beginning of the .heap section
#    5. Invokes C++ constructors between __ctors_start__ and __ctors_end__
#    6. Calls "void main()"


set(SRC
    cortexm7_vectors.S
    cortexm7_crt0.S)

set(LIBS
    PRIVATE
        ms::lib::threadx # vectors reference exception handlers defined in ThreadX
    )

add_library(${TARGET} STATIC ${SRC})
target_link_libraries(${TARGET} ${LIBS})

# Adding lint and format, but leaving it commented - format makes assembly files go wild, not configured for .S files.
#format_target(${TARGET})
#lint_target(${TARGET})

add_library(ms::vectors::cortex_m7 ALIAS vectors.cortex_m7)