#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int analyze_file(const char *path, uintmax_t counts[256],
                        uintmax_t *total_bytes)
{
    FILE *file;
    int value;

    file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Nie mozna otworzyc pliku '%s': %s\n", path,
                strerror(errno));
        return -1;
    }

    memset(counts, 0, 256U * sizeof(counts[0]));
    *total_bytes = 0;

    while ((value = fgetc(file)) != EOF) {
        const unsigned char symbol = (unsigned char)value;

        if (*total_bytes == UINTMAX_MAX || counts[symbol] == UINTMAX_MAX) {
            fprintf(stderr, "Plik jest zbyt duzy do reprezentacji licznikow.\n");
            (void)fclose(file);
            return -1;
        }
        ++counts[symbol];
        ++(*total_bytes);
    }

    if (ferror(file)) {
        fprintf(stderr, "Blad odczytu pliku '%s'.\n", path);
        (void)fclose(file);
        return -1;
    }

    if (fclose(file) != 0) {
        fprintf(stderr, "Blad zamykania pliku '%s'.\n", path);
        return -1;
    }

    return 0;
}

static unsigned int count_distinct_symbols(const uintmax_t counts[256])
{
    unsigned int distinct = 0;
    size_t i;

    for (i = 0; i < 256U; ++i) {
        if (counts[i] != 0) {
            ++distinct;
        }
    }
    return distinct;
}

static double calculate_entropy(const uintmax_t counts[256],
                                uintmax_t total_bytes)
{
    double entropy = 0.0;
    size_t i;

    if (total_bytes == 0) {
        return 0.0;
    }

    for (i = 0; i < 256U; ++i) {
        if (counts[i] != 0) {
            const double probability =
                (double)counts[i] / (double)total_bytes;
            entropy -= probability * log2(probability);
        }
    }
    return entropy;
}

int main(int argc, char *argv[])
{
    uintmax_t counts[256];
    uintmax_t total_bytes;
    unsigned int distinct;
    double entropy;
    double theoretical_size;

    if (argc != 2) {
        fprintf(stderr, "Uzycie: %s <plik>\n", argc > 0 ? argv[0] : "entropy");
        return 1;
    }

    if (analyze_file(argv[1], counts, &total_bytes) != 0) {
        return 1;
    }

    distinct = count_distinct_symbols(counts);
    entropy = calculate_entropy(counts, total_bytes);
    theoretical_size = (double)total_bytes * entropy / 8.0;

    printf("Rozmiar pliku: %" PRIuMAX " bajtow\n", total_bytes);
    printf("Liczba roznych wartosci bajtow: %u\n", distinct);
    printf("Entropia H: %.6f bitow/symbol\n", entropy);
    printf("Teoretyczny minimalny rozmiar po kompresji: %.6f bajtow\n",
           theoretical_size);

    return 0;
}
