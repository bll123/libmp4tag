/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
// #include <assert.h>

#include "libmp4tag.h"
#include "libmp4tagint.h"

enum {
  LEVEL_MAX = 300,
};

typedef struct {
  uint32_t    len;
  char        nm [MP4TAG_ID_LEN];
} boxhead_t;

typedef struct {
  uint64_t    len;
  char        nm [10];
  uint64_t    dlen;
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

static void process_mdhd (libmp4tag_t *libmp4tag, const char *data);
static void process_tag (libmp4tag_t *libmp4tag, const char *nm, size_t blen, const char *data);

void
mp4tag_parse_file (libmp4tag_t *libmp4tag)
{
  boxhead_t       bh;
  boxdata_t       bd;
  size_t          rrc;
  size_t          skiplen;
  bool            needdata = false;
  bool            processdata = false;
  bool            done = false;
  bool            inclevel = false;
  int             level = 0;
  size_t          currlen [LEVEL_MAX];
  size_t          usedlen [LEVEL_MAX];

  // assert (sizeof (boxhead_t) == 8);
  // assert (sizeof (boxmdhd4_t) == 24);
  // assert (sizeof (boxmdhd8pack_t) == 36);

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
      strcpy (bd.nm, PREFIX_STR);
      memcpy (bd.nm + strlen (PREFIX_STR), bh.nm + 1, MP4TAG_ID_LEN - 1);
      bd.nm [MP4TAG_ID_LEN + strlen (PREFIX_STR) - 1] = '\0';
    } else {
      memcpy (bd.nm, bh.nm, MP4TAG_ID_LEN);
      bd.nm [MP4TAG_ID_LEN] = '\0';
    }

    bd.data = NULL;
    skiplen = bd.len;
    needdata = false;
    inclevel = false;

// fprintf (stdout, "%*s %2d %.5s: %8ld %08lx\n", level*2, " ", level, bd.nm, bd.len, bd.len);

    /* track the current level's length */
    currlen [level] = bd.len;
    usedlen [level] = 0;

//        strcmp (bd.nm, "stbl") == 0 ||
//        strcmp (bd.nm, "minf") == 0
    /* to process a heirarchy, set the skiplen to the size of any */
    /* data associated with the current box. */
    if (strcmp (bd.nm, MP4TAG_MOOV) == 0 ||
        strcmp (bd.nm, MP4TAG_TRAK) == 0 ||
        strcmp (bd.nm, MP4TAG_UDTA) == 0 ||
        strcmp (bd.nm, MP4TAG_MDIA) == 0 ||
        strcmp (bd.nm, MP4TAG_ILST) == 0) {
      /* want to descend into this hierarchy */
      /* there is no data associated, don't need to skip anything */
      skiplen = 0;
      inclevel = true;
    }
    if (strcmp (bd.nm, MP4TAG_META) == 0) {
      /* want to descend into this hierarchy */
      /* skip the 4 bytes of flags */
      skiplen = sizeof (uint32_t);
      inclevel = true;
    }
    if (strcmp (bd.nm, MP4TAG_MDHD) == 0) {
      needdata = true;
    }
    if (processdata) {
      needdata = true;
    }

    if (needdata && bd.len > 0) {
      if (strcmp (bd.nm, MP4TAG_ILST) == 0) {
        libmp4tag->taglist_begin = ftell (libmp4tag->fh);
      }
      bd.data = malloc (bd.len);
      rrc = fread (bd.data, bd.len, 1, libmp4tag->fh);
      if (rrc != 1) {
        fprintf (stderr, "failed to read %" PRId64 " bytes\n", bd.len);
      }
    }
    if (! needdata && skiplen > 0) {
      rrc = fseek (libmp4tag->fh, skiplen, SEEK_CUR);
      if (rrc != 0) {
        fprintf (stderr, "failed to seek %" PRId64 " bytes\n", bd.len);
      }
    }

    if (needdata && bd.data != NULL && bd.len > 0) {
      if (strcmp (bd.nm, MP4TAG_MDHD) == 0) {
        process_mdhd (libmp4tag, bd.data);
      }
      if (processdata) {
        process_tag (libmp4tag, bd.nm, bd.len, bd.data);
      }
      free (bd.data);
    }

    if (processdata && strcmp (bd.nm, MP4TAG_DATA) == 0) {
      inclevel = false;
    }

    if (strcmp (bd.nm, MP4TAG_ILST) == 0) {
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
        /* out of ilst, do not process more tags */
        /* and don't need to process anything else at all */
        if (processdata) {
          done = true;
          processdata = false;
        }
      }
    }

    if (done) {
      break;
    }

    inclevel = false;
    rrc = fread (&bh, sizeof (boxhead_t), 1, libmp4tag->fh);
  }
}

