# Скопіюйте цей файл разом з іншими файлами starter/ у свій каталог
# lab04/submissions/ГРУПА_Прізвище/ і не змінюйте без потреби.
CC      ?= cc
OPT     ?= -O2
WARN     = -Wall -Wextra
CFLAGS   = $(OPT) -g $(WARN) -std=c11 -D_POSIX_C_SOURCE=200809L
SANFLAGS = -O1 -g $(WARN) -std=c11 -D_POSIX_C_SOURCE=200809L \
           -fsanitize=address,undefined -fno-omit-frame-pointer

COMMON = main.c scan.c wordfreq.h
LEVELS = wf-O0 wf-O1 wf-O2 wf-O3 wf-Os

# wordfreq        — ваша хеш-таблиця (частина 4)
# wordfreq-naive  — готова таблиця з лінійним пошуком
all: wordfreq wordfreq-naive

wordfreq: $(COMMON) table_hash.c
	$(CC) $(CFLAGS) -o $@ main.c scan.c table_hash.c

wordfreq-naive: $(COMMON) table_naive.c
	$(CC) $(CFLAGS) -o $@ main.c scan.c table_naive.c

# Збірки з санітайзерами (частина 3). Valgrind запускайте на звичайних збірках.
asan: wordfreq-asan wordfreq-naive-asan

wordfreq-asan: $(COMMON) table_hash.c
	$(CC) $(SANFLAGS) -o $@ main.c scan.c table_hash.c

wordfreq-naive-asan: $(COMMON) table_naive.c
	$(CC) $(SANFLAGS) -o $@ main.c scan.c table_naive.c

# Одна й та сама програма на п'яти рівнях оптимізації (частина 2).
levels: $(LEVELS)

wf-%: $(COMMON) table_naive.c
	$(CC) -$* -g $(WARN) -std=c11 -D_POSIX_C_SOURCE=200809L -o $@ main.c scan.c table_naive.c

# Навчальна програма з помилками (частина 3).
buggy: buggy.c
	$(CC) -O0 -g $(WARN) -o $@ $<

buggy-asan: buggy.c
	$(CC) -O0 -g -fsanitize=address -fno-omit-frame-pointer -o $@ $<

buggy-ubsan: buggy.c
	$(CC) -O0 -g -fsanitize=undefined -o $@ $<

# «Перевірка» переповнення двома компіляторами на двох рівнях (частина 2).
overflow: overflow-gcc-O0 overflow-gcc-O2 overflow-clang-O0 overflow-clang-O2

overflow-gcc-%: overflow.c
	gcc -$* -o $@ $<

overflow-clang-%: overflow.c
	clang -$* -o $@ $<

clean:
	rm -f wordfreq wordfreq-naive wordfreq-asan wordfreq-naive-asan wordfreq-O0 \
	      $(LEVELS) buggy buggy-O2 buggy-asan buggy-ubsan overflow-* \
	      broken linked linked-static *.o *.i *.s \
	      perf.data perf.data.old perf.corpus-* callgrind.out.* cg.naive

.PHONY: all asan levels overflow clean
