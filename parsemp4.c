/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <assert.h>

#include <endian.h>

#include "libmp4tag.h"
#include "libmp4tagint.h"

enum {
  IDENT_LEN = 4,
  LEVEL_MAX = 300,
};

#define COPYRIGHT_STR   "\xc2\xa9"

typedef struct {
  uint32_t    len;
  char        nm [IDENT_LEN];
} boxhead_t;

typedef struct {
  int         len;
  char        nm [10];
  int         dlen;
  char        *data;
} boxdata_t;

typedef struct {
  uint32_t    flags;
} boxmdhd_t;

typedef struct {
  uint32_t    flags;
  uint32_t    creationdate;
  uint32_t    modifieddate;
  uint32_t    timescale;
  uint32_t    duration;
  uint32_t    moreflags;
} boxmdhd4_t;

typedef struct {
  uint32_t    flags;
  uint64_t    creationdate;
  uint64_t    modifieddate;
  uint32_t    timescale;
  uint64_t    duration;
  uint32_t    moreflags;
} __attribute__((packed)) boxmdhd8pack_t;

typedef struct {
  uint32_t    flags;
  uint64_t    creationdate;
  uint64_t    modifieddate;
  uint32_t    timescale;
  uint64_t    duration;
  uint32_t    moreflags;
} boxmdhd8_t;

/* an idiotic way to do things, */
/* but we must convert any old gnre data to -gnr. */
/* itunes still puts data into the gnre field */
static const char *oldgenrelist [] = {
  "Blues",
  "Classic Rock",
  "Country",
  "Dance",
  "Disco",
  "Funk",
  "Grunge",
  "Hip-Hop",
  "Jazz",
  "Metal",
  "New Age",
  "Oldies",
  "Other",
  "Pop",
  "R&B",
  "Rap",
  "Reggae",
  "Rock",
  "Techno",
  "Industrial",
  "Alternative",
  "Ska",
  "Death Metal",
  "Pranks",
  "Soundtrack",
  "Euro-Techno",
  "Ambient",
  "Trip-Hop",
  "Vocal",
  "Jazz+Funk",
  "Fusion",
  "Trance",
  "Classical",
  "Instrumental",
  "Acid",
  "House",
  "Game",
  "Sound Clip",
  "Gospel",
  "Noise",
  "Alt. Rock",
  "Bass",
  "Soul",
  "Punk",
  "Space",
  "Meditative",
  "Instrumental Pop",
  "Instrumental Rock",
  "Ethnic",
  "Gothic",
  "Darkwave",
  "Techno-Industrial",
  "Electronic",
  "Pop-Folk",
  "Eurodance",
  "Dream",
  "Southern Rock",
  "Comedy",
  "Cult",
  "Gangsta Rap",
  "Top 40",
  "Christian Rap",
  "Pop/Funk",
  "Jungle",
  "Native American",
  "Cabaret",
  "New Wave",
  "Psychedelic",
  "Rave",
  "Showtunes",
  "Trailer",
  "Lo-Fi",
  "Tribal",
  "Acid Punk",
  "Acid Jazz",
  "Polka",
  "Retro",
  "Musical",
  "Rock & Roll",
  "Hard Rock",
  "Folk",
  "Folk-Rock",
  "National Folk",
  "Swing",
  "Fast-Fusion",
  "Bebop",
  "Latin",
  "Revival",
  "Celtic",
  "Bluegrass",
  "Avantgarde",
  "Gothic Rock",
  "Progressive Rock",
  "Psychedelic Rock",
  "Symphonic Rock",
  "Slow Rock",
  "Big Band",
  "Chorus",
  "Easy Listening",
  "Acoustic",
  "Humour",
  "Speech",
  "Chanson",
  "Opera",
  "Chamber Music",
  "Sonata",
  "Symphony",
  "Booty Bass",
  "Primus",
  "Porn Groove",
  "Satire",
  "Slow Jam",
  "Club",
  "Tango",
  "Samba",
  "Folklore",
  "Ballad",
  "Power Ballad",
  "Rhythmic Soul",
  "Freestyle",
  "Duet",
  "Punk Rock",
  "Drum Solo",
  "A Cappella",
  "Euro-House",
  "Dance Hall",
  "Goa",
  "Drum & Bass",
  "Club-House",
  "Hardcore",
  "Terror",
  "Indie",
  "BritPop",
  "Afro-Punk",
  "Polsk Punk",
  "Beat",
  "Christian Gangsta Rap",
  "Heavy Metal",
  "Black Metal",
  "Crossover",
  "Contemporary Christian",
  "Christian Rock",
  "Merengue",
  "Salsa",
  "Thrash Metal",
  "Anime",
  "JPop",
  "Synthpop",
  "Abstract",
  "Art Rock",
  "Baroque",
  "Bhangra",
  "Big Beat",
  "Breakbeat",
  "Chillout",
  "Downtempo",
  "Dub",
  "EBM",
  "Eclectic",
  "Electro",
  "Electroclash",
  "Emo",
  "Experimental",
  "Garage",
  "Global",
  "IDM",
  "Illbient",
  "Industro-Goth",
  "Jam Band",
  "Krautrock",
  "Leftfield",
  "Lounge",
  "Math Rock",
  "New Romantic",
  "Nu-Breakz",
  "Post-Punk",
  "Post-Rock",
  "Psytrance",
  "Shoegaze",
  "Space Rock",
  "Trop Rock",
  "World Music",
  "Neoclassical",
  "Audiobook",
  "Audio Theatre",
  "Neue Deutsche Welle",
  "Podcast",
  "Indie Rock",
  "G-Funk",
  "Dubstep",
  "Garage Rock",
  "Psybient",
};

