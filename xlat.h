#include "proc.h"

#define MIN_INSTR_SIZE 2
#define XBUF_FACTOR 32
#define XBUF_MIN (1u << 20) // 1 MB

struct xaddr {
    byte *haddr;
    uint8_t thumb;
};

struct fixaddr {
    byte *haddr;
    gaddr_t gaddr;
};

struct xlat {
    byte *xbuf;
    size_t xbuf_sz;
    size_t xbuf_used;
    size_t xbuf_off;

    struct xaddr *amap;
    size_t nmap;

    struct entaddr *todo;
    size_t ntodo;
    size_t todo_cap;

    struct fixaddr *fixup;
    size_t nfixup;
    size_t fixup_cap;
};

struct xlat *xlat_program(struct mm *mm, struct elf *elf, struct cpu *cpu, struct hargs *hargs);
