#include <stdio.h>
#include "util.h"
#include "xlat.h"

struct hargs *parse_args(int argc, char *argv[]) {
    struct hargs *hargs = calloc(1, sizeof(struct hargs));
    if (!hargs) fatal("Out of memory");

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
            fatal(help, argv[0]);
        }
    }

    if (i < argc) {
        hargs->path = argv[i];
        hargs->gargc = argc - i;
        hargs->gargv = argv + i;
    } else {
        fatal(help, argv[0]);
    }

    return hargs;
}

int main(int argc, char *argv[]) {
    struct hargs *hargs = parse_args(argc, argv);
    struct mm *mm = mm_init(hargs);
    struct elf *elf = elf_load(mm, hargs);
    struct cpu *cpu = proc_setup(mm, elf, hargs);
    struct xlat *xlat = xlat_program(mm, elf, cpu, hargs);
    //struct res *res = emu_execute(emu);
    return 0;
}
