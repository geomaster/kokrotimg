#ifndef __KOKROTIMG_COMMON_H__
#define __KOKROTIMG_COMMON_H__
#include "kokrotimg.h"
#include "config.h"
#include <assert.h>
#include <stdio.h>

typedef double real;
typedef uint32_t regint;
typedef uint64_t longint;

#define M2D(x, y, w) ((x) + (y) * (w))

#if KOKROT_SUPPORT_BIG_IMGS
typedef longint ct_t;
#else
typedef regint ct_t;
#endif

#ifdef INSTRUMENTATION_ENABLED
#define START_INSTRUMENTATION \
    instrumentation_start

#define STOP_INSTRUMENTATION \
    instrumentation_end

#define RECORD_METRIC(k, type) \
    do { \
        if (k->dbg_sink) { \
            k->dbg_sink->debug_record_metric(STOP_INSTRUMENTATION(), k->resident_set_bytes, (type), k->dbg_sink->callback_param); \
        } \
    } while (0)

#else
#define START_INSTRUMENTATION() 
#define STOP_INSTRUMENTATION()          -1.0
#define RECORD_METRIC(k, type) do { } while (0)
#endif

#if MAX_FINDER_CANDIDATES < 65535
typedef uint16_t fcount_t;
#else
typedef uint32_t fcount_t;
#endif

typedef longint sqct_t;
const char* kokrot_component;

typedef struct kok_point_t {
    sdimension x, y;
} kok_point_t;

typedef struct kok_bigpoint_t {
    sbigdimension x, y;
} kok_bigpoint_t;

typedef struct kok_quad_t {
    kok_point_t p1, p2, p3, p4;
} kok_quad_t;

typedef struct kok_pixelstrip_t {
    byte value;
    regint pixel_count;
    dimension start;
} kok_pixelstrip_t;

typedef struct kok_region_t {
    byte dead;
} kok_region_t;

typedef struct kok_regionbounds_t {
    sdimension left, right, top, bottom;
} kok_regionbounds_t;

typedef struct kok_pixelinfo_t {
    byte visited,
         small_bfs_visited;

    fcount_t region;
} kok_pixelinfo_t;

typedef struct kok_pointf_t {
    double x, y;
} kok_pointf_t;

typedef struct kok_finder_t {
    kok_point_t polygon[ MAX_FINDER_POLYGON_SIZE ],
                center;
    int         point_count,
                area;
} kok_finder_t;

typedef struct kok_alignpat_t { 
    kok_point_t  qrspace_pos;
    kok_pointf_t imgspace_pos;
    int32_t      active;
} kok_alignpat_t;

typedef struct kok_qr_module {
  int32_t hits, 
          sum;
} kok_qr_module;

typedef struct kok_queue_t {
  kok_point_t *queue;
  int32_t     start,
              end,
              size;
} kok_queue_t;

typedef struct kok_data_t {
    byte      *src_img, /* W * H */
              *binary_img, /* W * H */
              qr_code[ QR_BASESIZE ][ QR_BASESIZE ]; /* QRW * QRH */
    dimension src_img_w,
              src_img_h,
              qr_w;
    dimensionsq src_img_area;

    kok_pixelstrip_t *pixel_strips; /* max(W, H) */

    ct_t      *cumulative_table, /* W * H */
              qr_ct[ QR_BASESIZE ][ QR_BASESIZE ]; /* QRW * QRH */

    kok_point_t *finder_candidates; /* MAX_FINDER_CANDIDATES */
    kok_quad_t qr_finders[ MAX_QR_CODES ][ 3 ];
    kok_finder_t *finder_patterns; /* MAX_FINDER_PATTERNS */

    kok_queue_t bfs_queue; /* W + H */

    kok_point_t *edge_pixels, /* W + H */
                *edge_hull; /* W + H */

    kok_pixelinfo_t *pixelinfo; /* W * H */
    kok_region_t *regions; /* MAX_REGIONS */

    kok_quad_t qr_codes[ MAX_QR_CODES ];
    kok_alignpat_t alignment_patterns[ MAX_ALIGNMENT_PATTERNS ][ MAX_ALIGNMENT_PATTERNS ],
                   offgrid_patterns[ 4 ][ 2 ],
                   extra_offgrid_pattern;
    int alignment_patterns_1D;

    int column_start_positions[ MAX_QR_SIZE ],
        row_start_positions   [ MAX_QR_SIZE ],
        alignment_patterns_x,
        alignment_patterns_y;

    int qr_missing_quadrant;
    kok_point_t pattern_centers[6];
    int decoded_qr_dimension, decoded_qr_version;

    int horizontal_timing_start_y,
        vertical_timing_start_x;

    double horizontal_timing_slope,
           vertical_timing_slope;

    int finder_candidate_count, finder_pattern_count;
    int region_count, max_edge_pixels, edge_pixel_count, qr_code_count;
    int max_bfs_queue;

    kok_debug_sink_t *dbg_sink;
    int64_t resident_set_bytes,
            peak_resident_set_bytes;

    int context_finder_id;
    kok_qr_module final_qr[ QR_BASESIZE ][ QR_BASESIZE ]; /* QRW * QRH */
} kok_data_t;

typedef struct kok_memory_block_t {
    uint32_t size;

    /* pad to 16 bytes */
    byte     padding[12];
} kok_memory_block_t;


#define LOG_BUFSIZE                               4095

#define LOGS(str) \
    do { \
        if (k->dbg_sink) { \
            char s[LOG_BUFSIZE + 1]; \
            s[LOG_BUFSIZE] = '\0'; \
            snprintf(s, LOG_BUFSIZE, "[%s] " str, kokrot_component); \
            k->dbg_sink->debug_message(s, k->dbg_sink->callback_param); \
        } \
    } while (0)