enum {
  oldgenrelistsz = sizeof (oldgenrelist) / sizeof (const char *),
};

static void process_mdhd (libmp4tag_t *libmp4tag, const char *data);
static void process_tag (libmp4tag_t *libmp4tag, const char *nm, const char *data);

void
parsemp4 (libmp4tag_t *libmp4tag)
{
  boxhead_t       bh;
  boxdata_t       bd;
  size_t          rrc;
  size_t          skiplen;
  bool            needdata = false;
  bool            processdata = false;
  bool            inclevel = false;
  int             level = 0;
  size_t          currlen [LEVEL_MAX];
  size_t          usedlen [LEVEL_MAX];

  assert (sizeof (boxhead_t) == 8);
  assert (sizeof (boxmdhd4_t) == 24);
  assert (sizeof (boxmdhd8pack_t) == 36);

  for (int i = 0; i < LEVEL_MAX; ++i) {
    currlen [i] = 0;
    usedlen [i] = 0;
  }

  rrc = fread (&bh, sizeof (boxhead_t), 1, libmp4tag->fh);
  while (! feof (libmp4tag->fh) && rrc == 1) {
    /* the total length includes the length and the identifier */
    bd.len = be32toh (bh.len) - sizeof (boxhead_t);
    if (*bh.nm == '\xa9') {
      /* maximum 5 bytes */
      strcpy (bd.nm, COPYRIGHT_STR);
      memcpy (bd.nm + strlen (COPYRIGHT_STR), bh.nm + 1, IDENT_LEN - 1);
      bd.nm [IDENT_LEN + strlen (COPYRIGHT_STR) - 1] = '\0';
    } else {
      memcpy (bd.nm, bh.nm, IDENT_LEN);
      bd.nm [IDENT_LEN] = '\0';
    }

    bd.data = NULL;
    skiplen = bd.len;
    needdata = false;
    inclevel = false;

    // fprintf (stdout, "%*s %2d %.5s: %8d\n", level*2, " ", level, bd.nm, bd.len);

    /* track the current level's length */
    currlen [level] = bd.len;
    usedlen [level] = 0;

//        strcmp (bd.nm, "stbl") == 0 ||
//        strcmp (bd.nm, "minf") == 0
    /* to process an heirarchy, set the skiplen to the size of any */
    /* data associated with the current box. */
    if (strcmp (bd.nm, "moov") == 0 ||
        strcmp (bd.nm, "trak") == 0 ||
        strcmp (bd.nm, "udta") == 0 ||
        strcmp (bd.nm, "mdia") == 0 ||
        strcmp (bd.nm, "ilst") == 0) {
      /* want to descend into this hierarchy */
      /* there is no data associated, don't need to skip anything */
      skiplen = 0;
      inclevel = true;
    }
    if (strcmp (bd.nm, "meta") == 0) {
      /* want to descend into this hierarchy */
      /* skip the 4 bytes of flags */
      skiplen = 4;
      inclevel = true;
    }
    if (strcmp (bd.nm, "mdhd") == 0) {
      needdata = true;
    }
    if (processdata) {
      needdata = true;
    }

    if (needdata && bd.len > 0) {
      if (strcmp (bd.nm, "ilst") == 0) {
        libmp4tag->taglist_begin = ftell (libmp4tag->fh);
      }
      bd.data = malloc (bd.len);
      rrc = fread (bd.data, bd.len, 1, libmp4tag->fh);
      if (rrc != 1) {
        fprintf (stderr, "failed to read %d bytes\n", bd.len);
      }
    }
    if (! needdata && skiplen > 0) {
      rrc = fseek (libmp4tag->fh, skiplen, SEEK_CUR);
      if (rrc != 0) {
        fprintf (stderr, "failed to seek %d bytes\n", bd.len);
      }
    }

    if (needdata && bd.data != NULL && bd.len > 0) {
      if (strcmp (bd.nm, "mdhd") == 0) {
        process_mdhd (libmp4tag, bd.data);
      }
      if (processdata) {
        process_tag (libmp4tag, bd.nm, bd.data);
      }
    }

    if (processdata && strcmp (bd.nm, "data") == 0) {
      inclevel = false;
    }

    if (strcmp (bd.nm, "ilst") == 0) {
      processdata = true;
    }

    if (inclevel) {
      ++level;
    }

    if (! inclevel && level > 0) {
      int     plevel;

      plevel = level - 1;

      usedlen [plevel] += sizeof (boxhead_t);
      usedlen [plevel] += bd.len;

      while (currlen [plevel] == usedlen [plevel]) {
        --level;
        plevel = level - 1;
        if (plevel > 0) {
          usedlen [plevel] += sizeof (boxhead_t);
          usedlen [plevel] += usedlen [level];
        }
      }
    }

    inclevel = false;
    rrc = fread (&bh, sizeof (boxhead_t), 1, libmp4tag->fh);
  }
}

