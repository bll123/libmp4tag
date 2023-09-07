#!/bin/bash

for f in samples/stco.m4a samples/v-mdhd64-co64.mp4; do
  echo -n "chk: $f "
  cp $f tt.m4a
  rm -f x1 x2 x1b x2b
  ./build/mp4tagcli tt.m4a --debug 4 > x1
  ./build/mp4tagcli tt.m4a covr:1=samples/bdj4-y.png covr:1:name=xyzzy
  ./build/mp4tagcli tt.m4a covr:2=samples/bdj4-b.png covr:2:name=three
  ./build/mp4tagcli tt.m4a --debug 4 > x2
  sed -e '/^duration/,$ d' -e 's/[ 0-9]*: [0-9a-z]* //' x1 > x1b
  sed -e '/^duration/,$ d' -e 's/[ 0-9]*: [0-9a-z]* //' x2 > x2b
  diff x1b x2b > /dev/null 2>&1
  rm -f x1 x2 x1b x2b
  rc=$?
  if [[ $rc -eq 0 ]]; then
    echo " ok"
  else
    echo " fail"
  fi
done
