#ifndef __KOKROTIMG_H__
#define __KOKROTIMG_H__
#include <inttypes.h>

typedef uint8_t      byte;
typedef uint16_t     dimension;
typedef int16_t      sdimension;
typedef uint32_t     dimensionsq;
typedef int32_t      sdimensionsq;
typedef uint32_t     bigdimension;
typedef int32_t      sbigdimension;

#include "debug.h"

#define KOKROT_MAX_CODE_DIMENSION       256
#define KOKROT_SUPPORT_BIG_IMGS         0

#define KOKROT_API_VERSION              "1.0-arkona"
#define KOKROT_VERSION                  "1.0-morana"

/* 
 * Error return types for KokrotImg
 */
typedef enum kokrot_err_t {
    /* No error has occurred */
    kokrot_err_success,

    /* A generic failure during initialization has occurred */
    kokrot_err_generic_init_failure,

    /* Memory could not be allocated */
    kokrot_err_memory_allocation_failure,

    /* A generic failure during image processing has occurred */
    kokrot_err_generic_processing_failure,

    /* A QR code was not found in the image */
    kokrot_err_code_not_found,
} kokrot_err_t;

/* 
 * Initializes the KokrotImg subsystem 
 */
kokrot_err_t kokrot_initialize();

/*
 * Sets a KokrotImg debug sink. A debug sink is a set of function pointers that
 * allow you hook into intermediate steps and view data computed by KokrotImg
 * during the process. This is useful for debugging. Calling kokrot_set_debug_sink
 * with a NULL parameter disables debugging.
 */
void kokrot_set_debug_sink(
    /* The new sink to direct debug output, NULL to disable debugging */
    kok_debug_sink_t* in_Sink);

/*
 * Searches an image given for a QR code, and if a code is found,
 * returns its matrix data, suitable for further decoding
 */
kokrot_err_t kokrot_find_code(
        /* Row-major 8-bit grayscale */
        byte*       in_Image,

        /* Dimensions of the image */
        dimension   in_ImageWidth,
        dimension   in_ImageHeight,

        /* Output matrix (sized at least KOKROT_MAX_CODE_WIDTH x
         * KOKROT_MAX_CODE_HEIGHT, populated row-major with ones
         * and zeroes, describing the detected code's cells */
        byte        *out_CodeMatrix,

        /* Output matrix size */
        dimension   *out_CodeMatrixDimension);

/*
 * Returns the version string of the KokrotImg subsystem.
 */
const char* kokrot_version();

/*
 * Returns the version string of the KokrotImg API.
 */
const char* kokrot_api_version();

/* 
 * Frees all memory and releases all other resources used by the
 * KokrotImg subsystem 
 */
void kokrot_cleanup();

#endif // __KOKROTIMG_H__
