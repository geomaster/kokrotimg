#include "common.h"
#include <malloc.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

void* kokmalloc(kok_data_t* k, uint32_t bytes)
{
    void *ptr = malloc(sizeof(kok_memory_block_t) + bytes);
    if (!ptr) {
        return(NULL);
    } else {
        k->resident_set_bytes += bytes;
        update_memory_peak(k);
        ((kok_memory_block_t*)ptr)->size = bytes;
        return(ptr + sizeof(kok_memory_block_t));
    }
}

void kokfree(kok_data_t* k, void* ptr)
{
    ptr -= sizeof(kok_memory_block_t);
    uint32_t bytes = ((kok_memory_block_t*)ptr)->size;
    free(ptr);
    k->resident_set_bytes -= bytes;

}

void update_memory_peak(kok_data_t* k)
{
    if (k->resident_set_bytes > k->peak_resident_set_bytes) {
        k->peak_resident_set_bytes = k->resident_set_bytes;
    }
}

int queue_create(kok_data_t* k, int size, kok_queue_t* out_queue)
{
    kok_point_t *data = (kok_point_t*) kokmalloc(k, sizeof(kok_point_t) * size);
    if (!data) {
        return( 0 );
    }

    out_queue->queue = data;
    out_queue->start = out_queue->end = 0;
    out_queue->size = size;
}

void queue_push_back(kok_queue_t* q, kok_point_t pt)
{
    q->queue[q->end] = pt;
    q->end = (q->end + 1) % q->size;
    ASSERT(q->end != q->start, "The queue is full! This call shouldn't have happened.");
}

kok_point_t queue_pop_front(kok_queue_t* q)
{
    ASSERT(q->start != q->end, "The queue is empty! This call shouldn't have happened.");
    kok_point_t p = q->queue[q->start];
    q->start = (q->start + 1) % q->size;

    return( p );
}

int queue_get_size(kok_queue_t* q)
{
    return q->size;
}

void queue_clear(kok_queue_t* q)
{
    q->start = q->end = 0;
}

void queue_destroy(kok_data_t* k, kok_queue_t* q)
{
    kokfree(k, q->queue);
    q->size = 0; /* will make further calls to queue_push_back or queue_pop_front bomb
                    with a division by zero */
}

int convex_hull(kok_point_t* points, int count, kok_point_t* hull)
{
    /* Monotone chain algorithm, "Computational Geometry: Algorithms and Applications", de Berg et al. (2006),
     * implementation adapted from http://en.wikibooks.org/wiki/Algorithm_Implementation/Geometry/Convex_hull/Monotone_chain */

    /* compare by x, then by y */
    int pt_lex_compare(const void* a, const void* b) {
        kok_point_t * pa = (kok_point_t*)a, *pb = (kok_point_t*)b;
        int diff = pa->x - pb->x;
        if (diff != 0)
            return diff;
        else return pa->y - pb->y;
    }

    /* sort the points */
    qsort(points, count, sizeof(kok_point_t), &pt_lex_compare);

    int hsize = 0;

    /* compute the "upper hull" of the points using a classic Graham scan */
    int i;
    for (i = 0; i < count; ++i) {
        while (hsize >= 2 && SIGNED_AREA(hull[hsize - 2], hull[hsize - 1], points[i]) <= 0)
            --hsize;
        hull[hsize++] = points[i];
    }

    /* similarly, append now the lower hull of the points
     *
     * NB that we invert the direction of the scan so as to keep the hull strictly clockwise
     */
    int t;
    for (i = count - 2, t = hsize + 1; i >= 0; --i) {
        while (hsize >= t && SIGNED_AREA(hull[hsize - 2], hull[hsize - 1], points[i]) <= 0)
            --hsize;
        hull[hsize++] = points[i];
    }

    if (hsize <= 0)
        return 0;

    --hsize; /* avoid duplication of last elem */
    return hsize;
}

