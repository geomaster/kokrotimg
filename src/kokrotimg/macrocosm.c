#include "macrocosm.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

void compute_cumulative_tables(kok_data_t* k)
{
    ct_t* ct = k->cumulative_table;

    byte* img = k->src_img;
    dimension w = k->src_img_w, h = k->src_img_h;

    dimensionsq x, y;
    ct[0] = img[0];

    /* initializer pass to fill the first row with meaningful data */
    for (x = 1; x < w; ++x)
        ct[x] = ct[x - 1] + img[x];

    /* same for columns */
    for (x = w; x < w * h; x += w)
        ct[x] = ct[x - w] + img[x];

    /* fill the cumulative table */
    for (y = 1; y < h; ++y) {
        for (x = 1; x < w; ++x) {
            register dimensionsq i = M2D(x, y, w);
            ct[i] = ct[i - 1] + ct[i - w] - ct[i - 1 - w] + img[i];
        }
    }
}

void binarize_image(kok_data_t* k)
{
    dimension x, y;
    const dimension w = k->src_img_w, h = k->src_img_h;

    /* we are considering a d times d box around each pixel */
    const int d = SAUVOLA_WINDOW_SIZE(k->src_img_area), dh = d / 2, d2 = d * d;

    ct_t* ct = k->cumulative_table;
    byte* binary = k->binary_img, *src = k->src_img;

    /* for these values we're guaranteed not to bumb against any matrix edge */
    for (y = d; y < h - d; ++y) {
        for (x = d; x < w - d; ++x) {
            /* retrieve the sum in the box */
            ct_t C = (ct[M2D(x + dh, y + dh, w)] + ct[M2D(x - dh, y - dh, w)] 
                    -ct[M2D(x + dh, y - dh, w)] - ct[M2D(x - dh, y + dh, w)]);

            /* choose a threshold as a simple mean of the values */
            byte threshold = C / d2;
            byte val = src[M2D(x, y, w)];

            /* simple byte math to get 0 or 255 based on the threshold,
             * but compilers optimize this anyway */
            binary[M2D(x, y, w)] = -(byte)(val > threshold);
            /* binary[M2D(x, y, w)] = ((C / d2) > threshold ? 255 : 0 ); */
        }
    }

    /* hairy inner loop, basically what it does is safeguard against overflows
     * or underflows of indices and adjusts them accordingly, complete with the
     * area of the box to keep our accuracy */
# define BINARIZE_INNER_LOOP(check_minusx, check_plusx, check_minusy, check_plusy) \
    int left = x - dh, right = x + dh, top = y - dh, bottom = y + dh; \
    if (check_minusx && left < 0) left = 0; \
    if (check_plusx && right >= w) right = w - 1; \
    if (check_minusy && top < 0) top = 0; \
    if (check_plusy && bottom >= h) bottom = h - 1; \
    const int d2now = (bottom - top) * (right - left); \
    ct_t C = (ct[M2D(right, bottom, w)] + ct[M2D(left, top, w)] \
            -ct[M2D(right, top, w)] - ct[M2D(left, bottom, w)]); \
    int threshold = C / d2now; \
    byte val = src[M2D(x, y, w)]; \
    binary[M2D(x, y, w)] = -(byte)(val > threshold)

    /* pass through the corner cases */
    for (y = 0; y < d; ++y) {
        for (x = 0; x < w; ++x) {
            BINARIZE_INNER_LOOP(1, 1, 1, 0);
        }
    }
    for (y = h - d; y < h; ++y) {
        for (x = 0; x < w; ++x) {
            BINARIZE_INNER_LOOP(1, 1, 0, 1);
        }
    }    

    for (y = d; y < h - d; ++y) {
        for (x = 0; x < d; ++x) {
            BINARIZE_INNER_LOOP(1, 0, 1, 1);
        }

        for (x = w - d; x < w; ++x) {
            BINARIZE_INNER_LOOP(0, 1, 1, 1);
        }
    }
}


int locate_finder_candidates(kok_data_t* k)
{
    kok_pixelstrip_t* strips = k->pixel_strips;
    int stripcount = 0;
    dimension x, y, w = k->src_img_w, h = k->src_img_h;

    kok_pixelstrip_t strip;
    strip.value = strip.pixel_count = strip.start = 0;
    for (y = 0; y < h; ++y) {
        /* decompose this scanline into pixel strips: areas of the image
         * with consecutive pixel values. note that we're using a kerneled
         * sum to be tolerant to some amount of noise in the photo */
        ct_t sum = 0;
        int i;
        for (i = 0; i < FINDER_LOCATION_KERNEL_SIZE; ++i) {
            sum += k->binary_img[M2D(i, y, w)];
        }

        stripcount = 0;
        for (x = FINDER_LOCATION_KERNEL_CENTER; x < w - FINDER_LOCATION_KERNEL_CENTER - 1; ++x) {
            byte value = (byte) (sum / FINDER_LOCATION_KERNEL_SIZE);

            value = (value > FINDER_LOCATION_BLACK_THRESHOLD ? 1 : 0);
            if (value == strip.value || strip.pixel_count < FINDER_LOCATION_MIN_STRIP_WIDTH)
                ++strip.pixel_count;
            else {
                ASSERT(stripcount < k->src_img_w, "We shouldn't have more than W strips, what's happening?");

                strips[stripcount++] = strip;
                strip.start = x;
                strip.value = value;
                strip.pixel_count = 1;
            }

            ASSERT(x - FINDER_LOCATION_KERNEL_CENTER >= 0 &&
                   x - FINDER_LOCATION_KERNEL_CENTER < w,
                   "We shouldn't go out of the image with this coordinate, is the image very small?");

            /* remove the leftmost pixel of the 1D kernel from the sum and add
             * the new rightmost pixel to it */
            sum -= k->binary_img[M2D(x - FINDER_LOCATION_KERNEL_CENTER, y, w)];
            sum += k->binary_img[M2D(x + FINDER_LOCATION_KERNEL_CENTER + 1, y, w)];
        }

        /* go through the strips and find characteristical 1:1:3:1:1 patterns, allowing
         * a (pretty) huge measurement error, which is the only way we'll get most of
         * the real finders with this limited data (here we don't care much about the
         * false positives */
        for (i = 0; i < stripcount - 4; ++i) {
            kok_pixelstrip_t * base = &strips[i];
            dimension bwidth = base->pixel_count;

            dimension b0width = bwidth, 
                      b1width = strips[i + 1].pixel_count, 
                      b2width = strips[i + 2].pixel_count,
                      b3width = strips[i + 3].pixel_count, 
                      b4width = strips[i + 4].pixel_count;

            if (!FINDER_LOCATION_SATISFIED_PATTERN(b0width, b1width, 
                        b2width, b3width, b4width, w, h))
                continue;

            /* compute the center and some other math (should be trivial to understand */
            dimension centerx = strips[i + 2].start + strips[i + 2].pixel_count / 2;
            kok_point_t p = { centerx , y };

            dimension finderwidth = strips[i + 4].start + strips[i + 4].pixel_count - base->start;

            /* check for te same pattern in the remaining three directions, in order
             * for this to be considered a candidate, it must pass in at least one
             * of these */
            if (
                    !check_finder_pattern(k, p, 0, -1, finderwidth, base->value) &&
                    !check_finder_pattern(k, p, 1, -1, finderwidth, base->value) &&
                    !check_finder_pattern(k, p, -1, -1, finderwidth, base->value)
               )
                continue;

            /* this shouldn't really happen, but we have to safeguard: if our data
             * structure can't hold any more, run away */
            if (k->finder_candidate_count >= MAX_FINDER_CANDIDATES - 1) {
                /* bail out early, TODO: log this */
                return(0);
            }

            /* add this to the candidates */
            k->finder_candidates[k->finder_candidate_count++] = p;
        }

    }

    return(1);
}

