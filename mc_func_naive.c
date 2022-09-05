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

static inline float y1_upper(const float x, const float r)
{
    return r + sqrt(2 * x * r - x * x);
}

static inline float y1_lower(const float x, const float r)
{
    return r - sqrt(2 * x * r - x * x);
}

static inline float y2(const float x, const float r)
{
    return sqrt(4 * r * r - x * x);
}

uint64_t monte_carlo(const float radius, const uint64_t rand_samples)
{
    const float rmax = 2 * radius + 1;
    uint64_t i;
    uint64_t inside = 0;
    float x_dot, y_dot;
    float yu, yl, yt;
    pcg32_random_t thrd_rngx, thrd_rngy;
    pcg32_srand(&thrd_rngx, UINT64_C(42), UINT64_C(430));
    pcg32_srand(&thrd_rngy, UINT64_C(42), UINT64_C(431));

    for (i = 0; i < rand_samples; ++i)
    {
        x_dot = rmax * pcg32_rand(&thrd_rngx) / UINT32_MAX;
        y_dot = rmax * pcg32_rand(&thrd_rngy) / UINT32_MAX;
        yu = y1_upper(x_dot, radius);
        yl = y1_lower(x_dot, radius);
        yt = y2(x_dot, radius);
        /* left segment */
        if (radius * (5 - sqrt(7)) / 4.0 <= x_dot && x_dot < radius * (5 + sqrt(7)) / 4.0 &&
                 yt <= y_dot && y_dot < yu)
            ++inside;
        /* right segment */
        else if (radius * (5 + sqrt(7)) / 4.0 <= x_dot && yl <= y_dot && y_dot < yu)
            ++inside;
    }
    return inside;
}

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
