#include <stdlib.h>
#include <sys/mman.h>
#include "util.h"
#include "types.h"

#define GUEST_SIZE  (256u << 20) // 256 MB
#define MIN_SIZE    (32u  << 20) //  32 MB
#define STACK_SIZE  (8u   << 20) //   8 MB
#define NULL_GUARD  (64u  << 10) //  64 KB

struct mm {
    byte *base;
    size_t size;
    gaddr_t stack_top;
    gaddr_t stack_limit;
    gaddr_t mmap_next;
    gaddr_t brk_start;
    gaddr_t brk_cur;
};

struct mm* mm_init() {
    struct mm *mm = calloc(1, sizeof(struct mm));
    if (!mm) fatal("Out of memory");
    for (size_t size = GUEST_SIZE; size >= MIN_SIZE; size >>= 1) {
        mm->base = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (mm->base != MAP_FAILED) {
            mm->size = size;
            mm->stack_top = mm->size;
            mm->stack_limit = mm->stack_top - STACK_SIZE;
            mm->mmap_next = mm->stack_limit;
            break;
        }
    }
    if (mm->base == MAP_FAILED) fatal("Failed to allocate guest memory");
    if (mprotect(mm->base, NULL_GUARD, PROT_NONE) != 0) fatal("Failed to protect null guard page");
    return mm;
}
