#pragma once
#include <stdint.h>
#include <stdlib.h>

typedef uint8_t byte;
typedef uint32_t haddr_t; // host address
typedef uint32_t hmeta_t; // host address with metadata (i.e., thumb bit)
typedef uint32_t gaddr_t; // guest address
typedef uint32_t gmeta_t; // guest address with metadata (i.e., thumb bit)
typedef char Elf32_Str;

#define HARGS_DEBUG (1u << 0)

struct hargs {
    const char *path;
    uint32_t flags;
    int gargc;
    char **gargv;
};
