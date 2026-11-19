# ЛР 9: пакети. Збирає vercmp і pkgaudit; ціль deb — пакет lab09-pkgtools.
CC      ?= gcc
CFLAGS  ?= -std=c17 -O2 -g -Wall -Wextra
PROGS    = vercmp pkgaudit

PKG      = lab09-pkgtools
VERSION := $(shell dpkg-parsechangelog -l debian/changelog -S Version 2>/dev/null)
ARCH    := $(shell dpkg --print-architecture 2>/dev/null)
ROOT     = build/root
DEB      = $(PKG)_$(VERSION)_$(ARCH).deb
DOC      = $(ROOT)/usr/share/doc/$(PKG)

# Час останнього запису changelog — для відтворюваної збірки.
# make deb SOURCE_DATE_EPOCH= — зібрати без нього й порівняти.
SOURCE_DATE_EPOCH ?= $(shell dpkg-parsechangelog -l debian/changelog -S Timestamp 2>/dev/null)
ifneq ($(strip $(SOURCE_DATE_EPOCH)),)
export SOURCE_DATE_EPOCH
else
unexport SOURCE_DATE_EPOCH
endif

.PHONY: all clean install deb lint

all: $(PROGS)

vercmp: vercmp.c debver.c debver.h
	$(CC) $(CFLAGS) -o $@ vercmp.c debver.c

pkgaudit: pkgaudit.c debver.c debver.h
	$(CC) $(CFLAGS) -o $@ pkgaudit.c debver.c

# Звичайне «make install»: копіює файли в DESTDIR і нічого не запам'ятовує.
install: $(PROGS)
	install -D -m 0755 vercmp        $(DESTDIR)/usr/bin/vercmp
	install -D -m 0755 pkgaudit      $(DESTDIR)/usr/bin/pkgaudit
	install -D -m 0644 pkgaudit.conf $(DESTDIR)/etc/pkgaudit.conf

# Ті самі кроки, які в справжньому пакеті виконує debhelper.
deb: $(PROGS) debian/control debian/changelog debian/copyright debian/conffiles debian/postrm
	@test -n "$(VERSION)" || { echo "немає dpkg-parsechangelog (пакет dpkg-dev)"; exit 1; }
	rm -rf build $(DEB)
	$(MAKE) --no-print-directory install DESTDIR=$(CURDIR)/$(ROOT)
	strip --strip-unneeded --remove-section=.comment --remove-section=.note $(ROOT)/usr/bin/*
	install -D -m 0644 debian/copyright $(DOC)/copyright
	gzip -9n -c debian/changelog > $(DOC)/changelog.Debian.gz
	chmod 0644 $(DOC)/changelog.Debian.gz
	dpkg-shlibdeps -Tbuild/substvars $(ROOT)/usr/bin/vercmp $(ROOT)/usr/bin/pkgaudit
	mkdir -p $(ROOT)/DEBIAN
	dpkg-gencontrol -p$(PKG) -cdebian/control -ldebian/changelog \
		-Tbuild/substvars -fbuild/files -P$(ROOT)
	grep -v -e '^#' -e '^$$' debian/conffiles > $(ROOT)/DEBIAN/conffiles || true
	test -s $(ROOT)/DEBIAN/conffiles || rm -f $(ROOT)/DEBIAN/conffiles
	install -m 0755 debian/postrm    $(ROOT)/DEBIAN/postrm
	cd $(ROOT) && find usr -type f | LC_ALL=C sort | xargs md5sum > DEBIAN/md5sums
	dpkg-deb --root-owner-group -Zxz --build $(ROOT) $(DEB)

lint: deb
	lintian --no-tag-display-limit $(DEB)

clean:
	rm -rf $(PROGS) build *.deb
