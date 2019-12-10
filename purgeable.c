/* This is free and unencumbered software released into the public domain. */
#define _DEFAULT_SOURCE
#include <stdlib.h>
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

    struct purgeable *pg = malloc(sizeof(struct purgeable) + numpages);
    if (!pg) {
        munmap(p, numpages*pagesize);
        return 0;
    }
    pg->addr = p;
    pg->numpages = numpages;
    pg->pagesize = pagesize;
    purgeable_unlock(pg);
    return pg;
}

void
purgeable_free(struct purgeable *pg)
{
    munmap(pg->addr, pg->numpages*pg->pagesize);
    pg->addr = 0;
    free(pg);
}

void
purgeable_unlock(struct purgeable *pg)
{
    /* Save the original first byte, write 1 into the first byte of each
     * page, then set it all to MADV_FREE.
     */
    unsigned char *p = pg->addr;
    for (size_t i = 0; i < pg->numpages; i++) {
        pg->save[i] = p[i*pg->pagesize];
        p[i*pg->pagesize] = 1;
    }
    madvise(p, pg->numpages*pg->pagesize, MADV_FREE);
}

_Bool
purgeable_lock(struct purgeable *pg)
{
    /* Atomically swap the original first byte of each page back into
     * place, ensuring that the page has not been freed in the meantime.
     */
    for (size_t i = 0; i < pg->numpages; i++) {
        unsigned char *ptr = (unsigned char *)pg->addr + i*pg->pagesize;
        unsigned char oldval = 1;
        unsigned char newval = pg->save[i];
        if (!__sync_bool_compare_and_swap(ptr, oldval, newval)) {
            purgeable_free(pg);
            return 0;
        }
    }
    return 1;
}
