#include <stdio.h>
#include "mm.h"
#include "elf.h"

struct hargs* parse_args(int argc, char *argv[]) {
    struct hargs *hargs = calloc(1, sizeof(struct hargs));

    const char *help =
        "Usage: %s [OPTIONS] <path> [args...]\n"
        "Options:\n"
        "  -d             Enable debug output\n"
        "  -h             Show this help message and exit\n";

    int i = 1;

    while (i < argc && argv[i][0] == '-') {
        switch (argv[i++][1]) {
        case 'd':
            hargs->flags |= HARGS_DEBUG;
            break;
        case 'h':
            printf(help, argv[0]);
            exit(0);
        default:
            fprintf(stderr, help, argv[0]);
            exit(1);
        }
    }

    if (i < argc) {
        hargs->path = argv[i];
        hargs->gargc = argc - i;
        hargs->gargv = argv + i;
    } else {
        fprintf(stderr, help, argv[0]);
        exit(1);
    }

    return hargs;
}

int main(int argc, char *argv[]) {
    struct hargs *hargs = parse_args(argc, argv);
    struct mm *mm = mm_init();
    struct elf *elf = elf_load(mm, hargs);
    //struct pro *pro = proc_setup(mm);
    //struct emu *emu = xlat_program(pro);
    //struct res *res = emu_execute(emu);
    return 0;
}
