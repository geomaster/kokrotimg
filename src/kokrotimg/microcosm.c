#include "microcosm.h"
#include <math.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

/* TODO: check the vailidity of these, too lazy to recheck */
const int ALIGNMENT_PATTERN_COORDINATES[ MAX_QR_VERSION ][ 1 + MAX_ALIGNMENT_PATTERNS ] = {
    /* v1 */  {  2,      6,      14,    -1,     -1,     -1,     -1,     -1     },
    /* Note for v1: v1's don't actually have alignment patterns. However, we can
     * pretend they do, conjecture their location and then just not search for the
     * real location later. This elegant approach simplifies this from a completely
     * special case to just one if block. */

    /* v2 */  {  2,      6,      18,    -1,     -1,     -1,     -1,     -1     },
    /* v3 */  {  2,      6,      22,    -1,     -1,     -1,     -1,     -1     },
    /* v4 */  {  2,      6,      26,    -1,     -1,     -1,     -1,     -1     },
    /* v5 */  {  2,      6,      30,    -1,     -1,     -1,     -1,     -1     },
    /* v6 */  {  2,      6,      34,    -1,     -1,     -1,     -1,     -1     },
    /* v7 */  {  3,      6,      22,     38,    -1,     -1,     -1,     -1     },
    /* v8 */  {  3,      6,      24,     42,    -1,     -1,     -1,     -1     },
    /* v9 */  {  3,      6,      26,     46,    -1,     -1,     -1,     -1     },
    /* v10 */ {  3,      6,      28,     50,    -1,     -1,     -1,     -1     },
    /* v11 */ {  3,      6,      30,     54,    -1,     -1,     -1,     -1     },
    /* v12 */ {  3,      6,      32,     58,    -1,     -1,     -1,     -1     },
    /* v13 */ {  3,      6,      34,     62,    -1,     -1,     -1,     -1     },
    /* v14 */ {  4,      6,      26,     46,     66,    -1,     -1,     -1     },
    /* v15 */ {  4,      6,      26,     48,     70,    -1,     -1,     -1     },
    /* v16 */ {  4,      6,      26,     50,     74,    -1,     -1,     -1     },
    /* v17 */ {  4,      6,      30,     54,     78,    -1,     -1,     -1     },
    /* v18 */ {  4,      6,      30,     56,     82,    -1,     -1,     -1     },
    /* v19 */ {  4,      6,      30,     58,     86,    -1,     -1,     -1     },
    /* v20 */ {  4,      6,      34,     62,     90,    -1,     -1,     -1     },
    /* v21 */ {  5,      6,      28,     50,     72,     94,    -1,     -1     },
    /* v22 */ {  5,      6,      26,     50,     74,     98,    -1,     -1     },
    /* v23 */ {  5,      6,      30,     54,     78,     102,   -1,     -1     },
    /* v24 */ {  5,      6,      28,     54,     80,     106,   -1,     -1     },
    /* v25 */ {  5,      6,      32,     58,     84,     110,   -1,     -1     },
    /* v26 */ {  5,      6,      30,     58,     86,     114,   -1,     -1     },
    /* v27 */ {  5,      6,      34,     62,     90,     188,   -1,     -1     },
    /* v28 */ {  6,      6,      26,     50,     74,     98,     122,   -1     },
    /* v29 */ {  6,      6,      30,     54,     78,     102,    126,   -1     },
    /* v30 */ {  6,      6,      26,     52,     78,     104,    130,   -1     },
    /* v31 */ {  6,      6,      30,     56,     82,     108,    134,   -1     },
    /* v32 */ {  6,      6,      34,     60,     86,     112,    138,   -1     },
    /* v33 */ {  6,      6,      30,     58,     86,     114,    142,   -1     },
    /* v34 */ {  6,      6,      34,     62,     90,     118,    146,   -1     },
    /* v35 */ {  7,      6,      30,     54,     78,     102,    126,    150   },
    /* v36 */ {  7,      6,      24,     50,     76,     102,    128,    154   },
    /* v37 */ {  7,      6,      28,     54,     80,     106,    132,    158   },
    /* v38 */ {  7,      6,      32,     58,     84,     110,    136,    162   },
    /* v39 */ {  7,      6,      26,     54,     82,     110,    138,    166   },
    /* v40 */ {  7,      6,      30,     58,     86,     114,    142,    170   }
};

#define DEBUG_LINE(k, p1, p2, s, class) \
    do { \
        k->dbg_sink->debug_add_line(p1.x, p1.y, p2.x, p2.y, (s), class, k->dbg_sink->callback_param); \
    } while(0)

#define DEBUG_POINT(k, p, s, class) \
    do { \
        k->dbg_sink->debug_add_point(p.x, p.y, (s), class, k->dbg_sink->callback_param); \
    } while(0)

/* TODO: Why is this so elaborate?! */
#define ROTXY90(p)                         \
    { \
        __typeof__ (p) _p = (p); \
        __typeof__ (p.x) _x = _p.x; \
        __typeof__ (p.y) _y = _p.y; \
        __typeof__ (p.x) tmp = _y; \
        _y = _x; \
        _x = QR_BASESIZE - tmp - 1; \
        p.x = _x; \
        p.y = _y; \
    }

void project_code(kok_data_t* k, kok_quad_t code, int* pxsum)
{
    /* don't ask me. i derived this on paper and don't know to repeat
     * the process.
     *
     * l3 and l4 are lines that represent two (originally) parallel
     * lines of the code. with some fp math, we get equal slices of them,
     * piece by piece, and them plot a line between them, slicing through
     * that line to create the second dimension. */
    kok_point_t l3 = { code.p3.x - code.p2.x, code.p3.y - code.p2.y },
                l4 = { code.p4.x - code.p1.x, code.p4.y - code.p1.y };

    double      l1step = 1.0 / QR_BASESIZE,
                l3ystep = (double)l3.y / QR_BASESIZE,
                l3xstep = (double)l3.x / QR_BASESIZE,
                l4ystep = (double)l4.y / QR_BASESIZE,
                l4xstep = (double)l4.x / QR_BASESIZE;

    double xl3 = code.p2.x, xl4 = code.p1.x, yl3, yl4;
    int xi = 0, yi = 0;

    /* the loop works fine */
    for (yl3 = code.p2.y, yl4 = code.p1.y; ; yl3 += l3ystep, yl4 += l4ystep, xl3 += l3xstep, xl4 += l4xstep) {
        double alpha = 0.0;

        xi = 0;
        for (; alpha < 1.0; alpha += l1step) {
            double x, y;

            /* lerp between (xl4, yl4) and (xl3, yl3) */
            x = (1 - alpha) * xl4 + alpha * xl3;
            y = (1 - alpha) * yl4 + alpha * yl3;

            /* sanity checks */
            if (xi >= 0 && xi < QR_BASESIZE && yi >= 0 && yi < QR_BASESIZE) {
                /* do standard bilinear interpolation, with sanity checks */
                k->qr_code[yi][xi] = bilinear_interpolation(x, y, k->src_img_w, k->src_img_h, 
                        k->src_img);
            }
            ++xi;
            if (xi >= QR_BASESIZE)
                break;
        }

        ++yi;
        if (yi >= QR_BASESIZE)
            break;
    }
}


