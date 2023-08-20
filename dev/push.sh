#!/bin/bash

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
if [[ ! -f build/libmp4tag.so || ! -f build/mp4tagcli ]]; then
  echo "no binaries "
  exit 1
fi

fn=README.txt
sshpass -e rsync -v -e ssh ${fn} \
    bll123@frs.sourceforge.net:/home/frs/project/libmp4tag/${fn}

sshpass -e rsync -v -e ssh libmp4tag-src-${VERS}.tar.gz \
    bll123@frs.sourceforge.net:/home/frs/project/libmp4tag/

mkdir linux
cp -pf build/libmp4tag.so linux
cp -pf build/mp4tagcli linux
sshpass -e rsync -r -v -e ssh linux \
    bll123@frs.sourceforge.net:/home/frs/project/libmp4tag/${VERS}/
rm -rf linux

exit 0
