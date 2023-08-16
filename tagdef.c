// #include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libmp4tag.h"
#include "libmp4tagint.h"

/* must be sorted in ascii order */
const mp4tag_t mp4tags [] = {
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
  { "cpil", "COMPILATION", MP4TAG_TYPE_STR, },
  { "cprt", "COPYRIGHT", MP4TAG_TYPE_STR, },
  { "desc", "DESCRIPTION", MP4TAG_TYPE_STR, },
  { "disk", "DISCNUMBER", MP4TAG_TYPE_OTHER, },
  { "egid", "episodeglobaluniqueid", MP4TAG_TYPE_OTHER, },
  /* standard apple genre -- uses numeric id */
  { "gnre", "GENRE", MP4TAG_TYPE_OTHER, },
  { "mvc",  "MOVEMENTTOTAL", MP4TAG_TYPE_OTHER, },
  { "mvi",  "MOVEMENT", MP4TAG_TYPE_OTHER, },
  { "pcst", "podcast", MP4TAG_TYPE_BOOL, },
  { "catg", "podcastcategory", MP4TAG_TYPE_OTHER, },
  { "ldes", "podcastdescription", MP4TAG_TYPE_OTHER, },
  { "egid", "podcastid", MP4TAG_TYPE_OTHER, },
  { "keyw", "podcastkeywords", MP4TAG_TYPE_OTHER, },
  { "purl", "podcasturl", MP4TAG_TYPE_OTHER, },
  { "pgap", "gaplessplayback", MP4TAG_TYPE_BOOL, },
  { "purd", "purchasedate", MP4TAG_TYPE_OTHER, },
  { "rtng", "itunesrating", MP4TAG_TYPE_OTHER, },
  { "shwm", "SHOWMOVEMENT", MP4TAG_TYPE_BOOL, },
  { "soaa", "ALBUMARTISTSORT", MP4TAG_TYPE_STR, },
  { "soal", "ALBUMSORT", MP4TAG_TYPE_STR, },
  { "soar", "ARTISTSORT", MP4TAG_TYPE_STR, },
  { "soco", "COMPOSERSORT", MP4TAG_TYPE_STR, },
  { "sonm", "TITLESORT", MP4TAG_TYPE_STR, },
  { "tmpo", "BPM", MP4TAG_TYPE_OTHER, },
  { "trkn", "TRACKNUMBER", MP4TAG_TYPE_OTHER, },
  { "tven", "tvepisodenumber", MP4TAG_TYPE_OTHER, },
  { "tves", "tvepisode", MP4TAG_TYPE_OTHER, },
  { "tvnn", "tvnetworkname", MP4TAG_TYPE_OTHER, },
  { "tvsh", "tvshowname", MP4TAG_TYPE_OTHER, },
  { "tvsn", "tvseason", MP4TAG_TYPE_OTHER, },
  { "©ART", "ARTIST", MP4TAG_TYPE_STR, },
  { "©alb", "ALBUM", MP4TAG_TYPE_STR, },
  { "©cmt", "COMMENT", MP4TAG_TYPE_STR, },
  { "©dir", "director", MP4TAG_TYPE_STR, },
  { "©nrt", "NARRATOR", MP4TAG_TYPE_STR, },
  { "©day", "YEAR", MP4TAG_TYPE_OTHER, },
  { "©dir", "DIRECTOR", MP4TAG_TYPE_STR, },
  /* custom genre */
  { "©gen", "GENRE", MP4TAG_TYPE_STR, },
  { "©grp", "GROUPING", MP4TAG_TYPE_OTHER, },
  { "©lyr", "LYRICS", MP4TAG_TYPE_OTHER, },
  { "©pub", "PUBLISHER", MP4TAG_TYPE_STR, },
  { "©mvn", "MOVEMENTNAME", MP4TAG_TYPE_STR, },
  { "©nam", "TITLE", MP4TAG_TYPE_STR, },
  { "©too", "ENCODEDBY", MP4TAG_TYPE_STR, },
  { "©wrk", "WORK", MP4TAG_TYPE_STR, },
  { "©wrt", "COMPOSER", MP4TAG_TYPE_STR, },
  { "stik", "mediatype", MP4TAG_TYPE_OTHER, },
};

