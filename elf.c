#include <stdio.h>
#include <string.h>
#include "util.h"
#include "elf.h"

struct elf *elf_load(struct mm *mm, const struct hargs *hargs) {

    // Read ELF file
    FILE *file = fopen(hargs->path, "rb");
    if (!file) fatal("Failed to open ELF file");
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    byte *data = malloc(file_size);
    if (!data) { fclose(file); fatal("Failed to allocate memory for file data"); }
    size_t size = fread(data, 1, file_size, file);
    if (size != file_size) { fclose(file); fatal("Failed to read data from file"); }
    fclose(file);

    // Verify ELF header
    const Elf32_Ehdr *eh = (const Elf32_Ehdr*)data;
    if (size < sizeof(Elf32_Ehdr)) fatal("File too small to be a valid ELF file");
    if (eh->e_ident[EI_MAG0] != ELFMAG0 ||
        eh->e_ident[EI_MAG1] != ELFMAG1 ||
        eh->e_ident[EI_MAG2] != ELFMAG2 ||
        eh->e_ident[EI_MAG3] != ELFMAG3) fatal("Not an ELF file");
    if (eh->e_ident[EI_CLASS] != ELFCLASS32) fatal("Not a 32-bit ELF file");
    if (eh->e_ident[EI_DATA] != ELFDATA2LSB) fatal("Not a little-endian ELF file");
    if (eh->e_type != ET_EXEC) fatal("Not an executable ELF file");
    if (eh->e_machine != EM_ARM) fatal("Not an ARM ELF file");

    // Create elf struct
    struct elf *elf = calloc(1, sizeof(struct elf));
    if (!elf) fatal("Out of memory");
    elf->entry = eh->e_entry;
    elf->phnum = eh->e_phnum;
    elf->phentsize = eh->e_phentsize;

    // Load program segments
    const Elf32_Phdr *ph = (const Elf32_Phdr*)(data + eh->e_phoff);
    if (size < eh->e_phoff + eh->e_phnum * eh->e_phentsize) fatal("File too small for program headers");
    for (int i = 0; i < eh->e_phnum; i++) {
        const Elf32_Phdr *p = &ph[i];
        switch (p->p_type) {
        case PT_LOAD:
            if (p->p_filesz > p->p_memsz) fatal("Segment memory size is smaller than data size");
            if (p->p_filesz > size - p->p_offset) fatal("Segment exceeds file size");
            if (p->p_vaddr < NULL_GUARD) fatal("Segment overlaps null guard page");
            if (p->p_vaddr > mm->stack_limit - p->p_memsz) fatal("Segment overlaps stack");
            memcpy(mm->base + p->p_vaddr, data + p->p_offset, p->p_filesz);
            if (!elf->phdr_gaddr && eh->e_phoff >= p->p_offset && eh->e_phoff < p->p_offset + p->p_filesz) elf->phdr_gaddr = p->p_vaddr + (eh->e_phoff - p->p_offset);
            if (p->p_vaddr + p->p_memsz > mm->brk_start) mm->brk_start = p->p_vaddr + p->p_memsz;
            if (hargs->flags & HARGS_DEBUG) {
                printf("[DEBUG] Loaded segment %d: vaddr=0x%08x, memsz=0x%08x, filesz=0x%08x\n", i, p->p_vaddr, p->p_memsz, p->p_filesz);
                printf("[DEBUG] Segment data (first 16 bytes):");
                for (int j = 0; j < 16 && j < p->p_filesz; j++) printf(" %02x", (data + p->p_offset)[j]);
                printf("\n");
            }
            break;
        case PT_INTERP: fatal("Dynamic linking not supported");
        case PT_PHDR: elf->phdr_gaddr = p->p_vaddr; break;
        default: break;
        }
    }
    if (!elf->phdr_gaddr) fatal("No program header segment found");

    // Setup program break
    if (mm->brk_start % PAGE_SIZE) mm->brk_start += PAGE_SIZE - (mm->brk_start % PAGE_SIZE);
    if (mm->brk_start >= mm->stack_limit) fatal("Program brk start overlaps stack");
    mm->brk_cur = mm->brk_start;

    free(data);
    return elf;
}
