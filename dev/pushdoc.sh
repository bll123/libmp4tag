#!/bin/bash
#
# Copyright 2023 Brad Lanam Pleasant Hill CA
#

userid=bll123
project=libmp4tag

if [[ $SSHPASS == "" ]]; then
  echo -n "sourceforge Password: "
  read -s SSHPASS
  echo ""
  export SSHPASS
fi

sshpass -e rsync -aS --delete -e ssh doxygen \
    ${userid}@web.sourceforge.net:/home/project-web/${project}/htdocs

exit 0
