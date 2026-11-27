# Якщо нас викликала система збирання ядра (Kbuild), KERNELRELEASE
# визначено: досить сказати, які модулі зібрати.
ifneq ($(KERNELRELEASE),)
obj-m := lab11_opencount.o lab11_oops.o
else
# Ручне збирання поза Buildroot: make KDIR=шлях/до/ядра
KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean

.PHONY: all clean
endif
