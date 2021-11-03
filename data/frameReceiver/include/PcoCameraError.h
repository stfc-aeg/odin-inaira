/*
 * PcoCameraError.h - PCO camera error message implementation
 *
 * Created on: 03 Nov 2021
 *     Author: Tim Nicholls, STFC Detector Systems Software Group
 */

#ifndef PCOCAMERAERROR_H_
#define PCOCAMERAERROR_H_
#include "defs.h"

// Wrap the inclusion of the PCO error text header file in extern C so that C-linkage is used
// for the PCO_GetErrorText function
#ifdef __cplusplus
extern "C" {
#endif

#include "PCO_errt.h"

#ifdef __cplusplus
}
#endif

#endif // PCOCAMERAERROR_H_