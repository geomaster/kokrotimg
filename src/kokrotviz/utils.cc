#include "utils.h"
#include "exceptions.h"
#include <cmath>
using namespace kokrotviz;

void Utils::hsvToRgb(float h, float s, float v, float* r, float* g, float* b)
{
    if (h < 0) h = 0;
    if (h > 359) h = 359;
    if (s < 0) s = 0;
    if (s > 1) s = 100;
    if (v < 0) v = 0;
    if (v > 1) v = 100;

    float tmp = h/60.0;
    int hi = floor(tmp);
    float f = tmp - hi;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch (hi) {
        case 0:
            *r = v;
            *g = t;
            *b = p;
            break;
        case 1:
            *r = q;
            *g = v;
            *b = p;
            break;
        case 2:
            *r = p;
            *g = v;
            *b = t;
            break;
        case 3:
            *r = p;
            *g = q;
            *b = v;
            break;
        case 4:
            *r = t;
            *g = p;
            *b = v;
            break;
        case 5:
            *r = v;
            *g = p;
            *b = q;
            break;
    }
}


