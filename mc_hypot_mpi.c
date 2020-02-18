#include <inttypes.h>
#include <math.h>
#include <mpi.h>
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

uint64_t monte_carlo_core(double r, uint64_t startpoint, uint64_t endpoint)
{
    double rmax = 2 * r + 1;
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
        for (; startpoint < endpoint; ++startpoint)
        {
            x_dot = pcg32_rand(&thrd_rngx)/(double)UINT32_MAX * rmax;
            y_dot = pcg32_rand(&thrd_rngy)/(double)UINT32_MAX * rmax;
            d1 = hypot(r - x_dot, r - y_dot);
            d2 = hypot(2 * r - x_dot, y_dot);
            if (d1 < r && d2 >= 2 * r)
                ++inside;
        }
    }
    return inside;
}

int main(int argc, char **argv)
{
    uint64_t rand_samples = UINT64_C(1000000000);
    uint64_t each, adjust = 0;
    uint32_t radius = 5;

    double r = radius * rand_samples;

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
        printf("Each process: %" PRIu64 "points, adjust %" PRIu64 "\n", each, adjust);
        inside = monte_carlo_core(r, 0, each + adjust);
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
        inside =
            monte_carlo_core(r, me * each + adjust, (me + 1) * each + adjust);
        MPI_Send(&inside, 1, MPI_UNSIGNED_LONG_LONG, 0, 1, MPI_COMM_WORLD);
    }
    MPI_Finalize();
    return 0;
}

