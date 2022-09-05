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

// The area can be obtained with inside / rand_samples * 4 * radius * radius
uint64_t monte_carlo(const float radius, const uint64_t rand_samples)
{
    const float r = radius * rand_samples;
    const float rmax = 2 * r + 1;
    /* Scale onto the generated random number */
    const float scale = ldexp(rmax, -32);
    uint64_t inside = 0;

    float x_dot, y_dot;
    float d1, d2;
    pcg32_random_t thrd_rngx, thrd_rngy;
    uint64_t i;
    pcg32_srand(&thrd_rngx, UINT64_C(42), UINT64_C(430));
    pcg32_srand(&thrd_rngy, UINT64_C(42), UINT64_C(431));
    for (i = 0; i < rand_samples; ++i)
    {
        x_dot = pcg32_rand(&thrd_rngx) * scale;
        y_dot = pcg32_rand(&thrd_rngy) * scale;
        d1 = sqrt((r - x_dot) * (r - x_dot) + (r - y_dot) * (r - y_dot));
        d2 = sqrt(x_dot * x_dot + y_dot * y_dot);
        if (d1 < r && d2 >= 2 * r)
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
