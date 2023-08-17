#!/bin/bash

echo -n "sourceforge Password: "
read -s SSHPASS
echo ""
if [[ $SSHPASS == "" ]]; then
  echo "No password."
  exit 1
fi
export SSHPASS

fn=README.txt
sshpass -e rsync -v -e ssh ${fn} \
    bll123@frs.sourceforge.net:/home/frs/project/libmp4tag/${fn}
sshpass -e rsync -v -e ssh libmp4tag-src-*.tar.gz \
    bll123@frs.sourceforge.net:/home/frs/project/libmp4tag/
exit 0
