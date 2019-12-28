# Purgeable Memory Allocations for Linux

Purgeable memory is a contiguous allocation that can be reclaimed by the
kernel at any time while unlocked. Before accessing or using the
allocation, it must be locked, and then unlocked when no longer in
active use. See `purgeable.h` for API documentation.

```c
void *purgeable_alloc(size_t);
void  purgeable_unlock(void *);
void *purgeable_lock(void *);
void  purgeable_free(void *);
```

This idea was inspired by [OS hacking: Purgeable memory][vid]
([discussion][disc]).

To test it out, disable swap (`swapoff`), run one or more instances of
`test`, then run `hog`. As `hog` consumes more and more memory (before
being killed by the OOM killer), instances of `test` will see their
regions get reclaimed and exit.


[disc]: https://old.reddit.com/r/programming/comments/e8pbmk/implementing_macosstyle_purgeable_memory_in_my/
[vid]: https://www.youtube.com/watch?v=9l0nWEUpg7s
