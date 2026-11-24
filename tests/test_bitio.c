#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "bitio.h"

#define TEST_FILE "tests/test_bitio_tmp.bin"

static int fail(const char *message)
{
    fprintf(stderr, "[BLAD] %s\n", message);
    (void)remove(TEST_FILE);
    return 1;
}

static int test_required_sequence(void)
{
    FILE *file;
    BitWriter writer;
    BitReader reader;
    uint32_t a = 0, b = 0, c = 0, extra = UINT32_C(0xDEADBEEF);
    int first, second, third;

    file = fopen(TEST_FILE, "wb");
    if (file == NULL) return fail("Nie mozna utworzyc pliku kontrolnego.");

    bw_init(&writer, file);
    bw_write_bits(&writer, UINT32_C(0x5), 3);
    bw_write_bits(&writer, UINT32_C(0x1F), 5);
    bw_write_bits(&writer, UINT32_C(0x1), 2);
    bw_flush(&writer);
    if (ferror(file)) {
        (void)fclose(file);
        return fail("Blad zapisu w sekwencji kontrolnej.");
    }
    if (fclose(file) != 0) return fail("Blad zamkniecia pliku kontrolnego.");

    file = fopen(TEST_FILE, "rb");
    if (file == NULL) return fail("Nie mozna otworzyc pliku do porownania.");
    first = fgetc(file);
    second = fgetc(file);
    third = fgetc(file);
    if (first != 0xBF || second != 0x40 || third != EOF || ferror(file)) {
        (void)fclose(file);
        return fail("Sekwencja kontrolna musi dac dokladnie bajty BF 40.");
    }
    if (fclose(file) != 0) return fail("Blad zamkniecia pliku po porownaniu.");

    file = fopen(TEST_FILE, "rb");
    if (file == NULL) return fail("Nie mozna otworzyc pliku do odczytu bitow.");
    br_init(&reader, file);
    if (br_read_bits(&reader, 3, &a) != 0 ||
        br_read_bits(&reader, 5, &b) != 0 ||
        br_read_bits(&reader, 2, &c) != 0) {
        (void)fclose(file);
        return fail("Nie udalo sie odczytac sekwencji kontrolnej.");
    }
    if (a != UINT32_C(0x5) || b != UINT32_C(0x1F) || c != UINT32_C(0x1)) {
        (void)fclose(file);
        return fail("Odczytane wartosci nie zgadzaja sie z zapisanymi.");
    }
    /* Z 16 bitow pliku 10 to dane, a 6 to dopelnienie; 7. bitu nie ma. */
    if (br_read_bits(&reader, 7, &extra) != -1 || extra != UINT32_C(0xDEADBEEF)) {
        (void)fclose(file);
        return fail("EOF ma zwrocic -1 i nie zmieniac wyniku wyjsciowego.");
    }
    if (fclose(file) != 0) return fail("Blad zamkniecia pliku po odczycie.");
    return 0;
}

static int test_32_bits(void)
{
    FILE *file;
    BitWriter writer;
    BitReader reader;
    uint32_t value = 0;
    int byte;

    file = fopen(TEST_FILE, "wb");
    if (file == NULL) return fail("Nie mozna utworzyc pliku dla 32 bitow.");
    bw_init(&writer, file);
    bw_write_bits(&writer, UINT32_C(0x89ABCDEF), 32);
    bw_flush(&writer);
    {
        const int write_error = ferror(file);
        const int close_error = fclose(file);
        if (write_error || close_error != 0) {
            return fail("Blad zapisu 32 bitow.");
        }
    }

    file = fopen(TEST_FILE, "rb");
    if (file == NULL) return fail("Nie mozna otworzyc pliku 32-bitowego.");
    for (byte = 0; byte < 4; ++byte) {
        static const int expected[4] = {0x89, 0xAB, 0xCD, 0xEF};
        if (fgetc(file) != expected[byte]) {
            (void)fclose(file);
            return fail("Kolejnosc MSB-first dla 32 bitow jest niepoprawna.");
        }
    }
    if (fgetc(file) != EOF || ferror(file)) {
        (void)fclose(file);
        return fail("Zapis 32 bitow powinien miec dokladnie 4 bajty.");
    }
    if (fclose(file) != 0) return fail("Blad zamkniecia pliku 32-bitowego.");

    file = fopen(TEST_FILE, "rb");
    if (file == NULL) return fail("Nie mozna ponownie otworzyc pliku 32-bitowego.");
    br_init(&reader, file);
    if (br_read_bits(&reader, 32, &value) != 0 || value != UINT32_C(0x89ABCDEF)) {
        (void)fclose(file);
        return fail("Odczyt 32 bitow nie odtworzyl wartosci.");
    }
    if (fclose(file) != 0) return fail("Blad koncowego zamkniecia pliku.");
    return 0;
}

int main(void)
{
    if (test_required_sequence() != 0 || test_32_bits() != 0) return 1;
    if (remove(TEST_FILE) != 0) return fail("Nie mozna usunac pliku tymczasowego.");
    puts("[OK] BF 40, MSB-first, dopelnienie zerami, odczyt, EOF i 32 bity.");
    return 0;
}