static void
process_mdhd (libmp4tag_t *libmp4tag, const char *data)
{
  boxmdhd_t       mdhd;
  boxmdhd4_t      mdhd4;
  boxmdhd8pack_t  mdhd8p;
  boxmdhd8_t      mdhd8;

  memcpy (&mdhd, data, sizeof (boxmdhd_t));
  mdhd.flags = be32toh (mdhd.flags);
  if ((mdhd.flags & 0xff000000) == 0) {
    memcpy (&mdhd4, data, sizeof (boxmdhd4_t));
    mdhd8.flags = be32toh (mdhd4.flags);
    mdhd8.creationdate = be32toh (mdhd4.creationdate);
    mdhd8.modifieddate = be32toh (mdhd4.modifieddate);
    mdhd8.timescale = be32toh (mdhd4.timescale);
    mdhd8.duration = be32toh (mdhd4.duration);
    mdhd8.moreflags = be32toh (mdhd4.moreflags);
  } else {
    memcpy (&mdhd8p, data, sizeof (boxmdhd8pack_t));
    memcpy (&mdhd8.creationdate, &mdhd8p.creationdate, sizeof (mdhd8.creationdate));
    memcpy (&mdhd8.modifieddate, &mdhd8p.modifieddate, sizeof (mdhd8.modifieddate));
    memcpy (&mdhd8.duration, &mdhd8p.duration, sizeof (mdhd8.duration));
    memcpy (&mdhd8.timescale, &mdhd8p.timescale, sizeof (mdhd8.timescale));
    memcpy (&mdhd8.moreflags, &mdhd8p.moreflags, sizeof (mdhd8.moreflags));
    mdhd8.flags = be32toh (mdhd8.flags);
    mdhd8.creationdate = be64toh (mdhd8.creationdate);
    mdhd8.modifieddate = be64toh (mdhd8.modifieddate);
    mdhd8.timescale = be64toh (mdhd8.timescale);
    mdhd8.duration = be64toh (mdhd8.duration);
    mdhd8.moreflags = be32toh (mdhd8.moreflags);
  }
  libmp4tag->creationdate = mdhd8.creationdate;
  libmp4tag->modifieddate = mdhd8.modifieddate;
  libmp4tag->samplerate = mdhd8.timescale;
  libmp4tag->duration = (int64_t)
      ((double) mdhd8.duration * 1000.0 / (double) mdhd8.timescale);
  fprintf (stdout, "duration=%ld\n", (long) libmp4tag->duration);
  fprintf (stdout, "samplerate=%d\n", libmp4tag->samplerate);
}

