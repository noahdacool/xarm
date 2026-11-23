#include <sys/mman.h>
#include "xlat.h"
#include "util.h"
#include "decode.h"
#include "x86.h"

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
    xlat->todo = malloc(xlat->todo_cap * sizeof(*xlat->todo));
    xlat->fixup = malloc(xlat->fixup_cap * sizeof(*xlat->fixup));
    if (!xlat->todo || !xlat->fixup) fatal("Out of memory");

    return xlat;
}

static void xlat_add_amap(struct xlat *xlat, gaddr_t gaddr, byte *haddr) {
    size_t idx = (gaddr - xlat->xbuf_off) / MIN_INSTR_SIZE;
    if (idx >= xlat->nmap) fatal("Address 0x%08x out of translation range", gaddr);
    xlat->amap[idx] = haddr;
}

static void xlat_get_amap(struct xlat *xlat, gaddr_t gaddr, byte **haddr) {
    size_t idx = (gaddr - xlat->xbuf_off) / MIN_INSTR_SIZE;
    if (idx >= xlat->nmap) fatal("Address 0x%08x out of translation range", gaddr);
    *haddr = xlat->amap[idx];
}

static void xlat_add_todo(struct xlat *xlat, gmeta_t gmeta) {
    if (xlat->ntodo == xlat->todo_cap) {
        xlat->todo_cap *= 2;
        xlat->todo = realloc(xlat->todo, xlat->todo_cap * sizeof(*xlat->todo));
        if (!xlat->todo) fatal("Out of memory");
    }
    xlat->todo[xlat->ntodo] = gmeta;
    xlat->ntodo++;
}

static int xlat_get_todo(struct xlat *xlat, gmeta_t *gmeta) {
    if (xlat->ntodo == 0) return 0;
    *gmeta = xlat->todo[--xlat->ntodo];
    return 1;
}

static void xlat_add_fixup(struct xlat *xlat, byte *haddr, gaddr_t gaddr) {
    if (xlat->nfixup == xlat->fixup_cap) {
        xlat->fixup_cap *= 2;
        xlat->fixup = realloc(xlat->fixup, xlat->fixup_cap * sizeof(*xlat->fixup));
        if (!xlat->fixup) fatal("Out of memory");
    }
    xlat->fixup[xlat->nfixup].haddr = haddr;
    xlat->fixup[xlat->nfixup].gaddr = gaddr;
    xlat->nfixup++;
}

static void xlat_run(struct mm *mm, struct xlat *xlat, gmeta_t gmeta, struct hargs *hargs) {
    for (;;) {
        xlat_add_amap(xlat, gaddr_of(gmeta), xlat->xbuf + xlat->xbuf_used);
        struct intr *intr = decode(mm, xlat, gmeta, hargs);
        x86_emit_code(xlat, intr, hargs);
        if (intr->flags & INTR_FLAGS_TODO) {
            if (hargs->flags & HARGS_DEBUG) debug("TODO: instruction at 0x%08x not yet supported", gmeta);
            break;
        }
        gmeta += intr->insn_sz;
    }
}

static void xlat_all(struct mm *mm, struct elf *elf, struct xlat *xlat, struct hargs *hargs) {
    xlat_add_todo(xlat, elf->entry);
    for (size_t i = 0; i < elf->nregions; i++) xlat_add_todo(xlat, elf->regions[i]);
    for (size_t i = 0; i < elf->nfuncs; i++) xlat_add_todo(xlat, elf->funcs[i]);

    gmeta_t todo;
    while (xlat_get_todo(xlat, &todo)) {

        byte *haddr;
        xlat_get_amap(xlat, gaddr_of(todo), &haddr);
        if (haddr) {
            if (hargs->flags & HARGS_DEBUG) debug("Address 0x%08x already translated", todo);
            continue;
        }

        xlat_run(mm, xlat, todo, hargs);
    }
}

struct xlat *xlat_program(struct mm *mm, struct elf *elf, struct cpu *cpu, struct hargs *hargs) {
    struct xlat *xlat = xlat_init(elf, hargs);
    xlat_all(mm, elf, xlat, hargs);
    return xlat;
}
