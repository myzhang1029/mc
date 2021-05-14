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

static inline uint32_t pcg32_rand(pcg32_random_t *rng)
{
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

static inline void pcg32_srand(pcg32_random_t *rng, uint64_t initstate)
{
    rng->state = 0U;
    rng->inc = ((uint64_t)rng << 1u) | 1u;
    pcg32_rand(rng);
    rng->state += initstate;
    pcg32_rand(rng);
}

static inline void pcg32x2_srand(pcg32x2_random_t *rng, uint64_t initstates)
{
    pcg32_srand(rng->gen, initstates);
    pcg32_srand(rng->gen + 1, initstates);
}

static inline uint64_t pcg32x2_rand(pcg32x2_random_t *rng)
{
    return ((uint64_t)(pcg32_rand(rng->gen)) << 32) | pcg32_rand(rng->gen + 1);
}

static inline uint64_t pcg32x2_uniform(pcg32x2_random_t *rng, uint64_t bound)
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

/* When two uint64_ts multiply, it's a uint128.
 * so we use double */
#if __SIZEOF__INT128__ == 16
typedef __uint128_t bigint;
#else
typedef double bigint;
#endif

static inline double y1_upper(bigint x, uint64_t r)
{
    return r + sqrt(2 * x * r - x * x);
}

static inline double y1_lower(bigint x, uint64_t r)
{
    return r - sqrt(2 * x * r - x * x);
}

static inline double y2(bigint x, uint64_t r)
{
    return sqrt(4 * x * r - x * x);
}

// The area can be obtained with inside / rand_samples * 4 * radius * radius
uint64_t monte_carlo(uint32_t radius, uint64_t rand_samples)
{
    uint64_t r = radius * rand_samples;
    uint64_t rmax = 2 * r + 1;
    uint64_t i;
    /* Avoid data race */
    _Atomic uint64_t inside = 0;
    /* Compile time */
    const double q3msqrt7 = (3 - sqrt(7)) / 4.0, q3asqrt7 = (3 + sqrt(7)) / 4.0;

#pragma omp parallel
    {
        /* Using two rngs for x and y makes the
         * sequence more uniform, and it costs no
         * extra time */
        uint64_t x_dot, y_dot;
        double yu, yl, yt;
        pcg32x2_random_t thrd_rngx, thrd_rngy;
        pcg32x2_srand(&thrd_rngx, UINT64_C(42));
        pcg32x2_srand(&thrd_rngy, UINT64_C(42));
#pragma omp for
        for (i = 0; i < rand_samples; ++i)
        {
            x_dot = pcg32x2_uniform(&thrd_rngx, rmax);
            y_dot = pcg32x2_uniform(&thrd_rngy, rmax);
            yu = y1_upper(x_dot, r);
            yl = y1_lower(x_dot, r);
            yt = y2(x_dot, r);
            /* left segment */
            if (0 <= x_dot && x_dot < r * q3msqrt7)
                if (yl <= y_dot && y_dot < yu)
                    ++inside;
            /* right segment */
            if (r * q3msqrt7 <= x_dot && x_dot < r * q3asqrt7)
                if (yt <= y_dot && y_dot < yu)
                    ++inside;
        }
    }
    return inside;
}

#ifdef USE_MPI

#include <mpi.h>

int main(int argc, char **argv)
{
    uint64_t rand_samples = UINT64_C(1000000000);
    uint64_t each, adjust = 0;
    uint32_t radius = 5;

    uint64_t inside;
    double size;
    int nproc, me;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &me);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    each = rand_samples / nproc;
    if (rand_samples % nproc != 0)
        adjust = rand_samples - nproc * each;
    if (me == 0) /* Controller process */
    {
        printf("Each process: %" PRIu64 " points, adjust %" PRIu64 "\n", each,
               adjust);
        inside = monte_carlo(radius, each + adjust);
        for (int i = 1; i < nproc; ++i)
        {
            uint64_t in;
            MPI_Recv(&in, 1, MPI_UNSIGNED_LONG_LONG, i, 1, MPI_COMM_WORLD,
                     &status);
            inside += in;
        }
        size = inside / (double)rand_samples * 4 * radius * radius;
        printf("%" PRIu64 "/%" PRIu64 "\n", inside, rand_samples);
        printf("%g\n", size);
    }
    else /* Not controller */
    {
        inside = monte_carlo(radius, each + adjust);
        MPI_Send(&inside, 1, MPI_UNSIGNED_LONG_LONG, 0, 1, MPI_COMM_WORLD);
    }
    MPI_Finalize();
    return 0;
}

#else

int main()
{
    /* Algorithm efficiency:
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
    double radius = 5.0;
    uint64_t rand_samples = UINT64_C(1000000000);
    uint64_t inside = monte_carlo(radius, rand_samples);
    printf("%" PRIu64 "/%" PRIu64 "\n", inside, rand_samples);
    double size = inside / (double)rand_samples * 4 * radius * radius;
    printf("%g\n", size);
    return 0;
}

#endif
