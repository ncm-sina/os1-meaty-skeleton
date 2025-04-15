CC = i686-elf-gcc
CFLAGS = -ffreestanding -m32 -I../../../sysroot/usr/include -Wall -Wextra
LDFLAGS = -L../../../sysroot/usr/lib -nostdlib ../../../sysroot/usr/lib/crt0.o ../../../sysroot/usr/lib/crti.o -lc ../../../sysroot/usr/lib/crtn.o

PROGRAMS = cp mv rm ls cat vi echo head tail
SRCS = $(PROGRAMS:=.c)
OBJS = $(SRCS:.c=.o)

all: $(PROGRAMS)

$(PROGRAMS): %: %.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: $(PROGRAMS)
	cp $(PROGRAMS) ../../../sysroot/bin/

clean:
	rm -f $(OBJS) $(PROGRAMS)

.PHONY: all install clean
