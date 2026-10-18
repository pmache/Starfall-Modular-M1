# Starfall — Modular M1

**To jest zmodyfikowany projekt źródłowy, nie nowy gotowy EXE dla Windows.**
Kod rdzenia skompilowano i przetestowano w Linux. Kompilacji Windows, prawdziwego
OpenGL i skryptów PowerShell nie uruchomiono w tej sesji. Nie dołączono starego EXE
udającego nowe wydanie. Szczegóły i logi: `ACCEPTANCE.md`, `evidence/` w paczce źródłowej.

## Najprostsze uruchomienie na Twoim Windows

1. Rozpakuj projekt do **nowego folderu** — nie nadpisuj obecnego Starfall.
2. Uruchom `BUILD-MODULAR.cmd`. Domyślny toolchain pozostaje taki jak w przesłanym projekcie:
   `V:\dev\msys64\mingw64` (GCC, raylib 5.5, GLFW, runtime i licencje).
3. Po udanym buildzie ścieżka nowej paczki pojawi się w oknie oraz `LATEST-PACKAGE.txt`.
   Wejdź do tego folderu i uruchom **`PLAY-DEMO.cmd`** — kampanię z dwoma sektorami.

Z innym toolchainem:

```powershell
.\build.ps1 -Toolchain 'D:\narzedzia\msys64\mingw64'
```

Build tworzy osobny katalog `build/<id>` i `dist/<id>/Starfall-Squadron`.
Kompiluje grę, narzędzie zawartości i testy; sprawdza logikę, renderowanie,
przenośny start oraz klonowanie sektora bez zmiany EXE. Nie instaluje zależności.
Nie oznaczy nieudanego builda jako gotowej paczki. Windows nadal wymaga wykonania tego kroku.

## Co jest teraz modułowe

```text
content/
  game.json                 wybór domyślnej kampanii i aktywnych paczek
  base/
    pack.json               opis paczki
    campaigns/classic.json  oryginalna krótka gra
    sectors/outer_rim.json   kolejność fal, przerw i bossa
    waves/                  formacje i parametry ostrzału
    enemies/                HP, punktacja, sprite, możliwość nurkowania
    bosses/                 HP, punktacja, ruch i parametry ataków
  example/                  drugi sektor i przykładowa kampania
```

`src/content` wczytuje i sprawdza dane. `src/gameplay` prowadzi kampanię i symulację
bez zależności od raylib. `src/runtime` obsługuje okno, wejście i rysowanie.
Grafika `pixel_art.h` i font `pixel_font.h` pozostały identyczne z przesłaną paczką.

**Nowy sektor, układ fal albo wariant przeciwnika nie wymaga rekompilacji.**
Nowy rodzaj mechaniki, nowy sprite, broń lub efekt wymagają jeszcze kodu.
Nie ma w M1 graficznego edytora, Lua, zewnętrznych tekstur, systemu broni/pickupów
z JSON, dźwięku, zapisu kampanii ani rekordów. To fundament, nie całe docelowe narzędzie.

## Dodanie sektora bez kompilacji

W **zbudowanej paczce** uruchom `NEW-SECTOR.cmd` i wpisz np. `my_sector`.
Kreator kopiuje `example:relay`, tworzy `example:my_sector`, dopisuje go do kampanii
`example:two_sectors`, zapisuje kopię poprzedniej kampanii i waliduje wynik.
Istniejących identyfikatorów nie nadpisuje. Nowy sektor to wzorzec do dalszej edycji,
nie automatycznie unikatowy level.

Otwórz `content/example/sectors/my_sector.json` i zmień kroki.
Test samego poziomu:

```powershell
.\starfall.exe --sector example:my_sector
```

Pełna kampania z dodanym sektorem:

```powershell
.\starfall.exe --campaign example:two_sectors
```

**Pliki są wczytywane przy uruchomieniu procesu.** Zamknij grę i uruchom ją ponownie
po edycji danych. Klawisz R resetuje rozgrywkę, ale nie przeładowuje plików.
Pełny opis pól i ręczna metoda: `docs/CONTENT-GUIDE.md`.

## Polecenia w zbudowanej paczce

```powershell
.\starfall.exe --campaign base:classic
.\starfall.exe --campaign example:two_sectors
.\starfall.exe --sector example:relay
.\starfall-content.exe --validate-content
.\starfall-content.exe --simulate-campaign --campaign example:two_sectors
.\starfall.exe --self-test
.\starfall.exe --render-test
```

`--simulate-campaign` używa kontrolowanych trafień w prawdziwym kodzie kolizji.
Sprawdza przebieg kampanii, **nie** poziom trudności ani czas gry człowieka.
Walidator zwraca kod 2 przy błędzie, podaje plik i pole; błędy składni JSON także
wiersz i kolumnę. Gracz uruchamiający grę dwuklikiem otrzymuje komunikat błędu
Windows. Diagnostyka jest również zapisywana jako `starfall-content-error.log` obok EXE.

Domyślne `content` jest szukane **obok EXE**, nie w bieżącym katalogu terminala.
Jawna alternatywa: `--content-root "D:\Moja zawartosc"`. Identyfikatory kampanii i
sektorów nie są ścieżkami plików. Wszystkie włączone paczki są walidowane w całości.

## Sterowanie

Mysz lub WASD/strzałki — ruch. LPM/Spacja/Z — strzał.
P/Esc — pauza; utrata fokusu również pauzuje. F11/Alt+Enter — pełny ekran.
R/Enter po wygranej lub przegranej — restart wybranej kampanii albo testowanego sektora.
F12 — zrzut ekranu. Zachowano dotychczasowe trzy życia i moduł Trident.
Między sektorami zachowane są HP, życia, punkty, pozycja i pozostały czas Tridenta;
czas ulepszenia nadal upływa podczas przejścia. Pociski są czyszczone.

## Testy bez grafiki

CMake i kompilator C++17 wystarczą do rdzenia. Python 3 uruchamia dodatkowy test
zmian samych danych i nieprawidłowych paczek — nie jest potrzebny do gry.

```sh
cmake -S . -B out -DSTARFALL_BUILD_GAME=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build out
ctest --test-dir out --output-on-failure
```

Dla AddressSanitizer/UndefinedBehaviorSanitizer dodaj `-DSTARFALL_SANITIZE=ON` w konfiguracji.
CMake z grafiką wymaga zainstalowanego raylib. Dla Windows zalecany jest istniejący
proces `build.ps1`, który dodatkowo zbiera DLL, licencje i sprawdza przenośną paczkę.
