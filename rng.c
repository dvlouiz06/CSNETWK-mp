#include "rng.h"

static uint32_t rngState = 1;

void rng_init(uint32_t seed) {
    if (seed == 0) seed = 1;
    rngState = seed;
}

uint32_t rng_next() {
    rngState = (1664525u * rngState + 1013904223u);
    return rngState;
}
