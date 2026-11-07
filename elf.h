#pragma once
#include <elf.h>
#include "mm.h"

struct elf {
    gaddr_t entry;
    gaddr_t phdr_gaddr;
    uint16_t phnum;
    uint16_t phentsize;
};

struct elf* elf_load(struct mm *mm, const struct hargs *hargs);
