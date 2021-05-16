#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "pcg_impl/pcg.h"

// This function significantly speeds things up - with a very low risk of
// overflow Always positive
static inline double sqhypot(const double a, const double b)
{
    return fma(a, a, b * b);
}

// The area can be obtained with inside / rand_samples * 4 * radius * radius
uint64_t monte_carlo(const double radius, const uint64_t rand_samples)
{
    const double r = radius * rand_samples;
    const double sqr = r * r;
    const double rmax = 2 * r + 1;
    /* Scale onto the generated random number */
    const double scale = ldexp(rmax, -32);
    uint64_t inside = 0;

#pragma omp parallel
    {
        double x_dot, y_dot;
        double sqd1, sqd2;
        uint64_t i;
        pcg32_random_t thrd_rng;
        pcg32_srand(&thrd_rng, UINT64_C(42), UINT64_C(430));
#pragma omp for reduction(+ : inside)
        for (i = 0; i < rand_samples; ++i)
        {
            x_dot = pcg32_rand(&thrd_rng) * scale;
            y_dot = pcg32_rand(&thrd_rng) * scale;
            sqd1 = sqhypot(r - x_dot, r - y_dot);
            sqd2 = sqhypot(2 * r - x_dot, y_dot);
            if (sqd1 < sqr && sqd2 >= 4 * sqr)
                ++inside;
        }
    }
    return inside;
}

#ifdef USE_MPI

#include <mpi.h>

int main(int argc, char **argv)
{
    uint64_t rand_samples = UINT64_C(1280000000);
    // 146381717756/1000000000000
    uint64_t each, adjust = 0;
    double radius = 5;

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
    double radius = 5.0;
    uint64_t rand_samples = UINT64_C(1280000000);
    uint64_t inside = monte_carlo(radius, rand_samples);
    printf("%" PRIu64 "/%" PRIu64 "\n", inside, rand_samples);
    double size = inside / (double)rand_samples * 4 * radius * radius;
    printf("%g\n", size);
    return 0;
}

#endif