#define INVERT_VAL(v)                (255 - (v))
#define REFIND_WIDTH_TOLERANCE(w1, w2)    (2 * ((w1 + w2) / 2) / 3)

inline int check_finder_pattern(kok_data_t* k, kok_point_t pt, const int dx, const int dy, int d, byte val)
{
    /* TODO: Make this use a 1D box kernel too! */

    int x = pt.x, y = pt.y, count1 = 0, count2a = 0, count2b = 0, count3a = 0, count3b = 0;
    dimension w = k->src_img_w, h = k->src_img_h;

# define CHECK_FINDER_PATTERN_LOOP(x, y, dx, dy, count, val) \
    while (img[M2D(x, y, w)] == val) { \
        /* should be optimized by the compiler! */    \
        if (dy < 0 && y <= 0) break;    \
        else if (dy > 0 && y >= h - 1) break;    \
        if (dx < 0 && x <= 0) break;    \
        else if (dx > 0 && x >= w - 1) break;    \
        \
        ++count; \
        x += dx; \
        y += dy; \
        \
    }

    /* basically run a lot of boilerplate code and collect its result into
     * count variables */

    /* NB 
     *
     * count1 is the inner (3) black square
     * count2a/count2b are the (1) white borders in directions + and -,
     * respectively
     * count3a/count3b are the (1) black borders in directions + and -,
     * respectively
     *
     * x and y follow the same incremental convention but for count1 x/y
     * and x2/y2 are used leading to a off-by-one discrepancy
     */
    byte *img = k->binary_img;
    const int dx2 = -dx, dy2 = -dy;

    CHECK_FINDER_PATTERN_LOOP(x, y, dx, dy, count1, val);

    int x2 = pt.x, y2 = pt.y;
    CHECK_FINDER_PATTERN_LOOP(x2, y2, dx2, dy2, count1, val);

    int x3a = x, y3a = y;
    CHECK_FINDER_PATTERN_LOOP(x3a, y3a, dx, dy, count2a, INVERT_VAL(val));

    int x3b = x2, y3b = y2;
    CHECK_FINDER_PATTERN_LOOP(x3b, y3b, dx2, dy2, count2b, INVERT_VAL(val));

    /* bail out early to avoid unnecessary iteration when we know this'll fail */
    if (absdiff(count2b, count2a) > REFIND_WIDTH_TOLERANCE(count2b, count2a))
        return( 0 );

    int x4a = x3a, y4a = y3a;
    CHECK_FINDER_PATTERN_LOOP(x4a, y4a, dx, dy, count3a, val);

    int x4b = x3b, y4b = y3b;
    CHECK_FINDER_PATTERN_LOOP(x4b, y4b, dx2, dy2, count3b, val);

    /* again early bailout */
    if (absdiff(count3b, count3a) > REFIND_WIDTH_TOLERANCE(count3b, count3a))
        return( 0 );

    /* average it all up and test against the tolerance macros */
    int avgw = (count3b + count2b + count2a + count2b) / 4;
    int height = absdiff(y4b, y4a);
    return absdiff(count1, 3 * avgw) < REFIND_WIDTH_TOLERANCE(count1, 3 * avgw) &&
        absdiff(d, height) < REFIND_WIDTH_TOLERANCE(d, height);

}

void locate_finder_patterns(kok_data_t* k)
{

}

inline int bfs_expand_neighbors(kok_data_t* k, kok_point_t pt, kok_queue_t* q, byte val, int* is_edge)
{
    int count = 0, i, edge = 0;
    /* const int dx[] = { 1, -1, 1, -1 }, dy[] = { 1, -1, 0, -1 }; */
    const int dx[] = { 1, -1, 0, 0 }, dy[] = { 0, 0, 1, -1 };

    dimension w = k->src_img_w, h = k->src_img_h;
    for (i = 0; i < 4; ++i) {
        kok_point_t p = { .x = pt.x + dx[i], .y = pt.y + dy[i] };
        /* this ugly construct says: 
         *
         * if dx[i] < 0, then make sure we're within lower bounds for p.x
         * if dx[i] == 0, p.x should be okay
         * if dx[i] > 0, then make sure we're within uper bounds for p.y
         * 
         * AND
         *
         * (same for dy[i] and p.y)
         *
         * AND
         *
         * we haven't visited this pixel
         *
         * AND
         *
         * we have a fitting color
         */
        if (((dx[i] < 0 && p.x >= 0) ||
                    dx[i] == 0 ||
                    (dx[i] > 0 && p.x < w)) &&
                ((dy[i] < 0 && p.y >= 0) ||
                 dy[i] == 0 ||
                 (dy[i] > 0 && p.y < h))) {

            if (!k->pixelinfo[M2D(p.x, p.y, w)].visited &&
                    k->binary_img[M2D(p.x, p.y, w)] == val) {
                k->pixelinfo[M2D(p.x, p.y, w)].visited = 1;

                /* add to this queue */
                queue_push_back(q, p);
                ++count;
            } else if (k->pixelinfo[M2D(p.x, p.y, w)].visited && 
                    k->regions[k->pixelinfo[M2D(p.x, p.y, w)].region].dead) {
                /* this pixel was visited and is in a dead region, "infect"
                 * this region too by returning -1 (see calling code) */
                return( -1 );
            }

            /* at least one distinct-valued neighbor, means this is the edge */
            if (k->binary_img[M2D(p.x, p.y, w)] != val)
                edge = 1;
        }
    }

    /* return the edge and count */
    *is_edge = edge;
    return count;
}


