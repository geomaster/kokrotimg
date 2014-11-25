#ifndef __KOKROTIMG_DEBUG_H__
#define __KOKROTIMG_DEBUG_H__

typedef int kok_debug_class,
            kok_metric_type;

typedef struct kok_debug_sink_t {
    void *callback_param;

    /* (message, param) */
    void (* debug_message)          (const char*, void*);

    /* (new width, new height, param) */
    void (* debug_resize_canvas)    (dimension, dimension, void*);

    /* (row-major matrix of canvas w*canvas h, debug string or NULL, class, param) */
    void (* debug_add_backdrop)     (const byte*, const char*, kok_debug_class, void*);

    /* (point x, point y, debug string or NULL, class, param) */
    void (* debug_add_point)        (dimension, dimension, const char*, kok_debug_class, void*);

    /* (point1 x, point1 y, point2 x, point2 y, debug string or NULL, class, param) */
    void (* debug_add_line)         (dimension, dimension, dimension, dimension, const char*, kok_debug_class, void*);

    /* (array of point x's, array of point y's, point count, fill (true/false), debug string or NULL, class, param) */
    void (* debug_add_polygon)      (const dimension*, const dimension*, int, int, const char*, kok_debug_class, void*);

    /* (time taken in ms, total allocated memory in bytes, type, param) */
    void (* debug_record_metric)    (double, int, kok_metric_type, void*);
} kok_debug_sink_t;

#define KOKROTDBG_CLASS_MACRO_BASE                                1000

#define KOKROTDBG_CLASS_MACRO_FINDER_CANDIDATE                    1101
#define KOKROTDBG_CLASS_MACRO_INITIAL_FINDER_INNER_SQUARE         1102
#define KOKROTDBG_CLASS_MACRO_INITIAL_FINDER_OUTER_SQUARE         1103
#define KOKROTDBG_CLASS_MACRO_INITIAL_FINDER_OUTER_SQUARE_2       1104
#define KOKROTDBG_CLASS_MACRO_FINDER_PATTERN_QUAD                 1106
#define KOKROTBDG_CLASS_MACRO_FINDER_PATTERN_POLYGON              1107
#define KOKROTDBG_CLASS_MACRO_QR_CODE_PENTAGON                    1108
#define KOKROTDBG_CLASS_MACRO_QR_CODE_QUAD                        1109
#define KOKROTDBG_CLASS_MACRO_SMALL_BFS_STARTPOINT                1110

#define KOKROTDBG_CLASS_MACRO_ORIGINAL_IMAGE                      1202
#define KOKROTDBG_CLASS_MACRO_BINARIZED_IMAGE                     1201

#define KOKROTDBG_CLASS_MICRO_BASE                                2000

#define KOKROTDBG_CLASS_MICRO_FINDER_CENTERS                      2101
#define KOKROTDBG_CLASS_MICRO_TIMING_PATTERN_MAIN_LINE            2102
#define KOKROTDBG_CLASS_MICRO_CONJECTURED_AP_CENTERS              2103
#define KOKROTDBG_CLASS_MICRO_REFINED_AP_CENTERS                  2104
#define KOKROTDBG_CLASS_MICRO_FINAL_AP_CENTERS                    2105
#define KOKROTDBG_CLASS_MICRO_MODULE_CENTERS                      2106
#define KOKROTDBG_CLASS_MICRO_MODULE_BOXES                        2107

#define KOKROTDBG_CLASS_MICRO_QR_CODE_IMAGE                       2208
#define KOKROTDBG_CLASS_MICRO_BINARIZED_QR_CODE_IMAGE             2209
#define KOKROTDBG_CLASS_MICRO_REORIENTED_QR_CODE_IMAGE            2210

#define KOKROTDBG_METRIC_MACRO_BASE                               1000

#define KOKROTDBG_METRIC_MACRO_CUMULATIVE_TABLE_COMPUTATION       1001
#define KOKROTDBG_METRIC_MACRO_BINARIZATION                       1002
#define KOKROTDBG_METRIC_MACRO_FINDER_CANDIDATE_SEARCH            1003
#define KOKROTDBG_METRIC_MACRO_FINDER_SQUARE_PROCESSING           1005
#define KOKROTDBG_METRIC_MACRO_CODE_LOCATION_HYPOTHESIZATION      1006
#define KOKROTDBG_METRIC_MACRO_ALLOCATION_OVERHEAD                1007

#define KOKROTDBG_METRIC_MICRO_BASE                               2000

#define KOKROTDBG_METRIC_MICRO_CODE_PROJECTION                    2001
#define KOKROTDBG_METRIC_MICRO_CODE_BINARIZATION                  2002
#define KOKROTDBG_METRIC_MICRO_SAMPLING_GRID_BUILD                2003
#define KOKROTDBG_METRIC_MICRO_FINAL_READ                         2004

#endif