void build_qr_ct(kok_data_t* k)
{
    int x, y;
    const int w = QR_BASESIZE;

    /* first column and first row of the cumulative table */
    for (x = 1; x < w; ++x) {
        k->qr_ct[0][x] = k->qr_ct[0][x - 1] + k->qr_code[0][x];
        k->qr_ct[x][0] = k->qr_ct[x - 1][0] + k->qr_code[x][0];
    }

    /* populate the cumulative table */
    for (y = 0; y < w; ++y) {
        for (x = 0; x < w; ++x) {
            k->qr_ct[y][x] = k->qr_ct[y][x - 1] + k->qr_ct[y - 1][x] - k->qr_ct[y - 1][x - 1] + k->qr_code[y][x];
        }
    }
}

void binarize_code(kok_data_t* k)
{
    /* we employ the same technique, but with a smaller window size */
    int x, y;
    const int d = QR_BINARIZATION_WINDOW, d2 = d * d, dh = d / 2,
          w = QR_BASESIZE;

    /* we need the cumulative table for rapid lookups */
    build_qr_ct(k);
    
    for (y = 0; y < w; ++y) {
        for (x = 0; x < w; ++x) {
            int mx = x, my = y;
            if (mx < dh) mx = dh;
            else if (mx >= w - dh) mx = w - dh - 1;

            if (my < dh) my = dh;
            else if (my >= w - dh) my = w - dh - 1;

            /* find the sum using the table */
            int sum = k->qr_ct[my - dh][mx - dh] + k->qr_ct[my + dh][mx + dh] -
                (k->qr_ct[my - dh][mx + dh] + k->qr_ct[my + dh][mx - dh]);

            /* simple thresholding based on the mean */
            int threshold = sum / d2;
            k->qr_code[y][x] = (k->qr_code[y][x] > threshold ? 255 : 0);
        }
    }
}

void find_finder_patterns_in_code(kok_data_t* k)
{
    /* find the module width in qr space */
    double modulew = QR_BASESIZE / (double)k->decoded_qr_dimension;

    /* we plot a first guess at where, from the border of the image, the 
     * finder may be located */
    int finderbaseline = (int)round(modulew * 3.5),
        invbaseline = QR_BASESIZE - finderbaseline,
        upperbound = QR_BASESIZE / 2;

    /* set start x and y values for each quadrant in which we will search for a finder */
    const int startx[4] = { 0, QR_BASESIZE - 1, 0, QR_BASESIZE - 1 },
          starty[4] = { finderbaseline, finderbaseline, invbaseline, invbaseline },
          dx    [4] = { 1, -1, 1, -1 };

    int quadrant, hasfinder[4] = { 0, 0, 0, 0 };

    /* this will contain the finder inner square centers for each quadrant,
     * if we detect them */
    kok_point_t finders[4] = {
        { 0, 0 },
        { 0, 0 },
        { 0, 0 }
    };

    for (quadrant = 0; quadrant < 4; ++quadrant) {
        int sx = startx[quadrant], bsy = starty[quadrant], 
            ex = upperbound, qdx = dx[quadrant];

        /* start from starty - modulew and work your way to start + 2 * modulew 
         * by means of 6 increments of modulew / 2 */
        int sy = bsy - modulew, c;
        for (c = 0; c < 6; ++c) {
            /* do the same strip approach, only a little simpler */
            int pixelstrips[ QR_BASESIZE ], pxstn = 0,
                x = sx, prevpx = 255; /* so we don't register the first whitespace */
            while (x != ex) {
                if (k->qr_code[sy][x] != prevpx)
                    pixelstrips[pxstn++] = x;
                prevpx = k->qr_code[sy][x];
                x += qdx;
            }
            pixelstrips[pxstn++] = ex;

            if (pxstn >= 6) {
                /* compute the widths of the components */
                int p0 = abs(pixelstrips[1] - pixelstrips[0]),
                    p1 = abs(pixelstrips[2] - pixelstrips[1]),
                    p2 = abs(pixelstrips[3] - pixelstrips[2]),
                    p3 = abs(pixelstrips[4] - pixelstrips[3]),
                    p4 = abs(pixelstrips[5] - pixelstrips[4]);

                /* TODO: Remove these hardcoded constants */

                /* test if the widhts correspond to a 1:1:3:1:1 pattern */
                if (QR_PATTERN_WIDTH_EQUAL(5 * (p0 + p4) / 2, (p2)) && 
                        QR_PATTERN_WIDTH_EQUAL(p1, p3) && 
                        QR_PATTERN_WIDTH_EQUAL(p2, (int)round(3 * modulew)) &&
                        sx - pixelstrips[0] <= IGNORABLE_PIXELS) {
                    /* vote that this finder has a quadrant */
                    ++hasfinder[quadrant];

                    /* approximate the center */
                    int centerx;
                    if (qdx < 0)
                        centerx = pixelstrips[2] - p2 / 2;
                    else 
                        centerx = pixelstrips[2] + p2 / 2;

                    /* add to the x and y of the center, we'll divide this 
                     * later to get the average */
                    finders[quadrant].x += centerx;
                    finders[quadrant].y += sy;
                }
            }

            /* advance the scanline */
            sy += modulew / 2;
        }

        /* get the average x and y for the center */
        if (hasfinder[quadrant]) {
            finders[quadrant].x /= hasfinder[quadrant];
            finders[quadrant].y /= hasfinder[quadrant];
        }
    }


    /* stupidsort the quadrants by the number of votes */
    int qua2 = 0;
    int quadrant_order[] = { 0, 1, 2, 3 };
    for (quadrant = 0; quadrant < 4; ++quadrant) {
        for (qua2 = quadrant + 1; qua2 < 4; ++qua2) {
            if (hasfinder[quadrant] < hasfinder[qua2]) {
                swap(hasfinder[quadrant], hasfinder[qua2]);
                swap(quadrant_order[quadrant], quadrant_order[qua2]);
            }
        }
    }

    /* conclude which of the quadrants has a missing finder, this will
     * be useful to the reorientation code */
    int findermissing[4] = { 1, 1, 1, 1 };
    for (quadrant = 0; quadrant < 3; ++quadrant)
        findermissing[ quadrant_order[quadrant] ] = 0;

    int missingfinder = -1;
    for (quadrant = 0; quadrant < 4; ++quadrant) {
        if (findermissing[ quadrant ]) {
            missingfinder = quadrant;
            break;
        }
    }

    /* copy to the shared state */
    k->qr_missing_quadrant = missingfinder;
    memcpy(k->pattern_centers, finders, sizeof(kok_point_t) * 4);
}

