/* This is free and unencumbered software released into the public domain. */
#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include "purgeable.h"

/* Compute the required number of bookkeeping pages. We need 1 bit for
 * each page, plus room for a size_t to hold the number of pages. The
 * size_t will be stored just below the allocation returned to the
 * caller.
 */
static inline size_t
purgeable_numextra(size_t numpages, size_t pagesize)
{
    return (numpages + sizeof(size_t)*8 + pagesize*8 - 1) / pagesize*8;
}

void *
purgeable_alloc(size_t len)
{
    size_t pagesize = getpagesize();
    size_t numpages = (len + pagesize - 1) / pagesize;
    size_t numextra = purgeable_numextra(numpages, pagesize);

    int prot = PROT_READ|PROT_WRITE;
    int flags = MAP_PRIVATE|MAP_ANONYMOUS;
    char *p = mmap(0, (numpages + numextra)*pagesize, prot, flags, -1, 0);
    if (p == MAP_FAILED) {
        return 0;
    }

    char *buf = p + numextra*pagesize;
    ((size_t *)buf)[-1] = numpages;
    return buf;
}

void
purgeable_free(void *p)
{
    size_t pagesize = getpagesize();
    size_t numpages = ((size_t *)p)[-1];
    size_t numextra = purgeable_numextra(numpages, pagesize);
    munmap((char *)p - numextra*pagesize, (numpages + numextra)*pagesize);
}

void
purgeable_unlock(void *p)
{
    size_t pagesize = getpagesize();
    size_t numpages = ((size_t *)p)[-1];
    size_t numextra = purgeable_numextra(numpages, pagesize);
    unsigned long *save = (unsigned long *)((char *)p - numextra*pagesize);
    memset(save, 0, (numpages + 7)/8);

    /* If the original first page byte is zero, set to 1 and remember
     * that it should be zero. Then set everything to MADV_FREE.
     */
    unsigned char *buf = p;
    for (size_t i = 0; i < numpages; i++) {
        if (!buf[i*pagesize]) {
            save[i/LONG_BIT] |= 1UL << (i%LONG_BIT);
            buf[i*pagesize] = 1;
        }
    }
    madvise(p, numpages*pagesize, MADV_FREE);
}

void *
purgeable_lock(void *p)
{
    size_t pagesize = getpagesize();
    size_t numpages = ((size_t *)p)[-1];
    size_t numextra = purgeable_numextra(numpages, pagesize);
    unsigned long *save = (unsigned long *)((char *)p - numextra*pagesize);

    /* Do an atomic compare and swap on the first byte of each page to
     * ensure that 1) the MADV_FREE is canceled by a write, and 2) the
     * page wasn't freed just before the write.
     */
    unsigned char *buf = p;
    for (size_t i = 0; i < numpages; i++) {
        unsigned char *ptr = buf + i*pagesize;
        unsigned char oldval, newval;
        if (save[i/LONG_BIT] & (1UL << (i%LONG_BIT))) {
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
    return p;
}
