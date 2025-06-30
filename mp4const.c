/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "mp4tagint.h"

const char *COPYRIGHT_STR = "\xc2\xa9";   /* copyright symbol */

const char *boxids [] = {
  /* various idents that libmp4tag needs to descend into or use */
  [MP4TAG_CO64] = "co64",
  [MP4TAG_FREE] = "free",
  [MP4TAG_FTYP] = "ftyp",
  [MP4TAG_HDLR] = "hdlr",
  [MP4TAG_ILST] = "ilst",
  [MP4TAG_MDHD] = "mdhd",
  [MP4TAG_MDIA] = "mdia",
  [MP4TAG_META] = "meta",
  [MP4TAG_MINF] = "minf",
  [MP4TAG_MOOV] = "moov",
  [MP4TAG_STBL] = "stbl",
  [MP4TAG_STCO] = "stco",
  [MP4TAG_TRAK] = "trak",
  [MP4TAG_UDTA] = "udta",
  /* tags that have special handling */
  [MP4TAG_COVR] = "covr",
  [MP4TAG_CUSTOM] = "----",
  [MP4TAG_DISK] = "disk",
  [MP4TAG_GEN] = "gen",
  [MP4TAG_GNRE] = "gnre",
  [MP4TAG_TRKN] = "trkn",
  /* internal mp4 values used by tag idents */
  [MP4TAG_DATA] = "data",
  [MP4TAG_MEAN] = "mean",
  [MP4TAG_NAME] = "name",
};

