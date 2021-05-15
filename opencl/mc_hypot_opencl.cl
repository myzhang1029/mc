#define uint64_t ulong
#define uint32_t uint
#include "../pcg_impl/pcg.h"

/* mc kernel function */
kernel void monte_carlo(
    /* Calculate parameter */
    float scaled_radius,
    /* Array of results */
    global uchar *results,
    /* Modulo for results because memory might be limited to fit all samples */
    uint32_t modulo)
{
    float rmax;
    float x_dot, y_dot, d1, d2;
    pcg32_random_t rng;
    size_t rank = get_global_id(0);

    rmax = 2 * scaled_radius + 1;
    pcg32_srand(&rng, 42UL, 430UL);
    /* Skip "previous" workers */
    pcg32_advance(&rng, rank * 2);
    x_dot = pcg32_rand(&rng) / (float)UINT_MAX * rmax;
    y_dot = pcg32_rand(&rng) / (float)UINT_MAX * rmax;
    d1 = hypot(scaled_radius - x_dot, scaled_radius - y_dot);
    d2 = hypot(2 * scaled_radius - x_dot, y_dot);
    /* Store & Return */
    if (d1 < scaled_radius && d2 >= 2 * scaled_radius)
        results[rank % modulo] = 1;
    else
        results[rank % modulo] = 0;
}
