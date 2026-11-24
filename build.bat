@echo off
setlocal
cd /d "%~dp0"
where gcc >nul 2>nul
if errorlevel 1 (
    echo BLAD: Nie znaleziono gcc. Zainstaluj MinGW-w64 i dodaj gcc do PATH.
    exit /b 1
)
gcc -std=c11 -Wall -Wextra -Werror -O2 main.c bitio.c -o entropy.exe -lm
if errorlevel 1 (
    echo BLAD: Kompilacja nie powiodla sie.
    exit /b 1
)
echo OK: Zbudowano entropy.exe
exit /b 0