/* Must be sorted in ASCII order. */
/* This list is used to verify that a tag is valid if it is not found */
/* in the current tag list. */
/* The identifier type and length for a tag are stored here. */
/* If the priority field is updated, be sure to check MP4TAG_PRI_MAX */
/* in mp4tagint.h */
/* The priorities do not exactly match iTunes or mutagen */
/* priority, tag, identtype, len */
const mp4tagdef_t mp4taglist [] = {
  {  2, "aART", MP4TAG_ID_STRING, 0 },              // string (album artist)
  {  6, "akID", MP4TAG_ID_NUM, sizeof (uint8_t) },  // 1-byte (itunes)
  {  6, "atID", MP4TAG_ID_NUM, sizeof (uint32_t) }, // 4-byte (itunes artist id)
  {  7, "catg", MP4TAG_ID_STRING, 0 },              // string (category)
  {  6, "cmID", MP4TAG_ID_NUM, sizeof (uint32_t) }, // 4-byte (itunes composer id)
  {  6, "cnID", MP4TAG_ID_NUM, sizeof (uint32_t) }, // 4-byte (itunes catalog id)
  { 10, "covr", MP4TAG_ID_JPG, 0 },                 // or MP4TAG_ID_PNG
  {  5, "cpil", MP4TAG_ID_NUM, sizeof (uint8_t) },  // boolean, 1-byte
  {  7, "cprt", MP4TAG_ID_STRING, 0 },              // string (copyright)
  {  7, "desc", MP4TAG_ID_STRING, 0 },              // string (description)
  {  4, "disk", MP4TAG_ID_DATA, sizeof (uint32_t) + sizeof (uint16_t) }, // data, 4+2 byte
  {  7, "egid", MP4TAG_ID_STRING, 0 },              // string (itunes podcast GUID)
  {  6, "geID", MP4TAG_ID_NUM, sizeof (uint32_t) }, // 4-byte (itunes genre id)
  { MP4TAG_PRI_NOWRITE, "gnre", MP4TAG_ID_DATA, sizeof (uint16_t) }, // 2-byte, (apple id3v1 id, do not write)
  {  6, "hdvd", MP4TAG_ID_NUM, sizeof (uint8_t) },  // boolean, 1-byte (HD DVD)
  {  7, "keyw", MP4TAG_ID_STRING, 0 },              // string (keywords)
  {  7, "ldes", MP4TAG_ID_STRING, 0 },              // string (lyrics description)
  {  7, "ownr", MP4TAG_ID_STRING, 0 },              // string (owner)
  {  5, "pcst", MP4TAG_ID_BOOL, sizeof (uint8_t) }, // boolean, 1-byte (podcast)
  {  5, "pgap", MP4TAG_ID_BOOL, sizeof (uint8_t) }, // boolean, 1-byte (play gapless)
  {  6, "plID", MP4TAG_ID_NUM, sizeof (uint64_t) }, // 8-byte (itunes album id)
  {  7, "purd", MP4TAG_ID_STRING, 0 },              // string (purchase date)
  {  7, "purl", MP4TAG_ID_STRING, 0 },              // string (podcast url)
  {  6, "rtng", MP4TAG_ID_NUM, sizeof (uint8_t) },  // 1-byte (advisory rating)
  {  6, "sfID", MP4TAG_ID_NUM, sizeof (uint32_t) }, // 4-byte (itunes country id)
  {  6, "shwm", MP4TAG_ID_BOOL, sizeof (uint8_t) }, // boolean, 1-byte (show movement)
  {  7, "soaa", MP4TAG_ID_STRING, 0 },              // string (album artist sort)
  {  7, "soal", MP4TAG_ID_STRING, 0 },              // string (album sort)
  {  7, "soar", MP4TAG_ID_STRING, 0 },              // string (artist sort)
  {  7, "soco", MP4TAG_ID_STRING, 0 },              // string (composer sort)
  {  7, "sonm", MP4TAG_ID_STRING, 0 },              // string (title sort)
  {  7, "sosn", MP4TAG_ID_STRING, 0 },              // string (tv show sort)
  {  6, "stik", MP4TAG_ID_NUM, sizeof (uint8_t) },  // 1 byte (media type)
  {  5, "tmpo", MP4TAG_ID_NUM, sizeof (uint16_t) }, // 2 byte (bpm)
  {  4, "trkn", MP4TAG_ID_DATA, 8 },                // data 4+2+(2=unused)
  {  7, "tven", MP4TAG_ID_STRING, 0 },              // string (tv episode name)
  {  6, "tves", MP4TAG_ID_NUM, sizeof (uint32_t) }, // 4 byte (tv episode)
  {  7, "tvnn", MP4TAG_ID_STRING, 0 },              // string (tv network name)
  {  7, "tvsh", MP4TAG_ID_STRING, 0 },              // string (tv show name)
  {  6, "tvsn", MP4TAG_ID_NUM, sizeof (uint32_t) }, // 4 byte (tv season)
  {  1, "©ART", MP4TAG_ID_STRING, 0 },              // string (artist)
  {  2, "©alb", MP4TAG_ID_STRING, 0 },              // string (album)
  {  7, "©cmt", MP4TAG_ID_STRING, 0 },              // string (comment)
  {  5, "©day", MP4TAG_ID_STRING, 0 },              // string (year)
  {  7, "©dir", MP4TAG_ID_STRING, 0 },              // string (director)
  {  3, "©gen", MP4TAG_ID_STRING, 0 },              // string (custom genre)
  {  7, "©grp", MP4TAG_ID_STRING, 0 },              // string (grouping)
  {  9, "©lyr", MP4TAG_ID_STRING, 0 },              // string (lyrics)
  {  6, "©mvc", MP4TAG_ID_NUM, sizeof (uint16_t) }, // 2-byte (movement total)
  {  6, "©mvi", MP4TAG_ID_NUM, sizeof (uint16_t) }, // 2-byte (movement number)
  {  7, "©mvn", MP4TAG_ID_STRING, 0 },              // string (movement name)
  {  0, "©nam", MP4TAG_ID_STRING, 0 },              // string (title)
  {  7, "©nrt", MP4TAG_ID_STRING, 0 },              // string (narrator)
  {  7, "©pub", MP4TAG_ID_STRING, 0 },              // string (publisher)
  {  5, "©too", MP4TAG_ID_STRING, 0 },              // string (encoded by)
  {  7, "©wrk", MP4TAG_ID_STRING, 0 },              // string (work)
  {  2, "©wrt", MP4TAG_ID_STRING, 0 },              // string (writer/composer)
};

const int mp4taglistlen = sizeof (mp4taglist) / sizeof (mp4tagdef_t);