inline void bresenham_line(byte* buf, byte val, dimension w, kok_point_t a, kok_point_t b)
{
    int x0 = a.x, y0 = a.y, x1 = b.x, y1 = b.y, steep = absdiff(y1, y0) > absdiff(x1, x0);

    /* swap so our life is easier, x becomes y and vice versa */
    if (steep) {
        swap(x0, y0);
        swap(x1, y1);
    }

    /* so no negative steps */
    if (x0 > x1) {
        swap(x0, x1);
        swap(y0, y1);
    }

    /* classic bresennham line algorithm */
    int dx = x1 - x0, dy = absdiff(y1, y0), err = dx / 2, ystep, y = y0, x;
    ystep = (y0 < y1 ? 1 : -1);
    for (x = x0; x <= x1; ++x) {
        /* if (steep) then y and x are swapped, remember? */
        if (steep) {
            buf[M2D(y, x, w)] = val;
        } else {
            buf[M2D(x, y, w)] = val;
        }

        err -= dy;
        if (err < 0) {
            y += ystep;
            err += dx;
        }
    }
}

inline void bresenham_polygon(byte* buf, byte val, dimension w, kok_point_t* polygon, int count)
{
    int i;
    for (i = 0; i < count; ++i)
        bresenham_line(buf, val, w, polygon[i], polygon[(i + 1) % count]);
}

#define PIXEL_BLACK_MARK_MASK                 (1 << 7)
#define POLYGON_MARK_MASK                     (1 << 0)

