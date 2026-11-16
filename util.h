#pragma once
#include "types.h"

__attribute__((format(printf, 1, 2)))
void fatal(const char *format, ...);

__attribute__((format(printf, 1, 2)))
void warning(const char *format, ...);

__attribute__((format(printf, 1, 2)))
void debug(const char *format, ...);

const char *format_bytes(char *buf, const byte *data, size_t nbytes);

haddr_t haddr_of(hmeta_t hmeta);
gaddr_t gaddr_of(gmeta_t gmeta);
uint8_t ha_thumb(hmeta_t hmeta);
uint8_t ga_thumb(gmeta_t gmeta);
hmeta_t to_hmeta(haddr_t haddr, uint8_t thumb);
gmeta_t to_gmeta(gaddr_t gaddr, uint8_t thumb);