/* an idiotic way to do things, */
/* but we must convert any old gnre data to ©gen. */
/* itunes still puts data into the 'gnre' field, yick. */
/* this is the ID3 genre list */
const char *mp4tagoldgenrelist [] = {
  "Blues",              "Classic Rock",           "Country",
  "Dance",              "Disco",                  "Funk",
  "Grunge",             "Hip-Hop",                "Jazz",
  "Metal",              "New Age",                "Oldies",
  "Other",              "Pop",                    "R&B",
  "Rap",                "Reggae",                 "Rock",
  "Techno",             "Industrial",             "Alternative",
  "Ska",                "Death Metal",            "Pranks",
  "Soundtrack",         "Euro-Techno",            "Ambient",
  "Trip-Hop",           "Vocal",                  "Jazz+Funk",
  "Fusion",             "Trance",                 "Classical",
  "Instrumental",       "Acid",                   "House",
  "Game",               "Sound Clip",             "Gospel",
  "Noise",              "Alt. Rock",              "Bass",
  "Soul",               "Punk",                   "Space",
  "Meditative",         "Instrumental Pop",       "Instrumental Rock",
  "Ethnic",             "Gothic",                 "Darkwave",
  "Techno-Industrial",  "Electronic",             "Pop-Folk",
  "Eurodance",          "Dream",                  "Southern Rock",
  "Comedy",             "Cult",                   "Gangsta Rap",
  "Top 40",             "Christian Rap",          "Pop/Funk",
  "Jungle",             "Native American",        "Cabaret",
  "New Wave",           "Psychedelic",            "Rave",
  "Showtunes",          "Trailer",                "Lo-Fi",
  "Tribal",             "Acid Punk",              "Acid Jazz",
  "Polka",              "Retro",                  "Musical",
  "Rock & Roll",        "Hard Rock",              "Folk",
  "Folk-Rock",          "National Folk",          "Swing",
  "Fast-Fusion",        "Bebop",                  "Latin",
  "Revival",            "Celtic",                 "Bluegrass",
  "Avantgarde",         "Gothic Rock",            "Progressive Rock",
  "Psychedelic Rock",   "Symphonic Rock",         "Slow Rock",
  "Big Band",           "Chorus",                 "Easy Listening",
  "Acoustic",           "Humour",                 "Speech",
  "Chanson",            "Opera",                  "Chamber Music",
  "Sonata",             "Symphony",               "Booty Bass",
  "Primus",             "Porn Groove",            "Satire",
  "Slow Jam",           "Club",                   "Tango",
  "Samba",              "Folklore",               "Ballad",
  "Power Ballad",       "Rhythmic Soul",          "Freestyle",
  "Duet",               "Punk Rock",              "Drum Solo",
  "A Cappella",         "Euro-House",             "Dance Hall",
  "Goa",                "Drum & Bass",            "Club-House",
  "Hardcore",           "Terror",                 "Indie",
  "BritPop",            "Afro-Punk",              "Polsk Punk",
  "Beat",               "Christian Gangsta Rap",  "Heavy Metal",
  "Black Metal",        "Crossover",              "Contemporary Christian",
  "Christian Rock",     "Merengue",               "Salsa",
  "Thrash Metal",       "Anime",                  "JPop",
  "Synthpop",           "Abstract",               "Art Rock",
  "Baroque",            "Bhangra",                "Big Beat",
  "Breakbeat",          "Chillout",               "Downtempo",
  "Dub",                "EBM",                    "Eclectic",
  "Electro",            "Electroclash",           "Emo",
  "Experimental",       "Garage",                 "Global",
  "IDM",                "Illbient",               "Industro-Goth",
  "Jam Band",           "Krautrock",              "Leftfield",
  "Lounge",             "Math Rock",              "New Romantic",
  "Nu-Breakz",          "Post-Punk",              "Post-Rock",
  "Psytrance",          "Shoegaze",               "Space Rock",
  "Trop Rock",          "World Music",            "Neoclassical",
  "Audiobook",          "Audio Theatre",          "Neue Deutsche Welle",
  "Podcast",            "Indie Rock",             "G-Funk",
  "Dubstep",            "Garage Rock",            "Psybient",
};

const int mp4tagoldgenrelistsz = sizeof (mp4tagoldgenrelist) / sizeof (const char *);

