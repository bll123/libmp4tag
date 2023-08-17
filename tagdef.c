// #include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libmp4tag.h"
#include "libmp4tagint.h"

/* must be sorted in ascii order */
const mp4tagdef_t mp4tags [] = {
  { "----:com.apple.iTunes:CONDUCTOR" },
  { "----:com.apple.iTunes:MusicBrainz Album Artist Id" },
  { "----:com.apple.iTunes:MusicBrainz Album Id" },
  { "----:com.apple.iTunes:MusicBrainz Album Release Country" },
  { "----:com.apple.iTunes:MusicBrainz Album Status" },
  { "----:com.apple.iTunes:MusicBrainz Album Type" },
  { "----:com.apple.iTunes:MusicBrainz Artist Id" },
  { "----:com.apple.iTunes:MusicBrainz Disc Id" },
  { "----:com.apple.iTunes:MusicBrainz Original Album Id" },
  { "----:com.apple.iTunes:MusicBrainz Original Artist Id" },
  { "----:com.apple.iTunes:MusicBrainz Release Group Id" },
  { "----:com.apple.iTunes:MusicBrainz Release Track Id" },
  /* the musicbrainz-track-id is really the recording-id */
  { "----:com.apple.iTunes:MusicBrainz Track Id" },
  { "----:com.apple.iTunes:MusicBrainz Work Id" },
  { "aART" },
  { "catg" },
  { "covr" },
  { "cpil" },
  { "cprt" },
  { "desc" },
  { "disk" },
  { "egid" },
  { "egid" },
  /* standard apple genre -- uses id3v1 id */
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

