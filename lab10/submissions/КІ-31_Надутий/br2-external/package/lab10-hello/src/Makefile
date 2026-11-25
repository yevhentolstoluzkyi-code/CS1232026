# Якщо нас викликала система збирання ядра (Kbuild), KERNELRELEASE
# визначено: досить сказати, який модуль зібрати.
ifneq ($(KERNELRELEASE),)
obj-m := lab10_hello.o
else
# Ручне збирання поза Buildroot: make KDIR=шлях/до/ядра
KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean

.PHONY: all clean
endif
