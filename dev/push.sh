#!/bin/bash

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

make tclean

fn=README.txt
sshpass -e rsync -v -e ssh ${fn} \
    ${SFUSER}@frs.sourceforge.net:/home/frs/project/libmp4tag/${fn}

sshpass -e rsync -v -e ssh libmp4tag-src-${VERS}.tar.gz \
    ${SFUSER}@frs.sourceforge.net:/home/frs/project/libmp4tag/

# mkdir linux
# cp -pf build/libmp4tag.so linux
# cp -pf build/mp4tagcli linux

for d in linux win64 macos; do
  if [[ -d ${d} ]]; then
    sshpass -e rsync -r -v -e ssh ${d} \
        ${SFUSER}@frs.sourceforge.net:/home/frs/project/libmp4tag/${VERS}/
  fi
done

test -d linux && rm -rf linux

exit 0
