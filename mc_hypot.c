/*
 Hypot Monte Carlo CPU C code
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

// This function significantly speeds things up - with a very low risk of
// overflow Always positive
static inline float sqhypot(const float a, const double b)
{
    return fma(a, a, b * b);
}

// The area can be obtained with inside / rand_samples * 4 * radius * radius
uint64_t monte_carlo(const float radius, const uint64_t rand_samples)
{
    const float r = radius * rand_samples;
    const float sqr = r * r;
    const float rmax = 2 * r + 1;
    /* Scale onto the generated random number */
    const float scale = ldexp(rmax, -32);
    uint64_t inside = 0;

#pragma omp parallel
    {
        float x_dot, y_dot;
        float sqd1, sqd2;
        pcg32_random_t thrd_rngx, thrd_rngy;
        uint64_t i;
        pcg32_srand(&thrd_rngx, UINT64_C(42), UINT64_C(430));
        pcg32_srand(&thrd_rngy, UINT64_C(42), UINT64_C(431));
#pragma omp for reduction(+ : inside)
        for (i = 0; i < rand_samples; ++i)
        {
            x_dot = pcg32_rand(&thrd_rngx) * scale;
            y_dot = pcg32_rand(&thrd_rngy) * scale;
            sqd1 = sqhypot(r - x_dot, r - y_dot);
            sqd2 = sqhypot(x_dot, y_dot);
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
    float radius = 5;

    uint64_t inside;
    float size;
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
        size = inside / (float)rand_samples * 4 * radius * radius;
        printf("%" PRIu64 "/%" PRIu64 "\n", inside, rand_samples);
        printf("%f\n", size);
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
    float radius = 5.0;
    uint64_t rand_samples = UINT64_C(1280000000);
    uint64_t inside = monte_carlo(radius, rand_samples);
    printf("%" PRIu64 "/%" PRIu64 "\n", inside, rand_samples);
    float size = inside / (float)rand_samples * 4 * radius * radius;
    printf("%f\n", size);
    return 0;
}

#endif