#define MAX_FINDER_SQUARE_AREA(a)                 ((a) / 9)
#define MAX_OTHER_SIDE_SIZE(d)                    (9 * (d) / 6)
#define REGION_VERIFICATION_GRANULARITY           40

inline int bfs_process_point(kok_data_t* k, kok_point_t pt, kok_regionbounds_t *reg, fcount_t region, int depth)
{
    /* set the region tracking data field */
    k->pixelinfo[M2D(pt.x, pt.y, k->src_img_w)].region = region;

    /* update the primitive max-min x-y boundaries */
    if (pt.y < reg->top)
        reg->top = pt.y;
    else if (pt.y > reg->bottom)
        reg->bottom = pt.y;

    if (pt.x < reg->left)
        reg->left = pt.x;
    else if (pt.x > reg->right)
        reg->right = pt.x;

    /* in order to speed this method up, we only check for these conditions inside
     * if our depth is a multiple of REGION_VERIFICATION_GRANULARITY, it reduces
     * the runtime cost significantly and doesn't fuck with the intent of the
     * conditionals i.e. to reduce total data processed by bailing out early out
     * of BFSs. REGION_VERIFICATION_GRANULARITY works best as a value which will
     * create a branch taken-not taken pattern that the CPU branch predictor
     * would be able to follow so that this branch doesn't screw up the prefetcher
     * and end up even slower than if REGION_VERIFICATION_GRANULARITY was 1 */
    if (depth % REGION_VERIFICATION_GRANULARITY == 0) {
        /* this is pretty clever: if one side is so much bigger than the other, 
         * then this can't be a square, since the calling BFS makes sure it expands
         * equally in all directions */
        dimension side1 = reg->right - reg->left, side2 = reg->bottom - reg->top;
        if (max(side1, side2) > MAX_OTHER_SIDE_SIZE(min(side2, side1))) {
            return( 1 );
        }

        /* if the region occupies a lot of the image, this can't be a square either */
        dimensionsq area = side1 * side2;
        if (area > MAX_FINDER_SQUARE_AREA(k->src_img_area)) {
            return( 1 );
        }

        /* NB that on returning 1 we are telling the BFS to mark the region as dead,
         * and bfs_expand_neighbors will mark other regions that come to touch it as
         * dead, too. this also decreases running time */
    }
    return(0);
}

#define COLOR_MINIMUM_PERCENTAGE(a)          (8 * (a) / 10)
#define COLOR_MINIMUM_PERCENTAGE_STAGE2(a)   (6 * (a) / 10)
#define COLOR_MINIMUM_PERCENTAGE_STAGE3(a)   (5 * (a) / 10)
#define COLOR_MINIMUM_PERCENTAGE_STAGE4(a)   (5 * (a) / 10)

#define FINDER_MIN_AREA                      (20)

void bfs_finder_square(kok_data_t* k, kok_point_t startpt, fcount_t region)
{
    ASSERT(VALID_POINT(startpt, k->src_img_w, k->src_img_h), "Invalid starting point");

    kok_queue_t *q = &k->bfs_queue;
    queue_clear(q);

    /* some sanity checks and initializers */
    k->edge_pixel_count = 0;
    int count = 1;
    if (k->pixelinfo[M2D(startpt.x, startpt.y, k->src_img_w)].visited)
        return;

    queue_push_back(q, startpt);
    byte val = k->binary_img[M2D(startpt.x, startpt.y, k->src_img_w)];

    /* init the bounds to some ok values */
    kok_regionbounds_t reg;
    reg.left = reg.right = startpt.x;
    reg.bottom = reg.top = startpt.y;

    /* gstacktop is the number of edge pixels we have pushed */
    int depth = 1, gstacktop = 0;
    const int maxedgepx = k->max_edge_pixels;

    while (count > 0) {
        int votes = 0, thiscount, is_edge = 0;

        /* expand the neighbors of queueA to queueB */
        while (count > 0) {
            kok_point_t px = queue_pop_front(q);
            --count;

            ASSERT(VALID_POINT(px, k->src_img_w, k->src_img_h),
                    "Invalid point encountered during BFS node consumption");

            votes += bfs_process_point(k, px, &reg, region, depth);

            is_edge = 0;
            count += (thiscount = bfs_expand_neighbors(k, px, q, val, &is_edge));
            if (thiscount == -1) {
                LOGF("bfs_finder_square(%d): Touched a dead region, killing this one", k->context_finder_id);
                goto dead_region;
            } else if (is_edge) {
                if (gstacktop >= maxedgepx) {
                    LOGF("bfs_finder_square(%d): Maxxed out edge pixels, killing region", k->context_finder_id);
                    goto dead_region;
                }

                /* push this pixel to the list */
                k->edge_pixels[gstacktop++] = px;
            }
        }
        ++depth;

        /* some of the bfs_process_point's has told us that the region is dead,
         * make it so */
        if (votes > 0) {
            goto dead_region;
        }
    }

    /* record in the shared state */
    k->edge_pixel_count = gstacktop;

    return;

dead_region:
    k->regions[region].dead = 1;
    k->edge_pixel_count = 0;
    return ;
}

#define FULL_FINDER_SCAN_MARGIN(dx, dy)              (max(dx, dy) / 3)

