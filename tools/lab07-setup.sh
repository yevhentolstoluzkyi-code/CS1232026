#!/bin/sh
# Підготовка системи до ЛР 7. Запускається через sudo:
#
#   sudo sh tools/lab07-setup.sh КАТАЛОГ     # каталог із зібраними notes і pwcheck
#   sudo sh tools/lab07-setup.sh --remove    # прибрати все, що створив скрипт
#
# Створює:
#   lab7notes  — службовий користувач без входу; власник notes і сховища
#   lab7user   — другий звичайний користувач, пароль lab7-Pass!
#   /var/lib/lab07/          lab7notes:lab7notes 0700 — сховище нотаток
#   /opt/lab07/notes         lab7notes 4755 — SUID, але не root
#   /opt/lab07/pwcheck       root 0755
# Скрипт можна запускати повторно: він лише оновлює програми.
set -eu

if [ "$(id -u)" != 0 ]; then
    echo "запустіть через sudo" >&2
    exit 1
fi

if [ "${1:-}" = "--remove" ]; then
    rm -rf /opt/lab07 /var/lib/lab07
    for u in lab7user lab7notes; do
        if id "$u" >/dev/null 2>&1; then userdel -r "$u" 2>/dev/null || userdel "$u"; fi
    done
    echo "lab07: прибрано"
    exit 0
fi

DIR=${1:?вкажіть каталог із зібраними notes і pwcheck}
for p in notes pwcheck; do
    [ -x "$DIR/$p" ] || { echo "немає $DIR/$p — спершу make" >&2; exit 1; }
done

id lab7notes >/dev/null 2>&1 || \
    useradd --system --no-create-home --home-dir /nonexistent \
            --shell /usr/sbin/nologin lab7notes
id lab7user >/dev/null 2>&1 || useradd --create-home --shell /bin/bash lab7user
echo 'lab7user:lab7-Pass!' | chpasswd

install -d -o lab7notes -g lab7notes -m 0700 /var/lib/lab07
install -d -o root -g root -m 0755 /opt/lab07
# Порядок має значення: chown скидає біт SUID, тому права ставимо після власника.
install -o lab7notes -g lab7notes -m 0755 "$DIR/notes" /opt/lab07/notes
chmod 4755 /opt/lab07/notes
install -o root -g root -m 0755 "$DIR/pwcheck" /opt/lab07/pwcheck

ls -l /opt/lab07
ls -ld /var/lib/lab07
