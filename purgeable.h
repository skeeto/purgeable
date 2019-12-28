#pragma once

#include <stddef.h>

/**
 * Allocate a contiguous purgeable memory region in a locked state.
 * The returned pointer is the allocation itself, or NULL if the memory
 * could not be allocated.
 */
void *purgeable_alloc(size_t);

/**
 * Unlock an allocation allowing the kernel to reclaim it if needed.
 */
void purgeable_unlock(void *);

/**
 * Attempt to lock an allocation to prevent reclamation by the kernel.
 * Returns the address on success, or NULL if the allocation had been
 * reclaimed. Even if the allocation has been reclaimed, you must still
 * call purgeable_free().
 */
void *purgeable_lock(void *);

/**
 * Free an allocation made using purgeable_alloc().
 */
void purgeable_free(void *);
