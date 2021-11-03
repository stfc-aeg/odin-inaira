/*
 * PcoCameraError.c - PCO camera error message implementation
 *
 * This module implements a simple wrapper around the PCO SDK error text implementation. It is
 * not possible to call this directly from C++ modules due to non-ISO standard use of character
 * arrays in the error lookups in PCO_errt.h.
 *
 * Created on: 03 Nov 2021
 *     Author: Tim Nicholls, STFC Detector Systems Software Group
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// Reimplementation of non-ANSI standard sprintf_s function used by the PCO SDK
int sprintf_s(char* buf, int size, const char* cp, ...)
{
    va_list arglist;

    va_start(arglist, cp);
    return vsnprintf(buf, size, cp, arglist);
}

// Include the PCO error text header with the appropriate define to get PCO_GetErrorText is
// defined
#define PCO_ERRT_H_CREATE_OBJECT
#include "defs.h"
#include "PCO_errt.h"