void fix_code_orientation(kok_data_t* k)
{
    kok_point_t* finders = k->pattern_centers;
    int rotationcount = 0;

    /* this is intended to rustle your jimmies */
    switch (k->qr_missing_quadrant) {
        case 0:
            rotationcount = 2;
            break;

        case 1:
            rotationcount = 1; 
            break;

        case 2:
            rotationcount = 3;
            break;

        case 3:
        default:
            break;
    }

    while (rotationcount--) {
        /* rotate everything by 90 degrees, including the finder location quadrants
         * and their dimensions */
        rot90(&k->qr_code[0][0], QR_BASESIZE);

        finders[1] = finders[0];
        finders[0] = finders[2];
        finders[2] = finders[3];

        ROTXY90( finders[0] );
        ROTXY90( finders[1] );
        ROTXY90( finders[2] );
    }

    /* FIXME: remove debug cruft */
    /* for (yy = 0; yy < QR_BASESIZE; ++yy) { */
    /*     for (xx = 0; xx < QR_BASESIZE; ++xx) { */
    /*         /1* k->qr_code_dbg[ yy ][ xx ] = *1/ */ 
    /*             /1* k->qr_code[ yy ][ xx ] > 0 ? dbg_white : dbg_black; *1/ */
    /*     } */
    /* } */

    /* reuse qr_ct to store the cumulative table of the binarized code */
    build_qr_ct(k);

}

int parse_timing_patterns(kok_data_t* k)
{
    /* expand from the pattern centers upwards, downwards and in both sides
     * in hope of collecting an accurate measurement of local module dimensions
     * in order to locate a timing pattern main line */

    int findersizes[3][4], i;
    kok_point_t *finders = k->pattern_centers;

    int xl, xr, ya, yb;
    for (i = 0; i < 3; ++i) {
        infect_xy(&k->qr_code[0][0], finders[i], QR_BASESIZE, QR_BASESIZE, &xl, 
                &ya, &xr, &yb);

        findersizes[i][0] = xl;
        findersizes[i][1] = xr;
        findersizes[i][2] = ya;
        findersizes[i][3] = yb;
    }


    /**************************
      ======A         =======
      = === A         = === =
      = === A         = === =
      = === A         = === =
      BBBBBBBBBBBBCCCCCCCCCCC
            A
            A
            Z
            Z
      ======Z
      = === Z
      = === Z
      = === Z
      ======Z
    *************************/

    int f0blackx, f0whitex, f0blacky, f0whitey,
        f1blacky, f1whitey,
        f2blackx, f2whitex;

    black_white_module_size(k, finders[0].x, finders[0].y,
            1, 0, &f0blackx, &f0whitex);

    black_white_module_size(k, finders[0].x, finders[0].y,
            0, 1, &f0blacky, &f0whitey);

    black_white_module_size(k, finders[1].x, finders[1].y,
            0, 1, &f1blacky, &f1whitey);

    black_white_module_size(k, finders[2].x, finders[2].y,
            1, 0, &f2blackx, &f2whitex);

    int    By0 = f0whitey + finders[0].y + findersizes[0][3],
           By1 = By0 + f0blacky,
           Bx  = finders[0].x + findersizes[0][1] + (findersizes[0][1] + findersizes[0][0]) * 3 / 6,
           Cy0 = f1whitey + finders[1].y + findersizes[1][3],
           Cy1 = Cy0 + f1blacky,
           Cx  = finders[1].x - findersizes[1][0] - (findersizes[1][1] + findersizes[1][0]) * 3 / 6,

           Ax0 = f0whitex + finders[0].x + findersizes[0][1],
           Ax1 = Ax0 + f0blackx,
           Ay  = finders[0].y + findersizes[0][3] + (findersizes[0][2] + findersizes[0][3]) * 3 / 6,
           Zx0 = f2whitex + finders[2].x + findersizes[2][1],
           Zx1 = Zx0 + f2blackx,
           Zy  = finders[2].y - findersizes[2][2] - (findersizes[2][2] + findersizes[2][3]) * 3 / 6;

    if (k->dbg_sink) {
        kok_point_t B0 = { Bx, round(By0) }, C0 = { Cx, round(Cy0) },
                    A0 = { round(Ax0), Ay }, Z0 = { round(Zx0), Zy },
                    B1 = { Bx, round(By1) }, C1 = { Cx, round(Cy1) },
                    A1 = { round(Ax1), Ay }, Z1 = { round(Zx1), Zy };

        DEBUG_LINE(k, B0, C0, "X timing pattern line 1", KOKROTDBG_CLASS_MICRO_TIMING_PATTERN_LINES);
        DEBUG_LINE(k, B1, C1, "X timing pattern line 2", KOKROTDBG_CLASS_MICRO_TIMING_PATTERN_LINES);
        DEBUG_LINE(k, A0, Z0, "Y timing pattern line 1", KOKROTDBG_CLASS_MICRO_TIMING_PATTERN_LINES);
        DEBUG_LINE(k, A1, Z1, "Y timing pattern line 2", KOKROTDBG_CLASS_MICRO_TIMING_PATTERN_LINES);
    }

    int modulepositions[ 2 ][ MAX_TIMING_SCANLINES ][ MAX_QR_SIZE ];
    kok_point_t findings[ MAX_QR_SIZE ];

    int modulecounts[ 2 ][ MAX_QR_SIZE ], scanlinesx = 0, scanlinesy = 0;

    int initialdimension = k->decoded_qr_dimension;
    double step = 1.0 / ((By1 - By0 + Cy1 - Cy0 + Ax1 - Ax0 + Zx1 - Zx0) / 2), alpha;
    for (alpha = 0.0; alpha <= 1.0; alpha += step) {
        int count;

        double y0 = (By1 * alpha + By0 * (1 - alpha)),
               y1 = (Cy1 * alpha + Cy0 * (1 - alpha)),
               ystep = (y1 - y0) / (Cx - Bx);

        timing_pattern_scan(Bx, y0, Cx, y1, 1.0, ystep, findings, &count, 
                MAX_QR_SIZE - 14, &k->qr_code[0][0]);

        if (count >= initialdimension - 14 - 8 + 1 && count <= initialdimension - 14 + 8 + 1) {
            modulecounts[0][scanlinesx] = count;
            for (i = 0; i < count; ++i)
                modulepositions[0][scanlinesx][i] = findings[i].x;
            ++scanlinesx;
        } else { LOGF("bad count x %d", count); }

        double x0 = (Ax1 * alpha + Ax0 * (1 - alpha)),
               x1 = (Zx1 * alpha + Zx0 * (1 - alpha)),
               xstep = (x1 - x0) / (Zy - Ay);

        timing_pattern_scan(x0, Ay, x1, Zy, xstep, 1.0, findings, &count,
                MAX_QR_SIZE - 14, &k->qr_code[0][0]);

        if (count >= initialdimension - 14 - 8 + 1 && count <= initialdimension - 14 + 8 + 1) {
            modulecounts[1][scanlinesy] = count;
            for (i = 0; i < count; ++i)
                modulepositions[1][scanlinesy][i] = findings[i].y;
            ++scanlinesy;
        } else { LOGF("bad count y %d", count); }

        if (scanlinesx >= MAX_TIMING_SCANLINES || scanlinesy >= MAX_TIMING_SCANLINES)
            break;

    }

    int refineddimension, direction, scanlines, bestdimension = initialdimension - 14,
        bestvotes = 0;

    scanlines = scanlinesx;
    for (direction = 0; direction < 2; ++direction) {
        for (refineddimension = initialdimension - 14 - 8 + 1; refineddimension <= 
                initialdimension - 14 + 8 + 1; refineddimension += 4) {

            int scanline, votes = 0;
            for (scanline = 0; scanline < scanlines; ++scanline) {
                if (modulecounts[direction][scanline] == refineddimension)
                    ++votes;
            }

            if (votes >= scanlines / 2 && votes > bestvotes) {
                bestvotes = votes;
                bestdimension = refineddimension;
            }
        }
        scanlines = scanlinesy;
    }

    if (bestdimension + 14 - 1 < 21) {
        LOGF("Detected dimension (%d) too small, cannot work with this image", bestdimension + 14 - 1);
        return(0);
    }

    k->decoded_qr_dimension = bestdimension + 14 - 1;
    k->decoded_qr_version = (k->decoded_qr_dimension - 21) / 4 + 1;

    /* pick a fitting scanline with minimum variance (and hence standard deviation) */
    scanlines = scanlinesx;
    for (direction = 0; direction < 2; ++direction) {
        double minvar = -1.0;
        int scanline, mini = -1;
        for (scanline = 0; scanline < scanlines; ++scanline) {
            int modulecount = modulecounts[direction][scanline], sumofterms = 0, 
                sumofsquares = 0;

            if (modulecount != bestdimension)
                continue;

            for (i = 1; i < modulecount; ++i) {
                int thismodulesz = modulepositions[direction][scanline][i] -
                    modulepositions[direction][scanline][i - 1];

                sumofterms += thismodulesz;
                sumofsquares += thismodulesz * thismodulesz;
            }

            /* printf("sumofsquares is %d and sumofterms is %d\n", sumofsquares, sumofterms); */
            /* variance = E[X^2] - (E[X])^2 */
            int modsizecount = modulecount - 1;
            double variance = (double)(sumofsquares) / modsizecount  - (double)(sumofterms *
                    sumofterms) / (modsizecount * modsizecount);
            if (mini < 0 || variance < minvar) {
                mini = scanline;
                minvar = variance;
                /* printf("new variance %f\n", variance); */
            }
        }

        if (mini < 0) {
            return(0); /* this should happen very rarely */
        }

        if (direction == 0) {
            memcpy(k->column_start_positions + 7, &modulepositions[direction][mini][0],
                    modulecounts[direction][mini] * sizeof(int));
        } else if (direction == 1) {
            memcpy(k->row_start_positions + 7, &modulepositions[direction][mini][0],
                    modulecounts[direction][mini] * sizeof(int));
        }


        if (k->dbg_sink) {
            if (mini >= 0 && !direction) {
                for (i = 0; i < modulecounts[direction][mini]; ++i) {
                    kok_point_t pa = { .y = 0, .x = modulepositions[direction][mini][i] },
                                pb = { .y = QR_BASESIZE - 1, .x = modulepositions[direction][mini][i] };
                    DEBUG_LINE(k, pa, pb, "X grid line", KOKROTDBG_CLASS_MICRO_FIRST_GRID);
                    /* bresenham_line(&k->qr_code_dbg[0][0], dbg_darkblue, QR_BASESIZE, pa, pb); */
                }
            }
            if (mini >= 0 && direction) {
                for (i = 0; i < modulecounts[direction][mini]; ++i) {
                    kok_point_t pa = { .y = modulepositions[direction][mini][i], .x = 0 },
                                pb = { .y = modulepositions[direction][mini][i], .x = QR_BASESIZE - 1};
                    DEBUG_LINE(k, pa, pb, "Y grid line", KOKROTDBG_CLASS_MICRO_FIRST_GRID);
                    /* bresenham_line(&k->qr_code_dbg[0][0], dbg_darkred, QR_BASESIZE, pa, pb); */
                }

            }
        }
        scanlines = scanlinesy;
    }

    /* TODO: check if this is good! */
    k->horizontal_timing_slope = (double)(Cy1 - By1) / (double)(Cx - Bx);
    k->vertical_timing_slope = (double)(Zx1 - Ax1) / (double)(Zy - Ay) ;
    k->horizontal_timing_start_y = By1;
    k->vertical_timing_start_x = Zx1;

    return(1);
}

