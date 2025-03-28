#!/bin/sh
set -e
. ./headers.sh

for PROJECT in $PROJECTS; do
  echo "=-----=installing files: $PROJECT"
  (cd $PROJECT && DESTDIR="$SYSROOT" $MAKE install)
done
