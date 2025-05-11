DEFAULT_HOST!=../../scripts/default-host.sh
HOST?=DEFAULT_HOST
HOSTARCH!=../../scripts/target-triplet-to-arch.sh $(HOST)

CFLAGS?=-O2 -g
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
LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS) -nostdlib -lk -lgcc

ARCHDIR=arch/$(HOSTARCH)

include $(ARCHDIR)/make.config

CFLAGS:=$(CFLAGS) $(KERNEL_ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)
LDFLAGS:=$(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS:=$(LIBS) $(KERNEL_ARCH_LIBS)

KERNEL_OBJS=\
$(KERNEL_ARCH_OBJS) \
main.o \
drivers/timer.o \
drivers/keyboard.o \
drivers/vga.o \
pages/page.o \
pages/create_partition/create_partition_page.o \
pages/delete_partition/delete_partition_page.o \
pages/format_partition/format_partition_page.o \
pages/installing/installing_page.o \
pages/setup/setup_page.o \
pages/tos/tos_page.o \
pages/welcome/welcome_page.o \
utils/pci.o \
utils/memory.o \
utils/iso9660.o \
utils/fat32.o \
utils/disk.o \
utils/ahci.o \
utils/progress.o \
utils/reboot.o \
utils/print_xx.o \


OBJS=\
$(ARCHDIR)/crti.o \
$(ARCHDIR)/crtbegin.o \
$(KERNEL_OBJS) \
$(ARCHDIR)/crtend.o \
$(ARCHDIR)/crtn.o \

LINK_LIST=\
$(LDFLAGS) \
$(ARCHDIR)/crti.o \
$(ARCHDIR)/crtbegin.o \
$(KERNEL_OBJS) \
$(LIBS) \
$(ARCHDIR)/crtend.o \
$(ARCHDIR)/crtn.o \

.PHONY: all clean install install-headers install-kernel
.SUFFIXES: .o .c .S

all: os-installer.elf

os-installer.elf: $(OBJS) $(ARCHDIR)/linker.ld
	@printf "$(CC) -T $(ARCHDIR)/linker.ld -o $@ $(CFLAGS)\n $(foreach obj,$(LINK_LIST),$(obj) \ \n) --verbose\n" 
	$(CC) -T $(ARCHDIR)/linker.ld -o $@ $(CFLAGS) $(LINK_LIST)
	#grub-file --is-x86-multiboot os-installer.elf


$(ARCHDIR)/crtbegin.o $(ARCHDIR)/crtend.o:
	OBJ="../../../../../../../i686-elf-tools-windows/lib/gcc/i686-elf/7.1.0/$(@F)" && cp "$$OBJ" $@
	
.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

.S.o:
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)


clean:
	rm -f os-installer.elf
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d


install: install-headers install-kernel

install-headers:
	echo "-=-=-=-=-= $(LINK_LIST)"
	# mkdir -p $(DESTDIR)$(INCLUDEDIR)
	# cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

install-kernel: os-installer.elf
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp os-installer.elf $(DESTDIR)$(BOOTDIR)

-include $(OBJS:.o=.d)
