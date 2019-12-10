#include <stdio.h>
#include <unistd.h>
#include "purgeable.h"

int
main(void)
{
    struct purgeable *pg = purgeable_alloc(1<<24);
    printf("%p %zu\n", pg->addr, pg->numpages);
    for (;;) {
        if (!purgeable_lock(pg)) {
            return 0;
        }
        purgeable_unlock(pg);
        sleep(1);
    }
}
