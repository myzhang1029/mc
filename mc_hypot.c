#include <inttypes.h>
#include <math.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>

/* PCG generator */
typedef struct
{
    uint64_t state;
    uint64_t inc;
} pcg32_random_t;

typedef struct
{
    pcg32_random_t gen[2];
} pcg32x2_random_t;

inline uint32_t pcg32_rand(pcg32_random_t *rng)
{
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

inline void pcg32_srand(pcg32_random_t *rng, uint64_t initstate)
{
    rng->state = 0U;
    rng->inc = ((uint64_t)rng << 1u) | 1u;
    pcg32_rand(rng);
    rng->state += initstate;
    pcg32_rand(rng);
}

inline void pcg32x2_srand(pcg32x2_random_t *rng, uint64_t initstates)
{
    pcg32_srand(rng->gen, initstates);
    pcg32_srand(rng->gen + 1, initstates);
}

inline uint64_t pcg32x2_rand(pcg32x2_random_t *rng)
{
    return ((uint64_t)(pcg32_rand(rng->gen)) << 32) | pcg32_rand(rng->gen + 1);
}

inline uint64_t pcg32x2_uniform(pcg32x2_random_t *rng, uint64_t bound)
{
    uint64_t threshold = -bound % bound;
    for (;;)
    {
        uint64_t r = pcg32x2_rand(rng);
        if (r >= threshold)
            return r % bound;
    }
}
/* End PCG generator */

double monte_carlo(uint32_t radius, uint64_t rand_samples)
{
    uint64_t r = radius * rand_samples;
    uint64_t rmax = 2 * r + 1;
    uint64_t i;
    /* Avoid data race */
    _Atomic uint64_t inside = 0;

#pragma omp parallel
    {
        /* Using two rngs for x and y makes the
         * sequence more uniform, and it costs no
         * extra time */
        uint64_t x_dot, y_dot;
        double d1, d2;
        pcg32x2_random_t thrd_rngx, thrd_rngy;
        pcg32x2_srand(&thrd_rngx, UINT64_C(42));
        pcg32x2_srand(&thrd_rngy, UINT64_C(42));
#pragma omp for
        for (i = 0; i < rand_samples; ++i)
        {
            x_dot = pcg32x2_uniform(&thrd_rngx, rmax);
            y_dot = pcg32x2_uniform(&thrd_rngy, rmax);
            uint64_t dx = r > x_dot ? r - x_dot : x_dot-r;
            uint64_t dy = r > y_dot ? r - y_dot : y_dot-r;
            d1 = hypot(dx, dy);
            d2 = hypot(2 * r - x_dot, y_dot);
            if (d1 < r && d2 >= 2 * r)
                ++inside;
        }
    }
    printf("%" PRIu64 "/%" PRIu64 "\n", inside, rand_samples);
    return inside / (double)rand_samples * 4 * radius * radius;
}

int main()
{
    /* Algorithm sufficiency:
     * result: avg 3, effective rounding
     * samp result(avg)    timing(one)
     * 1e6  146286.0(3)         0.03s user 0.00s system 238% cpu 0.013 total
     * 1e7  1463853.7(3)        0.28s user 0.00s system 351% cpu 0.081 total
     * 1e8  14633653.3(3)       2.84s user 0.00s system 383% cpu 0.741 total
     * 1e9  146382682.0(3)     28.29s user 0.04s system 386% cpu 7.323 total
     * 1e10 1463808882.7(3)   275.08s user 0.61s system 317% cpu 1:26.79 total
     * 1e11 14638184449.0(2) 2834.62s user 3.75s system 377% cpu 12:32.64 total
     * 1e12 146381275885(1) 28990.61s user 35.68s system 367% cpu 2:11:39.73
     * total
     *
     * accur:
     * (pi-arccos(-sqrt(2)/4)-4*arccos(5*sqrt(2)/8)+2*sin(2*arccos(5*sqrt(2)
     * /8))-0.5*sin(2*pi-2*arccos(-sqrt(2)/4)))*r*r
     * =14.638125953034784
     */
    double size = monte_carlo(UINT64_C(5), UINT64_C(1000000000));
    printf("%g\n", size);
    return 0;
}
