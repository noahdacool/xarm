#pragma once

struct hargs {
    const char *path;
    int gargc;
    char **gargv;
};

struct img {

};

struct mm {

};

struct emu {

};

struct cpu {

};

struct elf {

};

struct proc {
    struct cpu cpu;
    struct elf elf;
    struct mm mm;
};
