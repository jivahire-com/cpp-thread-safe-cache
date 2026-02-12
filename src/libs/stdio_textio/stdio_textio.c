//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file stdio_textio.c
 * This file contains the implementation of stdio fns based on and forwards the data to a textio driver.
 */

/*------------- Includes -----------------*/

#include "stdio_textio.h"

#include <DfwkClient.h>
#include <FpFwUtils.h>
#include <bug_check.h>
#include <crash_dump_serial.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/stat.h>
#include <textio_pl011.h>
#include <tx_api.h>
#include <tx_thread.h>
#include <uart_pl011.h>

#ifndef _WIN32
    #include <unistd.h>
#endif

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

static DFWK_INTERFACE_HEADER* s_textio_interface;
static TX_MUTEX s_stdio_lock;

void stdio_textio_init(PDFWK_INTERFACE_HEADER textio_interface)
{
    DfwkClientInterfaceOpen(textio_interface);
    s_textio_interface = textio_interface;
    tx_mutex_create(&s_stdio_lock, "Stdio TextIo Mutex", TX_INHERIT);
}

/*-- stdio implementations --*/

int _write_r(struct _reent* reent, int fh, const unsigned char* buf, unsigned len)
{
    FPFW_UNUSED(reent);
    FPFW_UNUSED(fh);
    FPFW_UNUSED(buf);
    FPFW_UNUSED(len);

    int bytes_written = 0;
    bool tx_ctx_locked = false;

    //! Only lock if in thread context
    if (TX_THREAD_GET_SYSTEM_STATE() == ((ULONG)0))
    {
        //! thread context or system is idle
        tx_ctx_locked = (tx_mutex_get(&s_stdio_lock, TX_WAIT_FOREVER) == TX_SUCCESS);
    }

    if (s_textio_interface != NULL)
    {
        fpfw_text_io_sync_write_request_t write_request = {.header = {.RequestType = FPFW_TEXT_IO_REQUEST_ID_WRITE_SYNC},
                                                           .input = {
                                                               .buffer = (unsigned char*)buf,
                                                               .byte_count = len,
                                                           }};
        uint8_t* byte_ptr = (uint8_t*)write_request.input.buffer;
        textio_pl011_interface_t* pl011_interface = (textio_pl011_interface_t*)s_textio_interface;
        const textio_pl011_config_t* config = pl011_interface->device->config;
        // Ensure the output written bytes is 0
        write_request.output.written_bytes = 0;

        while (write_request.output.written_bytes < write_request.input.byte_count)
        {
            uart_pl011_write_byte(config->base_address, *byte_ptr);
            // Store the output in the crash dump buffer
            crash_dump_write_serial_byte(*byte_ptr);

            //! If the byte is a newline, & carriage return is not skipped, insert a carriage return (0x0D)
            //! This is to ensure compatibility with systems that expect a CRLF line ending.
            if ((*byte_ptr == '\n') && (!config->disable_auto_cr))
            {
                uart_pl011_write_byte(config->base_address, '\r');
                crash_dump_write_serial_byte('\r');
            }
            if (config->is_vuart_enabled)
            {
                // If the vuart base address is set, write to the virtual uart
                uart_pl011_write_assume_ready(config->vuart_base_address, (char*)byte_ptr, 1);
                if (*byte_ptr == '\n')
                {
                    char tmp_byte = '\r';
                    uart_pl011_write_assume_ready(config->vuart_base_address, &tmp_byte, 1);
                }
            }

            byte_ptr++;
            write_request.output.written_bytes++;
        }

        bytes_written += write_request.output.written_bytes;
    }

    if (tx_ctx_locked)
    {
        tx_mutex_put(&s_stdio_lock);
    }

    return bytes_written;
}

int _read_r(struct _reent* reent, int fh, unsigned char* buf, unsigned len)
{
    FPFW_UNUSED(reent);
    FPFW_UNUSED(fh);
    FPFW_UNUSED(buf);
    FPFW_UNUSED(len);

    return 0;
}

int _open_r(struct _reent* reent, const char* name, int openmode, int flags)
{
    int fh = 0;
    FPFW_UNUSED(reent);
    FPFW_UNUSED(name);
    FPFW_UNUSED(openmode);
    FPFW_UNUSED(flags);

    return fh;
}

int _close_r(struct _reent* reent, int fh)
{
    FPFW_UNUSED(reent);
    FPFW_UNUSED(fh);

    return 0;
}

int _isatty_r(struct _reent* reent, int fh)
{
    FPFW_UNUSED(reent);
    FPFW_UNUSED(fh);

    return 0;
}

int _lseek_r(struct _reent* reent, int fh, long pos)
{
    FPFW_UNUSED(reent);
    FPFW_UNUSED(fh);
    FPFW_UNUSED(pos);

    return -1;
}