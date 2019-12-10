#pragma once

#include <stddef.h>

struct purgeable {
    void *addr;
    size_t numpages;
    size_t pagesize;
    unsigned char save[];
};

/**
 * Allocate a contiguous purgeable memory region in a locked state.
 */
struct purgeable *purgeable_alloc(size_t);

/**
 * Unlock a purgeable memory region allowing the OS to reclaim if needed.
 */
void purgeable_unlock(struct purgeable *);

/**
 * Free a purgeable region of memory.
 */
void purgeable_free(struct purgeable *);

/**
 * Attempt to lock a purgeable memory region, returning true on success.
 * On failure, the purgeable region is automatically freed.
 */
_Bool purgeable_lock(struct purgeable *);