int process_finder_square(kok_data_t* k)
{
    int i, ptsinhull, edgepxs = k->edge_pixel_count;
    if (edgepxs <= 0)
        return(0); 

    /* obtain the convex hull of the edge pixels */
    ptsinhull = convex_hull(k->edge_pixels, edgepxs, k->edge_hull);
    if (ptsinhull <= 0 || ptsinhull >= MAX_FINDER_POLYGON_SIZE)
        return(0);

    /* retrieve the sum of the values inside that hull (polygon_sum) */
    int pxcount = 0,
        sum = polygon_sum(k->binary_img, k->src_img_w, k->src_img_h, k->edge_hull, ptsinhull, &pxcount),
        whitepixels = sum / 255;

    /* i should probably change this k */
    int whiteprivilege = 0;

    /* some basic tests on whether or not the qr code is inverted from
     * the standard black-inner-finder-square */
    if (whitepixels >= COLOR_MINIMUM_PERCENTAGE(pxcount))
        whiteprivilege = 1;
    else if ((pxcount - whitepixels) >= COLOR_MINIMUM_PERCENTAGE(pxcount))
        whiteprivilege = 0;
    else return(0);

    /* find the extreme points */
    kok_bigpoint_t centerpt;
    int left, right, top, bottom;
    left = right = k->edge_hull[0].x;
    top = bottom = k->edge_hull[0].y;
    for (i = 0; i < ptsinhull; ++i) {
        kok_point_t p = k->edge_hull[i];
        if (p.x < left) left = p.x;
        else if (p.x > right) right = p.x;

        if (p.y < top) top = p.y;
        else if (p.y > bottom) bottom = p.y;
    }

    ASSERT(left >= 0 && left < k->src_img_w &&
           right >= 0 && right < k->src_img_w &&
           top >= 0 && top < k->src_img_h &&
           bottom >= 0 && bottom < k->src_img_h,
           "Invalid extreme points computed");

    /* record the central points. previously i was using the center of gravity
     * approach (averaging all the points) but it didn't cope well with actual
     * test cases */
    centerpt.x = (right + left) / 2;
    centerpt.y = (top + bottom) / 2;

    /* FIXME: debug cruft */
    /* k->debug_img[M2D(centerpt.x, centerpt.y, k->src_img_w)] = dbg_red; */

    /* some basic lerp math, this will enlarge the hull so that it now encloses
     * the (1) white border, approximately of course, based on the inner square
     * data we have */
    kok_point_t scaledcenter = { centerpt.x * (-2) / 3, centerpt.y * (-2) / 3 };
    for (i = 0; i < ptsinhull; ++i) {
        k->edge_hull[i].x = scaledcenter.x + k->edge_hull[i].x * 5 / 3;
        k->edge_hull[i].y = scaledcenter.y + k->edge_hull[i].y * 5 / 3;

        if (!VALID_POINT(k->edge_hull[i], k->src_img_w, k->src_img_h) || 
                !VALID_POINT(k->edge_hull[i], k->src_img_w, k->src_img_h))
            return(0);

    }

    /* compute the polygon sum of this new hull, subtracting inner square data
     * to exclude it */
    int oldpxcount = pxcount, oldsum = sum;
    sum = polygon_sum(k->binary_img, k->src_img_w, k->src_img_h, k->edge_hull, ptsinhull, &pxcount) - sum;
    pxcount -= oldpxcount;
    int whitepixels2 = sum / 255;

    /* an ugly if, but it just asks whether or not there are enough pixels */
    if ((whiteprivilege && (pxcount - whitepixels2) <= COLOR_MINIMUM_PERCENTAGE_STAGE2(pxcount)) ||
            (!whiteprivilege && whitepixels2 <= COLOR_MINIMUM_PERCENTAGE_STAGE2(pxcount)))
        return(0);

    /* do the same and scale yet again */
    scaledcenter.x = centerpt.x * (-2) / 5;
    scaledcenter.y = centerpt.y * (-2) / 5;
    for (i = 0; i < ptsinhull; ++i) {
        k->edge_hull[i].x = scaledcenter.x + k->edge_hull[i].x * 7 / 5;
        k->edge_hull[i].y = scaledcenter.y + k->edge_hull[i].y * 7 / 5;
        
        if (!VALID_POINT(k->edge_hull[i], k->src_img_w, k->src_img_h) || 
                !VALID_POINT(k->edge_hull[i], k->src_img_w, k->src_img_h))
            return(0);
    }

    /* see above */
    oldpxcount = pxcount + oldpxcount;
    sum = polygon_sum(k->binary_img, k->src_img_w, k->src_img_h, k->edge_hull, ptsinhull, &pxcount) - sum - oldsum;
    pxcount -= oldpxcount;
    int whitepixels3 = sum / 255;

    /* see above */
    if ((whiteprivilege && whitepixels3 <= COLOR_MINIMUM_PERCENTAGE_STAGE3(pxcount)) ||
            (!whiteprivilege && (pxcount - whitepixels3) <= COLOR_MINIMUM_PERCENTAGE_STAGE3(pxcount)))
        return(0); 

    /* if we're seeing to many, just bail out
     *
     * TODO: make this more verbose */
    if (k->finder_pattern_count >= MAX_FINDER_PATTERNS)
        return(0);

    /* sometimes really small finders sneak up, snipe them */
    if (pxcount < FINDER_MIN_AREA)
        return(0);

    kok_finder_t* f = &k->finder_patterns[k->finder_pattern_count];
    f->center.x = centerpt.x;
    f->center.y = centerpt.y;
    f->area = pxcount;

    /* find extremums of the new hull (FIXME: redundant, we have them already) */
    for (i = 0; i < ptsinhull; ++i) {
        kok_point_t p = k->edge_hull[i];
        if (p.x < left) left = p.x;
        else if (p.x > right) right = p.x;

        if (p.y < top) top = p.y;
        else if (p.y > bottom) bottom = p.y;
    }

    /* now we'll run a bfs to get more precise info about the outermost finder part, 
     * as opposed to the inner square we were dealing with up to now */

    int y = (top + bottom) / 2, x;
    byte* binimg = k->binary_img;
    dimension w = k->src_img_w;

    /* go through the tiny scanline at the ycenter and find the first three consecutive
     * pixels of the desired value. this is so we can pinpoint at least one pixel of the
     * outermost black square/border so we can start bfs from there.
     */
    byte targetval = binimg[M2D(centerpt.x, centerpt.y, w)];
    for (x = left; x <= right - 3; ++x) {
        ASSERT(x >= 0 && x < w, "Out of bounds value for x");
        if (binimg[M2D(x, y, w)] == targetval && binimg[M2D(x + 1, y, w)] == targetval && binimg[M2D(x + 2, y, w)] == targetval)
            break;
    }

    /* compute the point where to start */
    kok_point_t newstartpt = { x + 1, (top + bottom) / 2 };
    if (k->dbg_sink) {
        k->dbg_sink->debug_add_point(newstartpt.x, newstartpt.y, NULL, KOKROTDBG_CLASS_MACRO_FINDER_CANDIDATE, k->dbg_sink->callback_param);
    }

    /* small_scale_bfs expects a rectangle within which to consider pixels. we'll pass 
     * a small (compared to the input image) but liberal rectangle so the real finder
     * will be inside, but if it is connected to some giant black region we don't
     * waste too much time there */
    int margin = FULL_FINDER_SCAN_MARGIN(right - left, bottom - top);
    left -= margin;
    top -= margin;
    right += margin;
    bottom += margin;

    /* sanity checks */
    if (left < 0)
        left = 0;
    if (right >= k->src_img_w)
        right = k->src_img_w - 1;

    if (top < 0)
        top = 0;
    if (bottom >= k->src_img_h)
        bottom = k->src_img_h - 1;

    kok_point_t topleft = { left, top }, bottomright = { right, bottom };

    /* runn small_scale_bfs on the data we found so far */
    int newpointcount = 0,
        newedgeptcount = small_scale_bfs(k, k->edge_pixels, k->binary_img, 
                k->src_img_w, k->max_edge_pixels, newstartpt, topleft, bottomright,
                (whiteprivilege? 255 : 0), &newpointcount);

    /* observe what happens: if the conditions are not satisfied (the area of the
     * outer square is not consistent with the area of the inner one and the
     * minimum threshold area was achieved, then the edge_hull shared state will
     * contain the *previous* hull.
     *
     * so we recognize that we've failed, and reasonably provide a crude approximation
     * of the finder square, instead of feeding later stages with bogus data */
    if (absdiff(newpointcount, pxcount) <= 
            COLOR_MINIMUM_PERCENTAGE_STAGE4((pxcount + newpointcount)) / 2 && 
            newpointcount > FINDER_MIN_AREA) {
        ptsinhull = convex_hull(k->edge_pixels, newedgeptcount, k->edge_hull);
        if (ptsinhull > MAX_FINDER_POLYGON_SIZE) {
            return(0);
        }
    } else {
            LOGF("pxcount %d; newpointcount %d, val %d)", pxcount, newpointcount, (whiteprivilege? 255 : 0));
        LOGF("process_finder_square(%d): Small scale BFS failed, must reuse previous hull",
                    k->context_finder_id);
    }

    f->point_count = ptsinhull;
    /* TODO: remove this debug */
    /* bresenham_polygon(k->debug_img, dbg_red, w, k->edge_hull, ptsinhull); */

    /* copy to the shared state */
    memcpy(f->polygon, k->edge_hull, sizeof(kok_point_t) * ptsinhull);
    ++k->finder_pattern_count;

    return(1);
}

