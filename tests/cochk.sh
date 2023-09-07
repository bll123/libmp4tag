#!/bin/bash

MP4TAGCLI=./build/mp4tagcli
if [[ ! -f ${MP4TAGCLI} ]]; then
  echo "executable not found"
  exit 1
fi

for f in samples/stco.m4a samples/v-mdhd64-co64.mp4; do
  echo -n "chk: $f "
  if [[ ! -f $f ]]; then
    echo "not found"
    continue
  fi
  cp $f tt.m4a
  rm -f x1 x2 x1b x2b
  ${MP4TAGCLI} tt.m4a --debug 4 > x1
  ${MP4TAGCLI} tt.m4a covr:1=samples/bdj4-y.png covr:1:name=xyzzy
  ${MP4TAGCLI} tt.m4a covr:2=samples/bdj4-b.png covr:2:name=three
  ${MP4TAGCLI} tt.m4a --debug 4 > x2
  sed -e '/^duration/,$ d' -e 's/[ 0-9]*: [0-9a-z]* //' x1 > x1b
  sed -e '/^duration/,$ d' -e 's/[ 0-9]*: [0-9a-z]* //' x2 > x2b
  diff x1 x2 > /dev/null 2>&1
  rca=$?
  diff x1b x2b > /dev/null 2>&1
  rm -f x1 x2 x1b x2b tt.m4a
  rcb=$?
  if [[ $rca -ne 0 && $rcb -eq 0 ]]; then
    echo " ok"
  else
    echo " fail"
  fi
done
