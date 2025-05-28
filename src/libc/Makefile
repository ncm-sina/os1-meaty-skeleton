DEFAULT_HOST!=../../scripts/default-host.sh
HOST?=DEFAULT_HOST
HOSTARCH!=../../scripts/target-triplet-to-arch.sh $(HOST)

CFLAGS?=-O2 -g
CPPFLAGS?=
LDFLAGS?=
LIBS?=

DESTDIR?=
PREFIX?=/usr/local
EXEC_PREFIX?=$(PREFIX)
INCLUDEDIR?=$(PREFIX)/include
LIBDIR?=$(EXEC_PREFIX)/lib

CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -D__is_libc -Iinclude
LIBK_CFLAGS:=$(CFLAGS)
LIBK_CPPFLAGS:=$(CPPFLAGS) -D__is_libk

ARCHDIR=arch/$(HOSTARCH)

include $(ARCHDIR)/make.config

CFLAGS:=$(CFLAGS) $(ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(ARCH_CPPFLAGS)
LIBK_CFLAGS:=$(LIBK_CFLAGS) $(KERNEL_ARCH_CFLAGS)
LIBK_CPPFLAGS:=$(LIBK_CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)

FREEOBJS=\
$(ARCH_FREEOBJS) \
stdio/init.o \
stdio/fclose.o \
stdio/fopen.o \
stdio/buffer.o \
stdio/printf.o \
stdio/printf2.o \
stdio/putchar.o \
stdio/sprintf.o \
stdio/scanf.o \
stdio/puts.o \
stdlib/abort.o \
string/memcmp.o \
string/memcpy.o \
string/memmove.o \
string/memset.o \
string/strcat.o \
string/strcpy.o \
string/strlen.o \
string/strcmp.o \
string/stricmp.o \
string/strncmp.o \
string/strncpy.o \
string/strchr.o \
string/strrchr.o \
string/snprintf.o \
errno.o \
fcntl.o \
mman.o \
signal.o \
unistd.o \

HOSTEDOBJS=\
$(CRTX) \
$(ARCH_HOSTEDOBJS) \

OBJS=\
$(FREEOBJS) \
$(HOSTEDOBJS) \

LIBK_OBJS=$(FREEOBJS:.o=.libk.o)

#BINARIES=libc.a libk.a # Not ready for libc yet.
BINARIES=libk.a libc.a
CRTX=$(ARCHDIR)/crt0.o $(ARCHDIR)/crti.o $(ARCHDIR)/crtn.o $(ARCHDIR)/crtbegin.o $(ARCHDIR)/crtend.o
# CRTX=$(ARCHDIR)/crti.o $(ARCHDIR)/crtn.o $(ARCHDIR)/crtbegin.o $(ARCHDIR)/crtend.o

.PHONY: all clean install install-headers install-libs
.SUFFIXES: .o .libk.o .libc.o .c .S

all: $(BINARIES) $(CRTX)

libc.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

libk.a: $(LIBK_OBJS)
	$(AR) rcs $@ $(LIBK_OBJS)

.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

# .c.S:
# 	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

.S.o:
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)


.c.libk.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(LIBK_CFLAGS) $(LIBK_CPPFLAGS)

.S.libk.o:
	$(CC) -MD -c $< -o $@ $(LIBK_CFLAGS) $(LIBK_CPPFLAGS)

$(ARCHDIR)/crtbegin.o $(ARCHDIR)/crtend.o:
	OBJ="../../../../../../../i686-elf-tools-windows/lib/gcc/i686-elf/7.1.0/$(@F)" && cp "$$OBJ" $@

# $(ARCHDIR)/crt0.o:
# 	touch $(ARCHDIR)/crt0.o

clean:
	rm -f $(BINARIES) *.a
	rm -f $(CRTX)
	rm -f $(OBJS) $(LIBK_OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) $(LIBK_OBJS:.o=.d) *.d */*.d */*/*.d

install: install-headers install-libs

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.	

install-libs: $(BINARIES) $(CRTX)
	mkdir -p $(DESTDIR)$(LIBDIR)
	mkdir -p $(DESTDIR)$(LIBDIR)/crt
	cp $(BINARIES) $(DESTDIR)$(LIBDIR)
	cp $(CRTX) $(DESTDIR)$(LIBDIR)/crt

-include $(OBJS:.o=.d)
-include $(LIBK_OBJS:.o=.d)
