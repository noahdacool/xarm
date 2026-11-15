#include <stdio.h>
#include <string.h>
#include "util.h"
#include "elf.h"

struct elf *elf_load(struct mm *mm, const struct hargs *hargs) {

    // Read ELF file
    FILE *file = fopen(hargs->path, "rb");
    if (!file) fatal("Failed to open file '%s'", hargs->path);
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    byte *data = malloc(file_size);
    if (!data) { fclose(file); fatal("Failed to allocate memory for file data"); }
    size_t size = fread(data, 1, file_size, file);
    if (size != file_size) { fclose(file); fatal("Failed to read data from file '%s'", hargs->path); }
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
    if (eh->e_phoff > size || eh->e_phnum * eh->e_phentsize > size - eh->e_phoff) fatal("File too small for program headers");
    for (int i = 0; i < eh->e_phnum; i++) {
        const Elf32_Phdr *p = &ph[i];
        switch (p->p_type) {
        case PT_LOAD:
            if (p->p_filesz > p->p_memsz) fatal("Segment '%d' memory size is smaller than data size", i);
            if (p->p_filesz > size - p->p_offset) fatal("Segment '%d' exceeds file size", i);
            if (p->p_vaddr < NULL_GUARD) fatal("Segment '%d' overlaps null guard page", i);
            if (p->p_vaddr > mm->stack_limit - p->p_memsz) fatal("Segment '%d' overlaps stack", i);
            memcpy(mm->base + p->p_vaddr, data + p->p_offset, p->p_filesz);
            if (!elf->phdr_gaddr && eh->e_phoff >= p->p_offset && eh->e_phoff < p->p_offset + p->p_filesz) elf->phdr_gaddr = p->p_vaddr + (eh->e_phoff - p->p_offset);
            if (p->p_vaddr + p->p_memsz > mm->brk_start) mm->brk_start = p->p_vaddr + p->p_memsz;
            if (p->p_flags & PF_X) {
                if (elf->ncode == MAX_CODE_RANGES) fatal("Too many executable segments");
                elf->code[elf->ncode].start = p->p_vaddr;
                elf->code[elf->ncode].end = p->p_vaddr + p->p_memsz;
                elf->ncode++;
            }
            if (hargs->flags & HARGS_DEBUG) {
                size_t nbytes = p->p_filesz < 16 ? p->p_filesz : 16;
                char buf[nbytes * 3 + 1];
                debug("Loaded segment %d: vaddr=0x%08x, memsz=0x%08x, filesz=0x%08x", i, p->p_vaddr, p->p_memsz, p->p_filesz);
                debug("Segment data (first %lu bytes): %s", nbytes, format_bytes(buf, data + p->p_offset, nbytes));
            }
            break;
        case PT_INTERP: fatal("Dynamic linking not supported");
        case PT_PHDR: elf->phdr_gaddr = p->p_vaddr; break;
        default: break;
        }
    }
    if (!elf->ncode) fatal("No executable segments");
    if (!elf->phdr_gaddr) fatal("No program header segment found");

    // Load symbols
    const Elf32_Shdr *sh = (const Elf32_Shdr*)(data + eh->e_shoff);
    if (eh->e_shoff > size || eh->e_shnum * eh->e_shentsize > size - eh->e_shoff) fatal("File too small for section headers");
    for (size_t i = 0; i < eh->e_shnum; i++) {
        if (sh[i].sh_type != SHT_SYMTAB) continue;

        const Elf32_Shdr *sh_sym = &sh[i];
        const Elf32_Shdr *sh_str = &sh[sh_sym->sh_link];
        if (sh_sym->sh_link >= eh->e_shnum) fatal ("Bad link from symbol table section header to string table section header");
        const Elf32_Sym *symtab = (const Elf32_Sym*)(data + sh_sym->sh_offset);
        const Elf32_Str *strtab = (const Elf32_Str*)(data + sh_str->sh_offset);

        if (sh_sym->sh_offset > size || sh_sym->sh_size > size - sh_sym->sh_offset) fatal("Symbol table extends past end of file");
        if (sh_str->sh_offset > size || sh_str->sh_size > size - sh_str->sh_offset) fatal("String table extends past end of file");
        if (sh_str->sh_size == 0) fatal("String table size is zero");
        if (strtab[sh_str->sh_size - 1] != '\0') fatal("String table not null terminated");

        size_t nsyms = sh_sym->sh_size / sh_sym->sh_entsize;
        elf->regions = calloc(nsyms, sizeof(*elf->regions));
        elf->funcs = calloc(nsyms, sizeof(*elf->funcs));
        if (!elf->regions || !elf->funcs) fatal("Out of memory");
        for (size_t j = 0; j < nsyms; j++) {
            const Elf32_Sym *sym = &symtab[j];
            const Elf32_Str *str = &strtab[sym->st_name];
            if (sym->st_name >= sh_str->sh_size) warning("Symbol %lu name not in string table", j);
            if (str[0] == '$' && (str[1] == 'a' || str[1] == 'd' || str[1] == 't') && (str[2] == '\0' || str[2] == '.')) {
                elf->regions[elf->nregions].gaddr = sym->st_value;
                elf->regions[elf->nregions].kind = str[1];
                elf->nregions++;
                if (hargs->flags & HARGS_DEBUG && elf->nregions <= 16) debug("Found mapping symbol (16 max) '0x%08x = %s'", sym->st_value, str);
            } else if (ELF32_ST_TYPE(sym->st_info) == STT_FUNC && sym->st_shndx != SHN_UNDEF) {
                elf->funcs[elf->nfuncs].gaddr = sym->st_value & ~1u;
                elf->funcs[elf->nfuncs].thumb = sym->st_value & 1u;
                elf->nfuncs++;
                if (hargs->flags & HARGS_DEBUG && elf->nfuncs <= 8) debug("Found function symbol (8 max) '0x%08x = %b'", sym->st_value & ~1u, sym->st_value & 1u);
            }
        }
        break;
    }
    if (!elf->regions) warning("No symbol table found");

    // Setup program break
    if (mm->brk_start % PAGE_SIZE) mm->brk_start += PAGE_SIZE - (mm->brk_start % PAGE_SIZE);
    if (mm->brk_start >= mm->stack_limit) fatal("Program brk start overlaps stack");
    mm->brk_cur = mm->brk_start;

    free(data);
    return elf;
}
