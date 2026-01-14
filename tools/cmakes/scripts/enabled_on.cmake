# Copyright (c) Microsoft Corporation. All rights reserved.

include_guard()

macro(set_valid_enable_flags)
    #message(STATUS "Environment setup for ${ARGV}")
    foreach(_arg_ ${ARGV})
        set("${_arg_}" 1)
        message(STATUS "Enable ${_arg_}")
    endforeach()
endmacro()

macro(enabled_on)

    # Iterate through arguments and see if 
    set(_enabled_ 0)
    foreach(_arg_ ${ARGV})
        if(${_arg_})
            set(_enabled_ 1)
            break()
        endif()
    endforeach()

    # Return from the parent context if not enabled.
    # Always unset the _enabled_ flag to clean-up nicely.
    if (NOT ${_enabled_})
        unset(_enabled_)
        return()
    else()
        unset(_enabled_)
    endif()
endmacro()
