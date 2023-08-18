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
  { "aART", MP4TAG_ID_STRING, 0 },    // string
  { "atID", MP4TAG_ID_NUM, 4 },       // 4-byte (itunes artist id)
  { "catg", MP4TAG_ID_STRING, 0 },
  { "cmID", MP4TAG_ID_NUM, 4 },       // 4-byte (itunes composer id)
  { "cnID", MP4TAG_ID_NUM, 4 },       // 4-byte (itunes catalog id)
  { "covr", MP4TAG_ID_PNG, 0 },       // or jpg
  { "cpil", MP4TAG_ID_NUM, 1 },       // boolean, 1-byte
  { "cprt", MP4TAG_ID_STRING, 0 },    // string
  { "desc", MP4TAG_ID_STRING, 0 },
  { "disk", MP4TAG_ID_DATA, 6 },      // data, 4+2 byte
  { "egid", MP4TAG_ID_DATA, 0 },
  { "geID", MP4TAG_ID_NUM, 4 },       // 4-byte (itunes genre id)
  /* apple genre -- uses id3v1 id */
  { "gnre", MP4TAG_ID_DATA, 2 },      // 2-byte, will not be written out
  { "hdvd", MP4TAG_ID_NUM, 1 },       // boolean, 1-byte
  { "keyw", MP4TAG_ID_STRING, 0 },
  { "ldes", MP4TAG_ID_STRING, 0 },
  { "ownr", MP4TAG_ID_STRING, 0 },    // string
  { "pcst", MP4TAG_ID_BOOL, 1 },      // boolean, 1-byte
  { "pgap", MP4TAG_ID_BOOL, 1 },      // boolean, 1-byte
  { "plID", MP4TAG_ID_NUM, 8 },       // 8-byte (itunes album id)
  { "purd", MP4TAG_ID_STRING, 0 },    // string (purchase date)
  { "purl", MP4TAG_ID_DATA, 0 },
  { "rate", MP4TAG_ID_NUM, 2 },       // need to verify this is valid
  { "rtng", MP4TAG_ID_NUM, 1 },       // 1-byte (advisory rating)
  { "sfID", MP4TAG_ID_NUM, 4 },       // 4-byte (itunes country id)
  { "shwm", MP4TAG_ID_BOOL, 1 },      // boolean, 1-byte
  { "soaa", MP4TAG_ID_STRING, 0 },    // string
  { "soal", MP4TAG_ID_STRING, 0 },    // string
  { "soar", MP4TAG_ID_STRING, 0 },    // string
  { "soco", MP4TAG_ID_STRING, 0 },    // string
  { "sonm", MP4TAG_ID_STRING, 0 },    // string
  { "sosn", MP4TAG_ID_STRING, 0 },    // string (tv show sort)
  { "stik", MP4TAG_ID_NUM, 1 },       // 1 byte  (media type)
  { "tmpo", MP4TAG_ID_NUM, 2 },       // 2 byte
  { "trkn", MP4TAG_ID_DATA, 8 },      // data 4+2+2-unused
  { "tven", MP4TAG_ID_STRING, 0 },
  { "tves", MP4TAG_ID_NUM, 1 },       // 1 byte (tv episode)
  { "tvnn", MP4TAG_ID_STRING, 0 },
  { "tvsh", MP4TAG_ID_STRING, 0 },
  { "tvsn", MP4TAG_ID_NUM, 1 },       // 1 byte (tv season)
  { "©ART", MP4TAG_ID_STRING, 0 },    // string
  { "©alb", MP4TAG_ID_STRING, 0 },    // string
  { "©cmt", MP4TAG_ID_STRING, 0 },    // string
  { "©day", MP4TAG_ID_STRING, 0 },    // string
  { "©dir", MP4TAG_ID_STRING, 0 },    // string (director)
  /* custom genre */
  { "©gen", MP4TAG_ID_STRING, 0 },    // string
  { "©grp", MP4TAG_ID_STRING, 0 },    // string
  { "©lyr", MP4TAG_ID_STRING, 0 },    // string
  { "©mvc", MP4TAG_ID_NUM, 2 },       // 2-byte
  { "©mvi", MP4TAG_ID_NUM, 2 },       // 2-byte
  { "©mvn", MP4TAG_ID_STRING, 0 },    // string
  { "©nam", MP4TAG_ID_STRING, 0 },    // string
  { "©nrt", MP4TAG_ID_STRING, 0 },
  { "©pub", MP4TAG_ID_STRING, 0 },
  { "©too", MP4TAG_ID_STRING, 0 },    // string
  { "©wrk", MP4TAG_ID_STRING, 0 },    // string
  { "©wrt", MP4TAG_ID_STRING, 0 },    // string
};

const int mp4taglistlen = sizeof (mp4taglist) / sizeof (mp4tagdef_t);
