CC = i686-elf-gcc
CFLAGS = -ffreestanding -m32 -I../../../sysroot/usr/include -Wall -Wextra
LDFLAGS = -L../../../sysroot/usr/lib -nostdlib ../../../sysroot/usr/lib/crt0.o ../../../sysroot/usr/lib/crti.o -lc ../../../sysroot/usr/lib/crtn.o

SRCS = shell.c
OBJS = $(SRCS:.c=.o)
TARGET = shell

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: $(TARGET)
	cp $(TARGET) ../../../sysroot/bin/

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all install clean
