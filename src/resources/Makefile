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
SHAREDDIR?=$(EXEC_PREFIX)/share
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

.PHONY: all clean install copy-resources create-folders fonts images icons themes locale
.SUFFIXES: .o .c .S .asm

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

install: create-folders copy-resources


fonts:
	mkdir -p $(DESTDIR)$(SHAREDDIR)/fonts
	cp fonts/. $(DESTDIR)$(SHAREDDIR)/fonts -fr
images:
	mkdir -p $(DESTDIR)$(SHAREDDIR)/images
	cp images/. $(DESTDIR)$(SHAREDDIR)/images -fr
icons:
	mkdir -p $(DESTDIR)$(SHAREDDIR)/icons
	cp icons/. $(DESTDIR)$(SHAREDDIR)/icons -fr
themes:
	mkdir -p $(DESTDIR)$(SHAREDDIR)/themes
	cp themes/. $(DESTDIR)$(SHAREDDIR)/themes -fr
locale:
	mkdir -p $(DESTDIR)$(SHAREDDIR)/locale
	cp locale/. $(DESTDIR)$(SHAREDDIR)/locale -fr

create-folders:
	mkdir -p fonts
	mkdir -p images
	mkdir -p icons
	mkdir -p themes
	mkdir -p locale

copy-resources: fonts images icons themes locale

	mkdir -p $(DESTDIR)$(SHAREDDIR)

-include $(OBJS:.o=.d)
