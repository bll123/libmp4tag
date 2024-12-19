#!/bin/bash

grep '^#define LIBMP4TAG_DEBUG 0' mp4tagint.h > /dev/null 2>&1
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "debugging is on"
  exit 1
fi

SFUSER=bll123

echo -n "sourceforge Password: "
read -s SSHPASS
echo ""
if [[ $SSHPASS == "" ]]; then
  echo "No password."
  exit 1
fi
export SSHPASS

VERSFN=tmp/vers.txt
VERS=$(cat ${VERSFN})

if [[ ! -f $VERSFN ]]; then
  echo "no version file"
  exit 1
fi
if [[ ! -f libmp4tag-src-${VERS}.tar.gz ]]; then
  echo "no source tar"
  exit 1
fi
if [[ ! -f libmp4tag-src-${VERS}.zip ]]; then
  echo "no source zip"
  exit 1
fi

make tclean

fn=README.txt
sshpass -e rsync -v -e ssh ${fn} \
    ${SFUSER}@frs.sourceforge.net:/home/frs/project/libmp4tag/${fn}

sshpass -e rsync -v -e ssh libmp4tag-src-${VERS}.tar.gz \
    ${SFUSER}@frs.sourceforge.net:/home/frs/project/libmp4tag/
sshpass -e rsync -v -e ssh libmp4tag-src-${VERS}.zip \
    ${SFUSER}@frs.sourceforge.net:/home/frs/project/libmp4tag/

exit 0
