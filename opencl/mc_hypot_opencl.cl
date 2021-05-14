/* PCG generator
 *Really* minimal PCG32 code / (c) 2014 M.E. O'Neill / pcg-random.org
 Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)
 */

typedef struct
{
    ulong state;
    ulong inc;
} pcg32_random_t;

inline uint pcg32_rand(pcg32_random_t *rng)
{
    ulong oldstate = rng->state;
    // Advance internal state
    rng->state = oldstate * 6364136223846793005ULL + (rng->inc | 1);
    // Calculate output function (XSH RR), uses old state for max ILP
    uint xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

inline void pcg32_srand(pcg32_random_t *rng, ulong initstate, ulong initseq)
{
    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    pcg32_rand(rng);
    rng->state += initstate;
    pcg32_rand(rng);
}

// Derived from PCG's pcg_setseq_64_advance_r and pcg_advance_lcg_64
inline void pcg32_advance(pcg32_random_t *rng, ulong delta)
{
    ulong acc_mult = 1u;
    ulong acc_plus = 0u;
    ulong cur_mult = 6364136223846793005ULL;
    ulong cur_plus = rng->inc;

    while (delta > 0)
    {
        if (delta & 1)
        {
            acc_mult *= cur_mult;
            acc_plus = acc_plus * cur_mult + cur_plus;
        }
        cur_plus = (cur_mult + 1) * cur_plus;
        cur_mult *= cur_mult;
        delta /= 2;
    }
    rng->state = acc_mult * rng->state + acc_plus;
}

/* End PCG generator */

/* mc kernel function */
kernel void monte_carlo(
    /* Calculate parameter */
    double scaled_radius,
    /* Array of results */
    global uchar *results,
    /* Modulo for results because memory might be limited to fit all samples */
    uint modulo)
{
    double rmax;
    double x_dot, y_dot, d1, d2;
    pcg32_random_t rng;
    size_t rank = get_global_id(0);

    rmax = 2 * scaled_radius + 1;
    pcg32_srand(&rng, 42UL, 430UL);
    /* Skip "previous" workers */
    pcg32_advance(&rng, rank * 2);
    x_dot = pcg32_rand(&rng) / (double)UINT_MAX * rmax;
    y_dot = pcg32_rand(&rng) / (double)UINT_MAX * rmax;
    d1 = hypot(scaled_radius - x_dot, scaled_radius - y_dot);
    d2 = hypot(2 * scaled_radius - x_dot, y_dot);
    /* Store & Return */
    if (d1 < scaled_radius && d2 >= 2 * scaled_radius)
        results[rank % modulo] = 1;
    else
        results[rank % modulo] = 0;
}
