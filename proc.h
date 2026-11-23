#pragma once
#include "elf.h"

#define CPU_REG_PC 15

struct cpu {
    uint32_t reg[16];
};

struct cpu *proc_setup(struct mm *mm, const struct elf *elf, const struct hargs *hargs);
