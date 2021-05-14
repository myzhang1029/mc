#include <inttypes.h>
#include <math.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>

/* PCG generator
 *Really* minimal PCG32 code / (c) 2014 M.E. O'Neill / pcg-random.org
 Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)
 */

typedef struct { uint64_t state;  uint64_t inc; } pcg32_random_t;

uint32_t pcg32_rand(pcg32_random_t* rng)
{
    uint64_t oldstate = rng->state;
    // Advance internal state
    rng->state = oldstate * 6364136223846793005ULL + (rng->inc|1);
    // Calculate output function (XSH RR), uses old state for max ILP
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

void pcg32_srand(pcg32_random_t *rng, uint64_t initstate)
{
    rng->state = 0U;
    rng->inc = ((uint64_t)rng << 1u) | 1u;
    pcg32_rand(rng);
    rng->state += initstate;
    pcg32_rand(rng);
}

/* End PCG generator */

// This line significantly speeds things up - with a very low risk of overflow
inline double hypot_smp(double a, double b) { return sqrt(a*a+b*b); }

double monte_carlo(double radius, uint64_t rand_samples)
{
    double r = radius * rand_samples;
    double rmax = 2 * r + 1;
    uint64_t i;
    /* Avoid data race */
    _Atomic uint64_t inside = 0;

#pragma omp parallel
    {
        double x_dot, y_dot;
        double d1, d2;
        pcg32_random_t thrd_rngx, thrd_rngy;
        pcg32_srand(&thrd_rngx, UINT64_C(42));
        pcg32_srand(&thrd_rngy, UINT64_C(42));
#pragma omp for
        for (i = 0; i < rand_samples; ++i)
        {
            x_dot = pcg32_rand(&thrd_rngx)/(double)UINT32_MAX * rmax;
            y_dot = pcg32_rand(&thrd_rngy)/(double)UINT32_MAX * rmax;
            d1 = hypot_smp(r - x_dot, r - y_dot);
            d2 = hypot_smp(2 * r - x_dot, y_dot);
            if (d1 < r && d2 >= 2 * r)
                ++inside;
        }
    }
    printf("%" PRIu64 "/%" PRIu64 "\n", inside, rand_samples);
    return inside / (double)rand_samples * 4 * radius * radius;
}

int main()
{
    /* Algorithm efficiency:
     * result: avg 3, effective rounding
     * samp result(avg)    timing(one)
     * 1e6  146667.7(3)         0.03s user 0.00s system 287% cpu 0.012 total
     * 1e7  1463004.0(3)        0.33s user 0.00s system 310% cpu 0.106 total
     * 1e8  14639563.7(3)       3.05s user 0.01s system 366% cpu 0.832 total
     * 1e9  146382682.0(3)     28.29s user 0.04s system 386% cpu 7.323 total
     * 1e10 1463808882.7(3)   275.08s user 0.61s system 317% cpu 1:26.79 total
     * 1e11 14638184449.0(2) 2834.62s user 3.75s system 377% cpu 12:32.64 total
     * 1e12 146381605453(1) 31440.66s user 57.17s system 389% cpu 2:14:56.27 total
     * total
     *
     * accur:
     * (pi-arccos(-sqrt(2)/4)-4*arccos(5*sqrt(2)/8)+2*sin(2*arccos(5*sqrt(2)
     * /8))-0.5*sin(2*pi-2*arccos(-sqrt(2)/4)))*r*r
     * =14.638125953034784
     */
    double size = monte_carlo(5.0, UINT64_C(1000000000));
    printf("%g\n", size);
    return 0;
}
