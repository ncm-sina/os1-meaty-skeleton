#!/bin/sh
set -e
. ./headers.sh

# make ${@} 2>&1 | perl -wln -M'Term::ANSIColor' -e '
# m/Building|gcc|g++|\bCC\b|\bcc\b/ and print "\e[1;32m", "$_", "\e[0m"
# or
# m/Error/i and print "\e[1;91m", "$_", "\e[0m"
# or
# m/Warning/i and print "\e[1;93m", "$_", "\e[0m"
# or
# m/Linking|\.a\b/ and print "\e[1;36m", "$_", "\e[0m"
# or
# print; '

# for PROJECT in $PROJECTS; do
#   (cd $PROJECT && DESTDIR="$SYSROOT" $MAKE install 2>&1 | perl -wln -M'Term::ANSIColor' -e '
# m/Building|gcc|g++|\bCC\b|\bcc\b/ and print "\e[1;32m", "$_", "\e[0m"
# or
# m/Error/i and print "\e[1;91m", "$_", "\e[0m"
# or
# m/Warning/i and print "\e[1;93m", "$_", "\e[0m"
# or
# m/Linking|\.a\b/ and print "\e[1;36m", "$_", "\e[0m"
# or
# print; ')
# done


for PROJECT in $PROJECTS; do
  (cd $PROJECT && DESTDIR="$SYSROOT" $MAKE install )
done

mkdir -p ../sysroot
mkdir -p ../sysroot/boot
mkdir -p ../sysroot/boot/grub

cp ../config/grub/. ../sysroot/boot/grub/. -fr

