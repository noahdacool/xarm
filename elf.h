#pragma once
#include <elf.h>
#include "mm.h"

#define MAX_CODE_RANGES 8

struct elf {
    gaddr_t entry;
    gaddr_t phdr_gaddr;
    uint16_t phnum;
    uint16_t phentsize;
    struct { gaddr_t start, end; } code[MAX_CODE_RANGES];
    struct { gaddr_t gaddr; char kind; } *regions;
    struct { gaddr_t gaddr; uint8_t thumb; } *funcs;
    size_t ncode;
    size_t nregions;
    size_t nfuncs;
};

struct elf *elf_load(struct mm *mm, const struct hargs *hargs);
