# Програми ЛР 11 для простору користувача і програми eBPF.
#
# Buildroot передає CC, CFLAGS, LDFLAGS (крос-компілятор цілі), а також
# BPF_CLANG, BPF_CFLAGS і LIBBPF_CFLAGS (див. lab11-bpf.mk). Вручну на своїй машині:
#   make BPF_CFLAGS="-I/usr/include/$(uname -m)-linux-gnu"
# (потрібні clang, libbpf-dev, libelf-dev, zlib1g-dev).

CC ?= cc
BPF_CLANG ?= clang
BPF_CFLAGS ?=
LIBBPF_CFLAGS ?=
LIBBPF_LIBS ?= -lbpf -lelf -lz

# Власні прапорці — в окремій змінній: CFLAGS з командного рядка make
# перекриває навіть CFLAGS += у Makefile (див. ЛР 10).
LAB_CFLAGS = -Wall -Wextra

# TODO 8: додайте сюди verifier/fixed_null.bpf.o
BPF_OBJS = opencount.bpf.o \
	verifier/bad_null.bpf.o verifier/bad_loop.bpf.o verifier/bad_userptr.bpf.o

all: opencount openbench $(BPF_OBJS)

opencount: opencount.c opencount.h
	$(CC) $(CFLAGS) $(LAB_CFLAGS) $(LIBBPF_CFLAGS) $(LDFLAGS) $< -o $@ $(LIBBPF_LIBS)

openbench: openbench.c
	$(CC) $(CFLAGS) $(LAB_CFLAGS) $(LDFLAGS) $< -o $@

# Байт-код BPF: -target bpf — «процесор» віртуальної машини BPF;
# -g — додає BTF, з якого libbpf дізнається типи ключів і значень мап;
# -O2 обов'язковий: без оптимізації верифікатор відкидає навіть правильний код.
%.bpf.o: %.bpf.c opencount.h
	$(BPF_CLANG) -O2 -g -target bpf -Wall $(BPF_CFLAGS) -I. -c $< -o $@

clean:
	rm -f opencount openbench $(BPF_OBJS)

.PHONY: all clean
