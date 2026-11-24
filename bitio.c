#include "bitio.h"

void bw_init(BitWriter *w, FILE *f)
{
    if (w == NULL) {
        return;
    }
    w->f = f;
    w->buf = 0;
    w->nbits = 0;
}

void bw_write_bit(BitWriter *w, int bit)
{
    if (w == NULL || w->f == NULL || w->nbits < 0 || w->nbits > 7 ||
        ferror(w->f)) {
        return;
    }

    w->buf = (uint8_t)((uint8_t)(w->buf << 1) | (uint8_t)(bit != 0));
    ++w->nbits;

    if (w->nbits == 8) {
        if (fputc((int)w->buf, w->f) == EOF) {
            return;
        }
        w->buf = 0;
        w->nbits = 0;
    }
}

void bw_write_bits(BitWriter *w, uint32_t value, int n)
{
    int bit_index;

    if (w == NULL || w->f == NULL || n < 0 || n > 32) {
        return;
    }

    for (bit_index = n - 1; bit_index >= 0; --bit_index) {
        bw_write_bit(w, (int)((value >> (unsigned int)bit_index) & UINT32_C(1)));
        if (ferror(w->f)) {
            return;
        }
    }
}

void bw_flush(BitWriter *w)
{
    if (w == NULL || w->f == NULL || w->nbits < 0 || w->nbits > 7 ||
        ferror(w->f)) {
        return;
    }

    if (w->nbits > 0) {
        const uint8_t padded = (uint8_t)(w->buf << (8 - w->nbits));
        if (fputc((int)padded, w->f) == EOF) {
            return;
        }
        w->buf = 0;
        w->nbits = 0;
    }
}

void br_init(BitReader *r, FILE *f)
{
    if (r == NULL) {
        return;
    }
    r->f = f;
    r->buf = 0;
    r->nbits = 0;
}

int br_read_bit(BitReader *r)
{
    int value;

    if (r == NULL || r->f == NULL || r->nbits < 0 || r->nbits > 8) {
        return -1;
    }

    if (r->nbits == 0) {
        value = fgetc(r->f);
        if (value == EOF) {
            return -1;
        }
        r->buf = (uint8_t)value;
        r->nbits = 8;
    }

    value = (int)((r->buf >> 7) & UINT8_C(1));
    r->buf = (uint8_t)(r->buf << 1);
    --r->nbits;
    return value;
}

int br_read_bits(BitReader *r, int n, uint32_t *out)
{
    uint32_t value = 0;
    int i;

    if (r == NULL || r->f == NULL || out == NULL || n < 0 || n > 32) {
        return -1;
    }

    for (i = 0; i < n; ++i) {
        const int bit = br_read_bit(r);
        if (bit == -1) {
            return -1;
        }
        value = (value << 1) | (uint32_t)bit;
    }

    *out = value;
    return 0;
}
