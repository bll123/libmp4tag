#!/bin/bash
#
# Copyright 2024-2025 Brad Lanam Pleasant Hill CA
#

MP4TAGCLI=./build/mp4tagcli
TFN=xx.m4a
OUTA=x1
OUTB=x2
RESA=xr1
RESB=xr2

if [[ ! -f ${MP4TAGCLI} ]]; then
  echo "executable not found"
  exit 1
fi

DBGLVL=32
while test $# -gt 0; do
  case $1 in
    --debug)
      shift
      DBGLVL=$1
      shift
      ;;
    *)
      echo "unknown option: $1"
      exit 1
      ;;
  esac
done

flist="samples/stco.m4a samples/v-mdhd64-co64.mp4 samples/has-tags.m4a"

rm -f ${OUTA} ${OUTB} ${RESA} ${RESB} ${TFN}
for f in $flist; do
  echo -n "chk: $f "
  if [[ ! -f $f ]]; then
    echo "not found"
    continue
  fi
  cp $f ${TFN}
  chmod u+w ${TFN}
  ${MP4TAGCLI} ${TFN} --debug ${DBGLVL} > ${OUTA}
  ${MP4TAGCLI} --freespace 64 --debug ${DBGLVL} ${TFN} covr:0=samples/bdj4-y.png covr:0:name=xyzzy
  ${MP4TAGCLI} --freespace 64 --debug ${DBGLVL} ${TFN} covr:1=samples/bdj4-y.jpg covr:1:name=plugh
  ${MP4TAGCLI} --freespace 64 --debug ${DBGLVL} ${TFN} covr:2=samples/bdj4-b.jpg covr:2:name=three
  ${MP4TAGCLI} ${TFN} --debug 4 > ${OUTB}
  sed -e '/^duration/,$ d' -e 's/[ 0-9]*: [0-9a-z]* //' ${OUTA} > ${RESA}
  sed -e '/^duration/,$ d' -e 's/[ 0-9]*: [0-9a-z]* //' ${OUTB} > ${RESB}
  diff ${OUTA} ${OUTB} > /dev/null 2>&1
  rca=$?
  diff ${RESA} ${RESB} > /dev/null 2>&1
  rm -f ${OUTA} ${OUTB} ${RESA} ${RESB} ${TFN}
  rcb=$?
  if [[ $rca -ne 0 && $rcb -eq 0 ]]; then
    echo " ok"
  else
    echo " fail"
  fi
done
