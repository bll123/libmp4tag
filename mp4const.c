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
  {  7, "aART", MP4TAG_ID_STRING, 0 },    // string (album artist)
  {  6, "akID", MP4TAG_ID_NUM, 1 },       // 1-byte (?)
  {  6, "atID", MP4TAG_ID_NUM, 4 },       // 4-byte (itunes artist id)
  {  7, "catg", MP4TAG_ID_STRING, 0 },    // string
  {  6, "cmID", MP4TAG_ID_NUM, 4 },       // 4-byte (itunes composer id)
  {  6, "cnID", MP4TAG_ID_NUM, 4 },       // 4-byte (itunes catalog id)
  { 10, "covr", MP4TAG_ID_PNG, 0 },      // or jpg
  {  5, "cpil", MP4TAG_ID_NUM, 1 },       // boolean, 1-byte
  {  7, "cprt", MP4TAG_ID_STRING, 0 },    // string
  {  7, "desc", MP4TAG_ID_STRING, 0 },    // string
  {  4, "disk", MP4TAG_ID_DATA, 6 },      // data, 4+2 byte
  {  7, "egid", MP4TAG_ID_STRING, 0 },    // string
  {  6, "geID", MP4TAG_ID_NUM, 4 },       // 4-byte (itunes genre id)
  { MP4TAG_PRI_NOWRITE, "gnre", MP4TAG_ID_DATA, 2 }, // 2-byte, (apple id3v1 id, do not write)
  {  6, "hdvd", MP4TAG_ID_NUM, 1 },       // boolean, 1-byte
  {  7, "keyw", MP4TAG_ID_STRING, 0 },    // string
  {  7, "ldes", MP4TAG_ID_STRING, 0 },
  {  7, "ownr", MP4TAG_ID_STRING, 0 },    // string
  {  5, "pcst", MP4TAG_ID_BOOL, 1 },      // boolean, 1-byte
  {  5, "pgap", MP4TAG_ID_BOOL, 1 },      // boolean, 1-byte
  {  6, "plID", MP4TAG_ID_NUM, 8 },       // 8-byte (itunes album id)
  {  7, "purd", MP4TAG_ID_STRING, 0 },    // string (purchase date)
  {  7, "purl", MP4TAG_ID_STRING, 0 },    // string (podcast url)
  {  6, "rate", MP4TAG_ID_NUM, 2 },       // ??? need to verify if this is valid
  {  6, "rtng", MP4TAG_ID_NUM, 1 },       // 1-byte (advisory rating)
  {  6, "sfID", MP4TAG_ID_NUM, 4 },       // 4-byte (itunes country id)
  {  6, "shwm", MP4TAG_ID_BOOL, 1 },      // boolean, 1-byte
  {  7, "soaa", MP4TAG_ID_STRING, 0 },    // string
  {  7, "soal", MP4TAG_ID_STRING, 0 },    // string
  {  7, "soar", MP4TAG_ID_STRING, 0 },    // string
  {  7, "soco", MP4TAG_ID_STRING, 0 },    // string
  {  7, "sonm", MP4TAG_ID_STRING, 0 },    // string
  {  7, "sosn", MP4TAG_ID_STRING, 0 },    // string (tv show sort)
  {  6, "stik", MP4TAG_ID_NUM, 1 },       // 1 byte  (media type)
  {  5, "tmpo", MP4TAG_ID_NUM, 2 },       // 2 byte
  {  4, "trkn", MP4TAG_ID_DATA, 8 },      // data 4+2+2-unused
  {  7, "tven", MP4TAG_ID_STRING, 0 },    // string (tv episode name)
  {  6, "tves", MP4TAG_ID_NUM, 4 },       // 4 byte (tv episode)
  {  7, "tvnn", MP4TAG_ID_STRING, 0 },    // string (tv network name)
  {  7, "tvsh", MP4TAG_ID_STRING, 0 },    // string (tv show name)
  {  6, "tvsn", MP4TAG_ID_NUM, 4 },       // 4 byte (tv season)
  {  1, "©ART", MP4TAG_ID_STRING, 0 },    // string (artist)
  {  2, "©alb", MP4TAG_ID_STRING, 0 },    // string (album)
  {  7, "©cmt", MP4TAG_ID_STRING, 0 },    // string (comment)
  {  5, "©day", MP4TAG_ID_STRING, 0 },    // string (year)
  {  7, "©dir", MP4TAG_ID_STRING, 0 },    // string (director)
  {  3, "©gen", MP4TAG_ID_STRING, 0 },    // string (custom genre)
  {  7, "©grp", MP4TAG_ID_STRING, 0 },    // string (grouping)
  {  9, "©lyr", MP4TAG_ID_STRING, 0 },    // string (lyrics)
  {  6, "©mvc", MP4TAG_ID_NUM, 2 },       // 2-byte (movement total)
  {  6, "©mvi", MP4TAG_ID_NUM, 2 },       // 2-byte (movement number)
  {  7, "©mvn", MP4TAG_ID_STRING, 0 },    // string (movement name)
  {  0, "©nam", MP4TAG_ID_STRING, 0 },    // string (title)
  {  7, "©nrt", MP4TAG_ID_STRING, 0 },
  {  7, "©pub", MP4TAG_ID_STRING, 0 },
  {  5, "©too", MP4TAG_ID_STRING, 0 },    // string
  {  7, "©wrk", MP4TAG_ID_STRING, 0 },    // string
  {  2, "©wrt", MP4TAG_ID_STRING, 0 },    // string
};

const int mp4taglistlen = sizeof (mp4taglist) / sizeof (mp4tagdef_t);

/* an idiotic way to do things, */
/* but we must convert any old gnre data to -gen. */
/* itunes still puts data into the gnre field, yick. */
const char *oldgenrelist [] = {
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

const int oldgenrelistsz = sizeof (oldgenrelist) / sizeof (const char *);

