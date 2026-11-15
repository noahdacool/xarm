#include <sys/mman.h>
#include "xlat.h"
#include "util.h"

static struct xlat *xlat_init(struct elf *elf, struct hargs *hargs) {
    struct xlat *xlat = calloc(1, sizeof(struct xlat));
    if (!xlat) fatal("Out of memory");

    gaddr_t code_lo = ~(gaddr_t)0;
    gaddr_t code_hi = 0;
    size_t code_bytes = 0;
    for (size_t i = 0; i < elf->ncode; i++) {
        if (elf->code[i].start < code_lo) code_lo = elf->code[i].start;
        if (elf->code[i].end > code_hi) code_hi = elf->code[i].end;
        code_bytes += elf->code[i].end - elf->code[i].start;
    }
    if (code_lo % MIN_INSTR_SIZE) code_lo -= code_lo % MIN_INSTR_SIZE;
    if (code_hi % MIN_INSTR_SIZE) code_hi += MIN_INSTR_SIZE - (code_hi % MIN_INSTR_SIZE);

    xlat->nmap = (code_hi - code_lo) / MIN_INSTR_SIZE;
    xlat->amap = calloc(xlat->nmap, sizeof(*xlat->amap));
    if (!xlat->amap) fatal("Out of memory");

    xlat->xbuf_off = code_lo;
    xlat->xbuf_sz = code_bytes * XBUF_FACTOR;
    if (xlat->xbuf_sz < XBUF_MIN) xlat->xbuf_sz = XBUF_MIN;
    xlat->xbuf = mmap(NULL, xlat->xbuf_sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (xlat->xbuf == MAP_FAILED) fatal("Failed to allocate translation buffer (%lu bytes)", xlat->xbuf_sz);

    xlat->todo_cap = 1024;
    xlat->fixup_cap = 4096;
    xlat->todo = calloc(xlat->todo_cap, sizeof(*xlat->todo));
    xlat->fixup = calloc(xlat->fixup_cap, sizeof(*xlat->fixup));
    if (!xlat->todo || !xlat->fixup) fatal("Out of memory");

    return xlat;
}

struct xlat *xlat_program(struct mm *mm, struct elf *elf, struct cpu *cpu, struct hargs *hargs) {
    struct xlat *xlat = xlat_init(elf, hargs);
    return xlat;
}
