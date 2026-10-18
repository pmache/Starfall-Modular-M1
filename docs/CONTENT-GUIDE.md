# Rozbudowa Starfall — format M1 / schema_version 1

## Najkrótsza droga

W paczce po buildzie: `NEW-SECTOR.cmd` → nazwa → edycja nowego JSON-a →
`VALIDATE-CONTENT.cmd` → ponowne uruchomienie `PLAY-DEMO.cmd`.
Skrypt kreatora jest dodatkiem dla Windows; **w tej sesji nie był uruchomiony**.
Równoważna operacja ręczna została przetestowana w działającym rdzeniu gry.

## Ręcznie: jeden sektor więcej

Skopiuj `content/example/sectors/relay.json` do `content/example/sectors/my_sector.json`.
Zmień `id` na `example:my_sector`, `name` na `MY SECTOR` i ułóż kroki, np.:

```json
{
  "schema_version": 1,
  "id": "example:my_sector",
  "name": "MY SECTOR",
  "steps": [
    { "type": "wave", "ref": "base:patrol", "complete_when": "enemies_cleared" },
    { "type": "wait", "seconds": 2 },
    { "type": "wave", "ref": "example:relay_guard", "complete_when": "enemies_cleared" }
  ]
}
```

W `content/example/campaigns/two_sectors.json` dopisz identyfikator do `sectors`:

```json
"sectors": ["base:outer_rim", "example:relay", "example:my_sector"]
```

To już wystarcza. Nazwa pliku jest dowolna, o ile kończy się `.json`.
Nie zmieniaj EXE ani C++. `--sector example:my_sector` uruchamia tylko nowy poziom.
Boss nie jest wymagany. Możesz też dać bossa w środku, kolejną falę albo kilku bossów
po kolei. Nie ma progu `wave == 3` ani stałego końcowego bossa.

## Paczki, identyfikatory i kampania

`content/game.json` wybiera `default_campaign` i listę folderów `packs`.
Każdy folder zawiera `pack.json` z `schema_version: 1`, `id` równym nazwie folderu i `name`.
Definicje w folderze `example` mają identyfikatory `example:...`. W nazwach identyfikatorów
używaj małych liter ASCII, cyfr, `_` i `-` (po 1–48 znaków przed i po dwukropku).
Odwołania do innych **włączonych** paczek działają normalnie. Duplikaty ID w tej samej
kategorii są błędem; nie ma cichego nadpisywania definicji zależnego od kolejności.

Kampania zawiera `schema_version`, `id`, `name`, `sectors` i `sector_transition_seconds`.
Lista sektorów musi mieć od 1 do 1024 pozycji. Ten sam sektor może wystąpić wiele razy.
Przejście między sektorami trwa 0–60 sekund. Koniec ostatniego kroku ostatniego sektora
wywołuje zwycięstwo. Sam boss zgłasza jedynie zakończenie swojego kroku.

`name` ma 1–32 znaki obsługiwane przez obecny font: A–Z, a–z, cyfry, spacje oraz
`: / - . ?`. Polskie znaki w nazwach wyświetlanych w HUD nie są jeszcze obsługiwane.
Ścieżki do folderów mogą zawierać polskie znaki; format JSON to UTF-8, także z BOM.

## Sektor i kroki

Sektor ma `schema_version`, `id`, `name` i `steps` (1–512 kroków).

| Typ | Pola | Kiedy kończy się krok |
|---|---|---|
| `wave` | `ref`, `complete_when: "enemies_cleared"` | Po zniszczeniu wszystkich przeciwników tej fali. |
| `boss` | `ref`, `complete_when: "boss_defeated"` | Po pokonaniu wskazanego bossa. |
| `wait` | `seconds` (0–600) | Po upływie czasu. |

Przerwy są jawne. Nie ma automatycznych 1,8 sekundy po każdej nowej fali;
oryginalny sektor zawiera takie przerwy w swoim JSON-ie. Pauza zatrzymuje też czas
kroku i przejścia między sektorami. Krok jest uruchamiany najwyżej raz na tick;
zerowa przerwa nie powoduje rekurencyjnej pętli.

