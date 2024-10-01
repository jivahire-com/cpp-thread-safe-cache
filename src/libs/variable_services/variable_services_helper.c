//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file variable_services_helper.c
 * Implementation for common helper functions for variable services sync & async APIs
 */

/*------------- Includes -----------------*/
#include "variable_services_helper.h"

#include "variable_services.h"

#include <bug_check.h>
#include <kng_error.h> // for KNG_E_INVALIDARG, KNG_E_NOTIMPL

/*-- Symbolic Constant Macros (defines) --*/
#define EFI_VARIABLE_ATTRIBUTES_MASK                                                             \
    (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | \
     EFI_VARIABLE_HARDWARE_ERROR_RECORD | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS | EFI_VARIABLE_APPEND_WRITE)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
//! Since variable services is always over hsp mbox, we can use a static variable to store the ICC context
//! instead of taking in ICC base crx as a param from user
static fpfw_icc_base_ctx_t* hsp_icc_ctx = NULL;

/*------------- Functions ----------------*/
int32_t variable_services_check_attribute(uint32_t attributes)
{
    //! Check for valid variable attribute.
    //! EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS is deprecated and should not be used.
    if ((attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0)
    {
        return KNG_E_NOTIMPL;
    }

    //! Make sure the Attributes combination is supported by the platform.
    if ((attributes & EFI_VARIABLE_ATTRIBUTES_MASK) == 0)
    {
        return KNG_E_INVALIDARG;
    }

    //! Only EFI_VARIABLE_NON_VOLATILE attribute is invalid
    if ((attributes & EFI_VARIABLE_ATTRIBUTES_MASK) == EFI_VARIABLE_NON_VOLATILE)
    {
        if ((attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0)
        {
            return KNG_E_NOTIMPL;
        }
        return KNG_E_INVALIDARG;
    }

    //! Make sure if runtime bit is set, boot service bit is set also.
    if ((attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == EFI_VARIABLE_RUNTIME_ACCESS)
    {

        if ((attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0)
        {
            return KNG_E_NOTIMPL;
        }
        return KNG_E_INVALIDARG;
    }

    //! Hardware error record is not supported
    if ((attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != 0)
    {
        return KNG_E_NOTIMPL;
    }

    //! EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS and EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS
    //! attribute cannot be set both.
    if (((attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) == EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) &&
        ((attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) == EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS))
    {
        return KNG_E_NOTIMPL;
    }

    return KNG_SUCCESS;
}

fpfw_icc_base_ctx_t* get_icc_base_ctx(void)
{
    return hsp_icc_ctx;
}

void variable_service_init(fpfw_icc_base_ctx_t* icc_ctx)
{
    //! Initialize the ICC context
    BUG_ASSERT(icc_ctx != NULL);
    hsp_icc_ctx = icc_ctx;
    //! What else needs to be done here?
}