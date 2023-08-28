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

project=libmp4tag
server=web.sourceforge.net
remuser=bll123
port=22
wwwpath=/home/project-web/${project}/htdocs

VERSFN=tmp/vers.txt
VERS=$(cat ${VERSFN})
TMP=webtmp

test -d ${TMP} && rm -rf ${TMP}
mkdir -p ${TMP}

sed "s/#VERSION#/${VERS}/" dev/${project}.html > ${TMP}/index.html

if [[ ! -d $TMP ]]; then
  echo "no $TMP"
  exit 1
fi

cd $TMP
sshpass -e rsync -v -e ssh -aS --delete \
    . \
    ${remuser}@${server}:${wwwpath}
cd ..
rm -rf ${TMP}

exit 0