void conjecture_alignment_patterns(kok_data_t* k)
{
    /* conjecture the locations first, according to a known criteria */
    kok_alignpat_t aligns[ MAX_ALIGNMENT_PATTERNS ][ MAX_ALIGNMENT_PATTERNS ];
    int dim = k->decoded_qr_dimension;

    /* since we don't know the start of this row and column (but we need them), we'll just
     * make a guess based on the previous row and column size */
    k->column_start_positions[6] = 2 * k->column_start_positions[7] - k->column_start_positions[8];
    k->row_start_positions[6] = 2 * k->row_start_positions[7] - k->row_start_positions[8];

    k->column_start_positions[5] = 2 * k->column_start_positions[6] - k->column_start_positions[7];
    k->row_start_positions[5] = 2 * k->row_start_positions[6] - k->row_start_positions[7];

    k->column_start_positions[dim - 6] = 2 * k->column_start_positions[dim - 8] - k->column_start_positions[dim - 7];
    k->row_start_positions[dim - 6] = 2 * k->row_start_positions[dim - 8] - k->row_start_positions[dim - 7];


#   define ADD_ALIGNMENT_PATTERN(px, py, id_x, id_y) \
    do { \
        kok_alignpat_t align; \
        align.qrspace_pos.x = (px); \
        align.qrspace_pos.y = (py); \
        align.active = 0; \
        aligns[ id_y ][ id_x ] = align; \
        \
        double imgx, imgy; \
        if (((px) <= 7 && (py) <= 7) || ((px) >= dim - 7 && (py) <= 7)) \
            break; \
        else if ((px) <= 7 && (py) >= dim - 7) \
            break; \
        \
        imgx = (3 * k->column_start_positions[(px)] / 2 - \
                       k->column_start_positions[(px) - 1] / 2) + k->vertical_timing_slope * \
                      (k->row_start_positions[(py)] - k->horizontal_timing_start_y), \
        imgy = (3 * k->row_start_positions[(py)] / 2 - \
                       k->row_start_positions[(py) - 1] / 2) + k->horizontal_timing_slope * \
                      (k->column_start_positions[(px)] - k->vertical_timing_start_x); \
        \
        align.imgspace_pos.x = imgx; \
        align.imgspace_pos.y = imgy; \
        align.active = 1; \
        \
        aligns[ id_y ][ id_x ] = align; \
        /* if(imgx>=0.0&&imgy>=0.0&&imgx<k->src_img_w&&imgy<k->src_img_h)k->qr_code_dbg[ (int)imgy ][ (int)imgx ] = dbg_red; \ */ \
    } while (0)

    const int *aligncoords = &ALIGNMENT_PATTERN_COORDINATES[ k->decoded_qr_version - 1 ][ 1 ],
              aligncoordsz = *(aligncoords - 1);

    printf("WHAT AM I SEEING? %d\n", ALIGNMENT_PATTERN_COORDINATES[ 4 ][ 2 ]);
    int i, j;
    for (i = 0; i < aligncoordsz; ++i) {
        for (j = 0; j < aligncoordsz; ++j) {
            printf("%d %d\n",aligncoords[i],aligncoords[j]);
            ADD_ALIGNMENT_PATTERN(aligncoords[i], aligncoords[j], i, j);
            k->alignment_patterns[j][i] = aligns[j][i];
        }
    }

    k->alignment_patterns_1D = aligncoordsz;
}

