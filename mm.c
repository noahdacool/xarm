#include <sys/mman.h>
#include <stdio.h>
#include "util.h"
#include "mm.h"

struct mm *mm_init(const struct hargs *hargs) {
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
        if (hargs->flags & HARGS_DEBUG) {
            printf("[DEBUG] Failed to allocate %zu bytes of guest memory\n", size);
        }
    }
    if (mm->base == MAP_FAILED) fatal("Failed to allocate guest memory");
    if (mprotect(mm->base, NULL_GUARD, PROT_NONE) != 0) fatal("Failed to protect null guard page");
    return mm;
}
