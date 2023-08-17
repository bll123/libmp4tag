/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "libmp4tagint.h"

/* must be sorted in ascii order */
/* this list is only needed to verify that the tag being set is valid */
const mp4tagdef_t mp4taglist [] = {
  { "aART" },
  { "catg" },
  { "covr" },
  { "cpil" },
  { "cprt" },
  { "desc" },
  { "disk" },
  { "egid" },
  { "egid" },
  /* apple genre -- uses id3v1 id */
  { "gnre" },
  { "hdvd" },
  { "keyw" },
  { "ldes" },
  { "ownr" },
  { "pcst" },
  { "pgap" },
  { "purd" },
  { "purl" },
  { "rtng" },
  { "shwm" },
  { "soaa" },
  { "soal" },
  { "soar" },
  { "soco" },
  { "sonm" },
  { "stik" },
  { "tmpo" },
  { "trkn" },
  { "tven" },
  { "tves" },
  { "tvnn" },
  { "tvsh" },
  { "tvsn" },
  { "©ART" },
  { "©alb" },
  { "©cmt" },
  { "©day" },
  { "©dir" },
  { "©dir" },
  /* custom genre */
  { "©gen" },
  { "©grp" },
  { "©lyr" },
  { "©mvc" },
  { "©mvi" },
  { "©mvn" },
  { "©nam" },
  { "©nrt" },
  { "©pub" },
  { "©too" },
  { "©wrk" },
  { "©wrt" },
};

const int mp4taglistlen = sizeof (mp4taglist) / sizeof (mp4tagdef_t);
