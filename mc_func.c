/*
 Standard Equational Monte Carlo CPU C code
 Copyright 2019-2021 Zhang Maiyun <myzhang1029@hotmail.com>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "pcg_impl/pcg.h"

/* When two uint64_ts multiply, it's a uint128.
 * so we use double */
#if __SIZEOF__INT128__ == 16
typedef __uint128_t bigint;
#else
typedef double bigint;
#endif

static inline double y1_upper(const bigint x, const uint64_t r)
{
    return r + sqrt(2 * x * r - x * x);
}

static inline double y1_lower(const bigint x, const uint64_t r)
{
    return r - sqrt(2 * x * r - x * x);
}

static inline double y2(const bigint x, const uint64_t r)
{
    return sqrt(4 * x * r - x * x);
}

// The area can be obtained with inside / rand_samples * 4 * radius * radius
uint64_t monte_carlo(const uint32_t radius, const uint64_t rand_samples)
{
    const uint64_t r = radius * rand_samples;
    const uint64_t rmax = 2 * r + 1;
    uint64_t i;
    uint64_t inside = 0;
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
        pcg32x2_srand(&thrd_rngx, UINT64_C(42), UINT64_C(430));
        pcg32x2_srand(&thrd_rngy, UINT64_C(42), UINT64_C(431));
#pragma omp for reduction(+ : inside)
        for (i = 0; i < rand_samples; ++i)
        {
            x_dot = pcg32x2_uniform(&thrd_rngx, rmax);
            y_dot = pcg32x2_uniform(&thrd_rngy, rmax);
            yu = y1_upper(x_dot, r);
            yl = y1_lower(x_dot, r);
            yt = y2(x_dot, r);
            /* left segment */
            if (x_dot < r * q3msqrt7 && yl <= y_dot && y_dot < yu)
                ++inside;
            /* right segment */
            else if (r * q3msqrt7 <= x_dot && x_dot < r * q3asqrt7 &&
                     yt <= y_dot && y_dot < yu)
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
    double radius = 5.0;
    uint64_t rand_samples = UINT64_C(1000000000);
    uint64_t inside = monte_carlo(radius, rand_samples);
    printf("%" PRIu64 "/%" PRIu64 "\n", inside, rand_samples);
    double size = inside / (double)rand_samples * 4 * radius * radius;
    printf("%g\n", size);
    return 0;
}

#endif
