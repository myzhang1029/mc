/*
 Hypot Monte Carlo kernel for Metal
 Copyright 2021 Zhang Maiyun <myzhang1029@hotmail.com>

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

#include <metal_stdlib>
using namespace metal;


/* PCG generator
 *Really* minimal PCG32 code / (c) 2014 M.E. O'Neill / pcg-random.org
 Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)
 */

typedef struct
{
    uint64_t state;
    uint64_t inc;
} pcg32_random_t;

typedef struct
{
    pcg32_random_t gen[2];
} pcg32x2_random_t;

static inline uint32_t pcg32_rand(thread pcg32_random_t *rng)
{
    uint64_t oldstate = rng->state;
    // Advance internal state
    rng->state = oldstate * 6364136223846793005ULL + (rng->inc | 1);
    // Calculate output function (XSH RR), uses old state for max ILP
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

static inline void pcg32_srand(thread pcg32_random_t *rng,
                               uint64_t initstate,
                               uint64_t initseq)
{
    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    pcg32_rand(rng);
    rng->state += initstate;
    pcg32_rand(rng);
}

// Derived from PCG's pcg_setseq_64_advance_r and pcg_advance_lcg_64
static inline void pcg32_advance(thread pcg32_random_t *rng,
                                 uint64_t delta)
{
    uint64_t acc_mult = 1u;
    uint64_t acc_plus = 0u;
    uint64_t cur_mult = 6364136223846793005ULL;
    uint64_t cur_plus = rng->inc;
    
    while (delta > 0)
    {
        if (delta & 1)
        {
            acc_mult *= cur_mult;
            acc_plus = acc_plus * cur_mult + cur_plus;
        }
        cur_plus = (cur_mult + 1) * cur_plus;
        cur_mult *= cur_mult;
        delta /= 2;
    }
    rng->state = acc_mult * rng->state + acc_plus;
}

/* End PCG generator */


static inline float sqhypot(const float a, const float b)
{
    return fma(a, a, b * b);
}

/* mc kernel function */
kernel void monte_carlo(/* Calculate parameter */
                        constant float &scaled_radius,
                        /* Array of results */
                        volatile device atomic_uint *results,
                        /* Modulo for results because memory might be limited to fit all samples */
                        constant uint32_t &modulo,
                        uint threadgroup_position_in_grid   [[ threadgroup_position_in_grid ]],
                        uint thread_position_in_threadgroup [[ thread_position_in_threadgroup ]],
                        uint threads_per_threadgroup        [[ threads_per_threadgroup ]])
{
    const float sqr = scaled_radius * scaled_radius;
    const float rmax = 2 * scaled_radius + 1;
    /* Scale onto the generated random number */
    const float scale = ldexp(rmax, -32);
    size_t rank = threads_per_threadgroup * threadgroup_position_in_grid + thread_position_in_threadgroup;
    
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
        atomic_fetch_add_explicit(results + rank % modulo, 1, memory_order_relaxed);
}