## Fala i formacja

Najprościej skopiuj istniejącą falę. Wspólne pola: `schema_version`, `id`, `name`.

| Pole | Zakres / znaczenie |
|---|---|
| `dive_interval` | 0,1–120 sekund między wyborem kolejnego nurkującego przeciwnika. Pierwszy wybór następuje po 2 sekundach jak w oryginale. |
| `shot_speed` | 1–1000 pikseli na sekundę; niezależne od numeru fali/sektora. |
| `fire_interval_min`, `fire_interval_max` | 0,1–120 sekund między strzałami; maksimum nie mniejsze od minimum. Pierwszy strzał losowany jak w oryginale: 1,1–3,1 s. |
| `spawns` | 1–128 przeciwników. Każdy ma `enemy`, `x`, `y`, opcjonalne `carrier` (domyślnie false). |

`x`: 230–455; `y`: 35–242. To docelowa pozycja w logicznym obrazie 480×270,
nie miejsce pierwszego pojawienia. Wejście formacji z prawej strony zostało zachowane.
`carrier: true` oznacza istniejący moduł naprawy/Tridenta, nie dowolny system lootów.

## Archetyp przeciwnika

Przykład:

```json
{
  "schema_version": 1,
  "id": "example:armored_hornet",
  "name": "ARMORED HORNET",
  "sprite": "hornet",
  "hp": 60,
  "score": 350,
  "can_dive": false
}
```

Wstaw go do `example/enemies/armored_hornet.json` i użyj ID w `spawns`.
Dostępne sprite'y: `hornet`, `manta`, `bomber` — dotychczasowa grafika.
`hp`: 1–100000, `score`: 0–1000000, `can_dive`: true/false (domyślnie true).
Premia 75 punktów za zabicie podczas nurkowania pozostała mechaniką silnika.

## Boss

M1 udostępnia jedno zachowanie: `behavior: "null_crown"` i istniejącą grafikę korony.
Nową kombinację parametrów możesz dodać bez rekompilacji; nowy rodzaj zachowania
wymaga kolejnego modułu w kodzie. `example:relay_core` pokazuje wariant parametrów,
**nie** niezależną nową grafikę czy zupełnie inny typ bossa.

| Pole | Zakres |
|---|---|
| `hp` | 1–1000000 |
| `score` | 0–1000000 |
| `fire_interval` | 0,1–120 s |
| `rage_threshold` | 0–1 części maksymalnego HP; wściekłość poniżej progu |
| `rage_interval_multiplier` | 0,1–1 mnożnika odstępu między strzałami |
| `shot_speed`, `rage_shot_speed` | 1–1000 pikseli/s |
| `orbit_amplitude` | 0–70 pikseli |
| `orbit_speed` | 0–10 |

## Błędy i ograniczenia

Walidacja obejmuje wszystkie włączone paczki, również definicje nieużywane w wybranej
kampanii. Nieznane pola i zachowania, brakujące odwołania, zły typ wartości, puste fale,
ułamkowe HP i duplikaty kluczy/ID powodują błąd zamiast milczącego użycia wartości domyślnej.
JSON nie dopuszcza komentarzy ani przecinka po ostatniej pozycji.

Limity: 32 paczki, 4096 definicji, 1 MiB na plik, 16 MiB wczytywanych plików,
48 poziomów zagnieżdżenia JSON, 100000 węzłów parsera na dokument.
Dowiązania symboliczne wewnątrz paczek nie są obsługiwane. Definicje są skanowane
bez podfolderów w każdej kategorii. Pliki `.bak` i `.tmp` nie są definicjami.

W tej wersji nie ma hot reloadu. R restartuje wybraną trasę z już wczytanymi danymi.
Do odczytania zmian zamknij proces i uruchom grę ponownie.
