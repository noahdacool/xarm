#pragma once
#include <elf.h>
#include "mm.h"

#define MAX_CODE_RANGES 8

struct code {
    gaddr_t start;
    gaddr_t end;
};

struct region {
    gaddr_t gaddr;
    char kind;
};

struct entaddr {
    gaddr_t gaddr;
    uint8_t thumb;
};

struct elf {
    gaddr_t entry;
    gaddr_t phdr_gaddr;
    uint16_t phnum;
    uint16_t phentsize;
    struct code code[MAX_CODE_RANGES];
    struct region *regions;
    struct entaddr *funcs;
    size_t ncode;
    size_t nregions;
    size_t nfuncs;
};

struct elf *elf_load(struct mm *mm, const struct hargs *hargs);
