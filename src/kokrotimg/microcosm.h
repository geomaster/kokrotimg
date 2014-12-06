#ifndef __KOKROTIMG_MICROCOSM_H__
#define __KOKROTIMG_MICROCOSM_H__
#include "common.h"

static void           project_code(kok_data_t* k, kok_quad_t code, int* pxsum);
static void           project_quad(byte* buf, dimension w, dimension h, kok_quad_t q, byte* obuf, dimension ow, dimension oh);
static void           binarize_code(kok_data_t* k);
static void           build_qr_ct(kok_data_t* k);
static void           find_finder_patterns_in_code(kok_data_t* k);
static void           fix_code_orientation(kok_data_t* k);
static int            parse_timing_patterns(kok_data_t* k);
static void           timing_pattern_scan(double x0, double y0, double tx, double ty, double dx, double dy, 
                                    kok_point_t* out_boundaries, int* out_size, int target_size, byte* buf);

static void           conjecture_alignment_patterns(kok_data_t* k);
static int            qr_cumulative_sum(kok_data_t* k, kok_point_t a, kok_point_t b);
static void           find_alignment_patterns(kok_data_t* k);
static kok_alignpat_t extend_px(kok_data_t* k, int apx, int apy);
static kok_alignpat_t extend_py(kok_data_t* k, int apx, int apy);
static kok_alignpat_t extend_mx(kok_data_t* k, int apx, int apy);
static kok_alignpat_t extend_my(kok_data_t* k, int apx, int apy);
static void           read_final_qr(kok_data_t* k);
static void           read_final_qr_quad(kok_data_t* k, kok_alignpat_t quad[4]);
static void           read_quad_module(kok_data_t* k, byte* ibuf, kok_point_t quad[4], int qrx, int qry, kok_qr_module* buf);
static kok_alignpat_t get_alignment_pattern(kok_data_t* k, int idx, int idy);
static void           interpret_final_qr(kok_data_t* k);
static int            place_virtual_alignment_patterns(kok_data_t* k);
static kok_point_t    most_likely_alignment_pattern(kok_data_t* k, kok_point_t wcenter, const int wextent);
static int            alignment_pattern_score(kok_data_t* k, kok_point_t center/*, int debugflag*/);
static void           decode_qr(kok_data_t* k, int qr_idx);

static void           dump_projected_code(kok_data_t* k);
static void           dump_binarized_code(kok_data_t* k);
static void           dump_final_qr(kok_data_t* k, byte* matrix);

typedef enum microcosm_err_t {
    micro_err_success,
    micro_err_timing_parse_failed,
    micro_err_virtual_pattern_place_failed,
    micro_err_unknown
} microcosm_err_t;
  
microcosm_err_t       kokrotimg_microcosm(kok_data_t* k, int qr_idx, byte* outmatrix);

#endif
