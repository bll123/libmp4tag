#!/bin/bash
#
# Copyright 2024-2025 Brad Lanam Pleasant Hill CA
#

SFUSER=bll123

project=libmp4tag
server=web.sourceforge.net
remuser=bll123
port=22
wwwpath=/home/project-web/${project}/htdocs

. ./VERSION.txt

VERS=$LIBMP4TAG_VERSION
TMP=webtmp

test -d ${TMP} && rm -rf ${TMP}
mkdir -p ${TMP}

inkscape web/${project}.svg -w 16 -h 16 -o ${project}-16.png > /dev/null 2>&1
icotool -c -o favicon.ico ${project}-16.png
mv favicon.ico ${TMP}
rm -f ${project}-16.png
inkscape web/${project}.svg -w 128 -h 128 -o ${project}.png > /dev/null 2>&1
mv ${project}.png ${TMP}
sed "s/#VERSION#/${VERS}/" web/${project}.html > ${TMP}/index.html
touch -r web/${project}.html ${TMP}/index.html
if [[ $VERSFN -nt web/${project}.html ]]; then
  touch -r $VERSFN ${TMP}/index.html
fi
cp -p web/libmp4tag.svg ${TMP}

if [[ ! -d $TMP ]]; then
  echo "no $TMP"
  exit 1
fi

cd $TMP
rsync -c -v -e ssh -aS --delete \
    . \
    ${remuser}@${server}:${wwwpath}
cd ..
rm -rf ${TMP}

exit 0
