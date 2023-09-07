#!/bin/bash

CS=©
TEXPA=x1
TEXPS=x1s
TACT=x2

MP4TAGCLI=./build/mp4tagcli
if [[ ! -f ${MP4TAGCLI} ]]; then
  echo "executable not found"
  exit 1
fi

# no-tags.m4a has no udta box
# alac.m4a tags are not at the end
for f in samples/no-tags.m4a samples/alac.m4a; do
  echo -n "chk: $f "
  if [[ ! -f $f ]]; then
    echo "not found"
    continue
  fi

  cp $f tt.m4a
  rm -f ${TEXPA} ${TEXPA}s x2

  # string tags
  for tag in aART catg cprt desc keyw ldes ownr purd purl soaa \
      soal soar soco sonm sosn tven tvnn tvsh ${CS}ART ${CS}alb ${CS}cmt \
      ${CS}day ${CS}dir ${CS}gen ${CS}grp ${CS}lyr ${CS}mvn \
      ${CS}nam ${CS}nrt ${CS}pub ${CS}too ${CS}wrk ${CS}wrt \
      ; do
    ${MP4TAGCLI} tt.m4a ${tag}=${tag}123456
    # don't save these in the final output file
  done

  # custom tags
  for tag in ----:TEST:A ----:TEST:BBB \
      ; do
    ${MP4TAGCLI} tt.m4a -- ${tag}=CCC
    echo ${tag}=CCC >> ${TEXPA}
  done

  # numeric tags
  for tag in cpil hdvd pgap shwm tmpo tves tvsn ${CS}mvc ${CS}mvi \
      ; do
    ${MP4TAGCLI} tt.m4a ${tag}=1
    echo ${tag}=1 >> ${TEXPA}
  done

  # disk/trkn tags
  for tag in disk trkn \
      ; do
    ${MP4TAGCLI} tt.m4a ${tag}=1/5
    echo ${tag}=1/5 >> ${TEXPA}
  done

  # cover images
  ${MP4TAGCLI} tt.m4a covr=samples/bdj4-b.png
  echo "covr=(data: 26027 bytes)" >> ${TEXPA}
  ${MP4TAGCLI} tt.m4a covr:0:name=xyzzy
  echo "covr:0:name=xyzzy" >> ${TEXPA}
  ${MP4TAGCLI} tt.m4a covr:1=samples/bdj4-y.jpg
  echo "covr:1=(data: 24477 bytes)" >> ${TEXPA}
  ${MP4TAGCLI} tt.m4a covr:1:name=plugh
  echo "covr:1:name=plugh" >> ${TEXPA}
  ${MP4TAGCLI} tt.m4a covr:2=samples/bdj4-b.jpg covr:2:name=three
  echo "covr:2=(data: 29776 bytes)" >> ${TEXPA}
  echo "covr:2:name=three" >> ${TEXPA}

  # string tags
  for tag in aART catg cprt desc keyw ldes ownr purd purl soaa \
      soal soar soco sonm sosn tven tvnn tvsh ${CS}ART ${CS}alb ${CS}cmt \
      ${CS}day ${CS}dir ${CS}gen ${CS}grp ${CS}lyr ${CS}mvn \
      ${CS}nam ${CS}nrt ${CS}pub ${CS}too ${CS}wrk ${CS}wrt \
      ; do
    ${MP4TAGCLI} tt.m4a ${tag}=${tag}
    echo ${tag}=${tag} >> ${TEXPA}
  done

  # any existing custom tags are not present in
  # the expected results file
  ${MP4TAGCLI} tt.m4a |
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
  rm -f ${TEXPA} ${TEXPS} ${TACT}
done
