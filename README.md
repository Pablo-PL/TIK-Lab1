# TIiK – Laboratorium L1

**Bitowe wejście/wyjście i entropia pliku**

Projekt realizuje wymagania ćwiczenia L1: program `entropy` oblicza statystyki bajtów i entropię Shannona, a moduł `bitio` udostępnia zapis oraz odczyt bitów w kolejności MSB-first.

## Wymagania i kompilacja

- język C11;
- GCC (Linux) lub MinGW-w64 GCC (Windows);
- kompilacja z flagami `-std=c11 -Wall -Wextra -O2` (w projekcie dodatkowo `-Werror`, aby ostrzeżenia przerywały kompilację);
- moduł programu korzysta ze standardowej biblioteki C i `libm` (`log2`).

Linux:

```sh
make
make test
```

Windows – MinGW-w64 w `PATH`:

```bat
build.bat
tests\run.bat
```

Alternatywnie w PowerShell:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests\run.ps1
```

## Program `entropy`

Uruchomienie:

```sh
./entropy sciezka/do/pliku
```

Windows:

```powershell
.\entropy.exe sciezka\do\pliku
```

Program otwiera plik w trybie binarnym (`rb`) i odczytuje go bajt po bajcie. Wyświetla:

1. rozmiar pliku w bajtach `n`;
2. liczbę różnych wartości bajtów;
3. entropię `H` w bitach na symbol;
4. teoretyczny minimalny rozmiar `n * H / 8` w bajtach.

Dla prawdopodobieństwa wystąpienia bajtu `i`, oznaczonego `p_i`, entropia wynosi:

\[
H = -\sum_{i:p_i>0} p_i\log_2(p_i)
\]

Dla pustego pliku przyjmowane jest `H = 0`. Liczniki przechowywane są w stałej tablicy 256 elementów, dlatego zużycie pamięci nie zależy od rozmiaru analizowanego pliku. Błędna liczba argumentów, brak pliku, błąd odczytu lub zamknięcia pliku powodują komunikat na `stderr` i niezerowy kod zakończenia.

`n * H / 8` jest wartością teoretyczną wynikającą z rozkładu pojedynczych bajtów. Nie jest obietnicą, że dowolny kompresor uzyska dokładnie taki rozmiar – rzeczywisty wynik zależy m.in. od modelu, narzutu formatu i zależności między symbolami.

## Moduł `bitio`

Interfejs w `bitio.h` odpowiada interfejsowi z instrukcji:

```c
typedef struct { FILE *f; uint8_t buf; int nbits; } BitWriter;
typedef struct { FILE *f; uint8_t buf; int nbits; } BitReader;