void find_alignment_patterns(kok_data_t* k)
{
    /* go through each alignment pattern conjecture, search in a predefined area around it
     * for the most likely position, refine the guess */
    if (k->decoded_qr_version == 1) 
        return; /* see comment at the top of the file */

    const int *aligncoords = &ALIGNMENT_PATTERN_COORDINATES[ k->decoded_qr_version - 1 ][ 1 ],
              aligncoordsz = *(aligncoords - 1);

    int i, j;
    for (i = 0; i < aligncoordsz; ++i) {
        for (j = 0; j < aligncoordsz; ++j) {
            if (!k->alignment_patterns[j][i].active)
                continue;

            kok_pointf_t oldcenter = k->alignment_patterns[j][i].imgspace_pos;
            kok_point_t newcenter = most_likely_alignment_pattern(k, ptf2pt(oldcenter),
                    ALIGNMENT_PATTERN_SEARCH_WINDOW(k->decoded_qr_version) / 2);

            if (newcenter.x > 0 && newcenter.y > 0) {
                k->alignment_patterns[j][i].imgspace_pos = pt2ptf(newcenter);
                /* k->qr_code_dbg[newcenter.y][newcenter.x] = dbg_green; */
            }
        }
    }
}

int place_virtual_alignment_patterns(kok_data_t* k)
{
    int sz = k->alignment_patterns_1D;
    kok_alignpat_t pat;

    pat = k->alignment_patterns[0][0];
    pat.imgspace_pos.x = (k->column_start_positions[6] + k->column_start_positions[7]) / 2.0;
    pat.imgspace_pos.y = (k->row_start_positions[6] + k->row_start_positions[7]) / 2.0;
    pat.active = 2;
    /* k->qr_code_dbg[(int)pat.imgspace_pos.y][(int)pat.imgspace_pos.x] = dbg_cyan; */
    k->alignment_patterns[0][0] = pat;

    pat = k->alignment_patterns[0][ sz - 1 ];
    pat.active = 2;
    double newx = (3 * k->column_start_positions[k->decoded_qr_dimension - 7] -
            k->column_start_positions[k->decoded_qr_dimension - 8]) / 2.0;
    double slope = (sz - 3 >= 0 ? get_slope(k->alignment_patterns[0][sz - 3].imgspace_pos, 
                k->alignment_patterns[0][sz - 2].imgspace_pos): k->horizontal_timing_slope);

    kok_pointf_t pbase = k->alignment_patterns[0][sz - 2].imgspace_pos;
    pbase.y = horizontal_slide(pbase, slope, newx);
    pbase.x = newx;
    pat.imgspace_pos = pbase; 
    /* k->qr_code_dbg[(int)round(pbase.y)][(int)round(pbase.x)] = dbg_cyan; */

    k->alignment_patterns[0][sz - 1] = pat;

    pat = k->alignment_patterns[sz - 1][0];
    pat.active = 2;
    double newy = (3 * k->row_start_positions[k->decoded_qr_dimension - 7] -
            k->row_start_positions[k->decoded_qr_dimension - 8]) / 2.0;
    double aslope = (sz - 3 >= 0 ? get_inv_slope(k->alignment_patterns[sz - 3][0].imgspace_pos, 
                k->alignment_patterns[sz - 2][0].imgspace_pos) : k->vertical_timing_slope);

    kok_pointf_t pbasey = k->alignment_patterns[sz - 2][0].imgspace_pos;
    pbasey.x = vertical_slide(pbasey, aslope, newy);
    pbasey.y = newy;
    /* k->qr_code_dbg[(int)round(pbasey.y)][(int)round(pbasey.x)] = dbg_cyan; */

    pat.imgspace_pos = pbasey;

    k->alignment_patterns[sz - 1][ 0 ] = pat;

    /* populate the offgrid virtual alignment patterns, used for reading 
     * data not between any currently populated patterns */
    int i;
    for (i = 0; i < sz; ++i) {
        /* 0 - right, 1 - bottom, 2 - left, 3 - top */
        k->extended_patterns[ 0 ][ i ] = extend_px(k, sz - 1, i);
        k->extended_patterns[ 1 ][ i ] = extend_py(k, i, sz - 1);
        k->extended_patterns[ 2 ][ i ] = extend_mx(k, 0, i);
        k->extended_patterns[ 3 ][ i ] = extend_my(k, i, 0);
    }

    kok_alignpat_t extra;
    extra.qrspace_pos.x = k->extended_patterns[ 0 ][ sz - 1 ].qrspace_pos.x;
    extra.qrspace_pos.y = k->extended_patterns[ 1 ][ sz - 1 ].qrspace_pos.y;

    extra.imgspace_pos.x = k->extended_patterns[ 0 ][ sz - 1 ].imgspace_pos.x;
    extra.imgspace_pos.y = k->extended_patterns[ 1 ][ sz - 1 ].imgspace_pos.y;

    extra.active = 2;

    k->extra_offgrid_pattern = extra;

/*     int ogp_y,ogp_x; */
/*     for (ogp_y=0;ogp_y<4;++ogp_y) */
/*         for(ogp_x=0;ogp_x<2;++ogp_x){ */
/*             kok_point_t p=ptf2pt(k->offgrid_patterns[ogp_y][ogp_x].imgspace_pos); */
/*             int clamp=0; */
/*             if(p.x<0)p.x=0,clamp=1; */
/*             if(p.y<0)p.x=0,clamp=1; */
/*             if(p.x>=QR_BASESIZE-1)p.x=QR_BASESIZE-2,clamp=1; */
/*             if(p.y>=QR_BASESIZE-1)p.y=QR_BASESIZE-2,clamp=1; */
/*             k->qr_code_dbg[p.y][p.x]=(clamp?dbg_darkred:dbg_red); */
/*         } */
/*     kok_alignpat_t q[4]; */
/*     q[0]=k->alignment_patterns[0][0]; */
/*     q[1]=k->alignment_patterns[0][1]; */
/*     q[2]=k->alignment_patterns[1][0]; */
/*     q[3]=k->alignment_patterns[1][1]; */

    return(1);
}

kok_alignpat_t extend_px(kok_data_t* k, int apx, int apy) 
{
    kok_alignpat_t *pat = &k->alignment_patterns[apy][apx];
    kok_pointf_t pt = pat->imgspace_pos;

    double hslope = -get_slope(pat->imgspace_pos, k->alignment_patterns[apy][apx - 1].imgspace_pos);

    int new_qrx = pat->qrspace_pos.x + 6;
    const int maxcol = k->decoded_qr_dimension - 1;
    double new_imgx = pat->imgspace_pos.x + k->column_start_positions[maxcol - 6] - 
        k->column_start_positions[maxcol - 12];

    kok_alignpat_t res = {
        { new_qrx, pat->qrspace_pos.y },
        { new_imgx, horizontal_slide(pt, hslope, new_imgx) },
        3
    };
    /* printf("[EXTENDPX] Placed you at %f %f\n", res.imgspace_pos.x, res.imgspace_pos.y); */

    return res;
}

