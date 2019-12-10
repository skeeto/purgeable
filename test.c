#define _POSIX_C_SOURCE 200112L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "purgeable.h"

static uint32_t
triple32(uint32_t x)
{
    x ^= x >> 17;
    x *= UINT32_C(0xed5ad4bb);
    x ^= x >> 11;
    x *= UINT32_C(0xac4c1b51);
    x ^= x >> 15;
    x *= UINT32_C(0x31848bab);
    x ^= x >> 14;
    return x;
}

int
main(void)
{
    size_t len = 1<<22;
    struct purgeable *pg = purgeable_alloc(len);
    if (!pg) abort();

    /* Fill memory with random data. */
    uint32_t *buf = pg->addr;
    for (size_t i = 0; i < len/4; i++) {
        buf[i] = triple32(~i);
    }

    purgeable_unlock(pg);
    for (;;) {
        struct timespec ts = {0, 250000000L};
        nanosleep(&ts, 0);

        /* Verify that data is still here and valid. */
        if (!purgeable_lock(pg)) {
            return 0;
        }
        for (size_t i = 0; i < len/4; i++) {
            uint32_t expect = triple32(~i);
            if (buf[i] != expect) {
                fputs("corruption detected\n", stderr);
                abort();
            }
        }

        purgeable_unlock(pg);
    }
}
