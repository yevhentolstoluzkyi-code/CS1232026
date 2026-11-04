# Лабораторна робота № 9

**Пакети: версії, аудит, власний .deb і підписаний репозиторій**

Повний текст роботи — у Google Classroom. Тут лише те, що потрібно для здачі.

## Що здається

```
lab09/submissions/ГРУПА_Прізвище/
├── Makefile                  ← скопіювати з lab09/starter/ без змін
├── debver.h, vercmp.c        ← без змін
├── debver.c                  — TODO 1: розбір версії; TODO 2: порівняння, як у dpkg
├── pkgaudit.c                — TODO 3: поля залежностей; TODO 4: листки з урахуванням Provides
├── pkgaudit.conf             — рядок keep з частини 4
├── debian/
│   ├── control               — TODO 5a: двійковий пакет
│   ├── conffiles             — TODO 5b
│   ├── postrm                — TODO 5c: purge, ідемпотентність
│   ├── changelog             — ваше ім'я; записи 1.0-1 і 1.1-1
│   └── copyright             — ваше ім'я
├── repo/                     ← без змін
├── rpm/lab09-pkgtools.spec   — ваше ім'я
└── REPORT.md
```

`make` має зібрати `vercmp` і `pkgaudit` без жодного попередження з `-Wall -Wextra`.
Бінарники, `build/`, `*.deb`, `*.rpm`, каталог репозиторію і ключі GnuPG у коміт не потрапляють.

Потрібна Ubuntu або Debian (WSL 2, віртуальна машина, контейнер `ubuntu:24.04`; macOS не підходить)
і пакети `build-essential dpkg-dev lintian apt-utils gnupg python3 git rpm`.
Частина 4 встановлює пакети від root: виконуйте її в контейнері або віртуальній машині.

## Перевірка й вимірювання

Команди виконуються з вашого каталогу `lab09/submissions/ГРУПА_Прізвище/`.

```bash
make
python3 ../../../tools/lab09-test.py .           # має бути 14/14

./vercmp show 1:2.43.0-1ubuntu7
./pkgaudit stats; ./pkgaudit leaves | tail -2
make deb && make lint
dpkg-deb -I lab09-pkgtools_1.0-1_*.deb

# частина 4 (контейнер або ВМ)
repo/publish.sh /srv/lab09-repo lab09-pkgtools_1.0-1_*.deb
sudo cp repo/lab09.sources /etc/apt/sources.list.d/
sudo apt update && sudo apt install lab09-pkgtools
pkgaudit diff
```

## Що має бути в REPORT.md

Середовище (система, ядро, версії `gcc`, `dpkg`, `apt`; де виконано частину 4) · вивід `lab09-test.py` ·
таблиці листків і відтворюваності · `dpkg-deb -I` і `lintian` · фрагменти частини 4 (встановлення,
оновлення з `.dpkg-dist`, `pkgaudit diff`, три підробки, `remove`/`purge`) · `rpm -qp --requires` ·
відповіді на десять питань для звіту · висновки · розділ «Використання ШІ», якщо користувалися.