int polygon_sum(byte *buf, dimension w, dimension h, const kok_point_t* points, int count, int *pixelcount)
{
    /* what we'll do here is a little tricky to read if you don't know the approach,
     * but is a very good optimization.
     *
     * we're given a convex polygon and we need to find pixels inside it. we could model an
     * AA bounding rectangle and test for each of the points, but the testing is linear
     * in the number of edges. 
     *
     * first, i have to assume that the points are sorted clockwise. the output of
     * convex_hull satisfies this property. next, i'll pick the topmost point and have
     * a scanline at that point. note that the number of pixels on the scanline and
     * in the polygon is 0.
     *
     * then, i will advance the scanline pixel by pixel, and in the process, draw 
     * bresenham lines (by "draw" i mean "keep track of x-coordinates for the current
     * y-coordinates") from the first point in two directions: clockwise and counter-
     * clockwise.
     *
     * this means that at each point i have the starting x and the ending x, and i just
     * need to sum the pixel values in between (i know the scanline). the complexity
     * is O(A), A being the polygon area. the first, naive approach, would have taken
     * O(An), where n is the number of points in the polygon. as n can be large, this
     * is not feasible.
     */

    int i, pxcount = 0, sum = 0;

    kok_point_t top = points[0], bottom = points[0];
    int topi = 0;

    /* find extreme points */
    for (i = 1; i < count; ++i) {
        kok_point_t p = points[i];
        if (p.y < top.y) {
            topi = i;
            top = p;
        } else if (p.y > bottom.y) {
            bottom = p;
        }
    }

    /* this is the structure to hold the bresenham line state */
    struct bresenham {
        int x0, y0, x1, y1, steep, dx, dy, err, ystep, x, y;
    } lines[ 2 ];

    /* this will initialize a bresenham line state structure to (x0, y0) and make
     * it draw towards (x1, y1) with each call to the y-advancing proc */
    inline void init_bresenham(struct bresenham* bh, int x0, int y0, int x1, int y1) 
    {
        bh->x0 = x0;
        bh->y0 = y0;
        bh->x1 = x1;
        bh->y1 = y1;

        bh->steep = absdiff(bh->y1, bh->y0) > absdiff(bh->x1, bh->x0);

        /* int i1 = M2D(x0, y0, w), i2 = M2D(x1, y1, w); */
        /* byte val1 = buf[i1] ^ mask; */
        /* byte val2 = buf[i2] ^ mask; */

        if (bh->steep) {
            swap(bh->x0, bh->y0);
            swap(bh->x1, bh->y1);
        }

        if (bh->x0 > bh->x1) {
            swap(bh->x0, bh->x1);
            swap(bh->y0, bh->y1);
        }

        bh->dx = bh->x1 - bh->x0;
        bh->dy = absdiff(bh->y1, bh->y0); 
        bh->err = bh->dx / 2;
        bh->y = bh->y0;
        bh->x = bh->x0;
        bh->ystep = (bh->y0 < bh->y1 ? 1 : -1);
    }

    /* this will advance the y-coordinate of the line and return the new x-coordinate,
     * findmin and corresponding code is to yield good values of x when a single y
     * coordinate corresponds to multiple coordinates */
    inline int bresenham_advance_y(struct bresenham* bh, const int findmin)
    {
        int oldy = (bh->steep? bh->x : bh->y);
        int resx = (bh->steep? bh->y : bh->x);

        /* dodgy... just take care of what is x and what is y, because of the 
         * .steep boolean. why not take pointers and let the optimizer care for
         * the rest? */
        int* py = (bh->steep? &bh->x : &bh->y), *px = (bh->steep? &bh->y : &bh->x);

        if (bh->steep) {
            if (bh->dx == 0)
                return 0;
        } else {
            if (bh->dy == 0)
                return 0;
        }

        while (*py == oldy && bh->x < bh->x1) {
            if (findmin && *px < resx)
                resx = *px;
            else if (!findmin && *px > resx)
                resx = *px;

            bh->err -= bh->dy;
            if (bh->err < 0) {
                bh->y += bh->ystep;
                bh->err += bh->dx;
            }

            ++bh->x;
        }

        if (!bh->steep && bh->ystep == -1)
            return (bh->x0 + (bh->x1 - resx));
        else return (resx);
    }

    /* initialize the points we're considering */
    int pt0i = (topi + count - 1) % count, pt1i = topi;
    kok_point_t pt0 = points[pt0i], pt0next = points[topi], pt1 = points[pt1i], pt1next = points[(topi + 1) % count];

    /* start the lines */
    init_bresenham(&lines[0], pt0next.x, pt0next.y, pt0.x, pt0.y);
    init_bresenham(&lines[1], pt1.x, pt1.y, pt1next.x, pt1next.y);

    int x, y;
    if (top.y < 0) top.y = 0;
    if (top.y >= h) top.y = h - 1;
    if (bottom.y < 0) bottom.y = 0;
    if (bottom.y >= h) bottom.y = h - 1;


    /* scanline by scanline */
    for (y = top.y; y < bottom.y; ++y) {
        /* advance the two lines in opposite directions */
        int xstart = bresenham_advance_y(&lines[0], 1);
        int xend = bresenham_advance_y(&lines[1], 0);

        if (xstart < 0) xstart = 0;
        if (xstart >= w) xstart = w - 1;

        for (x = xstart; x <= xend; ++x) {
            /* add this value to the sum */
            sum += (int)buf[M2D(x, y, w)];
            ++pxcount;
        }

        if (y >= pt0.y) {
            /* means we've passed the line y, we need a new one */
            do {
                pt0next = points[pt0i];
                pt0i = (pt0i + count - 1) % count;
                pt0 = points[pt0i];
            } while (pt0next.y == pt0.y); /* to skip the points with the same y */

            /* init the new line */
            init_bresenham(&lines[0], pt0next.x, pt0next.y, pt0.x, pt0.y);
        } else if (y >= pt1next.y) {
            do {
                pt1 = pt1next;
                pt1i = (pt1i + 1) % count;
                pt1next = points[(pt1i + 1) % count];
            } while (pt1next.y == pt1.y);

            init_bresenham(&lines[1], pt1.x, pt1.y, pt1next.x, pt1next.y);
            /* see above */
        }
    }

    /* pass the total pixel count and the pixel sum */
    *pixelcount = pxcount;
    return sum;
}

