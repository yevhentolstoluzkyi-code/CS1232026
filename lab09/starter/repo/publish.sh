#!/bin/sh
# Публікує .deb-пакети в «плоский» репозиторій APT і підписує його.
#
#   repo/publish.sh КАТАЛОГ_РЕПОЗИТОРІЮ ПАКЕТ.deb...
#
# Ключ підпису — перший секретний ключ у ~/.gnupg або LAB09_KEY=відбиток.
# Після публікації в каталозі лежать:
#   pool/*.deb        самі пакети;
#   Packages(.gz)     метадані всіх пакетів і SHA256 кожного .deb;
#   Release           SHA256 файлів Packages;
#   InRelease         той самий Release з вбудованим підписом;
#   lab09-repo.gpg    відкритий ключ для Signed-By у клієнта.
set -eu

[ $# -ge 2 ] || { echo "usage: $0 REPO_DIR PACKAGE.deb..." >&2; exit 2; }
repo=$1; shift
mkdir -p "$repo/pool"
cp "$@" "$repo/pool/"
cd "$repo"

# 1. Індекс пакетів: поля з DEBIAN/control кожного .deb + Filename, Size, SHA256.
apt-ftparchive packages pool > Packages
gzip -9nf -k Packages

# 2. Release: хеші індексів. Старий Release прибираємо з дороги, інакше
#    apt-ftparchive внесе в новий хеш старого.
[ -f Release ] && mv Release Release.old
apt-ftparchive \
    -o APT::FTPArchive::Release::Origin=Lab09 \
    -o APT::FTPArchive::Release::Label=Lab09 \
    -o APT::FTPArchive::Release::Suite=lab09 \
    release . > Release.new
mv Release.new Release
rm -f Release.old

# 3. Підпис: InRelease — Release разом із підписом в одному файлі.
key=${LAB09_KEY:-$(gpg --list-secret-keys --with-colons | awk -F: '$1=="fpr"{print $10; exit}')}
[ -n "$key" ] || { echo "немає секретного ключа: створіть його (розділ 7.2 ЛР)" >&2; exit 1; }
gpg --batch --yes --local-user "$key" --clearsign --output InRelease Release
gpg --batch --yes --export "$key" > lab09-repo.gpg

echo "published $(grep -c '^Package:' Packages) package(s) in $(pwd), key $key"
