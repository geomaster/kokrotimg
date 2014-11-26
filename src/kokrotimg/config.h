#ifndef __KOKROTIMG_CONFIG_H__
#define __KOKROTIMG_CONFIG_H__

#define MAX_FINDER_CANDIDATES       40000
#define MAX_FINDER_POLYGON_SIZE     1000
#define MAX_FINDER_PATTERNS         75
#define MAX_QR_VERSION              40
#define MAX_QR_CODES                16
#define MAX_QR_CANDIDATES           256
#define MAX_QR_SIZE                 177
#define MAX_ALIGNMENT_PATTERNS      8
#define MAX_REGIONS                 2000000
#define QR_BASESIZE                 384
#define MAX_TIMING_SCANLINES        35

#define INSTRUMENTATION_ENABLED

#define SAUVOLA_WINDOW_SIZE(sz)               (9 + (sz) / 32000) //(sz / 12000)
#define SAUVOLA_WINDOW_SIZE_SQ(sz)            (SAUVOLA_WINDOW_SIZE(sz) * SAUVOLA_WINDOW_SIZE(sz))
#define SAUVOLA_K                             0.06
#define SAUVOLA_R                             128
#define THRESHOLD_BIAS                        0 // -2.0

#define FINDER_LOCATION_KERNEL_SIZE           5
#define FINDER_LOCATION_KERNEL_CENTER          (FINDER_LOCATION_KERNEL_SIZE / 2)
#define FINDER_LOCATION_BLACK_THRESHOLD       127
#define FINDER_LOCATION_MIN_STRIP_WIDTH       3
#define FINDER_LOCATION_WIDTH_GLOBAL_TOLERANCE(d2)    ((d2) / 526000)
#define FINDER_LOCATION_WIDTH_LOCAL_TOLERANCE(w)      (2 + 8 * (w) / 10)
#define FINDER_LOCATION_WIDTH_TOLERANCE(w, d2)        (FINDER_LOCATION_WIDTH_GLOBAL_TOLERANCE(d2) * 4 / 10 + FINDER_LOCATION_WIDTH_LOCAL_TOLERANCE(w) * 6 / 10)
#define FINDER_LOCATION_MIN_CANDIDATE_WIDTH(a)        (2 + (a) / 200000)

#define QR_BINARIZATION_WINDOW                  75
#define TIMING_PATTERN_SCAN_THRESHOLD               127
#define ALIGNMENT_PATTERN_SEARCH_WINDOW_MODULES     7
#define ALIGNMENT_PATTERN_SEARCH_WINDOW(qr_ver)     (ALIGNMENT_PATTERN_SEARCH_WINDOW_MODULES * QR_BASESIZE / (17 + 4 * (qr_ver)))

#define SCORE_INNER_SUM_WEIGHT          150
#define SCORE_MIDDLE_SUM_WEIGHT         200
#define SCORE_OUTER_SUM_WEIGHT          340


#define FINDER_LOCATION_WIDTH_EQUAL(a, b, d2) (absdiff(a, b) < FINDER_LOCATION_WIDTH_TOLERANCE(b + a, d2) / 2)
#define FINDER_LOCATION_SATISFIED_PATTERN(b0w, b1w, b2w, b3w, b4w, w, h) \
    (FINDER_LOCATION_WIDTH_EQUAL(b0w, b4w, w*h) && \
     FINDER_LOCATION_WIDTH_EQUAL(b1w, b3w, w*h) && \
     FINDER_LOCATION_WIDTH_EQUAL((b0w + b1w + b3w + b4w) * 3 / 4, b2w, w*h) && \
     (b0w + b1w + b3w + b4w) >= FINDER_LOCATION_MIN_CANDIDATE_WIDTH(w * h))

#define IGNORABLE_PIXELS                   7
#define QR_PATTERN_WIDTH_TOLERANCE(a, b)   (4 * (a + b) / 8)
#define QR_PATTERN_WIDTH_EQUAL(a, b)       (absdiff(a, b) < QR_PATTERN_WIDTH_TOLERANCE(a, b))



#endif

