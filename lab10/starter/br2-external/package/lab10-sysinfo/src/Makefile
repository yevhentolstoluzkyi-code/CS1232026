# Buildroot передає CC, CFLAGS і LDFLAGS у командному рядку make
# ($(TARGET_CONFIGURE_OPTS)), а такі змінні перекривають присвоєння
# в цьому файлі, навіть «+=». Тому власні прапорці — в окремій змінній.
CC ?= cc
CFLAGS ?= -O2
LAB_CFLAGS = -std=c17 -Wall -Wextra
PREFIX ?= /usr

all: lab10-sysinfo

lab10-sysinfo: lab10-sysinfo.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LAB_CFLAGS) $(LDFLAGS) -o $@ $<

install: lab10-sysinfo
	install -D -m 0755 lab10-sysinfo $(DESTDIR)$(PREFIX)/bin/lab10-sysinfo

clean:
	rm -f lab10-sysinfo

.PHONY: all install clean
