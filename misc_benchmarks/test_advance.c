// Test PCG advance
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>

/* PCG generator
 *Really* minimal PCG32 code / (c) 2014 M.E. O'Neill / pcg-random.org
 Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)
 */

typedef struct
{
    uint64_t state;
    uint64_t inc;
} pcg32_random_t;

uint32_t pcg32_rand(pcg32_random_t *rng)
{
    uint64_t oldstate = rng->state;
    // Advance internal state
    rng->state = oldstate * 6364136223846793005ULL + (rng->inc | 1);
    // Calculate output function (XSH RR), uses old state for max ILP
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

void pcg32_srand(pcg32_random_t *rng, uint64_t initstate)
{
    rng->state = 0U;
    rng->inc = (8 << 1u) | 1u;
    pcg32_rand(rng);
    rng->state += initstate;
    pcg32_rand(rng);
}

// Derived from PCG's pcg_setseq_64_advance_r and pcg_advance_lcg_64
void pcg32_advance(pcg32_random_t *rng, uint64_t delta)
{
    uint64_t acc_mult = 1u;
    uint64_t acc_plus = 0u;
    uint64_t cur_mult = 6364136223846793005ULL;
    uint64_t cur_plus = rng->inc;

    while (delta > 0) {
        if (delta & 1) {
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

int main()
{
    pcg32_random_t a, b;
    pcg32_srand(&a, UINT64_C(43));
    pcg32_srand(&b, UINT64_C(43));
    printf("%" PRIu32 "\n", pcg32_rand(&a));
    printf("%" PRIu32 "\n", pcg32_rand(&a));
    printf("%" PRIu32 "\n", pcg32_rand(&a));
    printf("%" PRIu32 "\n", pcg32_rand(&a));
    pcg32_advance(&b, UINT64_C(3));
    printf("%" PRIu32 "\n", pcg32_rand(&b));
}