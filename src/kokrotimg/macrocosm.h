#ifndef __KOKROTIMG_MACROCOSM_H__
#define __KOKROTIMG_MACROCOSM_H__
#include "common.h"
#include "kokrotimg.h"

static void       compute_cumulative_tables(kok_data_t* k);
static void       binarize_image(kok_data_t* k);
static int        locate_finder_candidates(kok_data_t* k);
static void       locate_finder_patterns(kok_data_t* k);
static inline int check_finder_pattern(kok_data_t* k, kok_point_t pt, const int dx, const int dy, int d, byte val);
static inline int bfs_process_point(kok_data_t* k, kok_point_t pt, kok_regionbounds_t *reg, fcount_t region, int depth);
static inline int bfs_expand_neighbors(kok_data_t* k, kok_point_t pt, kok_queue_t* q, byte val, int* is_edge);
static inline int update_quad(kok_quad_t* t, kok_point_t* p);
static void       bfs_finder_square(kok_data_t* k, kok_point_t startpt, fcount_t region);
static int        process_finder_square(kok_data_t* k);
static void       hypothesize_code_locations(kok_data_t* k);

static void       dump_finder_candidates(kok_data_t* k);
static void       dump_qr_code_quads(kok_data_t* k);
static void       dump_qr_finders(kok_data_t* k);
static void       dump_images(kok_data_t* k);

static void       macrocosm_dealloc_all(kok_data_t* k);

typedef enum macrocosm_err_t {
    macro_err_success,
    macro_err_memory_allocation_failure,
    macro_err_candidate_location_failure,
    macro_err_no_codes_found,
    macro_err_unknown
} macrocosm_err_t;
  
macrocosm_err_t   kokrotimg_macrocosm(kok_data_t* k, int *out_code_count);

#endif
