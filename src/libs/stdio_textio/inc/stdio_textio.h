//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file stdio_textio.h
 * Header for the stdio textio implementation
 */

#pragma once

/*----------- Nested includes ------------*/

#include <fpfw_text_io_interface.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
void stdio_textio_init(PDFWK_INTERFACE_HEADER textio_interface);

#ifdef _WIN32
struct _reent;
int _write_r(struct _reent* reent, int fh, const unsigned char* buf, unsigned len);
#endif