static void
process_tag (libmp4tag_t *libmp4tag, const char *nm, const char *data)
{
  const char  *p;
  char        tnm [100];
  uint32_t    tflag;
  uint8_t     t8;
  uint16_t    t16;
  uint32_t    t32;
  uint64_t    t64;
  uint32_t    tlen;

  p = data;

  if (strcmp (nm, "free") == 0) {
    memcpy (&tlen, p, sizeof (uint32_t));
    tlen = be32toh (tlen);
    fprintf (stdout, "  free block: %d\n", (int) tlen);
    return;
  }

  strcpy (tnm, nm);
  if (strcmp (nm, "----") == 0) {
    size_t    len;

    strcpy (tnm, nm);
    len = strlen (nm);
    tnm [len++] = ':';
    tnm [len] = '\0';

    memcpy (&tlen, p, sizeof (uint32_t));
    tlen = be32toh (tlen);
    tlen -= sizeof (boxhead_t);
    tlen -= sizeof (uint32_t);
    /* what does 'mean' stand for? i would call it 'vendor' */
    /* ident len + ident (== "mean") + 4 bytes flags */
    p += sizeof (uint32_t);
    p += IDENT_LEN;
    p += sizeof (uint32_t);
    memcpy (tnm + len, p, tlen);
    len += tlen;
    tnm [len++] = ':';
    tnm [len] = '\0';
    p += tlen;

    memcpy (&tlen, p, sizeof (uint32_t));
    tlen = be32toh (tlen);
    tlen -= sizeof (boxhead_t);
    tlen -= sizeof (uint32_t);
    /* ident len + ident (== "name") + 4 bytes flags */
    p += sizeof (uint32_t);
    p += IDENT_LEN;
    p += sizeof (uint32_t);
    memcpy (tnm + len, p, tlen);
    len += tlen;
    tnm [len] = '\0';
    p += tlen;
  }

// ### search for tnm in tagdefs

  memcpy (&tlen, p, sizeof (uint32_t));
  tlen = be32toh (tlen);
  tlen -= sizeof (boxhead_t);
  tlen -= sizeof (uint32_t);  // flags
  tlen -= sizeof (uint32_t);  // reserved
// fprintf (stdout, "%s len: %d\n", tnm, (int) tlen);
  /* ident len + ident (== "data") */
  p += sizeof (uint32_t);
  p += IDENT_LEN;
  memcpy (&tflag, p, sizeof (uint32_t));
  tflag = be32toh (tflag);
// fprintf (stdout, "== flag: %08x\n", tflag);
  /* 4 bytes flags + reserved value */
  p += sizeof (uint32_t) + sizeof (uint32_t);

  /* general data */
  if ((tflag & 0x00ffffff) == 0) {
    /* what follows depends on the identifier */

// fprintf (stdout, "parse: %s %d\n", tnm, (int) tlen);
    if (strcmp (tnm, "disk") == 0 ||
        strcmp (tnm, "trkn") == 0) {
// fprintf (stdout, "  pair\n");
      memcpy (&t32, p, sizeof (uint32_t));
      t32 = be32toh (t32);
      p += sizeof (uint32_t);
      memcpy (&t16, p, sizeof (uint16_t));
      t16 = be16toh (t16);
      fprintf (stdout, "%s=(%d, %d)\n", tnm, (int) t32, (int) t16);
      /* trkn has an additional two trailing bytes */
    } else if (tlen == 8) {
// fprintf (stdout, "  64\n");
      memcpy (&t64, p, sizeof (uint64_t));
      t64 = be64toh (t64);
      fprintf (stdout, "%s=%ld\n", tnm, (long) t64);
    } else if (tlen == 1) {
// fprintf (stdout, "  8\n");
      memcpy (&t8, p, sizeof (uint8_t));
      fprintf (stdout, "%s=%d\n", tnm, t8);
    } else if (tlen == 2) {
// fprintf (stdout, "  16\n");
      memcpy (&t16, p, sizeof (uint16_t));
      t16 = be16toh (t16);
      if (strcmp (tnm, "gnre") == 0) {
        /* the itunes value is offset by 1 */
	t16 -= 1;
	if (t16 <= oldgenrelistsz) {
          fprintf (stdout, "%s=%s\n", tnm, oldgenrelist [t16]);
	}
      } else {
        fprintf (stdout, "%s=%d\n", tnm, t16);
      }
    } else if (tlen == 1) {
// fprintf (stdout, "  8\n");
      memcpy (&t8, p, sizeof (uint8_t));
      fprintf (stdout, "  ");
      fprintf (stdout, "%s=%d\n", (int) t8);
    } else if (tlen == 4) {
// fprintf (stdout, "  32\n");
      memcpy (&t32, p, sizeof (uint32_t));
      t32 = be32toh (t32);
      fprintf (stdout, "%s=%d\n", (int) t32);
    } else {
      fprintf (stdout, "%s=(binary data)\n", tnm);
    }
  }

  /* picture */
  if ((tflag & 0x00ffffff) == 0x0d) {
    fprintf (stdout, "%s=(%ld)\n", tnm, (long) tlen);
  }
  if ((tflag & 0x00ffffff) == 0x0e) {
    fprintf (stdout, "%s=(%ld)\n", tnm, (long) tlen);
  }

  /* tempo, compilation (cpil), shwm, pgap */
  if ((tflag & 0x00ffffff) == 0x15) {
    if (tlen == 2) {
      memcpy (&t16, p, sizeof (uint16_t));
      t16 = be16toh (t16);
      fprintf (stdout, "%s=%d\n", tnm, (int) t16);
    } else if (tlen == 1) {
      memcpy (&t8, p, sizeof (uint8_t));
      fprintf (stdout, "%s=%d\n", tnm, (int) t8);
    } else {
      fprintf (stdout, "  unhandled len\n");
    }
  }

  /* string type */
  if ((tflag & 0x00ffffff) == 1) {
    fprintf (stdout, "%s=%.*s\n", tnm, (int) tlen, p);
  }
}
