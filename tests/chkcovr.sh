#!/bin/bash

systype=$(uname -s)
sopt=-c
sfmt=%s
case ${systype} in
  Darwin)
    sopt=-f
    sfmt=%z
    ;;
esac

TFN=xx.m4a

MP4TAGCLI=./build/mp4tagcli
PICA=samples/bdj4-b.png
PICALEN=$(stat ${sopt} "${sfmt}" ${PICA})
PICB=samples/bdj4-y.png
PICBLEN=$(stat ${sopt} "${sfmt}" ${PICB})
PICC=samples/bdj4-b.jpg
PICCLEN=$(stat ${sopt} "${sfmt}" ${PICC})
PICD=samples/bdj4-y.jpg
PICDLEN=$(stat ${sopt} "${sfmt}" ${PICD})

if [[ ! -f ${MP4TAGCLI} ]]; then
  echo "executable not found"
  exit 1
fi

rm -f ${TFN}
FAILLIST=""

# no-tags.m4a has no udta box, tag space is unlimited
# alac.m4a tags are not at the end
for f in samples/no-tags.m4a samples/alac.m4a; do
  echo -n "chk: $f "
  if [[ ! -f $f ]]; then
    echo "not found"
    continue
  fi

  cp $f ${TFN}

  # cover images

  grc=0
  FAILLIST=""

  # this should end up at position 0
  ${MP4TAGCLI} ${TFN} covr:1=${PICD}
  # this will fail as there is no cover at position 1
  ${MP4TAGCLI} ${TFN} covr:1:name=plugh > /dev/null 2>&1
  rc=$?
  if [[ $rc -eq 0 ]]; then
    FAILLIST+="a-name "
    grc=1
  fi
  val=$(${MP4TAGCLI} ${TFN} | grep '^covr' | wc -l)
  if [[ $val -ne 1 ]]; then
    FAILLIST+="a-count "
    grc=1
  fi
  ${MP4TAGCLI} ${TFN} | grep "^covr=.data: jpg ${PICDLEN} bytes.$" > /dev/null 2>&1
  rc=$?
  if [[ $rc -ne 0 ]]; then
    FAILLIST+="a-data "
    grc=1
  fi

  # will replace position 0
  ${MP4TAGCLI} ${TFN} covr=${PICA}
  ${MP4TAGCLI} ${TFN} covr:0:name=xyzzy
  val=$(${MP4TAGCLI} ${TFN} | grep '^covr' | wc -l)
  if [[ $val -ne 2 ]]; then
    FAILLIST+="b-count "
    grc=1
  fi
  ${MP4TAGCLI} ${TFN} | grep "^covr=.data: png ${PICALEN} bytes.$" > /dev/null 2>&1
  rc=$?
  if [[ $rc -ne 0 ]]; then
    FAILLIST+="b-data "
    grc=1
  fi

  # add at position 1
  ${MP4TAGCLI} ${TFN} covr:1=${PICD}
  ${MP4TAGCLI} ${TFN} covr:1:name=plugh
  val=$(${MP4TAGCLI} ${TFN} | grep '^covr' | wc -l)
  if [[ $val -ne 4 ]]; then
    FAILLIST+="c-count "
    grc=1
  fi
  ${MP4TAGCLI} ${TFN} | grep "^covr:1=.data: jpg ${PICDLEN} bytes.$" > /dev/null 2>&1
  rc=$?
  if [[ $rc -ne 0 ]]; then
    FAILLIST+="c-data "
    grc=1
  fi

  # add at position 2
  ${MP4TAGCLI} ${TFN} covr:2=${PICC} covr:2:name=three
  val=$(${MP4TAGCLI} ${TFN} | grep '^covr' | wc -l)
  if [[ $val -ne 6 ]]; then
    FAILLIST+="d-count "
    grc=1
  fi
  ${MP4TAGCLI} ${TFN} | grep "^covr:2=.data: jpg ${PICCLEN} bytes.$" > /dev/null 2>&1
  rc=$?
  if [[ $rc -ne 0 ]]; then
    FAILLIST+="d-data "
    grc=1
  fi

  # replace cover 1 with an alternate
  # this should be in-place
  ${MP4TAGCLI} ${TFN} covr:1=${PICB}
  ${MP4TAGCLI} ${TFN} covr:1:name=alt
  val=$(${MP4TAGCLI} ${TFN} | grep '^covr' | wc -l)
  if [[ $val -ne 6 ]]; then
    FAILLIST+="e-count "
    grc=1
  fi
  ${MP4TAGCLI} ${TFN} | grep "^covr:1=.data: png ${PICBLEN} bytes.$" > /dev/null 2>&1
  rc=$?
  if [[ $rc -ne 0 ]]; then
    FAILLIST+="e-data "
    grc=1
  fi

  # and then change it back to what it was
  # this should be in-place
  ${MP4TAGCLI} ${TFN} covr:1=${PICD}
  ${MP4TAGCLI} ${TFN} covr:1:name=plugh
  val=$(${MP4TAGCLI} ${TFN} | grep '^covr' | wc -l)
  if [[ $val -ne 6 ]]; then
    grc=1
  fi
  ${MP4TAGCLI} ${TFN} | grep "^covr:1=.data: jpg ${PICDLEN} bytes.$" > /dev/null 2>&1
  rc=$?
  if [[ $rc -ne 0 ]]; then
    FAILLIST+="f-data "
    grc=1
  fi

  if [[ $grc -eq 0 ]]; then
    echo "ok"
  else
    echo "fail"
    echo $FAILLIST
  fi
  rm -f ${TFN}
done