/*kill me please*/
kok_alignpat_t extend_py(kok_data_t* k, int apx, int apy) 
{
    kok_alignpat_t *pat = &k->alignment_patterns[apy][apx];
    kok_pointf_t pt = pat->imgspace_pos;

    double vslope = -get_inv_slope(pat->imgspace_pos, k->alignment_patterns[apy - 1][apx].imgspace_pos);

    int new_qry = pat->qrspace_pos.y + 6;
    const int maxrow = k->decoded_qr_dimension - 1;
    double new_imgy = pat->imgspace_pos.y + k->row_start_positions[maxrow - 6] - 
        k->row_start_positions[maxrow - 12];

    kok_alignpat_t res = {
        { pat->qrspace_pos.x, new_qry },
        { vertical_slide(pt, vslope, new_imgy), new_imgy },
        3
    };
    /* printf("[EXTENDPY] Placed you at %f %f\n", res.imgspace_pos.x, res.imgspace_pos.y); */

    return res;
}

/*no more*/
kok_alignpat_t extend_mx(kok_data_t* k, int apx, int apy)
{
    kok_alignpat_t *pat = &k->alignment_patterns[apy][apx];
    kok_pointf_t pt = pat->imgspace_pos;

    double hslope = get_slope(pat->imgspace_pos, k->alignment_patterns[apy][apx + 1].imgspace_pos);

    int new_qrx = pat->qrspace_pos.x - 6;
    double new_imgx = pat->imgspace_pos.x + k->column_start_positions[6] - 
        k->column_start_positions[12];

    kok_alignpat_t res = {
        { new_qrx, pat->qrspace_pos.y },
        { new_imgx, horizontal_slide(pt, hslope, new_imgx) },
        3
    };
    /* printf("[EXTENDMX] Placed you at %f %f\n", res.imgspace_pos.x, res.imgspace_pos.y); */

    return res;

}

/*it's over*/
kok_alignpat_t extend_my(kok_data_t* k, int apx, int apy) 
{
    kok_alignpat_t *pat = &k->alignment_patterns[apy][apx];
    kok_pointf_t pt = pat->imgspace_pos;

    double vslope = get_inv_slope(pat->imgspace_pos, k->alignment_patterns[apy + 1][apx].imgspace_pos);

    int new_qry = pat->qrspace_pos.y - 6;
    double new_imgy = pat->imgspace_pos.y + k->row_start_positions[6] - 
        k->row_start_positions[12];

    kok_alignpat_t res = {
        { pat->qrspace_pos.x, new_qry },
        { vertical_slide(pt, vslope, new_imgy), new_imgy },
        3
    };
    /* printf("[EXTENDMY] Placed you at %f %f\n", res.imgspace_pos.x, res.imgspace_pos.y); */

    return res;
}



#include <assert.h>
int qr_cumulative_sum(kok_data_t* k, kok_point_t a, kok_point_t b)
{
    int psum = (int)k->qr_ct[b.y][b.x] + (int)k->qr_ct[a.y][a.x] - ((int)k->qr_ct[a.y][b.x] + (int)k->qr_ct[b.y][a.x]);
    return psum;
}

#define RECT_AREA(a, b)         (((b).x - (a).x) * ((b).y - (a).y))

int alignment_pattern_score(kok_data_t* k, kok_point_t center/*, int debugflag*/)
{
    double modulesize = (double)QR_BASESIZE / k->decoded_qr_dimension;
    
    kok_pointf_t lefttop = { center.x - 2.5 * modulesize, center.y - 2.5 * modulesize },
                 bottomright = { center.x + 2.5 * modulesize, center.y + 2.5 * modulesize };

    if (lefttop.x < 0.0 || lefttop.y < 0.0 || 
            bottomright.x >= QR_BASESIZE - 2.0 || bottomright.y >= QR_BASESIZE - 2.0) {
        return(INT_MIN);
    }

    kok_pointf_t lefttop2 = { center.x - 1.5 * modulesize, center.y - 1.5 * modulesize },
                 bottomright2 = { center.x + 1.5 * modulesize, center.y + 1.5 * modulesize },

                 lefttop3 = { center.x - 0.5 * modulesize, center.y - 0.5 * modulesize },
                 bottomright3 = { center.x + 0.5 * modulesize, center.y + 0.5 * modulesize };

    int innersum = qr_cumulative_sum(k, ptf2pt(lefttop3), ptf2pt(bottomright3)),
        outersum1 = qr_cumulative_sum(k, ptf2pt(lefttop2), ptf2pt(bottomright2)) - innersum,
        outersum = qr_cumulative_sum(k, ptf2pt(lefttop), ptf2pt(bottomright)) - outersum1 - innersum;

    /* /1* if(debugflag){ *1/ */
    /* bresenham_line(&k->qr_code_dbg[0][0], dbg_blue,QR_BASESIZE, ptf2pt(lefttop), ptf2pt(bottomright)); */
    /* bresenham_line(&k->qr_code_dbg[0][0], dbg_green,QR_BASESIZE,  ptf2pt(lefttop2), ptf2pt(bottomright2)); */
    /* bresenham_line(&k->qr_code_dbg[0][0], dbg_red, QR_BASESIZE, ptf2pt(lefttop3), ptf2pt(bottomright3)); */

    /* printf("For innersum: %f%%\n", 100.0 * innersum / (255 * modulesize * modulesize)); */
    /* printf("For outersum1: %f%%\n", 100.0 * outersum1 / (255 * 8 * modulesize * modulesize)); */
    /* printf("For outersum: %f%%\n", 100.0 * outersum / (255 * 16 * modulesize * modulesize)); */

    /* } */

    return - ((SCORE_INNER_SUM_WEIGHT * (innersum / 255)) / (int)(1 * modulesize * modulesize))
           + ((SCORE_MIDDLE_SUM_WEIGHT * (outersum1 / 255)) / (int)(8 * modulesize * modulesize))
           - ((SCORE_OUTER_SUM_WEIGHT * (outersum / 255)) / (int)(12 * modulesize * modulesize));
}

kok_point_t most_likely_alignment_pattern(kok_data_t* k, kok_point_t wcenter, const int wextent)
{
    int maxscore = INT_MIN, score;
    kok_point_t maxcenter = { -1, -1 };
    
    int x, y;

    /* printf("Searching around (%d, %d) with extent %d\n", wcenter.x, wcenter.y, wextent); */

    for (y = max(wcenter.y - wextent, 0); y <= min(wcenter.y + wextent, QR_BASESIZE - 1); ++y) {
        for (x = max(wcenter.x - wextent, 0); x <= min(wcenter.x + wextent, QR_BASESIZE - 1); ++x) {
            kok_point_t newcenter = { x, y };
            if ((score = alignment_pattern_score(k, newcenter)) > maxscore) {
                /* printf("New max score is %d\n", score); */
                maxscore = score;
                maxcenter = newcenter;
            }
            /* k->qr_code_dbg[y][x] = dbg_yellow; */
        }
    }

    /* printf("Max score is %d\n", maxscore); */
    return maxcenter;
}

void timing_pattern_scan(double x0, double y0, double tx, double ty, double dx, double dy, 
        kok_point_t* out_boundaries, int* out_size, int target_size, byte* buf)

