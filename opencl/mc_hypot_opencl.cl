/*
 Hypot Monte Carlo CPU OpenCL kernel
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

#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#define uint64_t ulong
#define uint32_t uint
#define UINT64_C(c) (c##UL)
#include "../pcg_impl/pcg.h"

static inline float sqhypot(const float a, const float b)
{
    return fma(a, a, b * b);
}

/* mc kernel function */
kernel void monte_carlo(
    /* Calculate parameter */
    float scaled_radius,
    /* Array of results */
    global uint *results,
    /* Modulo for results because memory might be limited to fit all samples */
    ulong modulo)
{
    const float sqr = scaled_radius * scaled_radius;
    const float rmax = 2 * scaled_radius + 1;
    /* Scale onto the generated random number */
    const float scale = ldexp(rmax, -32);
    size_t rank = get_global_id(0);

    float x_dot, y_dot, sqd1, sqd2;
    pcg32_random_t rng;

    pcg32_srand(&rng, 42UL, 430UL);
    /* Skip "previous" workers */
    pcg32_advance(&rng, rank * 2);

    x_dot = pcg32_rand(&rng) * scale;
    y_dot = pcg32_rand(&rng) * scale;

    sqd1 = sqhypot(scaled_radius - x_dot, scaled_radius - y_dot);
    sqd2 = sqhypot(2 * scaled_radius - x_dot, y_dot);
    /* Store & Return */
    if (sqd1 < sqr && sqd2 >= 4 * sqr)
        atomic_inc(results + rank % modulo);
}
