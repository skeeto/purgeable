/* This is free and unencumbered software released into the public domain. */
#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include "purgeable.h"

struct purgeable *
purgeable_alloc(size_t len)
{
    size_t pagesize = getpagesize();
    size_t numpages = (len + pagesize - 1) / pagesize;
    int prot = PROT_READ|PROT_WRITE;
    int flags = MAP_PRIVATE|MAP_ANONYMOUS;
    void *p = mmap(0, numpages*pagesize, prot, flags, -1, 0);
    if (p == MAP_FAILED) {
        return 0;
    }

    size_t nelems = (numpages + LONG_BIT - 1) / LONG_BIT;
    struct purgeable *pg = malloc(sizeof(*pg) + sizeof(pg->save[0])*nelems);
    if (!pg) {
        munmap(p, numpages*pagesize);
        return 0;
    }
    pg->addr = p;
    pg->numpages = numpages;
    pg->pagesize = pagesize;
    return pg;
}

void
purgeable_free(struct purgeable *pg)
{
    munmap(pg->addr, pg->numpages*pg->pagesize);
    free(pg);
}

void
purgeable_unlock(struct purgeable *pg)
{
    size_t nelems = (pg->numpages + LONG_BIT - 1) / LONG_BIT;
    memset(pg->save, 0, sizeof(pg->save[0])*nelems);

    /* If the original first page byte is zero, set to 1 and remember
     * that it should be zero. Then set everything to MADV_FREE.
     */
    unsigned char *p = pg->addr;
    for (size_t i = 0; i < pg->numpages; i++) {
        if (!p[i*pg->pagesize]) {
            pg->save[i/LONG_BIT] |= 1UL << (i%LONG_BIT);
            p[i*pg->pagesize] = 1;
        }
    }
    madvise(p, pg->numpages*pg->pagesize, MADV_FREE);
}

void *
purgeable_lock(struct purgeable *pg)
{
    /* Do an atomic compare and swap on the first byte of each page to
     * ensure that 1) the MADV_FREE is canceled by a write, and 2) the
     * page wasn't freed just before the write.
     */
    for (size_t i = 0; i < pg->numpages; i++) {
        unsigned char *ptr = (unsigned char *)pg->addr + i*pg->pagesize;
        unsigned char oldval, newval;
        if (pg->save[i/LONG_BIT] & (1UL << (i%LONG_BIT))) {
            /* Original value is 0, so placeholder is 1. */
            oldval = 1;
            newval = 0;
        } else {
            /* Original value is non-zero, so just swap it with itself. */
            oldval = newval = *ptr;
            if (!oldval) {
                return 0;
            }
        }
        if (!__sync_bool_compare_and_swap(ptr, oldval, newval)) {
            return 0;
        }
    }
    return pg->addr;
}