int small_scale_bfs(kok_data_t* k, kok_point_t* where, byte* buf, int w, int maxpts, kok_point_t start, kok_point_t topleft, kok_point_t bottomright, byte val, int* fullcount)
{
    kok_queue_t* queue = &k->bfs_queue;
    int count = 1;
    queue_clear(queue);
    queue_push_back(queue, start);

    int pts = 0;

    if (start.x < topleft.x || start.y >= bottomright.y || start.x >= bottomright.x || start.y < topleft.y)
        return 0;

    int x, y;
    for (y = topleft.y; y <= bottomright.y; ++y)
        for (x = topleft.x; x <= bottomright.x; ++x)
            k->pixelinfo[M2D(x, y, w)].small_bfs_visited = 0;

    k->pixelinfo[M2D(start.x, start.y, w)].small_bfs_visited = 1;
    int fc = 0;
    /* nothing interesting here, really.
     *
     * we just expand neighbors, check if we visited them before, check
     * if we're out of bounds, mark it etc, then return the data we found, 
     * note that we need to put edge pixels in where and that's * our only 
     * requirement */

    while (count > 0 && pts < maxpts) {
        kok_point_t p = queue_pop_front(queue);
        --count;

        const int dx[] = { +1, -1, 0, 0 }, dy[] = { 0, 0, +1, -1 };
        ++fc;

        /* k->debug_img[M2D(p.x, p.y, w)] = dbg_cyan; */

        int i, boundary = 0;
        for (i = 0; i < 4; ++i) {
            kok_point_t np = { p.x + dx[i], p.y + dy[i] };

            if (np.x < topleft.x || np.x >= bottomright.x || np.y < topleft.y || np.y >= bottomright.y)
                continue;

            if (buf[M2D(np.x, np.y, w)] != val) {
                ++boundary;
                continue;
            }

            if (k->pixelinfo[M2D(np.x, np.y, w)].small_bfs_visited)
                continue;

            k->pixelinfo[M2D(np.x, np.y, w)].small_bfs_visited = 1;
            queue_push_back(queue, np);
            ++count;
        }

        if (boundary > 0) {
            where[pts++] = p;
        }

        if (count >= k->max_bfs_queue - 1) {
            LOGF("small_scale_bfs(%d): Maxxed out BFS queue, returning", k->context_finder_id);
            *fullcount = fc;
            return pts;
        }
    }
    
    if (pts == maxpts) {
        LOGF("small_scale_bfs(%d): Maxxed out points, bailing out (pts = %d, fc = %d)", k->context_finder_id,
                pts, fc);
    }

    *fullcount = fc;
    return pts;
}

inline int line_intersection(kok_point_t p1, kok_point_t p2, kok_point_t p3, kok_point_t p4, kok_point_t *i)
{
    /* some basic linear algebra to intersect the lines described by points p1, p2 and
     * p3, p4. not the line segments, but the lines themselves */
    int determinant = (p1.x - p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x - p4.x);
    if (!determinant)
        return 0;

    int x1y2my1x2 = (p1.x * p2.y - p1.y * p2.x), x3y4my3x4 = (p3.x * p4.y - p3.y * p4.x);
    kok_point_t res = {
        .x = (x1y2my1x2 * (p3.x - p4.x) -
                (p1.x - p2.x) * x3y4my3x4) / determinant,
        .y = (x1y2my1x2 * (p3.y - p4.y) -
                (p1.y - p2.y) * x3y4my3x4) / determinant
    };

    *i = res;
    return 1;
}