{
    double x = x0, y = y0;
    int collectedsz = 0, prevcol = 0;
    
    while (collectedsz < target_size && (dx == 0.0 || (dx > 0.0 && x <= tx) || (dx < 0.0 
                    && x >= tx)) && (dy == 0.0 || (dy > 0.0 && y <= ty) || (dy < 0.0 &&
                        y >= ty))) {
        int val = (bilinear_interpolation(x, y, QR_BASESIZE, QR_BASESIZE, buf) >
                TIMING_PATTERN_SCAN_THRESHOLD);
        

        if (val != prevcol) {
            out_boundaries[collectedsz].x = (int) round(x);
            out_boundaries[collectedsz].y = (int) round(y);
            ++collectedsz;
        }

        prevcol = val;
        x += dx;
        y += dy;

        if (x >= QR_BASESIZE || x < 0.0 || y >= QR_BASESIZE || y < 0.0)
            break;
    }

    if (prevcol != 0) {
        out_boundaries[collectedsz].x = (int) round(x);
        out_boundaries[collectedsz].y = (int) round(y);
        ++collectedsz;
    }
    *out_size = collectedsz;
}


void decode_qr(kok_data_t* k, int qr_idx)
{
    kok_quad_t fullquad = k->qr_codes[qr_idx];
    int fullarea = abs(SIGNED_AREA(fullquad.p1, fullquad.p2, fullquad.p3)) +
        abs(SIGNED_AREA(fullquad.p2, fullquad.p3, fullquad.p4));

    int i, findersarea = 0;
    for (i = 0; i < 3; ++i) {
        kok_quad_t finder = k->qr_finders[qr_idx][i];
        findersarea += abs(SIGNED_AREA(finder.p1, finder.p2, finder.p3)) +
            abs(SIGNED_AREA(finder.p2, finder.p3, finder.p4));
    }

    double a = 16 * findersarea, b = 168 * findersarea, 
           c = 441 * findersarea - 147 * fullarea; 

    double q = (-b + sqrt(b*b-4*a*c)) / (2*a);
    double modules = 21 + 4 * round(q);
    int imodules = (int) modules;
    if (imodules < 21) imodules = 21;

    k->decoded_qr_dimension = imodules;
    k->decoded_qr_version = (imodules - 21) / 4 + 1;

    LOGF("Guessed version %d (dimension %d)", k->decoded_qr_version, k->decoded_qr_dimension);

}

void read_final_qr(kok_data_t* k)
{
    int aps = k->alignment_patterns_1D;
    int i, j;
    for (i = -1; i < aps; ++i) 
        for (j = -1; j < aps; ++j) {
            if ((i == -1 && j == -1) || /* upper-left corner, not needed */
                (i == -1 && j == aps - 1) || /* upper-right corner */
                (i == aps - 1 && j == -1)) /* lower-left corner */
                continue;

            kok_alignpat_t quad[] = {
                get_alignment_pattern(k, j, i),
                get_alignment_pattern(k, j + 1, i),
                get_alignment_pattern(k, j, i + 1),
                get_alignment_pattern(k, j + 1, i + 1)
            };
            read_final_qr_quad(k, quad);
        }
}

kok_alignpat_t get_alignment_pattern(kok_data_t* k, int idx, int idy)
{
    int aps = k->alignment_patterns_1D;
    if (idx == aps && idy == aps)
        return( k->extra_offgrid_pattern );
    else if (idx == aps && idy < aps)
        return( k->extended_patterns[ 0 ][ idy ] );
    else if (idy == aps && idx < aps)
        return( k->extended_patterns[ 1 ][ idx ] );
    else if (idx == -1 && idy < aps)
        return( k->extended_patterns[ 2 ][ idy ] );
    else if (idy == -1 && idx < aps)
        return( k->extended_patterns[ 3 ][ idx ] );
    else if (idx >= 0 && idy >= 0 && idy < aps && idx < aps)
        return( k->alignment_patterns[ idy ][ idx ] );
    else {
        kok_alignpat_t fail = { { 0, 0 }, { 0, 0 }, -1 };
        ASSERT(0, "Invalid position supplied to get_alignment_pattern()");
        return( fail );
    }
}


void read_final_qr_quad(kok_data_t* k, kok_alignpat_t q[4])
{
    kok_alignpat_t quad[4];
    memcpy(quad, q, sizeof(kok_alignpat_t) * 4);

    double qrmod = 0.5 * (double)QR_BASESIZE / k->decoded_qr_dimension;
    quad[0].imgspace_pos.x -= qrmod;
    quad[0].imgspace_pos.y -= qrmod;

    quad[1].imgspace_pos.x += qrmod;
    quad[1].imgspace_pos.y -= qrmod;

    quad[2].imgspace_pos.x -= qrmod;
    quad[2].imgspace_pos.y += qrmod;

    quad[3].imgspace_pos.x += qrmod;
    quad[3].imgspace_pos.y += qrmod;

    LOGF("Reading QR quad (%d, %d) (%d %d) (%d %d) (%d %d)",
            quad[0].qrspace_pos.x, quad[0].qrspace_pos.y,
            quad[1].qrspace_pos.x, quad[1].qrspace_pos.y,
            quad[2].qrspace_pos.x, quad[2].qrspace_pos.y,
            quad[3].qrspace_pos.x, quad[3].qrspace_pos.y);
    if (k->dbg_sink) {
        static sdimension xs[4], ys[4];
        int i;

        static const int order[] = { 0, 1, 3, 2 };
        for (i = 0; i < 4; ++i) {
            int j = order[i];

            xs[i] = (int)round(q[j].imgspace_pos.x);
            ys[i] = (int)round(q[j].imgspace_pos.y);
        }

        k->dbg_sink->debug_add_polygon(xs, ys, 4, 0, NULL, KOKROTDBG_CLASS_MICRO_FINAL_GRID, k->dbg_sink->callback_param);
    }

    int modulesx = quad[1].qrspace_pos.x - quad[0].qrspace_pos.x + 1,
        modulesy = quad[3].qrspace_pos.y - quad[1].qrspace_pos.y + 1;

    double alphastepx = 1.0 / modulesx,
           alphastepy = 1.0 / modulesy;

    int x, y;
    for (y = 0; y < modulesy; ++y) {
        double alphay = y * alphastepy,
               alphaendy = (y + 1) * alphastepy;

        kok_pointf_t leftspot = lerp(quad[0].imgspace_pos, quad[2].imgspace_pos, alphay),
                     rightspot = lerp(quad[1].imgspace_pos, quad[3].imgspace_pos, alphay),
                     leftendspot = lerp(quad[0].imgspace_pos, quad[2].imgspace_pos, alphaendy),
                     rightendspot = lerp(quad[1].imgspace_pos, quad[3].imgspace_pos,alphaendy);

        for (x = 0; x < modulesx; ++x) {

            double alphax = x * alphastepx, 
                   alphaendx = (x + 1) * alphastepx;

            kok_point_t bounds[] = {
                ptf2pt(lerp(leftspot, rightspot, alphax)),
                ptf2pt(lerp(leftspot, rightspot, alphaendx)),
                ptf2pt(lerp(leftendspot, rightendspot, alphaendx)),
                ptf2pt(lerp(leftendspot, rightendspot, alphax))
            };

            read_quad_module(k, &k->qr_code[0][0], bounds, x + quad[0].qrspace_pos.x, y + quad[0].qrspace_pos.y, &k->final_qr[0][0]);
        }
    }
}