#define DISTANCE_SQUARED_TOLERANCE(d)              (5 * (d) / 10)
#define DOT_PRODUCT(a, b)                          (a.x * b.x + a.y * b.y)
#define DOT_PRODUCT_TOLERANCE(d)                   (7 * (d) / 10)
#define FINDER_AREA_TOLERANCE(d)                   (4 * (d) / 10)
#define DOT_PRODUCT_ACCEPTABLE(prod, d1, d2)       (abs(prod) <= DOT_PRODUCT_TOLERANCE(d1 * d2))
#define DISTANCE_SQUARED_EQUALS(d1, d2)            (absdiff(d1, d2) <= DISTANCE_SQUARED_TOLERANCE(d1 + d2) / 2)
#define FINDER_AREA_EQUALS(d1, d2)                 (absdiff(d1, d2) <= FINDER_AREA_TOLERANCE(d1 + d2) / 2)

void hypothesize_code_locations(kok_data_t* k) 
{
    int i, j, l, n = k->finder_pattern_count;
    kok_finder_t *f = k->finder_patterns;

    /* `qrs` shall contain pointers to the finders which make up
     * a single qr (3 of them) */
    kok_finder_t* qrs[MAX_QR_CANDIDATES][3];
    kok_quad_t qrquads[MAX_QR_CANDIDATES];
    int qrareas[MAX_QR_CANDIDATES];
    int qrcount = 0;

    /* go through each of the possible finder pairs... */
    for (i = 0; i < n; ++i) {
        for (j = i + 1; j == i + 1 && j < n; ++j) {
            kok_finder_t a = f[i], b = f[j];
            /* we set the center to negative if we want to exclude this finder */
            if (a.center.x < 0 || b.center.x < 0) continue;

            /* distancesq is the distance between the centers of the two finders, squared
             * to avoid expensive sqrt fpops */
            sdimensionsq distancesq = (b.center.x - a.center.x) * (b.center.x - a.center.x) +
                (b.center.y - a.center.y) * (b.center.y - a.center.y);

            int avgarea = (a.area + b.area) / 2;
            kok_finder_t *bestcandidate = NULL;
            dimensionsq bestdsq = 0;
            
            /* ...and for the neighboring finders, check this: */
            for (l = j + 1; l <= j + 2 && l < n; ++l) {
                if (l == i || l == j) continue;

                kok_finder_t c = f[l];

                /* we set the center to negative if we want to exclude this finder */
                if (c.center.x < 0) continue;

                /* compute the distances to both a and b, and choose the smaller one */
                sdimensionsq cbdsq = (c.center.x - a.center.x) * (c.center.x - a.center.x) +
                    (c.center.y - a.center.y) * (c.center.y - a.center.y),
                    cadsq = (c.center.x - b.center.x) * (c.center.x - b.center.x) +
                        (c.center.y - b.center.y) * (c.center.y - b.center.y);

                sdimensionsq cdistsq = min(cbdsq, cadsq);

                /* check if the distance is ok within a threshold, and that the areas match
                 * (so we avoid pairing bigger finders with significantly smaller ones
                 * as they most likely do not constitute the same code) */
                if (DISTANCE_SQUARED_EQUALS(cdistsq, distancesq) && FINDER_AREA_EQUALS(c.area, avgarea)) {
                    /* compute some vectors for easy manipulation */
                    kok_point_t ab = { b.center.x - a.center.x, b.center.y - a.center.y },
                                ba = { -ab.x, -ab.y },
                                ac = { c.center.x - a.center.x, c.center.y - a.center.y },
                                bc = { c.center.x - b.center.x, c.center.y - b.center.y };

                    /* compute the dot product of the BA and BC vectors, also AB and AC,
                     * and take the smaller of them */
                    sdimensionsq dot1 = DOT_PRODUCT(ba, bc), dot2 = DOT_PRODUCT(ab, ac),
                                 dot = min(dot1, dot2);

                    /* okay, i lied, we didn't save on the sqrt, or maybe we did */
                    real distance = sqrt(distancesq),
                         cdis = sqrt(cdistsq);

                    /* this semantically tests for the angle, how close it is 
                     * to 90 degrees, because the dot product of perpendicular
                     * vectors is 0 */
                    if (DOT_PRODUCT_ACCEPTABLE(dot, cdis, distance)) {
                        if (!bestcandidate || cdistsq < bestdsq) {
                            bestcandidate = &f[l];
                            bestdsq = cdistsq;
                        }
                    }
                }
            }

            /* if we have a candidate that passes the thresholds */
            if (bestcandidate) {
                kok_finder_t c = *bestcandidate;

                kok_point_t codepoints[MAX_FINDER_POLYGON_SIZE * 4],
                            codehull[MAX_FINDER_POLYGON_SIZE * 4];
                int codepointcount = 0, codehullsize;

                /* copy all of the finder polygons into one big point cloud */
                if (a.point_count + b.point_count + c.point_count < MAX_FINDER_POLYGON_SIZE) {
                    memcpy(codepoints, a.polygon, sizeof(kok_point_t) * a.point_count);
                    codepointcount += a.point_count;
                    memcpy(codepoints + codepointcount, b.polygon, sizeof(kok_point_t) * b.point_count);
                    codepointcount += b.point_count;
                    memcpy(codepoints + codepointcount, c.polygon, sizeof(kok_point_t) * c.point_count);
                    codepointcount += c.point_count;

                    /* compute the convex hull of the point cloud */
                    codehullsize = convex_hull(codepoints, codepointcount, codehull);

                    /* reduce the hull to a 5-gon, actually approximate the 5-gon that can be
                     * derived from the hull that has the least possible area
                     *
                     * NB that reduce_polygon has a lot of cases where it fails to produce
                     * even a remotely ok polygon, but we're lucky that in this case it doesn't
                     * happen and we actually get a respectable 5-gon that represents a qr
                     * code quad that's been chopped of one piece (because there are only 3 
                     * finders!) */
                    codehullsize = reduce_polygon(codehull, codehullsize, 5);

                    int maxareai = 0, maxarea = 0;
                    kok_point_t newpt = { 0, 0 };

                    ASSERT(codehullsize == 5, "Bug in reduce_polygon!");

                    /* now, go through that 5-gon (codehullsize should be 5, but just to guard
                     * against some insane special case) and find which two adjacent points we
                     * can collapse into one such that: (the collapse is done as in reduce_polygon)
                     *
                     * (a) the area we gain is maximized;
                     * (b) the intersection point is within the image (so parallel lines that 
                     * make up the qr's right angles would be ruled out as they would meet 
                     * somewhere far away */
                    for (l = 0; l < 5; ++l) {
                        kok_point_t p0 = codehull[(l - 1 + 5) % 5],
                                    p1 = codehull[l],
                                    p2 = codehull[(l + 1) % 5],
                                    p3 = codehull[(l + 2) % 5];

                        kok_point_t intersect = {0, 0};
                        int can_intersect = line_intersection(p0, p1, p2, p3, &intersect);
                        if (!can_intersect)
                            continue;

                        int area = abs(SIGNED_AREA(p1, intersect, p2));
                        if (intersect.x > 0 && intersect.y > 0 && intersect.x < k->src_img_w && intersect.y < k->src_img_h &&  area > maxarea) {
                            maxareai = l;
                            maxarea = area;
                            newpt = intersect;
                        }
                    }

                    /* replace with the new point and shift to left */
                    codehull[maxareai] = newpt;
                    --codehullsize;
                    for (l = maxareai + 1; l < codehullsize; ++l) {
                        codehull[l] = codehull[l + 1];
                    }

                    /* if there are too many qr codes, bail out, obviously */
                    if (qrcount >= MAX_QR_CANDIDATES)
                        break ;

                    /* since now the hull effectively represents a quad, let's formalize it */
                    kok_quad_t thisquad = { codehull[0], codehull[1], codehull[2], codehull[3] };
                                        
                    /* record metadata */
                    qrs[qrcount][0] = &f[i];
                    qrs[qrcount][1] = &f[j];
                    qrs[qrcount][2] = bestcandidate;
                    qrareas[qrcount] = abs(SIGNED_AREA(thisquad.p1, thisquad.p2, thisquad.p4)) + 
                        abs(SIGNED_AREA(thisquad.p2, thisquad.p3, thisquad.p4));

                    qrquads[qrcount++] = thisquad;
                }
            }
        }
    }

    kok_finder_t** finalqrs[ MAX_QR_CODES ];
    kok_finder_t   qrfinders[ MAX_QR_CODES ][ 3 ];
    kok_quad_t* finalqrquads[ MAX_QR_CODES ];
    int finalqrcount = 0;

    /* now that there's a bunch of candidates, a lot of them are giant-ass false positives
     * and we'd like to be able to shave them off. we will, for each finder, find the best
     * qr code that contains it and then add it to the final shared state, others will be
     * dumped, at least for that finder
     *
     * note how the big O cost is fucked up, but it's ok because the numbers will be
     * fairly small and this is far from the real bottleneck */

    /* O(p * 2c), p ~ 150, c ~ 40 */
    for (i = 0; i < n; ++i) {
        kok_finder_t* pf = &k->finder_patterns[i];
        if (pf->center.x < 0)
            continue;

        kok_quad_t* bestqr = NULL;
        int bestarea = 0, bestj = 0;

        /* go through all qrs */
        for (j = 0; j < qrcount; ++j) {
            /* if they are valid and have this finder as a component */
            if (qrs[j][0]->center.x >= 0 && qrs[j][1]->center.x >= 0 &&
                    qrs[j][2]->center.x >= 0 && 
                    (qrs[j][0] == pf || qrs[j][1] == pf || qrs[j][2] == pf)) {
                /* just grade them based on their areas: the smallest wins */
                if (bestqr == NULL || qrareas[j] < bestarea) {
                    bestarea = qrareas[j];
                    bestqr = &qrquads[j];
                    bestj = j;
                }
            }
        }

        if (bestqr) {
            /* be sure not to file duplicates! */
            int duplicate = 0;
            for (j = 0; j < finalqrcount; ++j)
                if (finalqrs[j] == &qrs[i][0]) {
                    duplicate = 1;
                    break;
                }

            if (!duplicate) {
                /* add it to the data structure we use intermediately */
                finalqrs[finalqrcount] = &qrs[i][0];
                finalqrquads[finalqrcount] = bestqr;

                qrfinders[finalqrcount][0] = *qrs[bestj][0];
                qrfinders[finalqrcount][1] = *qrs[bestj][1];
                qrfinders[finalqrcount][2] = *qrs[bestj][2];

                ++finalqrcount;
            }
        }
    }

    /* finally, loop through the internal array and add qr's to the shared state */
    for (i = 0; i < finalqrcount; ++i) {
        k->qr_codes[i] = *finalqrquads[i];
        for (j = 0; j < 3; ++j) {
            /* find the finders it uses, approximate their quads and save them
             * to the shared state also */
            kok_finder_t* f = &qrfinders[i][j];
            int finalsz = reduce_polygon(f->polygon, f->point_count, 4);
            ASSERT(finalsz == 4, "Bug in reduce_polygon!");

            k->qr_finders[i][j].p1 = f->polygon[0];
            k->qr_finders[i][j].p2 = f->polygon[1];
            k->qr_finders[i][j].p3 = f->polygon[2];
            k->qr_finders[i][j].p4 = f->polygon[3];
        }
    }

    k->qr_code_count = finalqrcount;
}



