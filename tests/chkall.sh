#!/bin/bash

CS=©
TEXPA=x1
TEXPS=x1s
TACT=x2
TFN=xx.m4a

PICA=samples/bdj4-b.png
PICALEN=$(stat --format '%s' ${PICA})
PICB=samples/bdj4-y.png
PICBLEN=$(stat --format '%s' ${PICB})
PICC=samples/bdj4-b.jpg
PICCLEN=$(stat --format '%s' ${PICC})
PICD=samples/bdj4-y.jpg
PICDLEN=$(stat --format '%s' ${PICD})

MP4TAGCLI=./build/mp4tagcli
if [[ ! -f ${MP4TAGCLI} ]]; then
  echo "executable not found"
  exit 1
fi

rm -f ${TEXPA} ${TEXPS} ${TACT} ${TFN}
# no-tags.m4a has no udta box, tag space is unlimited
# alac.m4a tags are not at the end
for f in samples/no-tags.m4a samples/alac.m4a; do
  echo -n "chk: $f "
  if [[ ! -f $f ]]; then
    echo "not found"
    continue
  fi

  cp $f ${TFN}

  # string tags
  for tag in aART catg cprt desc keyw ldes ownr purd purl soaa \
      soal soar soco sonm sosn tven tvnn tvsh ${CS}ART ${CS}alb ${CS}cmt \
      ${CS}day ${CS}dir ${CS}gen ${CS}grp ${CS}lyr ${CS}mvn \
      ${CS}nam ${CS}nrt ${CS}pub ${CS}too ${CS}wrk ${CS}wrt \
      ; do
    ${MP4TAGCLI} ${TFN} ${tag}=${tag}123456
    # don't save these in the final output file
  done

  # custom tags
  for tag in ----:TEST:A ----:TEST:BBB \
      ; do
    ${MP4TAGCLI} ${TFN} -- ${tag}=CCC123456
    # don't save these in the final output file
  done

  # numeric tags
  for tag in cpil hdvd pgap shwm tmpo tves tvsn ${CS}mvc ${CS}mvi \
      ; do
    ${MP4TAGCLI} ${TFN} ${tag}=1
    echo ${tag}=1 >> ${TEXPA}
  done

  # disk/trkn tags
  for tag in disk trkn \
      ; do
    ${MP4TAGCLI} ${TFN} ${tag}=1/5
    echo ${tag}=1/5 >> ${TEXPA}
  done

  # cover images

  ${MP4TAGCLI} ${TFN} covr=${PICA}
  echo "covr=(data: ${PICALEN} bytes)" >> ${TEXPA}
  ${MP4TAGCLI} ${TFN} covr:0:name=xyzzy
  echo "covr:0:name=xyzzy" >> ${TEXPA}

  ${MP4TAGCLI} ${TFN} covr:1=${PICD}
  echo "covr:1=(data: ${PICDLEN} bytes)" >> ${TEXPA}
  ${MP4TAGCLI} ${TFN} covr:1:name=plugh
  echo "covr:1:name=plugh" >> ${TEXPA}

  ${MP4TAGCLI} ${TFN} covr:2=${PICC} covr:2:name=three
  echo "covr:2=(data: ${PICCLEN} bytes)" >> ${TEXPA}
  echo "covr:2:name=three" >> ${TEXPA}

  # replace cover 1 with an alternate
  # this should be in-place
  ${MP4TAGCLI} ${TFN} covr:1=${PICB}
  ${MP4TAGCLI} ${TFN} covr:1:name=alt

  # and then change it back to what it was
  # this should be in-place
  ${MP4TAGCLI} ${TFN} covr:1=${PICD}
  ${MP4TAGCLI} ${TFN} covr:1:name=plugh

  # string tags
  # these replacements should all be in-place,
  # as longer names were used above.
  for tag in aART catg cprt desc keyw ldes ownr purd purl soaa \
      soal soar soco sonm sosn tven tvnn tvsh ${CS}ART ${CS}alb ${CS}cmt \
      ${CS}day ${CS}dir ${CS}gen ${CS}grp ${CS}lyr ${CS}mvn \
      ${CS}nam ${CS}nrt ${CS}pub ${CS}too ${CS}wrk ${CS}wrt \
      ; do
    ${MP4TAGCLI} ${TFN} ${tag}=${tag}
    echo ${tag}=${tag} >> ${TEXPA}
  done

  # custom tags
  # these replacements should all be in-place,
  # as longer names were used above
  for tag in ----:TEST:A ----:TEST:BBB \
      ; do
    ${MP4TAGCLI} ${TFN} -- ${tag}=CCC
    echo ${tag}=CCC >> ${TEXPA}
  done

  # any existing custom tags are not present in
  # the expected results file
  ${MP4TAGCLI} ${TFN} |
      grep -E -v -- '(duration|----:com)' |
      LANG=C sort > ${TACT}

  LANG=C sort < ${TEXPA} > ${TEXPS}
  diff ${TEXPS} ${TACT} > /dev/null 2>&1
  rc=$?
  if [[ $rc -eq 0 ]]; then
    echo "ok"
  else
    echo "fail"
    diff ${TEXPS} ${TACT}
  fi
  rm -f ${TEXPA} ${TEXPS} ${TACT} ${TFN}
done