#define LOGF(fmt, ...) \
    do { \
        if (k->dbg_sink) { \
            char s[LOG_BUFSIZE + 1]; \
            s[LOG_BUFSIZE] = '\0'; \
            snprintf(s, LOG_BUFSIZE, "[%s]" fmt, kokrot_component, __VA_ARGS__); \
            k->dbg_sink->debug_message(s, k->dbg_sink->callback_param); \
        } \
    } while (0)

#define ASSERT(cond, msg) \
  assert((cond) && msg)

/* #define ASSERT(cond, msg) */

#define VECTOR_MAGNITUDE(v)   (sqrt(v.x * v.x + v.y * v.y))

/* Two times the signed area of the triangle ABC */
#define SIGNED_AREA(a, b, c) \
    ((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x))

#define VALID_POINT(pt, w, h) \
    (pt.x >= 0 && pt.x < w && pt.y >= 0 && pt.y < h)

#define ALLOCATE_FIELD(field, type, count) \
    do { \
        k->field = (type*) kokmalloc(k, (count) * sizeof(type)); \
        if (!k->field) { \
            macrocosm_dealloc_all(k); \
            return(macro_err_memory_allocation_failure); \
        } \
    } while (0)

#define DEALLOCATE_FIELD(field) \
    do { \
        kokfree(k, k->field); \
        k->field = NULL; \
    } while (0)

#define ZERO_FIELD(field, type, count) \
    do { \
        memset(k->field, 0, sizeof(type) * (count)); \
    } while(0)

#define ALLOCATE_ZERO_FIELD(field, type, count) \
    do { \
        ALLOCATE_FIELD(field, type, count); \
        ZERO_FIELD(field, type, count); \
    } while(0)


/* Convenience macros. Just to ensure no one has fucked with them
 * before, as C code is often prone to (including this one).
 */
#define max(a,b) \
    ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a,b) \
    ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

#define absdiff(a, b) \
    ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a > _b ? _a - _b : _b - _a; })

#define swap(a, b) \
    ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     __typeof__ (a) tmp = (_a); \
     (a) = _b; \
     (b) = tmp; \
     ; })

#define QUAD_TO_DEBUG_POLY(q, xs, ys) \
    do { \
        xs[0] = q.p1.x; \
        xs[1] = q.p2.x; \
        xs[2] = q.p3.x; \
        xs[3] = q.p4.x; \
        ys[0] = q.p1.y; \
        ys[1] = q.p2.y; \
        ys[2] = q.p3.y; \
        ys[3] = q.p4.y; \
    } while (0)


void*               kokmalloc(kok_data_t* k, uint32_t bytes);
void                kokfree(kok_data_t* k, void* ptr);

void                update_memory_peak(kok_data_t* k);

inline int          queue_create(kok_data_t* k, int size, kok_queue_t* out_queue);

/* The caller is responsible for ensuring the queue doesn't overflow or underflow */
inline void         queue_push_back(kok_queue_t* q, kok_point_t pt);
inline kok_point_t  queue_pop_front(kok_queue_t* q);

inline int          queue_get_size(kok_queue_t* q);
inline void         queue_destroy(kok_data_t* k, kok_queue_t* q);
inline void         queue_clear(kok_queue_t* q);

inline kok_point_t  makept(sdimension x, sdimension y);
inline kok_pointf_t makeptf(double x, double y);
inline kok_point_t  ptf2pt(kok_pointf_t p);
inline kok_pointf_t pt2ptf(kok_point_t p);
inline int          bilinear_interpolation(double x, double y, int w, int h, byte* buf);
inline double       horizontal_slide(kok_pointf_t p, double slope, double newx);
inline double       get_slope(kok_pointf_t a, kok_pointf_t b);
inline double       get_inv_slope(kok_pointf_t a, kok_pointf_t b);
inline double       vertical_slide(kok_pointf_t p, double inv_slope, double newy);
inline kok_pointf_t lerp(kok_pointf_t a, kok_pointf_t b, double alpha);
inline void         bresenham_line(byte* buf, byte val, dimension w, kok_point_t a, kok_point_t b);
inline void         bresenham_polygon(byte* buf, byte val, dimension w, kok_point_t* polygon, int count);
inline void         infect_xy(byte* buf, kok_point_t start, dimension w, dimension h, 
                                        int* xcount_left, int* ycount_above, int* xcount_right, int* ycount_below);

void                black_white_module_size(kok_data_t* k, int startx, int starty, int dx, int dy,
                                        int* blacksz, int* whitesz);

int                 polygon_sum(byte* buf, dimension w, dimension h, const kok_point_t* points, int count, int *pixelcount);
int                 convex_hull(kok_point_t* points, int count, kok_point_t* hull);
kok_quad_t          enclose_hull(kok_point_t* hull, int count);

void                flip_x(byte* buf, dimension w, dimension h);
void                flip_y(byte* buf, dimension w, dimension h);
void                rot90(byte* buf, dimension w);

int                 small_scale_bfs(kok_data_t* k, kok_point_t* where, byte* buf, int w, int maxpts, kok_point_t start, kok_point_t topleft, kok_point_t bottomright, byte val, int *fullcount);
int                 reduce_polygon(kok_point_t* points, int pointcount, int newpointcount);
inline int          line_intersection(kok_point_t p1, kok_point_t p2, kok_point_t p3, kok_point_t p4, kok_point_t* i);

void                instrumentation_start();
double              instrumentation_end();

#endif
