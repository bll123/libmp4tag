#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#

sfuser=bll123
project=libmp4tag
baseurl=https://sourceforge.net/rest/p/${project}/wiki
accessurl=${baseurl}/has_access
useragent=libmp4tag-wikiput.sh
cookiejar=tmp/cookiejar.txt
bearer=""
tmpfile=tmp/wiki-tmp.txt
filelist=tmp/wiki-files.txt

function gettitle {
  tfn=$1
  echo $tfn |
      sed \
      -e 's,^wiki/,,' \
      -e 's,/,-,g' \
      -e 's,\.txt$,,'
}

# this is a complete mess, but mostly works.
function gettext {
  tfn=$1

  # sourceforge's markdown has automatic line breaks.
  # these make editing the wiki files very difficult due to the very
  # long lines for each paragraph.
  # this regsub allows more intuitive paragraph handling
  # do not combine lines when:
  #   next line begins with a dash (probable table)
  #   next line begins with a ` (table)
  #   previous line ends with a ` (table)
  #   next line begins with a space (code)
  #   next line begins with a ~ (code)
  #   previous line begins with a ~ (code)
  #   previous line begins with a # (header)
  #   next line begins with a * (list)
  #   next line begins with a ^ (table marker) (will be removed)
  # remove leading ^ (table marker)
  # ? put a <br> before image captions, as the prior line will break them.
  # remove all trailing whitespace
  echo -n 'text=' > ${tmpfile}
  awk -e '
BEGIN {
  appendok = 0;
  text = "";
}
{
  gsub (/&/, "%26");
  sub (/\r$/, "");
  if ($0 ~ /Updated [0-9 :-]*; BDJ4 version /) {
    # do nothing
    next;
  }
# not known at this time if this is needed
#  if ($0 ~ /<span/) {
#    sub (/<span/, "<br><span");
#  }
  if ($0 ~ /^[^` *\^~#-]/ && appendok) {
    sub (/\n$/, "", text);
    text = text " ";
  } else {
    appendok = 0;
  }
  if ($0 ~ /[^`~]$/ && $0 !~ /^#/) {
    appendok = 1;
  }
  text = text $0;
  text = text "\n";
}
END {
  print text;
}
    ' \
    ${tfn} >> ${tmpfile}
}

function getaccesstoken {
  bearer=$(cat dev/wikibearer.txt)
}

function hasaccess {
  user=$1
  cmd="curl -L --silent -b ${cookiejar} -c ${cookiejar} -X GET \
        -H 'Authorization: Bearer $bearer' \
        -H 'User-Agent: $useragent' \
        --get \
        --data-urlencode 'user=${user}' \
        --data-urlencode 'perm=create' \
        '${accessurl}'"
  data=$(eval $cmd)
  access=F
  case $data in
    *\"result\":\ true*)
      access=T
      ;;
  esac
  echo $access
}

function getlist {
  cmd="curl -L --silent -b ${cookiejar} -c ${cookiejar} -X GET \
        -H 'Authorization: Bearer $bearer' \
        -H 'User-Agent: $useragent' \
        --output ${tmpfile} \
        '${baseurl}'"
  eval $cmd
  sed \
      -e 's,.*\[",,' -e 's,"\].*,,' \
      -e "s/\", \"/\n/g" \
      ${tmpfile} > ${filelist}
  echo "" >> ${filelist}
}

function get {
  tfn="$1"
  title=$(gettitle $tfn)
  cmd="curl -L --silent -b ${cookiejar} -c ${cookiejar} -X GET \
        -H 'Authorization: Bearer $bearer' \
        -H 'User-Agent: $useragent' \
        --output ${tmpfile} \
        '${baseurl}/${title}'"
  eval $cmd
  sed -e 's,.*"text": ",,' \
      -e 's,".*,,' \
      -e "s,\\\\r,,g" \
      -e "s,\\\\n,\n,g" \
      ${tmpfile} > $tfn
}

function put {
  tfn="$1"
  case ${tfn} in
    *~|*.orig)
      return
      ;;
  esac
  gettext $tfn
  title=$(gettitle $tfn)
  cmd="curl -L --silent -b ${cookiejar} -c ${cookiejar} -X POST \
      -H 'Authorization: Bearer $bearer' \
      -H 'User-Agent: $useragent' \
      --data-urlencode 'pagetitle=$title' \
      --data-binary @${tmpfile} \
      '${baseurl}/${title}'"
  eval $cmd > /dev/null
  echo "$tfn: updated"
}

function delete {
  # delete does not work on sourceforge
  tfn="$1"
  echo "action: delete: ${baseurl}/${title}"
  if [[ -f $tfn ]]; then
    hg rm $tfn
  fi
}

function getall {
  getlist
  for title in $(cat ${filelist}); do
    get "wiki/${title}"
  done
}

function putall {
  flist=$(find wiki -name '*.txt' -print)
  for f in $flist; do
    put $f
  done
}

function dispfilelist {
  getlist
  cat ${filelist}
}

getaccesstoken

case $1 in
  put)
    shift
    for f in "$@"; do
      if [[ -f "wiki/$f" ]]; then
        put "wiki/$f"
      fi
    done
    ;;
  putall)
    shift
    putall
    ;;
  getall)
    ;;
  dispfilelist)
    dispfilelist
    ;;
  get)
    shift
    for f in "$@"; do
      if [[ -f "wiki/$f" ]]; then
        get "wiki/$f"
      fi
    done
    ;;
  hasaccess)
    hasaccess
    ;;
  *)
    echo "Usage: $0 {putall|getall|dispfilelist|get <file>|put <file>}"
    ;;
esac

rm -f ${tmpfile} ${filelist} ${cookiejar}
exit 0
