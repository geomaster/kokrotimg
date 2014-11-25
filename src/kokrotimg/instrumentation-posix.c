#include "common.h"
#include <time.h>

clock_t g_InstrumentationTime;

void instrumentation_start()
{
    g_InstrumentationTime = clock();
}

double instrumentation_end()
{
    clock_t now = clock();
    return (double)(now - g_InstrumentationTime) / (double)(CLOCKS_PER_SEC / 1000.0);
}


