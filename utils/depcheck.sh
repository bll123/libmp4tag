#!/bin/bash
#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#

if [[ ! -d web || ! -d wiki ]]; then
  echo "wrong dir"
  exit 1
fi
cwd=$(pwd)

keep=F
if [[ $1 == --keep ]]; then
  keep=T
fi

systype=$(uname -s)

INCTC=dep-inctest.c
INCTO=dep-inctest.o
INCTOUT=dep-inctest.log
TIIN=dep-inc-in.txt
TISORT=dep-inc-sort.txt
TOIN=dep-obj-in.txt
TOSORT=dep-obj-sort.txt
grc=0

# check for missing copyrights
echo "## checking for missing copyright"

# this is run from the src/ directory
for fn in *.c *.h */*.sh CMakeLists.txt Makefile config.h.in; do
  case $fn in
    *tt.sh|*z.sh)
      continue
      ;;
    build/*)
      continue
      ;;
    dev/*)
      continue
      ;;
    utils/*.sh)
      # most of this can be skipped
      continue
      ;;
  esac
  grep "Copyright" $fn > /dev/null 2>&1
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "$fn : missing copyright"
    grc=$rc
  fi
done
if [[ $grc != 0 ]]; then
  exit $grc
fi

# check to make sure the include files do not have any duplicate exclusions
echo "## checking include file protections"

lc=$(grep '^#ifndef INC_' *.h config.h.in |
  sort |
  uniq -d |
  wc -l)
rc=0
if [[ $lc -gt 0 ]]; then
  rc=1
fi
if [[ $rc -ne 0 ]]; then
  grc=$rc
  exit $grc
fi
for fn in *.h config.h.in; do
  inc=$(grep '^#ifndef INC_' $fn | sed -e 's/.*INC_//' -e 's/_H/.h/' -e 's/-/_/g' -e 's/\.in//')
  tnm=$(echo $fn | sed -e 's/-/_/g' -e 's/\.in//')
  if [[ $tnm != ${inc@L} ]]; then
    echo "$fn : mismatched protection name $tnm ${inc@L}"
    grc=1
  fi
done
if [[ $grc -ne 0 ]]; then
  exit $grc
fi

# check to make sure the include files can be compiled w/o dependencies
echo "## building"
make distclean
cmake -DCMAKE_INSTALL_PREFIX=$(pwd)/x -S . -B build > cmake.log 2>&1
cmake --build build >> cmake.log 2>&1

echo "## checking include file compilation"
test -f $INCTOUT && rm -f $INCTOUT
for fn in *.h; do
  bfn=$(echo $fn | sed 's,include/,,')
  cat > $INCTC << _HERE_
#include "config.h"

#include "${bfn}"

int
main (int argc, char *argv [])
{
  return 0;
}
_HERE_
  cc -c -I build $INCTC >> $INCTOUT 2>&1
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "compile of $bfn failed"
    if [[ $rc -ne 0 ]]; then
      grc=$rc
    fi
  fi
  rm -f $INCTC $INCTO
done
rm -f $INCTC $INCTO
if [[ $grc -ne 0 ]]; then
  exit $grc
fi
rm -f $INCTOUT

# check the include file hierarchy for problems.
echo "## checking include file hierarchy"
> $TIIN
if [[ -f config.h ]]; then
  cfh=config.h
fi
if [[ -f build/config.h ]]; then
  cfh=build/config.h
fi
for fn in *.c *.h ${cfh}; do
  echo $fn $fn >> $TIIN
  grep -E '^# *include "' $fn |
      sed -e 's,^# *include ",,' \
      -e 's,".*$,,' \
      -e "s,^,$fn include/," >> $TIIN
done
tsort < $TIIN > $TISORT
rc=$?

if [[ $keep == F ]]; then
  rm -f $TIIN $TISORT > /dev/null 2>&1
fi
if [[ $rc -ne 0 ]]; then
  grc=$rc
  exit $grc
fi

# check the object file hierarchy for problems.
echo "## checking object file hierarchy"
#
tdir=.
if [[ -d build ]]; then
  tdir=build
fi
./utils/lorder $(find ${tdir} -name '*.o') > $TOIN
tsort < $TOIN > $TOSORT
rc=$?
if [[ $rc -ne 0 ]]; then
  grc=$rc
fi

if [[ $keep == F ]]; then
  rm -f $TIIN $TISORT $TOIN $TOSORT > /dev/null 2>&1
fi
rm -f $INCCT $INCTO $INCTOUT cmake.log

exit $grc
