# Granice modułów M1

`content/json.*` — mały, ścisły czytnik JSON bez zewnętrznych bibliotek. Odrzuca
nieprawidłowy UTF-8, zduplikowane klucze, niefinitywne liczby i nadmierne zasoby.
Nie jest ogólnym edytorem/serializatorem ani implementacją JSON Schema.

`content/catalog.*` — atomowo wczytuje aktywne paczki, waliduje pola/zakresy,
rejestruje identyfikatory i rozwiązuje wszystkie odwołania. Zwraca gotowy Catalog
albo wyjątek z lokalizacją. Nie uruchamia gry, skryptów ani kodu z paczek.

`gameplay/campaign.*` — prowadzi trasę kampanii: sektor, krok, oczekiwanie, przejście,
zakończenie. Wejściem są czas i sygnały zakończenia walki; wyjściem RunAction.
Runner nie wie o raylib, myszce, renderowaniu ani geometrii trafień.

`gameplay/game.*` — istniejąca symulacja, wydzielona z main.cpp. Wykonuje akcje
runnera i tworzy obiekty z definicji. Zniszczenie bossa zgłasza bossDefeated;
State::Victory ustawia wyłącznie obsługa RunAction::Victory. Na przejściu przenosi
stan gracza i zeruje pociski. Używa własnych prostych Vec2/Rgba/Rect, bez raylib.

`gameplay/simulate.*` — narzędzie testowe: kontrolowane pociski przez prawdziwe
kolizje i runner. Wynik to ślad ENTER/STEP/TRANSITION/VICTORY. To nie autopilot
udowadniający trudność albo czas przejścia. Ma limit 14 mln ticków.

`runtime/renderer.*` — dotychczasowa grafika/input/raylib plus cienkie adaptery typów.
HUD pokazuje sektor X/Y, nazwy z danych i koniec kampanii. Współrzędne, sterowanie,
piksele i sprite'y nie zostały zastąpione nową oprawą.

`runtime/options.*`, `runtime/paths.*` — argumenty, jawny wybór kampanii/sektora,
ścieżka EXE niezależna od CWD; dla Windows odczyt Unicode z natywnej linii poleceń.
Błąd przed utworzeniem okna blokuje start, zamiast uruchamiać zaszyty poziom awaryjny.

`main.cpp` — uruchamianie aplikacji, wybór trybu i istniejący test renderowania.

## Własność danych

Catalog jest niezmienny podczas rozgrywki i musi żyć dłużej niż Game/runner.
Game i aktywne obiekty przechowują wskaźniki do definicji z tego Catalogu.
Nie podmieniaj Catalogu w trakcie rundy. Reload wymaga nowego Catalogu i nowego Game,
a nie modyfikacji map używanych przez istniejące obiekty.

## Co świadomie zostaje na dalsze etapy

Zewnętrzne definicje broni i pickupów, pliki graficzne/audio, składane wzory ataków
bossów, cele misji inne niż fala/boss/czas, graficzny edytor i zapis rozgrywki.
Nie utworzono pustych katalogów/fikcyjnych interfejsów udających gotowe systemy.
M1 dostarcza działający w testach punkt rozszerzania kampanii i wariantów przeciwników.
