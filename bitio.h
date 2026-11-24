#ifndef BITIO_H
#define BITIO_H

#include <stdint.h>
#include <stdio.h>

typedef struct {
    FILE *f;
    uint8_t buf;
    int nbits;
} BitWriter;

typedef struct {
    FILE *f;
    uint8_t buf;
    int nbits;
} BitReader;

void bw_init(BitWriter *w, FILE *f);
void bw_write_bit(BitWriter *w, int bit);
void bw_write_bits(BitWriter *w, uint32_t value, int n);
void bw_flush(BitWriter *w);

void br_init(BitReader *r, FILE *f);
int br_read_bit(BitReader *r);
int br_read_bits(BitReader *r, int n, uint32_t *out);

#endif
