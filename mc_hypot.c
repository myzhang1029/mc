#include <inttypes.h>
#include <math.h>
#include <omp.h>
#include <stdint.h>
#include <stdio.h>

#include "pcg_impl/pcg.h"

// This line significantly speeds things up - with a very low risk of overflow
static inline double hypot_smp(const double a, const double b)
{
    return sqrt(fma(a, a, b * b));
}

// The area can be obtained with inside / rand_samples * 4 * radius * radius
uint64_t monte_carlo(const double radius, const uint64_t rand_samples)
{
    const double r = radius * rand_samples;
    const double rmax = 2 * r + 1;
    /* Scale onto the generated random number */
    const double scale = ldexp(rmax, -32);
    uint64_t inside = 0;

#pragma omp parallel
    {
        double x_dot, y_dot;
        double d1, d2;
        uint64_t i;
        pcg32_random_t thrd_rng;
        pcg32_srand(&thrd_rng, UINT64_C(42), UINT64_C(430));
#pragma omp for reduction(+ : inside)
        for (i = 0; i < rand_samples; ++i)
        {
            x_dot = pcg32_rand(&thrd_rng) * scale;
            y_dot = pcg32_rand(&thrd_rng) * scale;
            d1 = hypot_smp(r - x_dot, r - y_dot);
            d2 = hypot_smp(2 * r - x_dot, y_dot);
            if (d1 < r && d2 >= 2 * r)
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
    /* Algorithm efficiency:
     * result: avg 3, effective rounding
     * samp result(avg)    timing(one)
     * 1e6  146667.7(3)         0.03s user 0.00s system 287% cpu 0.012 total
     * 1e7  1463004.0(3)        0.33s user 0.00s system 310% cpu 0.106 total
     * 1e8  14639563.7(3)       3.05s user 0.01s system 366% cpu 0.832 total
     * 1e9  146382682.0(3)     28.29s user 0.04s system 386% cpu 7.323 total
     * 1e10 1463808882.7(3)   275.08s user 0.61s system 317% cpu 1:26.79 total
     * 1e11 14638184449.0(2) 2834.62s user 3.75s system 377% cpu 12:32.64 total
     * 1e12 146381605453(1) 31440.66s user 57.17s system 389% cpu 2:14:56.27
     * total total
     *
     * accur:
     * (pi-arccos(-sqrt(2)/4)-4*arccos(5*sqrt(2)/8)+2*sin(2*arccos(5*sqrt(2)
     * /8))-0.5*sin(2*pi-2*arccos(-sqrt(2)/4)))*r*r
     * =14.638125953034784
     */
    double radius = 5.0;
    uint64_t rand_samples = UINT64_C(1280000000);
    uint64_t inside = monte_carlo(radius, rand_samples);
    printf("%" PRIu64 "/%" PRIu64 "\n", inside, rand_samples);
    double size = inside / (double)rand_samples * 4 * radius * radius;
    printf("%g\n", size);
    return 0;
}

#endif
