#pragma once
#include <stdint.h>
#include <stdlib.h>

typedef uint8_t byte;
typedef uint32_t haddr_t;
typedef uint32_t gaddr_t;

#define HARGS_DEBUG (1u << 0)

struct hargs {
    const char *path;
    uint32_t flags;
    int gargc;
    char **gargv;
};