int reduce_polygon(kok_point_t* points, int pointcount, int newpointcount)
{
    /* this will approximate a newpointcount-gon from a previous pointcount-gon
     * that has a minimum area increase.
     *
     * we just loop over lines which we can collapse, like so:
     *
     *                               C
     *                             /   \
     *                            /     \
     *     A*____*B              A*-----*B
     *     /      \             /         \
     *    /        \           /           \
     *
     * (in the particular case, AB was collapsed into C)
     *
     * we just look at all adjacent points, and see what area will the polygon
     * gain if we do the collapse. at eand of each cycle, we pick the collapse
     * with the minimum area increase, commit it, and repeat the process until
     * there are only as much points as we need.
     *
     * this is a greedy approximation algorithm, but for our cases works remarkably
     * well. in particular, it seems to excel when angle_turn * line_length for all
     * lines is low. if there are big lines with big angle turns, the algorithm
     * can be easily fooled to commit an unoptimal sequence of collapses. */

    int i, j, oldpc = pointcount;
    while (pointcount > newpointcount) {
        /* find the first point */
        kok_point_t p0 = points[oldpc - 1];
        j = oldpc - 1;
        while (p0.x < 0) {
            p0 = points[(j = (j - 1 + oldpc) % oldpc)];
        }

        int minareai = -1, minareanexti = -1, minarea = 0;
        kok_point_t newpoint = { 0, 0 };
        
        for (i = 0; i < oldpc; ++i) {
            if (points[i].x < 0) {
                continue;
            }

            /* find the remaining points */
            kok_point_t p1 = points[i];

            j = i;
            do {
                j = (j + 1) % oldpc;
            } while (points[j].x < 0);
            kok_point_t p2 = points[j];
            int p2i = j;

            do {
                j = (j + 1) % oldpc;
            } while (points[j].x < 0);
            kok_point_t p3 = points[j];

            /* find the intersection point, if it doesn't exist, proceed */
            kok_point_t C;
            int intersection = line_intersection(p0, p1, p3, p2, &C);
            if (!intersection) continue;

            /* find the area of the triangle we're adding and test it against
             * the minimum */
            int area = abs(SIGNED_AREA(p1, C, p2));
            if (minareai == -1 || area < minarea) {
                minareai = i;
                minareanexti = p2i;
                newpoint = C;
                minarea = area;
            }
            p0 = p1;
        }

        /* there was not even a triangle! get out! */
        if (minareai == -1)
            return pointcount;

        /* perform the collapse and flag the next point as unneeded */
        points[minareai] = newpoint;
        points[minareanexti].x = -1;
        --pointcount;
    }

    /* remove all x==-1 points we have marked and deliver a clean n-gon
     * to the caller */
    j = 0;
    for (i = 0; i < oldpc; ++i) {
        if (points[i].x != -1)
            points[j++] = points[i];
    }

    return pointcount;
}


inline int bilinear_interpolation(double x, double y, int w, int h, byte* buf)
{
    int    x0i = (int)( x ), x1i = (int)( x + 0.5 ),
           y0i = (int)( y ), y1i = (int)( y + 0.5 );

    if (x0i < 0) x0i = 0;
    if (x1i >= w) x1i = w - 1;

    if (y0i < 0) y0i = 0;
    if (y1i >= h) y1i = h - 1;

    double xalpha = x - floor(x),
           yalpha = y - floor(y);

    int p00 = buf[M2D(x0i, y0i, w)], p01 = buf[M2D(x1i, y0i, w)],
        p10 = buf[M2D(x0i, y1i, w)], p11 = buf[M2D(x1i, y1i, w)];

    int val = (int)(
            ((1 - xalpha) * p00 + xalpha * p01) * (1 - yalpha) +
            ((1 - xalpha) * p10 + xalpha * p11) * (yalpha)
            );

    return( val );

}
void black_white_module_size(kok_data_t* k, int startx, int starty, int dx, int dy,
        int* blacksz, int* whitesz)
{
    int x = startx, y = starty;

#       define GO_WHILE_VAL(v) \
    while (x >= 0 && x < QR_BASESIZE && y >= 0 && y < QR_BASESIZE && \
            k->qr_code[y][x] == (v)) { \
        x += dx; \
        y += dy; \
    }

    /* k->qr_code_dbg[starty][startx]=dbg_darkred; */
    GO_WHILE_VAL(0);
    int whitestartx = x, whitestarty = y;
    GO_WHILE_VAL(255);
    int blackstartx = x, blackstarty = y;
    GO_WHILE_VAL(0);
    int blackstopx = x, blackstopy = y;

    if (dy == 0) {
        *blacksz = blackstopx - blackstartx;
        *whitesz = blackstartx - whitestartx;
    } else {
        *blacksz = blackstopy - blackstarty;
        *whitesz = blackstarty - whitestarty;
    }
#       undef GO_WHILE_VAL
}
inline double horizontal_slide(kok_pointf_t p, double slope, double newx)
{
    return p.y + slope * (newx - p.x);
}

