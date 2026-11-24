
$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
Set-Location $Root

$Gcc = Get-Command gcc -ErrorAction SilentlyContinue
if (-not $Gcc) {
    throw 'Nie znaleziono gcc. Zainstaluj MinGW-w64 i dodaj gcc do PATH.'
}

$Asset = $PSScriptRoot
$Entropy = Join-Path $Root 'entropy.exe'
$TestExe = Join-Path $PSScriptRoot 'test_bitio.exe'
$Results = Join-Path $PSScriptRoot 'wyniki_pomiarow_windows.txt'

function Assert-Range([double]$Value, [double]$Min, [double]$Max, [string]$Name) {
    if ($Value -lt $Min -or $Value -gt $Max) {
        throw "$Name`: entropia H=$Value poza zakresem [$Min, $Max]."
    }
}

# ============================================================
# [1/5] Kompilacja programu entropy
# ============================================================

Write-Host '[1/5] Kompilacja entropy'

& gcc -std=c11 -Wall -Wextra -Werror -O2 main.c bitio.c -o $Entropy -lm

if ($LASTEXITCODE -ne 0) {
    throw 'Kompilacja entropy nie powiodla sie.'
}

# ============================================================
# [2/5] Kompilacja testu bitio
# ============================================================

Write-Host '[2/5] Kompilacja testu bitio'

& gcc -std=c11 -Wall -Wextra -Werror -O2 -I. tests\test_bitio.c bitio.c -o $TestExe

if ($LASTEXITCODE -ne 0) {
    throw 'Kompilacja testu bitio nie powiodla sie.'
}

# ============================================================
# [3/5] Test modulu bitowego
# ============================================================

Write-Host '[3/5] Test modulu bitowego'

& $TestExe

if ($LASTEXITCODE -ne 0) {
    throw 'Test bitio nie powiodl sie.'
}

# ============================================================
# [4/5] Analiza wymaganych przypadkow
# ============================================================

$Cases = @(
    @{ Name = 'Pusty plik'; File = 'pusty.bin'; Min = 0.0; Max = 0.0 },
    @{ Name = '1000 identycznych bajtow'; File = 'powtorzenia.bin'; Min = 0.0; Max = 0.0 },
    @{ Name = 'Tekst polski'; File = 'tekst.txt'; Min = 4.0; Max = 5.0 },
    @{ Name = 'Losowe dane 1 MiB'; File = 'losowy.bin'; Min = 7.95; Max = 8.0 },
    @{ Name = 'Archiwum ZIP'; File = 'archiwum.zip'; Min = 7.80; Max = 8.0 }
)

$OutputLines = @(
    'Wyniki pomiarow programu entropy (Windows / MinGW-w64)',
    ''
)

Write-Host '[4/5] Analiza szesciu wymaganych przypadkow'

foreach ($Case in $Cases) {

    $Path = if ([IO.Path]::IsPathRooted([string]$Case.File)) {
        [string]$Case.File
    }
    else {
        Join-Path $Asset $Case.File
    }

    if (-not (Test-Path $Path)) {
        throw "Brak zestawu testowego: $Path"
    }

    $Lines = & $Entropy $Path 2>&1

    if ($LASTEXITCODE -ne 0) {
        throw "Program entropy zakonczyl sie bledem: $($Case.Name)"
    }

    $Lines | ForEach-Object {
        Write-Host $_
    }

    $EntropyLine = $Lines |
        Where-Object { $_ -match '^Entropia H: ' } |
        Select-Object -First 1

    $Match = [regex]::Match(
        [string]$EntropyLine,
        'Entropia H: ([0-9]+\.[0-9]+)'
    )

    if (-not $Match.Success) {
        throw "Nie mozna odczytac H dla: $($Case.Name)"
    }

    $H = [double]::Parse(
        $Match.Groups[1].Value,
        [Globalization.CultureInfo]::InvariantCulture
    )

    Assert-Range $H $Case.Min $Case.Max $Case.Name

    $OutputLines += "=== $($Case.Name) ==="
    $OutputLines += $Lines | ForEach-Object { [string]$_ }
    $OutputLines += ''
}

# ============================================================
# Analiza pliku wykonywalnego
# ============================================================

$Lines = & $Entropy $Entropy 2>&1

if ($LASTEXITCODE -ne 0) {
    throw 'Nie udalo sie przeanalizowac pliku wykonywalnego.'
}

$Lines | ForEach-Object {
    Write-Host $_
}

$EntropyLine = $Lines |
    Where-Object { $_ -match '^Entropia H: ' } |
    Select-Object -First 1

$Match = [regex]::Match(
    [string]$EntropyLine,
    'Entropia H: ([0-9]+\.[0-9]+)'
)

if (-not $Match.Success) {
    throw 'Nie mozna odczytac H pliku wykonywalnego.'
}

$ExeH = [double]::Parse(
    $Match.Groups[1].Value,
    [Globalization.CultureInfo]::InvariantCulture
)

Assert-Range $ExeH 0.0 7.99 'Plik wykonywalny'

$OutputLines += '=== Plik wykonywalny entropy.exe ==='
$OutputLines += $Lines | ForEach-Object { [string]$_ }

# ============================================================
# [5/5] Testy bledow
# ============================================================

Write-Host '[5/5] Testy bledow'

# Tymczasowo zmieniamy obsluge bledow PowerShell.
# Komunikaty stderr programu C nie powinny przerywac testow.

$PreviousErrorActionPreference = $ErrorActionPreference
$ErrorActionPreference = 'Continue'

try {

    # Test 1: brak argumentow

    & $Entropy 2>$null
    $ExitCode = $LASTEXITCODE

    if ($ExitCode -eq 0) {
        throw 'Brak argumentow powinien zwrocic blad.'
    }

    # Test 2: nadmiarowe argumenty

    & $Entropy 'a' 'b' 2>$null
    $ExitCode = $LASTEXITCODE

    if ($ExitCode -eq 0) {
        throw 'Nadmiarowy argument powinien zwrocic blad.'
    }

    # Test 3: nieistniejacy plik

    $Missing = Join-Path $env:TEMP 'plik-ktory-nie-istnieje-tik-l1.bin'

    & $Entropy $Missing 2>$null
    $ExitCode = $LASTEXITCODE

    if ($ExitCode -eq 0) {
        throw 'Brak pliku powinien zwrocic blad.'
    }

}
finally {
    # Przywracamy poprzednie ustawienie PowerShell.

    $ErrorActionPreference = $PreviousErrorActionPreference
}

# ============================================================
# Zapis wynikow pomiarow
# ============================================================

[IO.File]::WriteAllLines(
    $Results,
    $OutputLines,
    [Text.UTF8Encoding]::new($false)
)

Write-Host "Wszystkie testy zakonczone powodzeniem. Wyniki: $Results" -ForegroundColor Green

# ============================================================
# Opcjonalna kontrola pamieci Dr. Memory
# ============================================================

$DrMemory = Get-Command drmemory -ErrorAction SilentlyContinue

if ($DrMemory) {

    Write-Host 'Uruchamianie Dr. Memory...'

    & drmemory -- $Entropy (Join-Path $Asset 'tekst.txt')

    if ($LASTEXITCODE -ne 0) {
        throw 'Dr. Memory zglosil blad.'
    }

}
else {
    Write-Host 'Dr. Memory nie jest zainstalowany - kontrola pamieci pominieta.'
}
