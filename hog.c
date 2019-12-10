#include <stdlib.h>

int
main(void)
{
    for (;;) *(int *)malloc(4096) = rand();
}
