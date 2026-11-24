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

static int file_has_bytes(const unsigned char *expected, size_t expected_size)
{
    FILE *file = fopen(TEST_FILE, "rb");
    size_t i;
    int value;

    if (file == NULL) {
        return 0;
    }

    for (i = 0; i < expected_size; ++i) {
        value = fgetc(file);
        if (value == EOF || (unsigned char)value != expected[i]) {
            (void)fclose(file);
            return 0;
        }
    }

    value = fgetc(file);
    if (value != EOF || ferror(file)) {
        (void)fclose(file);
        return 0;
    }

    return fclose(file) == 0;
}

static int test_required_sequence(void)
{
    static const unsigned char expected[] = {0xBF, 0x40};
    FILE *file;
    BitWriter writer;
    BitReader reader;
    uint32_t a = 0, b = 0, c = 0;
    uint32_t unchanged = UINT32_C(0xDEADBEEF);

    file = fopen(TEST_FILE, "wb");
    if (file == NULL) return fail("Nie mozna utworzyc pliku kontrolnego.");

    bw_init(&writer, file);
    bw_write_bits(&writer, UINT32_C(0x5), 3);
    bw_write_bits(&writer, UINT32_C(0x1F), 5);
    bw_write_bits(&writer, UINT32_C(0x1), 2);
    bw_flush(&writer);

    if (ferror(file)) {
        (void)fclose(file);
        return fail("Blad zapisu sekwencji kontrolnej.");
    }
    if (fclose(file) != 0) return fail("Blad zamkniecia pliku kontrolnego.");

    if (!file_has_bytes(expected, sizeof(expected))) {
        return fail("Sekwencja kontrolna musi dac dokladnie bajty BF 40.");
    }

    file = fopen(TEST_FILE, "rb");
    if (file == NULL) return fail("Nie mozna otworzyc pliku do odczytu.");
    br_init(&reader, file);

    if (br_read_bits(&reader, 3, &a) != 0 ||
        br_read_bits(&reader, 5, &b) != 0 ||
        br_read_bits(&reader, 2, &c) != 0) {
        (void)fclose(file);
        return fail("Nie udalo sie odczytac sekwencji kontrolnej.");
    }

    if (a != UINT32_C(0x5) || b != UINT32_C(0x1F) ||
        c != UINT32_C(0x1)) {
        (void)fclose(file);
        return fail("Odczytane wartosci nie zgadzaja sie z zapisanymi.");
    }

    /* Po 10 bitach danych pozostaje 6 bitow dopelnienia. */
    if (br_read_bits(&reader, 6, &a) != 0 || a != 0) {
        (void)fclose(file);
        return fail("Dopelnienie ostatniego bajtu powinno skladac sie z zer.");
    }

    if (br_read_bits(&reader, 1, &unchanged) != -1 ||
        unchanged != UINT32_C(0xDEADBEEF)) {
        (void)fclose(file);
        return fail("EOF ma zwrocic -1 i nie zmieniac *out.");
    }

    if (fclose(file) != 0) return fail("Blad zamkniecia pliku po odczycie.");
    return 0;
}

static int test_32_bits(void)
{
    static const unsigned char expected[] = {0x89, 0xAB, 0xCD, 0xEF};
    FILE *file;
    BitWriter writer;
    BitReader reader;
    uint32_t value = 0;

    file = fopen(TEST_FILE, "wb");
    if (file == NULL) return fail("Nie mozna utworzyc pliku dla 32 bitow.");

    bw_init(&writer, file);
    bw_write_bits(&writer, UINT32_C(0x89ABCDEF), 32);
    bw_flush(&writer);

    if (ferror(file)) {
        (void)fclose(file);
        return fail("Blad zapisu 32 bitow.");
    }
    if (fclose(file) != 0) return fail("Blad zamkniecia pliku 32-bitowego.");

    if (!file_has_bytes(expected, sizeof(expected))) {
        return fail("Wartosc 32-bitowa nie zostala zapisana w kolejnosci MSB-first.");
    }

    file = fopen(TEST_FILE, "rb");
    if (file == NULL) return fail("Nie mozna otworzyc pliku 32-bitowego.");
    br_init(&reader, file);

    if (br_read_bits(&reader, 32, &value) != 0 ||
        value != UINT32_C(0x89ABCDEF)) {
        (void)fclose(file);
        return fail("Odczyt 32 bitow nie odtworzyl wartosci.");
    }

    if (fclose(file) != 0) return fail("Blad koncowego zamkniecia pliku.");
    return 0;
}

static int test_empty_flush_and_invalid_lengths(void)
{
    FILE *file;
    BitWriter writer;
    BitReader reader;
    uint32_t output = UINT32_C(0x12345678);

    file = fopen(TEST_FILE, "wb");
    if (file == NULL) return fail("Nie mozna utworzyc pliku testowego.");

    bw_init(&writer, file);

    /* Pusty flush nie powinien dopisac zadnego bajtu. */
    bw_flush(&writer);
    bw_flush(&writer);

    /* Niepoprawne dlugosci nie powinny zapisac danych. */
    bw_write_bits(&writer, UINT32_C(0xFFFFFFFF), -1);
    bw_write_bits(&writer, UINT32_C(0xFFFFFFFF), 33);
    bw_flush(&writer);

    if (ferror(file)) {
        (void)fclose(file);
        return fail("Blad strumienia podczas testu pustego flush.");
    }
    if (fclose(file) != 0) return fail("Blad zamkniecia pliku testowego.");

    if (!file_has_bytes(NULL, 0)) {
        return fail("Pusty flush lub niepoprawna dlugosc wygenerowaly bajty.");
    }

    file = fopen(TEST_FILE, "rb");
    if (file == NULL) return fail("Nie mozna otworzyc pustego pliku.");
    br_init(&reader, file);

    if (br_read_bits(&reader, 0, &output) != 0 || output != 0) {
        (void)fclose(file);
        return fail("Odczyt zera bitow powinien zwrocic sukces i wartosc 0.");
    }

    if (fclose(file) != 0) return fail("Blad zamkniecia pustego pliku.");
    return 0;
}

int main(void)
{
    if (test_required_sequence() != 0) return 1;
    if (test_32_bits() != 0) return 1;
    if (test_empty_flush_and_invalid_lengths() != 0) return 1;

    if (remove(TEST_FILE) != 0) {
        return fail("Nie mozna usunac pliku tymczasowego.");
    }

    puts("[OK] BF 40, MSB-first, padding, EOF, 32 bity, pusty flush i walidacja n.");
    return 0;
}