int
mp4tag_parse_ftyp (libmp4tag_t *libmp4tag)
{
  int         ok = 0;
  int         rrc;
  uint32_t    idx;
  uint32_t    len;
  boxhead_t   bh;
  char        *buff;

  rrc = fread (&bh, sizeof (boxhead_t), 1, libmp4tag->fh);
  if (rrc == 1) {
    /* the total length includes the length and the identifier */
    len = be32toh (bh.len) - sizeof (boxhead_t);
    if (memcmp (bh.nm, MP4TAG_FTYP, MP4TAG_ID_LEN) != 0) {
      return -1;
    }
    ++ok;

    buff = malloc (len);
    if (buff == NULL) {
      return -1;
    }
    rrc = fread (buff, len, 1, libmp4tag->fh);

    idx = 0;
    while (idx < len && rrc == 1) {
      if (idx == 0) {
        /* major brand, generally M4A */
        memcpy (libmp4tag->maintype, buff + idx, 4);
        libmp4tag->maintype [4] = '\0';
        /* ran into a .m4a file where mp42 was put into the maintype field */
	/* may as well check for mp41 also */
        if (strcmp (libmp4tag->maintype, "M4A ") == 0 ||
            strcmp (libmp4tag->maintype, "M4V ") == 0 ||
            strcmp (libmp4tag->maintype, "mp41") == 0 ||
            strcmp (libmp4tag->maintype, "mp42") == 0) {
          ++ok;
        }
      }
      if (idx == 4) {
        /* version */
        uint32_t    vers;

        memcpy (&vers, buff + idx, sizeof (uint32_t));
        vers = be32toh (vers);
        if (((vers & 0x0000ff00) >> 8) == 0x02) {
          ++ok;
        }
      }
      if (idx >= 8) {
        if (memcmp (buff + idx, "mp41", 4) == 0 ||
            memcmp (buff + idx, "mp42", 4) == 0) {
          memcpy (libmp4tag->mp4version, buff + idx, 4);
          libmp4tag->mp4version [4] = '\0';
          ++ok;
        }
        if (memcmp (buff + idx, "M4A ", 4) == 0) {
          /* aac audio w/itunes info */
          // fprintf (stdout, "== m4a \n");
          ++ok;
        }
        if (memcmp (buff + idx, "M4B ", 4) == 0) {
          /* aac audio w/itunes position */
          // fprintf (stdout, "== m4b \n");
          ++ok;
        }
        if (memcmp (buff + idx, "M4P ", 4) == 0) {
          /* aes encrypted audio */
          // fprintf (stdout, "== m4p \n");
        }
        if (memcmp (buff + idx, "mp71", 4) == 0 ||
            memcmp (buff + idx, "mp7b", 4) == 0) {
          /* mpeg-7 meta data */
          fprintf (stdout, "== mpeg-7 meta data\n");
          libmp4tag->mp7meta = true;
        }
        /* isom, iso2, qt, avc1, 3gp, mmp4 */
      }
      idx += 4;
    }

    free (buff);
  }

  return ok >= 3 ? 0 : -1;
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
    mdhd8.flags = be32toh (mdhd.flags);
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
}