#define TRY_DEALLOCATE_FIELD(field) \
    do { \
        if (k->field) { \
            kokfree(k, k->field); \
            k->field = NULL; \
        } \
    } while (0)

void macrocosm_dealloc_all(kok_data_t* k)
{
    TRY_DEALLOCATE_FIELD(binary_img);
    TRY_DEALLOCATE_FIELD(pixel_strips);
    TRY_DEALLOCATE_FIELD(cumulative_table);
    TRY_DEALLOCATE_FIELD(finder_candidates);
    TRY_DEALLOCATE_FIELD(finder_patterns);
    TRY_DEALLOCATE_FIELD(edge_pixels);
    TRY_DEALLOCATE_FIELD(edge_hull);
    TRY_DEALLOCATE_FIELD(pixelinfo);
    TRY_DEALLOCATE_FIELD(regions);
}

static void dump_finder_candidates(kok_data_t* k)
{
    int j;

    void* param = k->dbg_sink->callback_param;

    for (j = 0; j < k->finder_candidate_count; ++j) {
        kok_point_t p = k->finder_candidates[j];

        char s[64];
        s[63] = '\0';
        snprintf(s, 63, "Finder candidate %d", j);

        k->dbg_sink->debug_add_point(p.x, p.y, s, KOKROTDBG_CLASS_MACRO_FINDER_CANDIDATE, param);
    }

}

