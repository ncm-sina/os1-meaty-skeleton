CC = i686-elf-gcc
OBJCP = i686-elf-objcopy
CFLAGS = -ffreestanding -m32 -I../../../sysroot/usr/include -Wall -Wextra
# LDFLAGS = -L../../../sysroot/usr/lib -nostdlib ../../../sysroot/usr/lib/crt/crt0.o ../../../sysroot/usr/lib/crt/crti.o -lc ../../../sysroot/usr/lib/crt/crtn.o
LDFLAGS = -L../../../sysroot/usr/lib -nostdlib -lc

PROGRAMS = init.elf
BIN_PROGRAMS = $(PROGRAMS:.elf=.bin)
SRCS = \
init.c \
../etc/serial.c

OBJS = $(SRCS:.c=.o)


all: $(PROGRAMS)

elf2bin:
	$(OBJCP) -O binary $< $@

$(PROGRAMS): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(BIN_PROGRAMS): %.bin: %.elf
	$(OBJCP) -O binary $< $@

.S.o:
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

# ../etc/serial.o: ../etc/serial.c
# 	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: $(PROGRAMS) $(BIN_PROGRAMS)
	mkdir ../../../sysroot/sbin -p
	cp $(PROGRAMS) ../../../sysroot/sbin/
	cp $(BIN_PROGRAMS) ../../../sysroot/sbin/

clean:
	rm -f $(OBJS) $(PROGRAMS)

.PHONY: all install clean elf2bin
