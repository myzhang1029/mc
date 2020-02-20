/* PCG generator
 *Really* minimal PCG32 code / (c) 2014 M.E. O'Neill / pcg-random.org
 Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)
 */

typedef struct
{
    ulong state;
    ulong inc;
} pcg32_random_t;

inline void
pcg32_rand(
    pcg32_random_t* rng,
    uint *result
)
{
    ulong oldstate = rng->state;
    // Advance internal state
    rng->state = oldstate * 6364136223846793005ULL + (rng->inc|1);
    // Calculate output function (XSH RR), uses old state for max ILP
    uint xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint rot = oldstate >> 59u;
    *result = (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

inline void
pcg32_srand(
    pcg32_random_t *rng,
    ulong initstate,
    ulong initseq
)
{
    uint _; /* Unused */
    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    pcg32_rand(rng, &_);
    rng->state += initstate;
    pcg32_rand(rng, &_);
}

/* End PCG generator */

/* mc kernel function */
kernel void
monte_carlo(
    double radius,
    ulong rand_samples,
    /* Array of workers */
    global ulong *pinside,
    /* Workers in total */
    ulong count,
    /* Somehow the GPU doesn't seed well */
    ulong seed
)
{
    double r;
    double rmax;
    ulong i, inside;
    double x_dot, y_dot, d1, d2;
    uint rand_result;
    pcg32_random_t rng;
    size_t rank = get_global_id(0);
    
    if (rank > count)
        return;

    r = radius * rand_samples;
    rmax = 2 * r + 1;
    inside = 0;
    pcg32_srand(&rng, 42UL, seed + rank);

    for (i = 0; i < rand_samples; ++i)
    {
        pcg32_rand(&rng, &rand_result);
        x_dot = rand_result/(double)UINT_MAX * rmax;
        pcg32_rand(&rng, &rand_result);
        y_dot = rand_result/(double)UINT_MAX * rmax;
        d1 = hypot(r - x_dot, r - y_dot);
        d2 = hypot(2 * r - x_dot, y_dot);
        if (d1 < r && d2 >= 2 * r)
            ++inside;
    }
    /* Store & Return */
    pinside[rank] = inside;
}
