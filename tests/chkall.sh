#!/bin/bash

systype=$(uname -s)
sopt=--format
sfmt=%s
case ${systype} in
  Darwin)
    sopt=-f
    sfmt=%z
    ;;
esac

DBGLVL=0
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

CS=©
TEXPA=test-exp-tmp.txt
TEXPS=test-exp-sort.txt
TACT=test-actual.txt
TFN=test-tmp.m4a

PICA=samples/bdj4-b.png
PICALEN=$(stat ${sopt} "${sfmt}" ${PICA})
PICB=samples/bdj4-y.png
PICBLEN=$(stat ${sopt} "${sfmt}" ${PICB})
PICC=samples/bdj4-b.jpg
PICCLEN=$(stat ${sopt} "${sfmt}" ${PICC})
PICD=samples/bdj4-y.jpg
PICDLEN=$(stat ${sopt} "${sfmt}" ${PICD})

MP4TAGCLI=./build/mp4tagcli
if [[ ! -f ${MP4TAGCLI} ]]; then
  echo "executable not found"
  exit 1
fi

#flist="samples/no-tags.m4a samples/alac.m4a test-files/array-keys.m4a"
flist="samples/alac.m4a"
rm -f ${TEXPA} ${TEXPS} ${TACT} ${TFN}
# no-tags.m4a has no udta box, tag space is unlimited
# alac.m4a tags are not at the end
# array-keys.m4a has multiple free boxes.
for f in $flist; do
  grc=0
  echo -n "chk: $f "
  if [[ ! -f $f ]]; then
    echo "not found"
    continue
  fi

  cp $f ${TFN}
  chmod u+w ${TFN}

  # string tags
  for tag in aART catg cprt desc keyw ldes ownr purd purl soaa \
      soal soar soco sonm sosn tven tvnn tvsh ART alb cmt \
      day dir gen grp lyr mvn \
      nam nrt pub too wrk wrt \
      ; do
    ${MP4TAGCLI} --debug ${DBGLVL} ${TFN} ${tag}=${tag}123456
    rc=$?
    if [[ $rc -ne 0 ]]; then
      echo "fail a"
      exit 1
    fi
    # don't save these in the final output file
    # as they are over-written later
  done

  for idx in 1 2 3; do
    ${MP4TAGCLI} --debug ${DBGLVL} ${TFN} wrt:${idx}=wrt-${idx}
    rc=$?
    if [[ $rc -ne 0 ]]; then
      echo "fail a2"
      exit 1
    fi
    echo wrt:${idx}=wrt-${idx} >> ${TEXPA}
  done

  # custom tags
  for tag in ----:TEST:A ----:TEST:BBB \
      ; do
    ${MP4TAGCLI} --debug ${DBGLVL} ${TFN} -- ${tag}=CCC123456
    rc=$?
    if [[ $rc -ne 0 ]]; then
      echo "fail b"
      exit 1
    fi
    # don't save these in the final output file
  done

  # numeric tags
  for tag in cpil hdvd pgap shwm tmpo tves tvsn mvc mvi \
      ; do
    ${MP4TAGCLI} --debug ${DBGLVL} ${TFN} ${tag}=1
    rc=$?
    if [[ $rc -ne 0 ]]; then
      echo "fail c"
      exit 1
    fi
    echo ${tag}=1 >> ${TEXPA}
  done

  # disk/trkn tags
  for tag in disk trkn \
      ; do
    ${MP4TAGCLI} --debug ${DBGLVL} ${TFN} ${tag}=1/5
    rc=$?
    if [[ $rc -ne 0 ]]; then
      echo "fail d"
      exit 1
    fi
    echo ${tag}=1/5 >> ${TEXPA}
  done

  # cover images

  ${MP4TAGCLI} --debug ${DBGLVL} ${TFN} covr=${PICA}
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "fail e"
    exit 1
  fi
  echo "covr=(data: png ${PICALEN} bytes)" >> ${TEXPA}
  ${MP4TAGCLI} --debug ${DBGLVL} ${TFN} covr:0:name=xyzzy
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "fail f"
    exit 1
  fi
  echo "covr:0:name=xyzzy" >> ${TEXPA}

  ${MP4TAGCLI} --debug ${DBGLVL} ${TFN} covr:1=${PICD}
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "fail g"
    exit 1
  fi
  echo "covr:1=(data: jpg ${PICDLEN} bytes)" >> ${TEXPA}
  ${MP4TAGCLI} --debug ${DBGLVL} ${TFN} covr:1:name=plugh
  echo "covr:1:name=plugh" >> ${TEXPA}

  ${MP4TAGCLI} --debug ${DBGLVL} ${TFN} covr:2=${PICC} covr:2:name=three
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "fail h"
    exit 1
  fi
  echo "covr:2=(data: jpg ${PICCLEN} bytes)" >> ${TEXPA}
  echo "covr:2:name=three" >> ${TEXPA}

  # replace cover 1 with an alternate
  # this should be in-place
  ${MP4TAGCLI} --debug ${DBGLVL} ${TFN} covr:1=${PICB}
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "fail i"
    exit 1
  fi
  ${MP4TAGCLI} --debug ${DBGLVL} ${TFN} covr:1:name=alt
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "fail j"
    exit 1
  fi

  # and then change it back to what it was
  # this should be in-place
  ${MP4TAGCLI} --debug ${DBGLVL} ${TFN} covr:1=${PICD}
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "fail k"
    exit 1
  fi
  ${MP4TAGCLI} --debug ${DBGLVL} ${TFN} covr:1:name=plugh
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "fail l"
    exit 1
  fi

  # string tags
  # these replacements should all be in-place,
  # as longer names were used above.
  for tag in aART catg cprt desc keyw ldes ownr purd purl soaa \
      soal soar soco sonm sosn tven tvnn tvsh ART alb cmt \
      day dir gen grp lyr mvn \
      nam nrt pub too wrk wrt \
      ; do
    ${MP4TAGCLI} --debug ${DBGLVL} ${TFN} ${tag}=${tag}
    rc=$?
    if [[ $rc -ne 0 ]]; then
      echo "fail m"
      exit 1
    fi
    echo ${tag}=${tag} >> ${TEXPA}
  done

  # custom tags
  # these replacements should all be in-place,
  # as longer names were used above
  for tag in ----:TEST:A ----:TEST:BBB \
      ; do
    ${MP4TAGCLI} --debug ${DBGLVL} ${TFN} -- ${tag}=CCC
    rc=$?
    if [[ $rc -ne 0 ]]; then
      echo "fail n"
      exit 1
    fi
    echo ${tag}=CCC >> ${TEXPA}
  done

  # any existing custom tags are not present in
  # the expected results file
  ${MP4TAGCLI} ${TFN} |
      grep -E -v -- '(duration|----:com)' |
      sed 's,©,,g' |
      LANG=C sort > ${TACT}

  LANG=C sort < ${TEXPA} > ${TEXPS}
  diff ${TEXPS} ${TACT} > /dev/null 2>&1
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo -n "diff-fail "
diff ${TEXPS} ${TACT}
    grc=1
  else
    echo -n "diff-ok "
  fi

  # string tags
  for tag in aART catg cprt desc keyw ldes ownr purd purl soaa \
      soal soar soco sonm sosn tven tvnn tvsh ART alb cmt \
      day dir gen grp lyr mvn \
      nam nrt pub too wrk wrt \
      ; do
    ${MP4TAGCLI} ${TFN} ${tag}=
  done

  # custom tags
  for tag in ----:TEST:A ----:TEST:BBB \
      ; do
    ${MP4TAGCLI} ${TFN} -- ${tag}=
  done

  # numeric tags
  for tag in cpil hdvd pgap shwm tmpo tves tvsn mvc mvi \
      ; do
    ${MP4TAGCLI} ${TFN} ${tag}=
  done

  # disk/trkn tags
  for tag in disk trkn; do
    ${MP4TAGCLI} ${TFN} ${tag}=
  done

  # delete in reverse order, as the dataidx is relative
  for idx in 3 2 1; do
    ${MP4TAGCLI} ${TFN} wrt:${idx}=
  done

  ${MP4TAGCLI} ${TFN} covr:2=
  ${MP4TAGCLI} ${TFN} covr:1=
  ${MP4TAGCLI} ${TFN} covr=

  val=$(${MP4TAGCLI} ${TFN} |
      grep -E -v -- '(duration|----:com)' | wc -l)
  if [[ $val -ne 0 ]]; then
    echo "del-fail"
    grc=1
  else
    echo "del-ok"
  fi

  rm -f ${TEXPA} ${TEXPS} ${TACT} ${TFN}
done

if [[ $grc -eq 0 ]]; then
  echo "OK"
else
  echo "FAIL"
fi

