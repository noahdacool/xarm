#pragma once
#include "elf.h"

struct cpu {
    uint32_t reg[16];
};

struct cpu *proc_setup(struct mm *mm, const struct elf *elf, const struct hargs *hargs);