static void dump_qr_finders(kok_data_t* k)
{
    static dimension xs[ MAX_FINDER_POLYGON_SIZE ],
                     ys[ MAX_FINDER_POLYGON_SIZE ];

    void* param = k->dbg_sink->callback_param;

    int j, l, m;
    for (j = 0; j < k->finder_pattern_count; ++j) {
        kok_finder_t f = k->finder_patterns[j];

        char s[128];
        s[127] = '\0';
        snprintf(s, 127, "Finder pattern %d, area %d, %d points", j, f.area, f.point_count);

        for (l = 0; l < f.point_count; ++l) {
            xs[l] = f.polygon[l].x;
            ys[l] = f.polygon[l].y;
        }

        k->dbg_sink->debug_add_polygon(xs, ys, f.point_count, 0, s, KOKROTBDG_CLASS_MACRO_FINDER_PATTERN_POLYGON, param);
    }

    for (j = 0; j < k->qr_code_count; ++j) {
        for (l = 0; l < 3; ++l) {
            kok_quad_t q = k->qr_finders[j][l];

            char s[64];
            s[63] = '\0';
            snprintf(s, 63, "Finder quad #%d of QR %d", l, j);

            QUAD_TO_DEBUG_POLY(q, xs, ys);
            
            k->dbg_sink->debug_add_polygon(xs, ys, 4, 0, s, KOKROTDBG_CLASS_MACRO_FINDER_PATTERN_QUAD, param);
        }

    }
}

static void dump_qr_code_quads(kok_data_t* k)
{
    static dimension xs[ 4 ],
                     ys[ 4 ];

    void* param = k->dbg_sink->callback_param;

    int j;
    for (j = 0; j < k->qr_code_count; ++j) {
        kok_quad_t qr = k->qr_codes[j];

        char s[64];
        s[63] = '\0';
        snprintf(s, 63, "QR code quad %d", j);
 
        QUAD_TO_DEBUG_POLY(qr, xs, ys);
        k->dbg_sink->debug_add_polygon(xs, ys, 4, 0, s, KOKROTDBG_CLASS_MACRO_QR_CODE_QUAD, param);
    }
}


void report_prss(kok_data_t* k)
{
    LOGF("Peak resident set size (MB): %f", (double)k->peak_resident_set_bytes / (1024.0 * 1024.0));
}


void dump_images(kok_data_t* k)
{
    void* param = k->dbg_sink->callback_param;

    k->dbg_sink->debug_resize_canvas(k->src_img_w, k->src_img_h, param);
    k->dbg_sink->debug_add_backdrop(k->src_img, "Original image", KOKROTDBG_CLASS_MACRO_ORIGINAL_IMAGE, param);
    k->dbg_sink->debug_add_backdrop(k->binary_img, "Binarized image", KOKROTDBG_CLASS_MACRO_BINARIZED_IMAGE, param);
}

