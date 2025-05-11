# SYSTEM_HEADER_PROJECTS="../src/libc ../src/os-installer"
# PROJECTS="../src/libc ../src/os-installer/  ../src/modules/ ../src/resources/"
SYSTEM_HEADER_PROJECTS="../src/libc ../src/os-installer ../src/kernel"
PROJECTS="../src/libc ../src/os-installer ../src/kernel ../src/modules/ ../src/resources/"
# PROJECTS="../src/libc ../src/kernel ../src/user"

export MAKE=${MAKE:-make}
export HOST=${HOST:-$(./default-host.sh)}

export AR=${HOST}-ar
export AS=${HOST}-as
export CC=${HOST}-gcc

export PREFIX=/usr
export EXEC_PREFIX=$PREFIX
export BOOTDIR=/boot
export LIBDIR=$EXEC_PREFIX/lib
export INCLUDEDIR=$PREFIX/include

export CFLAGS='-O2 -g '
# export CFLAGS='-O2 -g -fdiagnostics-color=always'
export CPPFLAGS=''

# Configure the cross-compiler to use the desired system root.
# export SYSROOT="$(pwd)/../sysroot"
export SYSROOT="../../sysroot"
echo $SYSROOT
export CC="$CC --sysroot=$SYSROOT"

# Work around that the -elf gcc targets doesn't have a system include directory
# because it was configured with --without-headers rather than --with-sysroot.
if echo "$HOST" | grep -Eq -- '-elf($|-)'; then
  export CC="$CC -isystem=$INCLUDEDIR"
fi
