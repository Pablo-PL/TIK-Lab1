#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"
CC="${CC:-gcc}"
ASSET="tests"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

fail() { echo "[BLAD] $*" >&2; exit 1; }

get_entropy() {
    sed -n 's/^Entropia H: \([0-9.]*\).*/\1/p' "$1" | head -n 1
}
check_range() {
    local value="$1" min="$2" max="$3" name="$4"
    awk -v v="$value" -v lo="$min" -v hi="$max" 'BEGIN { exit !(v >= lo && v <= hi) }' \
        || fail "$name: H=$value poza wymaganym zakresem [$min, $max]."
}
run_case() {
    local name="$1" path="$2" out="$3"
    ./entropy "$path" > "$out" || fail "Program zwrocil blad dla: $name"
    echo "--- $name ---"
    cat "$out"
    echo
}

[ -f "$ASSET/pusty.bin" ] || fail "Brak tests/pusty.bin"
[ -f "$ASSET/powtorzenia.bin" ] || fail "Brak tests/powtorzenia.bin"
[ -f "$ASSET/tekst.txt" ] || fail "Brak tests/tekst.txt"
[ -f "$ASSET/losowy.bin" ] || fail "Brak tests/losowy.bin"
[ -f "$ASSET/archiwum.zip" ] || fail "Brak tests/archiwum.zip"

printf '[1/5] Kompilacja programu i testu bitio\n'
"$CC" -std=c11 -Wall -Wextra -Werror -O2 main.c bitio.c -o entropy -lm
"$CC" -std=c11 -Wall -Wextra -Werror -O2 -I. tests/test_bitio.c bitio.c -o "$TMP_DIR/test_bitio"

printf '[2/5] Test modułu bitowego\n'
"$TMP_DIR/test_bitio"

printf '[3/5] Testy zachowania entropy\n'
run_case 'Pusty plik' "$ASSET/pusty.bin" "$TMP_DIR/empty.out"
grep -q 'Rozmiar pliku: 0 bajtow' "$TMP_DIR/empty.out" || fail 'Niepoprawny rozmiar pustego pliku.'
grep -q 'Liczba roznych wartosci bajtow: 0' "$TMP_DIR/empty.out" || fail 'Pusty plik powinien miec 0 symboli.'
grep -q 'Entropia H: 0.000000' "$TMP_DIR/empty.out" || fail 'Entropia pustego pliku powinna wynosic 0.'

run_case '1000 identycznych bajtow' "$ASSET/powtorzenia.bin" "$TMP_DIR/repeated.out"
grep -q 'Rozmiar pliku: 1000 bajtow' "$TMP_DIR/repeated.out" || fail 'Niepoprawny rozmiar danych powtarzalnych.'
grep -q 'Liczba roznych wartosci bajtow: 1' "$TMP_DIR/repeated.out" || fail 'Dane powtarzalne powinny miec 1 symbol.'
grep -q 'Entropia H: 0.000000' "$TMP_DIR/repeated.out" || fail 'Entropia identycznych bajtow powinna wynosic 0.'

run_case 'Tekst polski' "$ASSET/tekst.txt" "$TMP_DIR/text.out"
text_h="$(get_entropy "$TMP_DIR/text.out")"
check_range "$text_h" 4.0 5.0 'Tekst polski'

run_case 'Losowe dane 1 MiB' "$ASSET/losowy.bin" "$TMP_DIR/random.out"
random_h="$(get_entropy "$TMP_DIR/random.out")"
check_range "$random_h" 7.95 8.0 'Losowy plik'

run_case 'Archiwum ZIP' "$ASSET/archiwum.zip" "$TMP_DIR/zip.out"
zip_h="$(get_entropy "$TMP_DIR/zip.out")"
check_range "$zip_h" 7.80 8.0 'Archiwum ZIP'

run_case 'Plik wykonywalny entropy' './entropy' "$TMP_DIR/exe.out"
exe_h="$(get_entropy "$TMP_DIR/exe.out")"
check_range "$exe_h" 0.0 7.99 'Plik wykonywalny'

printf '[4/5] Testy blednych argumentow i brakujacego pliku\n'
if ./entropy >"$TMP_DIR/noargs.out" 2>"$TMP_DIR/noargs.err"; then fail 'Brak argumentu powinien zwrocic kod bledu.'; fi
[ -s "$TMP_DIR/noargs.err" ] || fail 'Brak komunikatu dla niepoprawnych argumentow.'
if ./entropy a b >"$TMP_DIR/manyargs.out" 2>"$TMP_DIR/manyargs.err"; then fail 'Nadmiarowy argument powinien zwrocic kod bledu.'; fi
if ./entropy "$TMP_DIR/nie-istnieje.bin" >"$TMP_DIR/missing.out" 2>"$TMP_DIR/missing.err"; then fail 'Brak pliku powinien zwrocic kod bledu.'; fi
[ -s "$TMP_DIR/missing.err" ] || fail 'Brak komunikatu o bledzie otwarcia pliku.'

printf '[5/5] Zapis wynikow pomiarow\n'
{
    echo 'Wyniki pomiarow programu entropy (Linux, GCC, -std=c11 -Wall -Wextra -Werror -O2)'
    echo 'Archiwum ZIP jest dolaczonym zestawem testowym; plik wykonywalny jest budowany lokalnie.'
    for pair in \
        'Pusty plik:'"$TMP_DIR/empty.out" \
        '1000 identycznych bajtow:'"$TMP_DIR/repeated.out" \
        'Tekst polski:'"$TMP_DIR/text.out" \
        'Losowe dane 1 MiB:'"$TMP_DIR/random.out" \
        'Archiwum ZIP:'"$TMP_DIR/zip.out" \
        'Plik wykonywalny:'"$TMP_DIR/exe.out"; do
        label="${pair%%:*}"
        path="${pair#*:}"
        printf '\n=== %s ===\n' "$label"
        cat "$path"
    done
} > tests/wyniki_pomiarow.txt

printf 'Wszystkie testy zakonczone powodzeniem. Wyniki: tests/wyniki_pomiarow.txt\n'