macrocosm_err_t kokrotimg_macrocosm(kok_data_t* k, int *out_code_count)
{
#ifdef INSTRUMENTATION_ENABLED
    double allocOverhead = 0.0;
#endif

#ifdef INSTRUMENTATION_ENABLED
#define ADD_ALLOC_OVERHEAD() \
    allocOverhead += STOP_INSTRUMENTATION();
#else
#define ADD_ALLOC_OVERHEAD()
#endif

    LOGS("Entering Macrocosm");
    kokrot_component = "macrocosm";
  
    START_INSTRUMENTATION();
    ALLOCATE_FIELD(
            cumulative_table, 
            ct_t, 
            k->src_img_area);
    ADD_ALLOC_OVERHEAD();

    LOGS("Computing cumulative tables");

    START_INSTRUMENTATION();
    compute_cumulative_tables(k);
    RECORD_METRIC(k, KOKROTDBG_METRIC_MACRO_CUMULATIVE_TABLE_COMPUTATION);

    LOGS("Done computing cumulative tables");
    
    START_INSTRUMENTATION();
    ALLOCATE_FIELD(
            binary_img, 
            byte, 
            k->src_img_area);
    ADD_ALLOC_OVERHEAD();
 

    LOGS("Binarizing image");

    START_INSTRUMENTATION();
    binarize_image(k); 
    RECORD_METRIC(k, KOKROTDBG_METRIC_MACRO_BINARIZATION);

    LOGS("Done binarizing image");

    START_INSTRUMENTATION();
    DEALLOCATE_FIELD(
            cumulative_table);
    ADD_ALLOC_OVERHEAD();

    if (k->dbg_sink) {
        LOGS("Dumping images to debug sink");
        dump_images(k);
        LOGS("Done dumping images to debug sink");
    }
    
    START_INSTRUMENTATION();
    ALLOCATE_FIELD(
            pixel_strips, 
            kok_pixelstrip_t, 
            max(k->src_img_w, k->src_img_h));

    ALLOCATE_FIELD(
            finder_candidates, 
            kok_point_t, 
            MAX_FINDER_CANDIDATES);
    ADD_ALLOC_OVERHEAD();

    LOGS("Locating finder candidates");

    START_INSTRUMENTATION();
    if (!locate_finder_candidates(k)) {
        LOGS("Finder candidate location reported errors, bailing out");

        macrocosm_dealloc_all(k);
        return(macro_err_candidate_location_failure);
    }
    RECORD_METRIC(k, KOKROTDBG_METRIC_MACRO_FINDER_CANDIDATE_SEARCH);

    LOGS("Done locating finder candidates");
    LOGF("Found %d finder candidates", k->finder_candidate_count);

    if (k->dbg_sink) {
        LOGS("Dumping found candidate positions");
        dump_finder_candidates(k);
    }

    START_INSTRUMENTATION();
    DEALLOCATE_FIELD(
            pixel_strips);

    ALLOCATE_ZERO_FIELD(
            regions,
            kok_region_t,
            MAX_REGIONS);

    ALLOCATE_ZERO_FIELD(
            pixelinfo,
            kok_pixelinfo_t,
            k->src_img_area);

    ALLOCATE_FIELD(
            edge_pixels,
            kok_point_t,
            8 * (k->src_img_w + k->src_img_h));

    ALLOCATE_FIELD(
            edge_hull,
            kok_point_t,
            k->src_img_w + k->src_img_h);

    ALLOCATE_FIELD(
            finder_patterns,
            kok_finder_t,
            MAX_FINDER_PATTERNS);

    k->max_edge_pixels = 8 * (k->src_img_w + k->src_img_h);

    if (!queue_create(k, 2 * (k->src_img_w + k->src_img_h), &k->bfs_queue)) {
            macrocosm_dealloc_all(k);
            return(macro_err_memory_allocation_failure);
    }

    k->max_bfs_queue = 2 * (k->src_img_w + k->src_img_h);// k->src_img_area;

    ADD_ALLOC_OVERHEAD();

    int j;
    LOGS("Processing finder candidates");

    START_INSTRUMENTATION();
    for (j = 0; j < k->finder_candidate_count; ++j) {
        k->context_finder_id = j;
        bfs_finder_square(k, k->finder_candidates[j], j + 1);
        process_finder_square(k);
    }
    RECORD_METRIC(k, KOKROTDBG_METRIC_MACRO_FINDER_SQUARE_PROCESSING);

    START_INSTRUMENTATION();
    DEALLOCATE_FIELD(
            finder_candidates);

    DEALLOCATE_FIELD(
            binary_img);

    DEALLOCATE_FIELD(
            edge_pixels);

    queue_destroy(k, &k->bfs_queue);
    DEALLOCATE_FIELD(
            pixelinfo);
    
    DEALLOCATE_FIELD(
            regions);

    ADD_ALLOC_OVERHEAD();
    
    LOGS("Hypothesizing code locations");

    START_INSTRUMENTATION();
    hypothesize_code_locations(k);
    RECORD_METRIC(k, KOKROTDBG_METRIC_MACRO_CODE_LOCATION_HYPOTHESIZATION);

    LOGS("Done hypothesizing code locations");

    LOGF("Have %d finder patterns", k->finder_pattern_count);
    if (k->qr_code_count <= 0) {
        *out_code_count = 0;
        macrocosm_dealloc_all(k);
        LOGS("No codes found.");
        report_prss(k);
        return(macro_err_no_codes_found);
    }
       
    LOGF("Found %d codes", k->qr_code_count);
    if (k->dbg_sink) {
        LOGS("Dumping QR quads");
        dump_qr_code_quads(k);
        LOGS("Done dumping QR quads");

        LOGS("Dumping QR finders");
        dump_qr_finders(k);
        LOGS("Done dumping QR finders");
    }

    START_INSTRUMENTATION();
    DEALLOCATE_FIELD(
            finder_patterns);

    DEALLOCATE_FIELD(
            edge_hull);

    macrocosm_dealloc_all(k);
    ADD_ALLOC_OVERHEAD();

#ifdef INSTRUMENTATION_ENABLED
    if (k->dbg_sink) {
        k->dbg_sink->debug_record_metric(allocOverhead, 0, KOKROTDBG_METRIC_MACRO_ALLOCATION_OVERHEAD,
                k->dbg_sink->callback_param);
    }
#endif

    *out_code_count = k->qr_code_count;
    report_prss(k);
    LOGS("Exiting Macrocosm, all successful");
    return(macro_err_success);
}

