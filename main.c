#include <stdio.h>
#include <stdlib.h>

#include "main.h"

struct hargs parse_args(int argc, char *argv[]) {
    struct hargs hargs;

    const char *help =
        "Usage: %s [OPTIONS] <path> [args...]\n"
        "Options:\n"
        "  -h             Show this help message and exit\n";

    int i = 0;

    while (i < argc && argv[i][0] == '-') {
        switch (argv[i++][1]) {
            case 'h':
                printf(help, argv[0]);
                exit(0);
            default:
                fprintf(stderr, help, argv[0]);
                exit(1);
        }
    }

    if (i < argc) {
        hargs.path = argv[i];
        hargs.gargc = argc - i;
        hargs.gargv = argv + i;
    } else {
        fprintf(stderr, help, argv[0]);
        exit(1);
    }

    return hargs;
}

extern struct mm* mm_init();
extern struct elf* elf_load(struct mm *mm, struct hargs hargs);
extern struct pro* proc_setup(struct mm *mm);
extern struct emu* xlat_program(struct pro *pro);
extern struct res* emu_execute(struct emu *emu);

int main(int argc, char *argv[]) {
    struct hargs hargs = parse_args(argc, argv);
    struct mm *mm = mm_init();
    struct elf *elf = elf_load(mm, hargs);
    struct pro *pro = proc_setup(mm);
    struct emu *emu = xlat_program(pro);
    struct res *res = emu_execute(emu);
    return 0;
}