void interpret_final_qr(kok_data_t* k)
{
    /* int i, j, sz = k->decoded_qr_dimension; */
    /* for (i = 0; i < sz; ++i) { */
    /*     for (j = 0; j < sz; ++j) { */
    /*         if (k->final_qr[i][j].hits <= 2 * k->final_qr[i][j].sum) */
    /*             printf("1"); /1* k->decoded_qr[i][j] = 1; *1/ */
    /*         else printf("0");/1*k->decoded_qr[i][j] = 0;*1/ */

    /*     } */
    /*     printf("\n"); */
    /* } */

}

void read_quad_module(kok_data_t* k, byte* ibuf, kok_point_t quad[4], int qrx, int qry, kok_qr_module* buf)
{
    int pxc = 0;
    int sum = polygon_sum(ibuf, QR_BASESIZE, QR_BASESIZE, quad, 4, &pxc);

    sum /= 255;
    sum = pxc - sum;

    buf[M2D(qrx, qry, MAX_QR_SIZE)].hits += pxc;
    buf[M2D(qrx, qry, MAX_QR_SIZE)].sum += sum;

    if (k->dbg_sink) {
        kok_point_t p = { quad[0].x + quad[1].x + quad[2].x + quad[3].x, 
                          quad[0].y + quad[1].y + quad[2].y + quad[3].y, };

        p.x /= 4;
        p.y /= 4;

        DEBUG_POINT(k, p, NULL, KOKROTDBG_CLASS_MICRO_MODULE_CENTERS);
    }
}

void flip_y(byte* buf, dimension w, dimension h)
{
    int i, j;
    for (i = 0; i < h; ++i) {
        for (j = 0; j < w / 2; ++j) {
            swap(buf[M2D(i, j, w)], buf[M2D(i, w - j - 1, w)]);
        }
    }
}

void dump_projected_code(kok_data_t* k)
{
    void* param = k->dbg_sink->callback_param;

    k->dbg_sink->debug_resize_canvas(QR_BASESIZE, QR_BASESIZE, param);
    k->dbg_sink->debug_add_backdrop(&k->qr_code[0][0], "Projected code", KOKROTDBG_CLASS_MICRO_QR_CODE_IMAGE, param);
}

void dump_binarized_code(kok_data_t* k)
{
    void* param = k->dbg_sink->callback_param;
    k->dbg_sink->debug_add_backdrop(&k->qr_code[0][0], "Binarized code", KOKROTDBG_CLASS_MICRO_BINARIZED_QR_CODE_IMAGE, param);
}

void dump_final_qr(kok_data_t* k, byte* outmatrix)
{
    double modw = QR_BASESIZE / (double)k->decoded_qr_dimension;
    int qi, qj;

    memset(&k->qr_code[0][0], 0xff, sizeof(k->qr_code));
    for (qi = 0; qi < k->decoded_qr_dimension; ++qi) {
        for (qj = 0; qj < k->decoded_qr_dimension; ++qj) {
            int starti = floor(qi * modw), endi = ceil((qi + 1) * modw),
                startj = floor(qj * modw), endj = ceil((qj + 1) * modw);

            int i, j;
            byte col = (outmatrix[qj + qi * k->decoded_qr_dimension] == 1 ? 0 : 255);
            for (i = starti; i <= endi; ++i)
                for (j = startj; j <= endj; ++j) {
                    if (i < 0 || j < 0 || i >= QR_BASESIZE - 1 || j >= QR_BASESIZE - 1)
                        continue;

                    k->qr_code[i][j] = col;
                }
        }
    }

    k->dbg_sink->debug_add_backdrop(&k->qr_code[0][0], "Final code", KOKROTDBG_CLASS_MICRO_FINAL_QR_CODE, 
            k->dbg_sink->callback_param);

}

microcosm_err_t kokrotimg_microcosm(kok_data_t* k, int qr_idx, byte* outmatrix)
{
    LOGS("Entering Microcosm");
    kokrot_component = "microcosm";

    LOGS("Projecting code");

    START_INSTRUMENTATION();
    project_code(k, k->qr_codes[qr_idx], NULL);
    RECORD_METRIC(k, KOKROTDBG_METRIC_MICRO_CODE_PROJECTION);

    LOGS("Done projecting code");

    if (k->dbg_sink) {
        LOGS("Dumping projected code");
        dump_projected_code(k);
        LOGS("Done dumping projected code");
    }

    LOGS("Binarizing code");

    START_INSTRUMENTATION();
    binarize_code(k);
    RECORD_METRIC(k, KOKROTDBG_METRIC_MICRO_CODE_BINARIZATION);

    LOGS("Done binarizing code");

    START_INSTRUMENTATION();

    LOGS("Guessing code version");
    decode_qr(k, qr_idx);
    LOGS("Done guessing code version");

    LOGS("Finding finder patterns in code");
    find_finder_patterns_in_code(k);
    LOGS("Done finding finder patterns in code");

    LOGS("Fixing code orientation");
    fix_code_orientation(k);
    LOGS("Done fixing code orientation");

    RECORD_METRIC(k, KOKROTDBG_METRIC_MICRO_CODE_REORIENTATION);

    if (k->dbg_sink) {
        LOGS("Dumping binarized code");
        dump_binarized_code(k);
        LOGS("Done dumping binarized code");
    }
    
    START_INSTRUMENTATION();

    LOGS("Parsing timing patterns");
    if (!parse_timing_patterns(k)) {
        LOGS("Timing pattern parsing failed.");
        return(micro_err_timing_parse_failed);
    }
    LOGS("Done parsing timing patterns");

    LOGS("Conjecturing alignment pattern locations");
    conjecture_alignment_patterns(k);
    LOGS("Done conjecturing alignment pattern locations");

    LOGS("Refining guesses for alignment pattern locations");
    find_alignment_patterns(k);
    LOGS("Done refining guesses for alignment pattern locations");

    LOGS("Placing virtual alignment patterns");
    if (!place_virtual_alignment_patterns(k)) {
        LOGS("Placing virtual alignment patterns failed.");
        return(micro_err_virtual_pattern_place_failed);
    }
    LOGS("Done placing virtual alignment patterns");

    RECORD_METRIC(k, KOKROTDBG_METRIC_MICRO_SAMPLING_GRID_BUILD);

    LOGS("Reading final QR");

    START_INSTRUMENTATION();
    read_final_qr(k);
    RECORD_METRIC(k, KOKROTDBG_METRIC_MICRO_FINAL_READ);

    LOGS("Done reading final QR");

    int dim;
    dim = k->decoded_qr_dimension;
    
    LOGS("Writing to output matrix");
    int i, j;
    for (i = 0; i < dim; ++i) {
        for (j = 0; j < dim; ++j) {
            if (k->final_qr[i][j].hits <= 2 * k->final_qr[i][j].sum)
                outmatrix[j + i * dim] = 1;
            else outmatrix[j + i * dim] = 0;

            printf("%c", outmatrix[j + i * dim] + '0');
        }
        printf("\n");
    }
    LOGS("Done writing to output matrix");

    if (k->dbg_sink) {
        LOGS("Dumping final QR image");
        dump_final_qr(k, outmatrix);
        LOGS("Done dumping final QR image");
    }

    LOGS("Exiting Microcosm, all successful");
    return(micro_err_success);
}