static void
process_tag (libmp4tag_t *libmp4tag, const char *nm, size_t blen, const char *data)
{
  const char  *p;
  char        tnm [MP4TAG_ID_MAX];
  uint32_t    tflag;
  uint8_t     t8;
  uint16_t    t16;
  uint32_t    t32;
  uint64_t    t64;
  uint32_t    tlen;
  char        tmp [40];

  p = data;

  if (strcmp (nm, MP4TAG_FREE) == 0) {
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
    p += MP4TAG_ID_LEN;
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
    p += MP4TAG_ID_LEN;
    p += sizeof (uint32_t);
    memcpy (tnm + len, p, tlen);
    len += tlen;
    tnm [len] = '\0';
    p += tlen;
  }

  memcpy (&tlen, p, sizeof (uint32_t));
  tlen = be32toh (tlen);
  tlen -= sizeof (boxhead_t);
  tlen -= sizeof (uint32_t);  // flags
  tlen -= sizeof (uint32_t);  // reserved
  /* ident len + ident (== "data") */
  p += sizeof (uint32_t);
  p += MP4TAG_ID_LEN;
  memcpy (&tflag, p, sizeof (uint32_t));
  tflag = be32toh (tflag);
  tflag = tflag & 0x00ffffff;
  /* 4 bytes flags + reserved value */
  p += sizeof (uint32_t) + sizeof (uint32_t);

// fprintf (stdout, "  %s %02x %d\n", tnm, tflag, (int) tlen);

  /* general data */
  if (tflag == MP4TAG_ID_DATA ||
      tflag == MP4TAG_ID_NUM) {
    /* what follows depends on the identifier */

    if (strcmp (tnm, MP4TAG_COVR) == 0) {
      /* a cover image without a proper identifier, mark as jpeg */
      mp4tag_add_tag (libmp4tag, tnm, p, tlen, MP4TAG_ID_JPG, tlen);
    } else if (strcmp (tnm, MP4TAG_DISK) == 0 ||
        strcmp (tnm, MP4TAG_TRKN) == 0) {
      /* pair of 32 bit and 16 bit numbers */
      memcpy (&t32, p, sizeof (uint32_t));
      t32 = be32toh (t32);
      p += sizeof (uint32_t);
      memcpy (&t16, p, sizeof (uint16_t));
      t16 = be16toh (t16);
      /* trkn has an additional two trailing bytes */

      snprintf (tmp, sizeof (tmp), "(%d, %d)", (int) t32, (int) t16);
      mp4tag_add_tag (libmp4tag, tnm, tmp, MP4TAG_STRING, tflag, tlen);
    } else if (tlen == 4) {
      memcpy (&t32, p, sizeof (uint32_t));
      t32 = be32toh (t32);
      snprintf (tmp, sizeof (tmp), "%d", (int) t32);
      mp4tag_add_tag (libmp4tag, tnm, tmp, MP4TAG_STRING, tflag, tlen);
    } else if (tlen == 2) {
      memcpy (&t16, p, sizeof (uint16_t));
      t16 = be16toh (t16);
      if (strcmp (tnm, MP4TAG_GNRE) == 0) {
        /* the itunes value is offset by 1 */
        t16 -= 1;
        if (t16 < oldgenrelistsz) {
          /* do not use the 'gnre' identifier */
          strcpy (tnm, PREFIX_STR MP4TAG_GEN);
          mp4tag_add_tag (libmp4tag, tnm, oldgenrelist [t16],
              MP4TAG_STRING, MP4TAG_ID_STRING, strlen (oldgenrelist [t16]));
        }
      } else {
        snprintf (tmp, sizeof (tmp), "%d", (int) t16);
        mp4tag_add_tag (libmp4tag, tnm, tmp, MP4TAG_STRING, tflag, tlen);
      }
    } else if (tlen == 8) {
      memcpy (&t64, p, sizeof (uint64_t));
      t64 = be64toh (t64);
      snprintf (tmp, sizeof (tmp), "%" PRId64, t64);
      mp4tag_add_tag (libmp4tag, tnm, tmp, MP4TAG_STRING, tflag, tlen);
    } else if (tlen == 1) {
      memcpy (&t8, p, sizeof (uint8_t));
      snprintf (tmp, sizeof (tmp), "%d", (int) t8);
      mp4tag_add_tag (libmp4tag, tnm, tmp, MP4TAG_STRING, tflag, tlen);
    } else {
      /* binary data */
      mp4tag_add_tag (libmp4tag, tnm, p, tlen, tflag, tlen);
    }
  }

  if (tflag == MP4TAG_ID_JPG) {
    /* jpeg */
    mp4tag_add_tag (libmp4tag, tnm, p, tlen, tflag, tlen);
  }
  if (tflag == MP4TAG_ID_PNG) {
    /* png */
    mp4tag_add_tag (libmp4tag, tnm, p, tlen, tflag, tlen);
  }

  /* string type */
  if (tflag == MP4TAG_ID_STRING && tlen > 0) {
    /* pass as negative len to indicate a string that needs a terminator */
    mp4tag_add_tag (libmp4tag, tnm, p, - (ssize_t) tlen, tflag, tlen);
  }

  mp4tag_sort_tags (libmp4tag);
}
