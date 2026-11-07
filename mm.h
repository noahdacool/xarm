#pragma once
#include "types.h"

#define GUEST_SIZE  (256u << 20) // 256 MB
#define MIN_SIZE    (32u  << 20) //  32 MB
#define STACK_SIZE  (8u   << 20) //   8 MB
#define NULL_GUARD  (64u  << 10) //  64 KB
#define PAGE_SIZE   (4u   << 10) //   4 KB

struct mm {
    byte *base;
    size_t size;
    gaddr_t stack_top;
    gaddr_t stack_limit;
    gaddr_t mmap_next;
    gaddr_t brk_start;
    gaddr_t brk_cur;
};

struct mm* mm_init();
