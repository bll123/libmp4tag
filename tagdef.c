// #include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libmp4tag.h"
#include "libmp4tagint.h"

/* must be sorted in ascii order */
const mp4tagdef_t mp4tags [] = {
  { "----:com.apple.iTunes:MusicBrainz Release Group Id", "MUSICBRAINZ_RELEASEGROUPID", MP4TAG_TYPE_STR, },
  { "----:com.apple.iTunes:MusicBrainz Original Artist Id", "MUSICBRAINZ_ORIGINALARTISTID", MP4TAG_TYPE_STR, },
  { "----:com.apple.iTunes:MusicBrainz Original Album Id", "MUSICBRAINZ_ORIGINALALBUMID", MP4TAG_TYPE_STR, },
  { "----:com.apple.iTunes:MusicBrainz Disc Id", "MUSICBRAINZ_DISCID", MP4TAG_TYPE_STR, },
  { "----:com.apple.iTunes:MusicBrainz Artist Id", "MUSICBRAINZ_ARTISTID", MP4TAG_TYPE_STR, },
  { "----:com.apple.iTunes:MusicBrainz Album Type", "MUSICBRAINZ_ALBUMTYPE", MP4TAG_TYPE_STR, },
  { "----:com.apple.iTunes:MusicBrainz Album Status", "MUSICBRAINZ_ALBUMSTATUS", MP4TAG_TYPE_STR, },
  { "----:com.apple.iTunes:MusicBrainz Album Release Country", "MUSICBRAINZ_ALBUMRELEASECOUNTRY", MP4TAG_TYPE_STR, },
  { "----:com.apple.iTunes:MusicBrainz Album Id", "MUSICBRAINZ_ALBUMID", MP4TAG_TYPE_STR, },
  { "----:com.apple.iTunes:MusicBrainz Album Artist Id", "MUSICBRAINZ_ALBUMARTISTID", MP4TAG_TYPE_STR, },
  { "----:com.apple.iTunes:CONDUCTOR", "CONDUCTOR", MP4TAG_TYPE_STR, },
  { "----:com.apple.iTunes:MusicBrainz Release Track Id", "MUSICBRAINZ_RELEASETRACKID", MP4TAG_TYPE_STR, },
  /* the musicbrainz-track-id is really the recording-id */
  { "----:com.apple.iTunes:MusicBrainz Track Id", "MUSICBRAINZ_TRACKID", MP4TAG_TYPE_STR, },
  { "----:com.apple.iTunes:MusicBrainz Work Id", "MUSICBRAINZ_WORKID", MP4TAG_TYPE_STR, },
  { "aART", "ALBUMARTIST", MP4TAG_TYPE_STR, },
  { "covr", "METADATA_BLOCK_PICTURE", MP4TAG_TYPE_PIC, },
  { "cpil", "COMPILATION", MP4TAG_TYPE_8, },
  { "cprt", "COPYRIGHT", MP4TAG_TYPE_STR, },
  { "desc", "DESCRIPTION", MP4TAG_TYPE_STR, },
  { "disk", "DISCNUMBER", MP4TAG_TYPE_3216, },
  { "egid", "episodeglobaluniqueid", MP4TAG_TYPE_STR, },
  /* standard apple genre -- uses numeric id */
  { "gnre", "GENRE", MP4TAG_TYPE_16, },
  { "hdvd", "hdvideo", MP4TAG_TYPE_8, },
  { "pcst", "podcast", MP4TAG_TYPE_BOOL, },
  { "catg", "podcastcategory", MP4TAG_TYPE_STR, },
  { "ldes", "podcastdescription", MP4TAG_TYPE_UNKNOWN, },
  { "egid", "podcastid", MP4TAG_TYPE_UNKNOWN, },
  { "keyw", "podcastkeywords", MP4TAG_TYPE_UNKNOWN, },
  { "ownr", "owner", MP4TAG_TYPE_STR, },
  { "purl", "podcasturl", MP4TAG_TYPE_STR, },
  { "pgap", "gaplessplayback", MP4TAG_TYPE_BOOL, },
  { "purd", "purchasedate", MP4TAG_TYPE_STR, },
  { "rtng", "itunesrating", MP4TAG_TYPE_8, },
  { "shwm", "SHOWMOVEMENT", MP4TAG_TYPE_8, },
  { "soaa", "ALBUMARTISTSORT", MP4TAG_TYPE_STR, },
  { "soal", "ALBUMSORT", MP4TAG_TYPE_STR, },
  { "soar", "ARTISTSORT", MP4TAG_TYPE_STR, },
  { "soco", "COMPOSERSORT", MP4TAG_TYPE_STR, },
  { "sonm", "TITLESORT", MP4TAG_TYPE_STR, },
  { "tmpo", "BPM", MP4TAG_TYPE_16, },
  { "trkn", "TRACKNUMBER", MP4TAG_TYPE_3216, },
  { "tven", "tvepisodenumber", MP4TAG_TYPE_UNKNOWN, },
  { "tves", "tvepisode", MP4TAG_TYPE_32, },
  { "tvnn", "tvnetworkname", MP4TAG_TYPE_UNKNOWN, },
  { "tvsh", "tvshowname", MP4TAG_TYPE_STR, },
  { "tvsn", "tvseason", MP4TAG_TYPE_32, },
  { "©ART", "ARTIST", MP4TAG_TYPE_STR, },
  { "©alb", "ALBUM", MP4TAG_TYPE_STR, },
  { "©cmt", "COMMENT", MP4TAG_TYPE_STR, },
  { "©dir", "director", MP4TAG_TYPE_STR, },
  { "©nrt", "NARRATOR", MP4TAG_TYPE_STR, },
  { "©day", "YEAR", MP4TAG_TYPE_STR, },
  { "©dir", "DIRECTOR", MP4TAG_TYPE_STR, },
  /* custom genre */
  { "©gen", "GENRE", MP4TAG_TYPE_STR, },
  { "©grp", "GROUPING", MP4TAG_TYPE_STR, },
  { "©lyr", "LYRICS", MP4TAG_TYPE_STR, },
  { "©mvc",  "MOVEMENTTOTAL", MP4TAG_TYPE_16, },
  { "©mvi",  "MOVEMENT", MP4TAG_TYPE_16, },
  { "©pub", "PUBLISHER", MP4TAG_TYPE_STR, },
  { "©mvn", "MOVEMENTNAME", MP4TAG_TYPE_STR, },
  { "©nam", "TITLE", MP4TAG_TYPE_STR, },
  { "©too", "ENCODEDBY", MP4TAG_TYPE_STR, },
  { "©wrk", "WORK", MP4TAG_TYPE_STR, },
  { "©wrt", "COMPOSER", MP4TAG_TYPE_STR, },
  { "stik", "mediatype", MP4TAG_TYPE_8, },
};

