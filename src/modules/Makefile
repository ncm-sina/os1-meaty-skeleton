DEFAULT_HOST!=../../scripts/default-host.sh
HOST?=DEFAULT_HOST
HOSTARCH!=../../scripts/target-triplet-to-arch.sh $(HOST)

NASM = nasm
NASMFLAGS = -f elf32
NASMFLAGS2 = -f bin

ASFLAGS = --32

CFLAGS=-O0 -g -ffreestanding
CFLAGS2=-O0 -g  -ffreestanding
CPPFLAGS?=
LDFLAGS?=
LIBS?=

OBJ=""

DESTDIR?=
PREFIX?=/usr/local
EXEC_PREFIX?=$(PREFIX)
BOOTDIR?=$(EXEC_PREFIX)/boot
INCLUDEDIR?=$(PREFIX)/include

CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -D__is_kernel -Iinclude
LDFLAGS:=$(LDFLAGS) -nostdlib

ARCHDIR=arch/$(HOSTARCH)

# include $(ARCHDIR)/make.config

CFLAGS:=$(CFLAGS) $(KERNEL_ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)
LDFLAGS:=$(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS:=$(LIBS) $(KERNEL_ARCH_LIBS)


OBJS=\
16bit-executer/mod-switcher.o \


LINK_LIST=\
$(LDFLAGS) \
$(OBJS) \

.PHONY: all clean install install-headers install-kernel
.SUFFIXES: .o .c .S

all: 16bit-executer/16bit-executer.mod

16bit-executer/16bit-executer.mod: $(OBJS) 16bit-executer/$(ARCHDIR)/linker.ld
	@printf "$(CC) -T 16bit-executer/$(ARCHDIR)/linker.ld -o $@ $(CFLAGS)\n $(foreach obj,$(LINK_LIST),$(obj) \ \n) --verbose\n" 
	$(CC) -T 16bit-executer/$(ARCHDIR)/linker.ld -o $@ $(CFLAGS) $(LINK_LIST)
	#grub-file --is-x86-multiboot myos.kernel


# 16bit-executer/vbe-run.mod: 16bit-executer/vbe-run.o
# 	ld -Ttext=0x2000 --oformat binary -o $@ $<

	
.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)


16bit-executer/mod-switcher.o: 16bit-executer/mod-switcher.asm
	$(NASM) $(NASMFLAGS) $< -o $@

16bit-executer/vbe-run.bin: 16bit-executer/vbe-run.asm
	$(NASM) $(NASMFLAGS2) $< -o $@

clean:
	rm -f 16bit-executer/16bit-executer.mod
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d

install: install-mods

install-mods: 16bit-executer/16bit-executer.mod 16bit-executer/vbe-run.bin
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp $^ $(DESTDIR)$(BOOTDIR)

-include $(OBJS:.o=.d)
