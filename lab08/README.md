# Лабораторна робота № 8

**Час, локалізація і служби: годинники ядра, часові пояси, локаль і systemd**

Повний текст роботи — у Google Classroom. Тут лише те, що потрібно для здачі.

## Що здається

```
lab08/submissions/ГРУПА_Прізвище/
├── Makefile                  ← скопіювати з lab08/starter/ без змін
├── clocks.c                  — TODO 1: ціна читання годинника (серії, медіана)
├── ticker.c                  — TODO 2: абсолютні дедлайни і пропущені періоди
├── whenis.c                  — TODO 3: усі моменти для місцевого часу (дірка і повтор)
├── loc.c                     — TODO 4a, 4b: числа для машини — у локалі «C»
├── units/
│   ├── lab08-ticker.service  ← без змін
│   ├── lab08-stamp.service   ← без змін
│   └── lab08-stamp.timer     — TODO 5: кожна парна хвилина, точність 1 с
└── REPORT.md
```

`make` має зібрати `clocks`, `ticker`, `whenis` і `loc` без жодного попередження з `-Wall -Wextra`.

Потрібен Linux з glibc (WSL 2, віртуальна машина або власна система; macOS не підходить), пакети
`build-essential python3 git strace locales tzdata` і локалі `uk_UA.UTF-8`, `de_DE.UTF-8`
(`sudo locale-gen uk_UA.UTF-8 de_DE.UTF-8`). Частина 5 потребує systemd: у Docker її не виконати.

## Перевірка й вимірювання

Команди виконуються з вашого каталогу `lab08/submissions/ГРУПА_Прізвище/`.

```bash
make
python3 ../../../tools/lab08-test.py .            # має бути 13/13

./clocks cost
./ticker abs 10 200 2
./whenis conv '2026-10-25 03:30' Europe/Kyiv     # ambiguous 2
LC_ALL=uk_UA.UTF-8 ./loc parse cfg.txt

make units                                        # служби користувача
systemctl --user enable --now lab08-stamp.timer
journalctl --user -u lab08-stamp -n 6
make uninstall-units                              # після здачі
```

## Що має бути в REPORT.md

Середовище (`uname -r`, процесор, джерело тактів, версії `gcc` і glibc) · вивід `lab08-test.py` ·
таблиці `clocks cost` і `ticker` · зведена таблиця · фрагменти журналу служби і таймера ·
відповіді на десять питань для звіту · висновки · розділ «Використання ШІ», якщо користувалися.
