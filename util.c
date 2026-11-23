#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "util.h"

void fatal(const char *format, ...) {
    char buf[512];
    snprintf(buf, sizeof(buf), "[FATAL] %s\n", format);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, buf, args);
    va_end(args);
    exit(1);
}

void warning(const char *format, ...) {
    char buf[512];
    snprintf(buf, sizeof(buf), "[WARNING] %s\n", format);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, buf, args);
    va_end(args);
}

void debug(const char *format, ...) {
    char buf[512];
    snprintf(buf, sizeof(buf), "[DEBUG] %s\n", format);
    va_list args;
    va_start(args, format);
    vfprintf(stdout, buf, args);
    va_end(args);
}

const char *format_bytes(char *buf, const byte *data, size_t nbytes) {
    for (size_t i = 0; i < nbytes; i++) sprintf(buf + i * 3, "%02x ", data[i]);
    buf[nbytes * 3] = '\0';
    return buf;
}

inline gaddr_t gaddr_of(gmeta_t gmeta) { return gmeta & ~1u; }
inline uint8_t ga_thumb(gmeta_t gmeta) { return gmeta &  1u; }
inline gmeta_t to_gmeta(gaddr_t gaddr, uint8_t thumb) { return (gaddr & ~1u) | (thumb & 1u); }
