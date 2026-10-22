# Лабораторна робота № 7

**Користувачі, права, паролі і SUID: процес з погляду безпеки**

Повний текст роботи — у Google Classroom. Тут лише те, що потрібно для здачі.

## Що здається

```
lab07/submissions/ГРУПА_Прізвище/
├── Makefile        ← скопіювати з lab07/starter/ без змін
├── idinfo.c        — TODO 1: усі групи користувача через getgrouplist
├── permcheck.c     — TODO 2: рішення ядра про доступ (один клас, правила root)
├── pwcheck.c       — TODO 3: перевірка пароля crypt_rn; TODO 4: функція розмови PAM
├── notes.c         — TODO 5: скинути привілеї назавжди і перевірити це
└── REPORT.md
```

`make` має зібрати `idinfo`, `permcheck`, `pwcheck` і `notes` без жодного попередження з `-Wall -Wextra`.

## Специфікація notes

```
notes id             → uid R E S / gid R E S    (посвідчення після старту)
notes add ТЕКСТ…     → OK                       (від імені справжнього UID)
notes list           → свої нотатки; останній рядок notes N
notes export ФАЙЛ    → exported N               (файл відкривається з вашими правами)
помилка              → повідомлення в stderr, код 2
```

Потрібен Linux з glibc, libxcrypt і Linux-PAM (WSL 2 у домашньому каталозі Linux, віртуальна
машина або Docker з `ubuntu:24.04` від звичайного користувача з `sudo`; macOS не підходить),
пакети `build-essential libcrypt-dev libpam0g-dev python3 acl libcap2-bin whois`.

## Перевірка й вимірювання

Команди виконуються з вашого каталогу `lab07/submissions/ГРУПА_Прізвище/` від звичайного користувача.

```bash
make
make install                                     # sudo: користувачі lab7notes, lab7user і /opt/lab07
sudo -v
python3 ../../../tools/lab07-test.py .           # має бути 10/10

./idinfo $USER
./permcheck scan /etc
./pwcheck bench
/opt/lab07/notes id                              # uid R R R

sudo sh ../../../tools/lab07-setup.sh --remove   # після того як PR влито
```

Робота, у якій `notes` не проходить `t_notes_drop`, не зараховується за частину 4.

## Що має бути в REPORT.md

Середовище (`uname -r`, процесор, версії `gcc`, glibc і libxcrypt) · вивід `lab07-test.py` ·
таблиці `permcheck scan` і `pwcheck bench` · зведена таблиця · відповіді на десять питань для звіту · висновки.
