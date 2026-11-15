#pragma once
#include "types.h"

__attribute__((format(printf, 1, 2)))
void fatal(const char *format, ...);

__attribute__((format(printf, 1, 2)))
void warning(const char *format, ...);

__attribute__((format(printf, 1, 2)))
void debug(const char *format, ...);

const char *format_bytes(char *buf, const byte *data, size_t nbytes);