inline double vertical_slide(kok_pointf_t p, double inv_slope, double newy)
{
    return p.x + inv_slope * (newy - p.y);
}

inline double get_slope(kok_pointf_t a, kok_pointf_t b)
{
    return (b.y - a.y) / (b.x - a.x);
}

inline double get_inv_slope(kok_pointf_t a, kok_pointf_t b)
{
    return (b.x - a.x) / (b.y - a.y);
}
kok_point_t makept(sdimension x, sdimension y)
{
    kok_point_t p = { x, y };
    return( p );
}

kok_pointf_t makeptf(double x, double y)
{
    kok_pointf_t p = { x, y };
    return( p );
}


kok_point_t ptf2pt(kok_pointf_t p)
{
    kok_point_t p2 = { round(p.x), round(p.y) };
    return(p2);
}

kok_pointf_t pt2ptf(kok_point_t p)
{
    kok_pointf_t p2 = { p.x, p.y };
    return(p2);
}
inline void infect_xy(byte* buf, kok_point_t start, dimension w, dimension h, 
        int* xcount_left, int* ycount_above, int* xcount_right, int* ycount_below)
{
    byte val = buf[M2D(start.x, start.y, w)];
    int x = start.x, y = start.y;

    while (y >= 0 && buf[M2D(x, y, w)] == val)
        --y;

    *ycount_above = start.y - y - 1;
    y = start.y;
    while (y < h && buf[M2D(x, y, w)] == val)
        ++y;

    *ycount_below = y - start.y - 1;
    y = start.y;
    while (x >= 0 && buf[M2D(x, y, w)] == val)
        --x;

    *xcount_left = start.x - x - 1;
    x = start.x;
    while (x < w && buf[M2D(x, y, w)] == val)
        ++x;

    *xcount_right = x - start.x - 1;
}

inline kok_pointf_t lerp(kok_pointf_t a, kok_pointf_t b, double alpha)
{
    return makeptf(a.x * (1.0 - alpha) + b.x * alpha, a.y * (1.0 - alpha) + b.y * alpha);
}

void flip_x(byte* buf, dimension w, dimension h)
{
    int i, j;
    for (j = 0; j < w; ++j) {
        for (i = 0; i < h / 2; ++i) {
            swap(buf[M2D(h - i - 1, j, w)], buf[M2D(i, j, w)]);
        }
    }
}

void rot90(byte* buf, dimension w)
{
    int x, y;
    for (x = 0; x < w / 2; ++x) {
        for (y = 0; y < w / 2; ++y) {
            byte tmp = buf[ M2D(x, y, w) ];
            buf[ M2D(x, y, w) ] = buf[ M2D( y, w - 1 - x, w) ];
            buf[ M2D(y, w - 1 - x, w) ] = buf[ M2D(w - 1 - x, w - 1 - y, w) ];
            buf[ M2D(w - 1 - x, w - 1 - y, w) ] = buf[ M2D(w - 1 - y, x, w) ];
            buf[ M2D(w - 1 - y, x, w) ] = tmp;
            /* swap(buf[M2D(i, j, w)], buf[M2D(j, i, w)]); */
        }
    }
}
