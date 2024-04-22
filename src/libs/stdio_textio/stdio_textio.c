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
#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

static DFWK_INTERFACE_HEADER* s_textio_interface;

void stdio_textio_init(PDFWK_INTERFACE_HEADER textio_interface)
{
    DfwkClientInterfaceOpen(textio_interface);
    s_textio_interface = textio_interface;
}

/*-- stdio implementations --*/

int _write_r(struct _reent* reent, int fh, const unsigned char* buf, unsigned len)
{
    FPFW_UNUSED(reent);
    FPFW_UNUSED(fh);
    FPFW_UNUSED(buf);
    FPFW_UNUSED(len);

    int bytes_written = 0;
    if (s_textio_interface != NULL)
    {
        fpfw_text_io_sync_write_request_t write_request = {.header = {.RequestType = FPFW_TEXT_IO_REQUEST_ID_WRITE_SYNC},
                                                           .input = {
                                                               .buffer = (unsigned char*)buf,
                                                               .byte_count = len,
                                                           }};
        int32_t status = DfwkInterfaceSendSync(s_textio_interface, &write_request.header);
        FPFW_RUNTIME_ASSERT(status == DFWK_SUCCESS);
        bytes_written += write_request.output.written_bytes;
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