void bw_init(BitWriter *w, FILE *f);
void bw_write_bit(BitWriter *w, int bit);
void bw_write_bits(BitWriter *w, uint32_t value, int n);
void bw_flush(BitWriter *w);
void br_init(BitReader *r, FILE *f);
int br_read_bit(BitReader *r);
int br_read_bits(BitReader *r, int n, uint32_t *out);
```

- bity zapisywane i odczytywane są od najbardziej znaczącego bitu (MSB-first);
- `bw_write_bits` zapisuje `n` najmłodszych bitów `value`;
- `bw_flush` dopisuje niepełny bajt i uzupełnia jego najmłodsze pozycje zerami;
- `br_read_bit` zwraca `0`, `1` lub `-1` przy końcu danych/błędzie odczytu;
- `br_read_bits` zwraca `0` po odczycie wszystkich żądanych bitów albo `-1`, gdy ich brakuje lub argumenty są niepoprawne. Przy niepowodzeniu nie zmienia wartości `*out`.

Wymagana sekwencja:

```c
bw_write_bits(&w, 0x5, 3);
bw_write_bits(&w, 0x1F, 5);
bw_write_bits(&w, 0x1, 2);
bw_flush(&w);
```

musi dać dokładnie dwa bajty: `BF 40`. Funkcje zapisu mają w instrukcji typ `void`, dlatego błędy zapisu należy sprawdzać przez `ferror(w.f)` i wynik `fclose(w.f)`.

## Testy i zestawy danych

Zestawy testowe znajdują się w `tests/` i są dołączone do repozytorium:

| Plik | Zawartość | Oczekiwane zachowanie |
|---|---|---|
| `pusty.bin` | 0 bajtów | `H = 0` |
| `powtorzenia.bin` | 1000 bajtów `0x41` | `H = 0` |
| `tekst.txt` | polski tekst UTF-8 | entropia orientacyjnie 4–5 bitów/symbol |
| `losowy.bin` | 1 MiB danych pseudolosowych | entropia bliska 8 |
| `archiwum.zip` | archiwum ZIP zawierające dane pseudolosowe | entropia bliska 8 |
| `entropy` / `entropy.exe` | program po lokalnej kompilacji | entropia wyraźnie niższa niż 8 |

### Przykładowe wyniki pomiarów (Linux, GCC, 24.11.2026)

Poniższe wartości pochodzą z uruchomienia `tests/run.sh` w tym repozytorium. Archiwum ZIP jest dołączonym zestawem testowym, a plik wykonywalny jest budowany lokalnie. Wyniki dla pliku wykonywalnego mogą zależeć od platformy i kompilatora.

| Przypadek | Rozmiar [B] | Różne bajty | Entropia H [bit/symbol] | `n·H/8` [B] |
|---|---:|---:|---:|---:|
| Pusty plik | 0 | 0 | 0.000000 | 0.000000 |
| 1000 identycznych bajtów | 1000 | 1 | 0.000000 | 0.000000 |
| Tekst polski UTF-8 | 156 | 27 | 4.353715 | 84.897433 |
| Dane pseudolosowe 1 MiB | 1 048 576 | 256 | 7.999839 | 1 048 554.932827 |
| Archiwum ZIP | 1 049 014 | 256 | 7.999838 | 1 048 992.759676 |
| Plik wykonywalny `entropy` | 16 768 | 244 | 2.853712 | 5 981.380578 |

**Interpretacja:** pusty plik i plik z jednym powtarzanym bajtem mają entropię równą zero, ponieważ nie występuje niepewność co do kolejnego symbolu. Tekst ma mniejszą entropię niż dane losowe ze względu na nierównomierne częstości bajtów. Dane losowe i archiwum ZIP osiągają niemal 8 bitów na bajt – jest to maksimum dla alfabetu 256 wartości. Plik wykonywalny ma w tym pomiarze entropię 2.853712, ponieważ jego rozkład bajtów jest nierównomierny. Warto pamiętać, że obliczana jest entropia marginalnego rozkładu bajtów, a nie pełny model zależności między kolejnymi bajtami.

Testy sprawdzają również błędną liczbę argumentów, brak pliku, dokładny bajtowy wynik `BF 40`, odczyt MSB-first, dopełnienie ostatniego bajtu i zachowanie na EOF.

Uruchomienie testów zapisuje wyniki pomiarów do `tests/wyniki_pomiarow.txt` (Linux) lub `tests/wyniki_pomiarow_windows.txt` (Windows). Są to wyniki generowane lokalnie na podstawie użytego kompilatora i pliku wykonywalnego; nie są sztywno wpisanymi wartościami.

## Kontrola pamięci

Linux, jeżeli Valgrind jest zainstalowany:

```sh
make memcheck
```

Windows, jeżeli Dr. Memory jest zainstalowany i dostępny w `PATH`, skrypt `tests/run.ps1` uruchamia go automatycznie po testach. Można też wywołać:

```powershell
drmemory -- .\entropy.exe .\tests\tekst.txt
```

## Zawartość repozytorium

```text
.
├── main.c
├── bitio.c
├── bitio.h
├── Makefile
├── build.bat
├── README.md
├── dane.txt
├── stderr/.gitkeep
└── tests
    ├── run.sh
    ├── run.bat
    ├── run.ps1
    ├── test_bitio.c
    ├── losowy.bin
    ├── archiwum.zip
    ├── powtorzenia.bin
    ├── pusty.bin
    └── tekst.txt
```
