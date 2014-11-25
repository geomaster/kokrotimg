#include "kokrotimg.h"
#include "macrocosm.h"
#include "microcosm.h"
#include "common.h"
#include <malloc.h>
#include <string.h>
static kok_data_t* kokrot;

kokrot_err_t kokrot_initialize()
{
    kokrot = (kok_data_t*) malloc(sizeof(kok_data_t));
    if (!kokrot) {
        return( kokrot_err_memory_allocation_failure );
    }

    memset(kokrot, 0, sizeof(kok_data_t));
    return( kokrot_err_success );
}

void kokrot_cleanup()
{
    free(kokrot);
}

void kokrot_set_debug_sink(kok_debug_sink_t* in_Sink)
{
    kokrot->dbg_sink = in_Sink;
}

const char* kokrot_version()
{
    return KOKROT_VERSION;
}

const char* kokrot_api_version()
{
    return KOKROT_API_VERSION;
}

kokrot_err_t kokrot_find_code(byte* in_Image, dimension in_ImageWidth, dimension in_ImageHeight,
        byte* out_CodeMatrix, dimension* out_CodeMatrixDimension)
{
    kok_data_t* k = kokrot;
    kokrot_component = "kokrotimg";

    LOGF("*** KokrotImg v%s (API v%s) reporting for duty!", KOKROT_VERSION, KOKROT_API_VERSION);
    k->src_img = in_Image;
    k->src_img_w = in_ImageWidth;
    k->src_img_h = in_ImageHeight;
    k->src_img_area = in_ImageWidth * in_ImageHeight;
    k->peak_resident_set_bytes = k->resident_set_bytes = sizeof(kok_data_t);

    int ccount = 0;
    macrocosm_err_t ret = kokrotimg_macrocosm(k, &ccount);
    if (ret != macro_err_success) {
        return( kokrot_err_generic_processing_failure );
    }

    kokrot_component = "kokrotimg";
    LOGS("Macrocosm returned no errors. Proceeding to decode first QR");

    kokrotimg_microcosm(k, 0);

    return( -1 );
}
