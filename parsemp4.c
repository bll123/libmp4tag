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

static void process_mdhd (const char *data);
static void process_tag (const char *nm, const char *data);

void
parsemp4 (FILE *fh)
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

  rrc = fread (&bh, sizeof (boxhead_t), 1, fh);
  while (! feof (fh) && rrc == 1) {
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

    fprintf (stdout, "%*s %2d %.5s: %8d\n", level*2, " ", level, bd.nm, bd.len);

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
      bd.data = malloc (bd.len);
      rrc = fread (bd.data, bd.len, 1, fh);
      if (rrc != 1) {
        fprintf (stderr, "failed to read %d bytes\n", bd.len);
      }
    }
    if (! needdata && skiplen > 0) {
      rrc = fseek (fh, skiplen, SEEK_CUR);
      if (rrc != 0) {
        fprintf (stderr, "failed to seek %d bytes\n", bd.len);
      }
    }

    if (needdata && bd.data != NULL && bd.len > 0) {
      if (strcmp (bd.nm, "mdhd") == 0) {
        process_mdhd (bd.data);
      }
      if (processdata) {
        process_tag (bd.nm, bd.data);
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
    rrc = fread (&bh, sizeof (boxhead_t), 1, fh);
  }
}

static void
process_mdhd (const char *data)
{
  boxmdhd_t       mdhd;
  boxmdhd4_t      mdhd4;
  boxmdhd8pack_t  mdhd8p;
  boxmdhd8_t      mdhd8;

  memcpy (&mdhd, data, sizeof (boxmdhd_t));
  if ((mdhd.flags & 0xff000000) == 0) {
    memcpy (&mdhd4, data, sizeof (boxmdhd4_t));
    mdhd8.creationdate = be32toh (mdhd4.creationdate);
    mdhd8.modifieddate = be32toh (mdhd4.modifieddate);
    mdhd8.timescale = be32toh (mdhd4.timescale);
    mdhd8.duration = be32toh (mdhd4.duration);
    mdhd8.moreflags = mdhd4.moreflags;
  } else {
    memcpy (&mdhd8p, data, sizeof (boxmdhd8pack_t));
    memcpy (&mdhd8.creationdate, &mdhd8p.creationdate, sizeof (mdhd8.creationdate));
    memcpy (&mdhd8.modifieddate, &mdhd8p.modifieddate, sizeof (mdhd8.modifieddate));
    memcpy (&mdhd8.duration, &mdhd8p.duration, sizeof (mdhd8.duration));
    memcpy (&mdhd8.timescale, &mdhd8p.timescale, sizeof (mdhd8.timescale));
    memcpy (&mdhd8.moreflags, &mdhd8p.moreflags, sizeof (mdhd8.moreflags));
    mdhd8.creationdate = be64toh (mdhd8p.creationdate);
    mdhd8.modifieddate = be64toh (mdhd8p.modifieddate);
    mdhd8.timescale = be64toh (mdhd8p.timescale);
    mdhd8.duration = be64toh (mdhd8p.duration);
  }
  fprintf (stdout, "  ");
  fprintf (stdout, "moddate: %ld ", (long) mdhd8.modifieddate);
  fprintf (stdout, "timescale: %ld ", (long) mdhd8.timescale);
  fprintf (stdout, "duration: %.0f ",
      (double) mdhd8.duration * 1000.0 / (double) mdhd8.timescale);
  fprintf (stdout, "\n");
}

static void
process_tag (const char *nm, const char *data)
{
  const char  *p;

// ### bsearch tagdefs for the correct entry. (tagdef.c)
// ### check for string

  if (strcmp (nm, COPYRIGHT_STR "too") == 0 ||
      strcmp (nm, "aART") == 0 ||
      strcmp (nm, COPYRIGHT_STR "nam" ) == 0 ||
      strcmp (nm, COPYRIGHT_STR "alb" ) == 0 ||
      strcmp (nm, COPYRIGHT_STR "ART") == 0) {
    p = data;
    /* ident len + ident (== "data") */
    p += sizeof (int32_t);
    p += IDENT_LEN;
    /* 4 bytes flags + reserved value */
    p += 4 + sizeof (int32_t);
    fprintf (stdout, "  ");
    fprintf (stdout, "%s\n", p);
  }
}
