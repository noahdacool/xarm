#include <string.h>
#include <stdio.h>
#include <sys/random.h>
#include <sys/auxv.h>
#include <unistd.h>
#include "proc.h"
#include "util.h"

gaddr_t push_stack(struct mm *mm, gaddr_t *sp, const void *data, size_t size) {
    *sp -= size;
    if (*sp < mm->stack_limit) fatal("Initial stack overflow");
    memcpy(mm->base + *sp, data, size);
    return *sp;
}

struct cpu *proc_setup(struct mm *mm, const struct elf *elf, const struct hargs *hargs) {
    gaddr_t sp = mm->stack_top - 4; // push four bytes of zeros

    uint32_t envc = 0;
    char *envp[] = { "PATH=/bin", NULL };
    while (envp[envc]) envc++;
    if (envc > 64) fatal("Too many environment variables");

    uint32_t argc = hargs->gargc;
    char **argv = hargs->gargv;
    if (argc > 64) fatal("Too many guest args");

    char *execfn = argv[0];
    gaddr_t execfn_addr = push_stack(mm, &sp, execfn, strlen(execfn) + 1);

    gaddr_t envp_addr[64];
    for (int i = envc - 1; i >= 0; i--) envp_addr[i] = push_stack(mm, &sp, envp[i], strlen(envp[i]) + 1);

    gaddr_t argv_addr[64];
    for (int i = argc - 1; i >= 0; i--) argv_addr[i] = push_stack(mm, &sp, argv[i], strlen(argv[i]) + 1);

    byte random[16];
    getrandom(random, 16, 0);
    gaddr_t random_addr = push_stack(mm, &sp, random, 16);

    if (sp % 16) sp -= 16 - (sp % 16); // align stack to 16 bytes (all following pushes are 32-bit multiples)

    uint32_t auxv[32];
    int auxc = 0;
    auxv[auxc++] = AT_PHDR;     auxv[auxc++] = elf->phdr_gaddr;
    auxv[auxc++] = AT_PHENT;    auxv[auxc++] = elf->phentsize;
    auxv[auxc++] = AT_PHNUM;    auxv[auxc++] = elf->phnum;
    auxv[auxc++] = AT_PAGESZ;   auxv[auxc++] = PAGE_SIZE;
    auxv[auxc++] = AT_BASE;     auxv[auxc++] = 0; // no interpreter
    auxv[auxc++] = AT_ENTRY;    auxv[auxc++] = elf->entry;
    auxv[auxc++] = AT_RANDOM;   auxv[auxc++] = random_addr;
    auxv[auxc++] = AT_EXECFN;   auxv[auxc++] = execfn_addr;
    auxv[auxc++] = AT_SECURE;   auxv[auxc++] = 0;
    auxv[auxc++] = AT_UID;      auxv[auxc++] = getuid();
    auxv[auxc++] = AT_EUID;     auxv[auxc++] = geteuid();
    auxv[auxc++] = AT_GID;      auxv[auxc++] = getgid();
    auxv[auxc++] = AT_EGID;     auxv[auxc++] = getegid();
    auxv[auxc++] = AT_NULL;     auxv[auxc++] = 0;

    push_stack(mm, &sp, auxv, auxc * sizeof(uint32_t));
    sp -= sizeof(uint32_t); // push four bytes of zeros
    push_stack(mm, &sp, envp_addr, envc * sizeof(gaddr_t));
    sp -= sizeof(uint32_t); // push four bytes of zeros
    push_stack(mm, &sp, argv_addr, argc * sizeof(gaddr_t));
    push_stack(mm, &sp, &argc, sizeof(int));

    if (hargs->flags & HARGS_DEBUG) {
        // Confirm 32-bit values are pushed as little-endian
        printf("\n");
        printf("[DEBUG] local argc = %08x\n", argc);
        printf("[DEBUG] stack argc = ");
        for (int i = 0; i < sizeof(uint32_t); i++) {
            printf("%02x", (mm->base + sp)[i]);
        }
        printf("\n");
        printf("[DEBUG] local argv_addr[0] = %08x\n", argv_addr[0]);
        printf("[DEBUG] stack argv_addr[0] = ");
        for (int i = 0; i < sizeof(gaddr_t); i++) {
            printf("%02x", (mm->base + sp + sizeof(int))[i]);
        }
        printf("\n");

        // Print full stack contents
        printf("\n");
        printf("[DEBUG] Stack contents (from top to bottom):\n");
        for (gaddr_t addr = mm->stack_top - 16; addr >= sp; addr -= 16) {
            printf("[DEBUG] 0x%08x:", addr);
            for (int i = 0; i < 16; i++) {
                printf(" %02x", mm->base[addr + i]);
            }
            printf("\n");
        }
    }

    struct cpu *cpu = calloc(1, sizeof(struct cpu));
    if (!cpu) fatal("Out of memory");
    cpu->reg[13] = sp;
    cpu->reg[15] = elf->entry;

    return cpu;
}
