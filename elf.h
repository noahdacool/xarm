#pragma once
#include <elf.h>
#include "mm.h"

#define MAX_CODE_RANGES 8

struct code {
    gmeta_t start;
    gmeta_t end;
};

struct elf {
    gmeta_t entry;
    gaddr_t phdr_gaddr;
    uint16_t phnum;
    uint16_t phentsize;
    struct code code[MAX_CODE_RANGES];
    size_t ncode;
    gmeta_t *regions;
    size_t nregions;
    gmeta_t *funcs;
    size_t nfuncs;
};

struct elf *elf_load(struct mm *mm, const struct hargs *hargs);
