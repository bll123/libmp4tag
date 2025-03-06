#!/bin/bash

SFUSER=bll123

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

inkscape dev/${project}.svg -w 16 -h 16 -o ${project}-16.png > /dev/null 2>&1
icotool -c -o favicon.ico ${project}-16.png
mv favicon.ico ${TMP}
rm -f ${project}-16.png
inkscape dev/${project}.svg -w 128 -h 128 -o ${project}.png > /dev/null 2>&1
mv ${project}.png ${TMP}
sed "s/#VERSION#/${VERS}/" dev/${project}.html > ${TMP}/index.html
touch -r dev/${project}.html ${TMP}/index.html
if [[ $VERSFN -nt dev/${project}.html ]]; then
  touch -r $VERSFN ${TMP}/index.html
fi
cp -p dev/libmp4tag.svg ${TMP}

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
