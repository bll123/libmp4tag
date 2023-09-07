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
#include <assert.h>

#include "libmp4tag.h"
#include "mp4tagint.h"
#include "mp4tagbe.h"

enum {
  LEVEL_MAX = 40,
};

typedef struct {
  uint32_t    len;
  char        nm [MP4TAG_ID_LEN];
} boxhead_t;

typedef struct {
  uint64_t    boxlen;
  uint64_t    len;
  char        nm [MP4TAG_ID_DISP_LEN];
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
static void mp4tag_process_mdhd (libmp4tag_t *libmp4tag, const char *data);
static void mp4tag_process_tag (libmp4tag_t *libmp4tag, const char *tag, uint32_t blen, const char *data);
static void mp4tag_process_covr (libmp4tag_t *libmp4tag, const char *tag, uint32_t blen, const char *data);
static void mp4tag_process_data (const char *p, uint32_t *tlen, uint32_t *flags);
static void mp4tag_parse_check_end (libmp4tag_t *libmp4tag);
static ssize_t mp4tag_get_curr_offset (libmp4tag_t *libmp4tag);
/* debugging */
static void mp4tag_dump_co (libmp4tag_t *libmp4tag, const char *ident, size_t len, const char *data);
static void mp4tag_dump_data (libmp4tag_t *libmp4tag, uint64_t offset);

int
mp4tag_parse_file (libmp4tag_t *libmp4tag, uint32_t boxlen, int level)
{
  boxhead_t       bh;
  boxdata_t       bd;
  size_t          rrc;
  uint32_t        skiplen;
  ssize_t         remaininglen;
  bool            needdata = false;
  bool            descend = false;

  assert (sizeof (boxhead_t) == 8);
  assert (sizeof (boxhead_t) == MP4TAG_BOXHEAD_SZ);
  assert (sizeof (boxmdhd4_t) == 24);
  assert (sizeof (boxmdhd8pack_t) == 36);

  remaininglen = boxlen;
  /* subtract the box's header size, as it is not included */
  /* within the container's contents */
  remaininglen -= MP4TAG_BOXHEAD_SZ;

  rrc = fread (&bh, MP4TAG_BOXHEAD_SZ, 1, libmp4tag->fh);
  if (rrc != 1) {
    libmp4tag->mp4error = MP4TAG_ERR_FILE_READ_ERROR;
  }

  while (! feof (libmp4tag->fh) && rrc == 1) {
    /* the total length includes the length and the identifier */
    bd.boxlen = be32toh (bh.len);
    bd.len = bd.boxlen - MP4TAG_BOXHEAD_SZ;
    if (boxlen == 0) {
      remaininglen = bd.boxlen;
    }

    /* save the name of the box */
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
    bd.dlen = 0;
    skiplen = bd.len;
    needdata = false;

    if (libmp4tag->dbgflags & MP4TAG_DBG_PRINT_FILE_STRUCTURE) {
      fprintf (stdout, "%*s %2d %.5s: %" PRId64 " %" PRId64 " (%" PRId64 ")\n", level*2, " ", level, bd.nm, bd.boxlen, bd.len, remaininglen);
    }

    descend = false;

    /* hierarchies used: */
    /*   moov.trak.mdia.mdhd  (has duration) */
    /*   moov.trak.mdia.minf.stbl.stco  (offset table to update) */
    /*   moov.trak.mdia.minf.stbl.co64  (offset table to update) */
    /*   moov.udta.meta.ilst.*  (tags) */
    if (strcmp (bd.nm, MP4TAG_MOOV) == 0 ||
        strcmp (bd.nm, MP4TAG_TRAK) == 0 ||
        strcmp (bd.nm, MP4TAG_UDTA) == 0 ||
        strcmp (bd.nm, MP4TAG_MDIA) == 0 ||
        strcmp (bd.nm, MP4TAG_STBL) == 0 ||
        strcmp (bd.nm, MP4TAG_MINF) == 0 ||
        strcmp (bd.nm, MP4TAG_ILST) == 0) {
      /* want to descend into this hierarchy */
      /* container only, don't need to skip over any data */
      descend = true;
      skiplen = 0;
    }
    if (strcmp (bd.nm, MP4TAG_META) == 0) {
      /* want to descend into this hierarchy */
      /* skip the 4 bytes of flags */
      skiplen = MP4TAG_META_SZ - MP4TAG_BOXHEAD_SZ;
      libmp4tag->insert_delta += MP4TAG_META_SZ;
      descend = true;
    }

    /* save off any offsets before any processing is done */

    if (strcmp (bd.nm, MP4TAG_UDTA) == 0) {
      /* need to save this offset in case there is no 'ilst' box */
      libmp4tag->noilst_offset = mp4tag_get_curr_offset (libmp4tag) -
          MP4TAG_BOXHEAD_SZ;
      libmp4tag->after_ilst_offset =
          libmp4tag->noilst_offset + MP4TAG_BOXHEAD_SZ;
      libmp4tag->insert_delta = MP4TAG_BOXHEAD_SZ;
    }

    if (strcmp (bd.nm, MP4TAG_ILST) == 0) {
      libmp4tag->parentidx = level - 1;
      libmp4tag->taglist_offset = mp4tag_get_curr_offset (libmp4tag);
      libmp4tag->taglist_base_offset =
          libmp4tag->taglist_offset - MP4TAG_BOXHEAD_SZ;
      /* do not include the ident-len and ident lengths */
      libmp4tag->taglist_orig_len = bd.len;
      libmp4tag->taglist_len = bd.len;
      libmp4tag->after_ilst_offset =
          libmp4tag->taglist_offset + libmp4tag->taglist_len;

      libmp4tag->processdata = true;
      if (bd.len == 0) {
        /* there are no tags */
        libmp4tag->processdata = false;
        libmp4tag->checkforfree = true;
      }
    }

    if (strcmp (bd.nm, MP4TAG_STCO) == 0) {
      libmp4tag->stco_offset = mp4tag_get_curr_offset (libmp4tag);
      libmp4tag->stco_len = bd.len;
      if (libmp4tag->dbgflags & MP4TAG_DBG_DUMP_CO) {
        needdata = true;
      }
    }

    if (strcmp (bd.nm, MP4TAG_CO64) == 0) {
      libmp4tag->co64_offset = mp4tag_get_curr_offset (libmp4tag);
      libmp4tag->co64_len = bd.len;
      if (libmp4tag->dbgflags & MP4TAG_DBG_DUMP_CO) {
        needdata = true;
      }
    }

    if (libmp4tag->checkforfree) {
      if (strcmp (bd.nm, MP4TAG_FREE) == 0) {
        libmp4tag->taglist_len += bd.boxlen;
        libmp4tag->after_ilst_offset += bd.boxlen;
        /* continue on and see if there are more 'free' boxes to add */
      } else {
        /* if this spot was reached, there is some other */
        /* box after the 'ilst' or 'free' boxes */
        /* unlimited will be false */
        libmp4tag->checkforfree = false;
        if (! (libmp4tag->dbgflags & MP4TAG_DBG_PRINT_FILE_STRUCTURE)) {
          break;
        }
      }
    }

    if (descend && bd.len > 0) {
      if (level < MP4TAG_BASE_OFF_MAX) {
        ssize_t      offset;

        offset = ftell (libmp4tag->fh);
        if (offset < 0) {
          libmp4tag->mp4error = MP4TAG_ERR_FILE_TELL_ERROR;
        }

        libmp4tag->base_lengths [level] = bd.boxlen;
        snprintf (libmp4tag->base_name [level], sizeof (libmp4tag->base_name [level]),
            "%s", bd.nm);
        libmp4tag->base_offsets [level] = offset - MP4TAG_BOXHEAD_SZ;
        // fprintf (stdout, "%*s %2d store base %s %d len:%ld offset:%08lx\n", level*2, " ", level, bd.nm, level, bd.len + MP4TAG_BOXHEAD_SZ, libmp4tag->base_offsets [level]);
        libmp4tag->base_offset_count = level + 1;
//        libmp4tag->parentidx = level;
      }

      if (skiplen > 0) {
        if (fseek (libmp4tag->fh, skiplen, SEEK_CUR) != 0) {
          libmp4tag->mp4error = MP4TAG_ERR_FILE_SEEK_ERROR;
        }
      }

      mp4tag_parse_file (libmp4tag, bd.boxlen - skiplen, level + 1);
      /* when descending, the box's data has already been skipped or read */
      skiplen = 0;
    }

    /* if descended into the hierarchy, now done */

    if (strcmp (bd.nm, MP4TAG_MOOV) == 0 &&
        libmp4tag->noilst_offset == 0) {
      libmp4tag->noilst_offset = mp4tag_get_curr_offset (libmp4tag);
      libmp4tag->after_ilst_offset = libmp4tag->noilst_offset;
    }

    /* out of 'ilst', do not process more tags */
    /* only need to check for any 'free' boxes trailing the 'ilst'.*/
    if (strcmp (bd.nm, MP4TAG_ILST) == 0) {
      libmp4tag->processdata = false;
      libmp4tag->checkforfree = true;    // not quite done yet
    }

    if (strcmp (bd.nm, MP4TAG_MDHD) == 0) {
      needdata = true;
    }
    if (libmp4tag->processdata) {
      needdata = true;
    }

    if (needdata && bd.len > 0) {
      bd.data = malloc (bd.len);
      if (bd.data == NULL) {
        libmp4tag->mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
        return libmp4tag->mp4error;
      }
      rrc = fread (bd.data, bd.len, 1, libmp4tag->fh);
      if (rrc != 1) {
        libmp4tag->mp4error = MP4TAG_ERR_FILE_READ_ERROR;
      }
    }
    if (! needdata && skiplen > 0) {
      rrc = fseek (libmp4tag->fh, skiplen, SEEK_CUR);
      if (rrc != 0) {
        libmp4tag->mp4error = MP4TAG_ERR_FILE_SEEK_ERROR;
      }
    }

    if (needdata && bd.data != NULL && bd.len > 0) {
      if (strcmp (bd.nm, MP4TAG_STCO) == 0 ||
          strcmp (bd.nm, MP4TAG_CO64) == 0) {
        /* debugging */
        mp4tag_dump_co (libmp4tag, bd.nm, bd.len, bd.data);
      }
      if (strcmp (bd.nm, MP4TAG_MDHD) == 0) {
        mp4tag_process_mdhd (libmp4tag, bd.data);
      }
      if (libmp4tag->processdata) {
        if (strcmp (bd.nm, MP4TAG_COVR) == 0) {
          mp4tag_process_covr (libmp4tag, bd.nm, bd.len, bd.data);
        } else {
          mp4tag_process_tag (libmp4tag, bd.nm, bd.len, bd.data);
        }
      }
      free (bd.data);
    }

    remaininglen -= bd.boxlen;
    if (libmp4tag->dbgflags & MP4TAG_DBG_PRINT_FILE_STRUCTURE) {
      fprintf (stdout, "%*s %2d %.5s: %" PRId64 " %" PRId64 " (%" PRId64 ") end\n", level*2, " ", level, bd.nm, bd.boxlen, bd.len, remaininglen);
    }

    if (remaininglen <= 0 && boxlen != 0) {
      return libmp4tag->mp4error;
    }

    rrc = fread (&bh, MP4TAG_BOXHEAD_SZ, 1, libmp4tag->fh);
  }

  if (level == 0) {
    if (libmp4tag->checkforfree) {
      /* if checkforfree is still true, check and see if the end of file */
      /* was reached */
      mp4tag_parse_check_end (libmp4tag);
    }

    mp4tag_sort_tags (libmp4tag);
  }

  return libmp4tag->mp4error;
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
  char        tmp [MP4TAG_ID_LEN + 1];

  rrc = fread (&bh, MP4TAG_BOXHEAD_SZ, 1, libmp4tag->fh);
  if (rrc != 1) {
    libmp4tag->mp4error = MP4TAG_ERR_FILE_READ_ERROR;
    return libmp4tag->mp4error;
  }

  /* the total length includes the length and the identifier */
  len = be32toh (bh.len) - MP4TAG_BOXHEAD_SZ;
  if (memcmp (bh.nm, MP4TAG_FTYP, MP4TAG_ID_LEN) != 0) {
    libmp4tag->mp4error = MP4TAG_ERR_NOT_MP4;
    return libmp4tag->mp4error;
  }
  ++ok;

  buff = malloc (len);
  if (buff == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
    return libmp4tag->mp4error;
  }
  rrc = fread (buff, len, 1, libmp4tag->fh);
  if (rrc != 1) {
    libmp4tag->mp4error = MP4TAG_ERR_FILE_READ_ERROR;
    return libmp4tag->mp4error;
  }

  idx = 0;
  while (idx < len && rrc == 1) {
    if (idx == 0) {
      /* major brand */
      memcpy (tmp, buff + idx, MP4TAG_ID_LEN);
      tmp [MP4TAG_ID_LEN] = '\0';
      if (strcmp (tmp, "M4A ") == 0 ||
          strcmp (tmp, "kddi") == 0 ||
          strcmp (tmp, "isom") == 0 ||
          strcmp (tmp, "mp41") == 0 ||
          strcmp (tmp, "mp42") == 0) {
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
      if (memcmp (buff + idx, "3g2a", 4) == 0) {
        /* I don't really know what this is */
        ++ok;
      }
      if (memcmp (buff + idx, "isom", 4) == 0) {
        /* generic iso media */
        ++ok;
      }
      /* isom, iso2, qt, avc1, 3gp, mmp4 */
    }
    idx += 4;
  }

  free (buff);

  return ok >= 3 ? MP4TAG_OK : MP4TAG_ERR_NOT_MP4;
}

static void
mp4tag_process_mdhd (libmp4tag_t *libmp4tag, const char *data)
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
    mdhd8.flags = be32toh (mdhd8p.flags);
    mdhd8.creationdate = be64toh (mdhd8.creationdate);
    mdhd8.modifieddate = be64toh (mdhd8.modifieddate);
    mdhd8.timescale = be32toh (mdhd8.timescale);
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
mp4tag_process_tag (libmp4tag_t *libmp4tag, const char *tag,
    uint32_t blen, const char *data)
{
  const char  *p;
  /* tnm must be large enough to hold any custom tag name */
  char        tnm [MP4TAG_ID_MAX];
  uint32_t    tflag;
  uint8_t     t8;
  uint16_t    t16;
  uint32_t    t32;
  uint64_t    t64;
  uint32_t    tlen;
  char        tmp [40];

  p = data;

  if (strcmp (tag, MP4TAG_FREE) == 0) {
    return;
  }

  snprintf (tnm, sizeof (tnm), "%s", tag);
  if (strcmp (tag, "----") == 0) {
    size_t    len;

    len = strlen (tnm);
    tnm [len++] = ':';
    tnm [len] = '\0';

    memcpy (&tlen, p, sizeof (uint32_t));
    tlen = be32toh (tlen);
    tlen -= MP4TAG_BOXHEAD_SZ;
    tlen -= sizeof (uint32_t);
    /* what does 'mean' stand for? I would call it 'vendor' */
    /* ident len + ident (== "mean") + 4 bytes flags */
    p += MP4TAG_BOXHEAD_SZ;
    p += sizeof (uint32_t);
    memcpy (tnm + len, p, tlen);
    len += tlen;
    tnm [len++] = ':';
    tnm [len] = '\0';
    p += tlen;

    memcpy (&tlen, p, sizeof (uint32_t));
    tlen = be32toh (tlen);
    tlen -= MP4TAG_BOXHEAD_SZ;
    tlen -= sizeof (uint32_t);
    /* ident len + ident (== "name") + 4 bytes flags */
    p += MP4TAG_BOXHEAD_SZ;
    p += sizeof (uint32_t);
    memcpy (tnm + len, p, tlen);
    len += tlen;
    tnm [len] = '\0';
    p += tlen;
  }

  mp4tag_process_data (p, &tlen, &tflag);
  p += MP4TAG_DATA_SZ;

// fprintf (stdout, "  %s %02x %d (%d)\n", tnm, tflag, (int) tlen, (int) blen);

  /* general data */
  if (tflag == MP4TAG_ID_DATA ||
      tflag == MP4TAG_ID_NUM) {

    /* 'disk' and 'trkn' must be handle as special cases. */
    /* they are marked as data (0x00). */
    if (strcmp (tnm, MP4TAG_DISK) == 0 ||
        strcmp (tnm, MP4TAG_TRKN) == 0) {
      /* pair of 32 bit and 16 bit numbers */
      memcpy (&t32, p, sizeof (uint32_t));
      t32 = be32toh (t32);
      p += sizeof (uint32_t);
      memcpy (&t16, p, sizeof (uint16_t));
      t16 = be16toh (t16);
      /* trkn has an additional two trailing bytes that are not used */

      if (t16 == 0) {
        snprintf (tmp, sizeof (tmp), "%d", (int) t32);
      } else {
        snprintf (tmp, sizeof (tmp), "%d/%d", (int) t32, (int) t16);
      }
      mp4tag_add_tag (libmp4tag, tnm, tmp, MP4TAG_STRING, tflag, tlen, NULL);
    } else if (tlen == 4) {
      memcpy (&t32, p, sizeof (uint32_t));
      t32 = be32toh (t32);
      snprintf (tmp, sizeof (tmp), "%d", (int) t32);
      mp4tag_add_tag (libmp4tag, tnm, tmp, MP4TAG_STRING, tflag, tlen, NULL);
    } else if (tlen == 2) {
      memcpy (&t16, p, sizeof (uint16_t));
      t16 = be16toh (t16);

      /* the 'gnre' tag is converted to '©gen' */
      /* hard-coded lists of genres are not good */
      if (strcmp (tnm, MP4TAG_GNRE) == 0) {
        /* the itunes value is offset by 1 */
        t16 -= 1;
        if (t16 < mp4tagoldgenrelistsz) {
          /* do not use the 'gnre' identifier */
          strcpy (tnm, PREFIX_STR MP4TAG_GEN);
          mp4tag_add_tag (libmp4tag, tnm, mp4tagoldgenrelist [t16],
              MP4TAG_STRING, MP4TAG_ID_STRING, strlen (mp4tagoldgenrelist [t16]), NULL);
        }
      } else {
        snprintf (tmp, sizeof (tmp), "%d", (int) t16);
        mp4tag_add_tag (libmp4tag, tnm, tmp, MP4TAG_STRING, tflag, tlen, NULL);
      }
    } else if (tlen == 8) {
      memcpy (&t64, p, sizeof (uint64_t));
      t64 = be64toh (t64);
      snprintf (tmp, sizeof (tmp), "%" PRId64, t64);
      mp4tag_add_tag (libmp4tag, tnm, tmp, MP4TAG_STRING, tflag, tlen, NULL);
    } else if (tlen == 1) {
      memcpy (&t8, p, sizeof (uint8_t));
      snprintf (tmp, sizeof (tmp), "%d", (int) t8);
      mp4tag_add_tag (libmp4tag, tnm, tmp, MP4TAG_STRING, tflag, tlen, NULL);
    } else {
      /* binary data */
      mp4tag_add_tag (libmp4tag, tnm, p, tlen, tflag, tlen, NULL);
    }
  }

  /* string type */
  if (tflag == MP4TAG_ID_STRING && tlen > 0) {
    /* pass as negative len to indicate a string that needs a terminator */
    mp4tag_add_tag (libmp4tag, tnm, p, - (ssize_t) tlen, tflag, tlen, NULL);
  }
}

/* 'covr' can have additional data */
/* there can be multiple images, and names present */
static void
mp4tag_process_covr (libmp4tag_t *libmp4tag, const char *tag,
    uint32_t blen, const char *data)
{
  uint32_t    tlen;
  uint32_t    clen = 0;
  uint32_t    type = MP4TAG_ID_JPG;
  const char  *p = data;
  int         cflag = 0;
  const char  *cdata = NULL;
  const char  *cname = NULL;

  while (blen > 0) {
    memcpy (&tlen, p, sizeof (uint32_t));
    tlen = be32toh (tlen);
    blen -= tlen;
    if (memcmp (p + sizeof (uint32_t), MP4TAG_DATA, MP4TAG_ID_LEN) == 0) {
      if (cflag > 0 && cdata != NULL) {
        mp4tag_add_tag (libmp4tag, MP4TAG_COVR, cdata, clen, type, clen, cname);
        cname = NULL;
        cdata = NULL;
        cflag = 0;
      }
      mp4tag_process_data (p, &tlen, &type);
      if (type == 0) {
        type = MP4TAG_ID_JPG;
      }
      p += MP4TAG_DATA_SZ;
      clen = tlen;
      cdata = p;
      ++cflag;
    } else if (memcmp (p + sizeof (uint32_t), MP4TAG_NAME, MP4TAG_ID_LEN) == 0) {
      cname = p + MP4TAG_BOXHEAD_SZ;
    }
    p += tlen;
  }
  if (cflag > 0 && cdata != NULL) {
    mp4tag_add_tag (libmp4tag, MP4TAG_COVR, cdata, clen, type, clen, cname);
  }
}

static void
mp4tag_process_data (const char *p, uint32_t *plen, uint32_t *pflag)
{
  uint32_t    tlen;
  uint32_t    tflag;

  memcpy (&tlen, p, sizeof (uint32_t));
  tlen = be32toh (tlen);
  tlen -= MP4TAG_DATA_SZ;

  p += MP4TAG_BOXHEAD_SZ;
  memcpy (&tflag, p, sizeof (uint32_t));
  tflag = be32toh (tflag);
  tflag = tflag & 0x00ffffff;
  *plen = tlen;
  *pflag = tflag;
}

static void
mp4tag_parse_check_end (libmp4tag_t *libmp4tag)
{
  ssize_t     currpos;

  currpos = ftell (libmp4tag->fh);
  if (currpos < 0) {
    libmp4tag->mp4error = MP4TAG_ERR_FILE_TELL_ERROR;
  }
  if (currpos >= 0 && (size_t) currpos == libmp4tag->filesz) {
    libmp4tag->unlimited = true;
  }
}

static ssize_t
mp4tag_get_curr_offset (libmp4tag_t *libmp4tag)
{
  ssize_t      offset;

  offset = ftell (libmp4tag->fh);
  if (offset < 0) {
    libmp4tag->mp4error = MP4TAG_ERR_FILE_TELL_ERROR;
  }
  return offset;
}

static void
mp4tag_dump_co (libmp4tag_t *libmp4tag, const char *ident, size_t len, const char *data)
{
  const char    *dptr = data;
  uint32_t      numoffsets;
  int           offsetsz;
  uint32_t      t32;
  uint64_t      t64 = 0;
  uint64_t      origoffset;

  /* preserve the current position */
  origoffset = ftell (libmp4tag->fh);

  if (strcmp (ident, MP4TAG_STCO) == 0) {
    offsetsz = sizeof (uint32_t);
  }
  if (strcmp (ident, MP4TAG_CO64) == 0) {
    offsetsz = sizeof (uint64_t);
  }

  /* version/flags */
  dptr += sizeof (uint32_t);
  memcpy (&t32, dptr, sizeof (uint32_t));
  dptr += sizeof (uint32_t);
  numoffsets = be32toh (t32);

  for (uint32_t i = 0; i < numoffsets; ++i) {
    if (offsetsz == sizeof (uint32_t)) {
      memcpy (&t32, dptr, sizeof (uint32_t));
      t64 = be32toh (t32);
    }
    if (offsetsz == sizeof (uint64_t)) {
      memcpy (&t64, dptr, sizeof (uint64_t));
      t64 = be64toh (t64);
    }
    fprintf (stdout, "%4d: %08" PRIx64 " ", i, t64);
    mp4tag_dump_data (libmp4tag, t64);
    fprintf (stdout, " ");
    mp4tag_dump_data (libmp4tag, t64 + 200);
    fprintf (stdout, "\n");
    dptr += offsetsz;
  }

  /* restore the original position */
  fseek (libmp4tag->fh, origoffset, SEEK_SET);
}

static void
mp4tag_dump_data (libmp4tag_t *libmp4tag, uint64_t offset)
{
  unsigned char    buff [8];

  if (fseek (libmp4tag->fh, offset, SEEK_SET) == 0) {
    if (fread (buff, sizeof (buff), 1, libmp4tag->fh) == 1) {
      for (size_t j = 0; j < sizeof (buff); ++j) {
        fprintf (stdout, "%02x", buff [j]);
      }
    }
  }
}
