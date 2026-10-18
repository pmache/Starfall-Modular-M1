# Starfall Modular M1 — status 2026-09-24

**Kod rdzenia: PASS w testach automatycznych na Linux.**
**Windows EXE / rzeczywisty OpenGL / kreator PowerShell: NOT RUN.**
**Odbiór użytkownika i grywalność: NOT RUN. Cały milestone nie ma jeszcze pełnego PASS.**

Praca wykonana na kopii przesłanego `starfall.zip`; instalacja użytkownika nie została zmieniona.
Desktop Commander odmówił wywołania z powodu miesięcznego limitu. W środowisku tej sesji
brakowało toolchaina Windows/raylib, a pobieranie zależności sieciowych było niedostępne.
Nie dołączono starego EXE ani starych screenshotów jako dowodu nowego builda.

## Wyniki wykonanych testów

| Kontrola | Wynik | Dowód |
|---|---|---|
| Build rdzenia GCC, Release, -Wall -Wextra -Werror, aktywne kontrole mimo NDEBUG | PASS | `evidence/release-build.log`, `evidence/environment.txt` |
| Zachowana symulacja: strzały, śmierć, respawn, Trident, pauza, ruch, skalowanie, klasyczna kampania i restart | 64 kontrole PASS | `evidence/release-tests.log` |
| Nowe kontrole parsera, runnera, stanu gracza, restartu/preview, wielu bossów i trzech sektorów | 112 kontroli PASS | `evidence/release-tests.log` |
| Testy procesu: błędne dane, ścieżki ze spacjami/Unicode, niewłaściwy CWD, duplikaty, CLI | 91 kontroli PASS | `evidence/release-tests.log` |
| Klon sektora + zmiana JSON + ta sama binarka symulatora: 2 → 3 sektory | PASS | SHA-256 przed/po w `evidence/release-tests.log` |
| Łącznie zestawy CTest | 6/6 PASS | `evidence/release-tests.log` |
| Te same zestawy z AddressSanitizer i UndefinedBehaviorSanitizer | 6/6 PASS | `evidence/asan-tests.log` |
| Sprite'y i font bez zmian względem przesłanych źródeł | PASS, identyczne bajty | `evidence/environment.txt` |

267 to suma **64 + 112 + 91 sprawdzeń**, nie 267 pełnych rozgrywek.
Sprawdzenie 2 → 3 sektorów dotyczy natywnego linuksowego narzędzia z prawdziwą
symulacją i kolizjami, nie zbudowanego i uruchomionego Windows EXE gry.

## Testy przygotowane, lecz niewykonane tutaj

`build.ps1` ma kompilować Windows EXE z raylib, wykonać pełny self-test łącznie
z kontrolami sprite'ów/fontu, 12 dotychczasowych testów prawdziwego renderowania,
nowe testy rdzenia, uruchomienie z izolowanym PATH i Unicode oraz smoke-test
`new-sector.ps1` i rozszerzenie kampanii do trzech sektorów bez zmiany EXE.
Dopiero udane wykonanie utworzy świeży BUILD-INFO.json z bieżącymi hashami.

Wykonano pomocniczą kontrolę składni C++ części graficznej na lokalnych deklaracjach
API. Nie jest to kompilacja/linkowanie względem prawdziwego raylib ani test renderowania;
nie zaliczono jej do powyższych 267 sprawdzeń. Deklaracje pomocnicze nie są w paczce.

## Ręczny odbiór po buildzie

Uruchom PLAY-CLASSIC: czytelność i sterowanie jak wcześniej.
Uruchom PLAY-DEMO: pokonanie pierwszego Null Crown przechodzi do Relay, bez wygranej
całej kampanii; życie, punkty i pozostały czas Tridenta przechodzą dalej.
Uruchom TEST-SECTOR: start bez przechodzenia Outer Rim; R po końcu wraca do Relay.
Uruchom NEW-SECTOR, wpisz my_sector: plik powstaje, pojawia się trzeci sektor,
istniejące pliki nie są nadpisane, kopia wcześniejszej kampanii pozostaje obok niej.
Sprawdź zmianę okna, F11, Alt+Enter, Alt+Tab i mysz/klawiaturę na docelowym ekranie.

Drugi sektor jest przykładem wykorzystania tych samych klocków, nie obietnicą
20–30 minut ani gotową długą kampanią. Grafikę bossów/typy broni nadal określa kod.
