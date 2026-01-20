/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>

#include "libmp4tag.h"
#include "mp4tagint.h"
#include "mp4tagbe.h"

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

/* note that boxmdhd4_t is already packed */
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

static bool assertchecked = false;

static void mp4tag_process_mdhd (libmp4tag_t *libmp4tag, const char *data);
static void mp4tag_process_tag (libmp4tag_t *libmp4tag, const char *tag, uint32_t blen, const char *data);
static void mp4tag_process_covr (libmp4tag_t *libmp4tag, const char *tag, uint32_t blen, const char *data);
static void mp4tag_process_data (const char *p, uint32_t *tlen, uint32_t *flags);
static void mp4tag_parse_check_end (libmp4tag_t *libmp4tag);
static int mp4tag_data_seek (libmp4tag_t *libmp4tag, int64_t skiplen);
static int mp4tag_data_read (libmp4tag_t *libmp4tag, void *buff, size_t sz);
static time_t mp4tag_get_time (void);
/* debugging */
static void mp4tag_dump_co (libmp4tag_t *libmp4tag, const char *ident, size_t len, const char *data);
static void mp4tag_dump_data (libmp4tag_t *libmp4tag, int64_t offset);

/* uses a recursive descent method */
int
mp4tag_parse_file (libmp4tag_t *libmp4tag, uint32_t boxlen, int level)
{
  boxhead_t       bh;
  boxdata_t       bd;
  size_t          rrc;          /* read return-code */
  uint32_t        skiplen;
  uint32_t        boxheadsz;
  bool            needdata = false;
  bool            descend = false;

  if (! assertchecked) {
    assert (sizeof (boxhead_t) == 8);
    assert (sizeof (boxhead_t) == MP4TAG_BOXHEAD_SZ);
    assert (sizeof (boxmdhd4_t) == 24);
    assert (sizeof (boxmdhd8pack_t) == 36);
    assertchecked = true;
  }

  if (level >= MP4TAG_LEVEL_MAX) {
    libmp4tag->mp4error = MP4TAG_ERR_UNABLE_TO_PROCESS;
    return libmp4tag->mp4error;
  }

  if (libmp4tag->parsedone) {
    return libmp4tag->mp4error;
  }

  libmp4tag->rem_length [level] = boxlen;
  /* subtract the box's header size, as it is not included */
  /* within the container's contents */
  libmp4tag->rem_length [level] -= MP4TAG_BOXHEAD_SZ;

  rrc = mp4tag_data_read (libmp4tag, &bh, MP4TAG_BOXHEAD_SZ);

  while (rrc == MP4TAG_READ_OK) {
    boxheadsz = MP4TAG_BOXHEAD_SZ;

    /* the box-length includes the length and the identifier */
    bd.boxlen = be32toh (bh.len);

    if (bd.boxlen == 0) {
      /* indicates that the 'mdat' box continues to the end of the file */
      libmp4tag->parsedone = true;
      break;
    }

    if (bd.boxlen == 1) {
      uint64_t    t64 = 0;

      rrc = mp4tag_data_read (libmp4tag, &t64, sizeof (uint64_t));
      if (rrc != MP4TAG_READ_OK) {
        break;
      }

      bd.boxlen = be64toh (t64);
      boxheadsz += sizeof (uint64_t);
    }

    bd.len = bd.boxlen - boxheadsz;
    if (boxlen == 0) {
      libmp4tag->rem_length [level] = bd.boxlen;
    }

    /* save the name of the box */
    if (*bh.nm == '\xa9') {
      /* maximum 5 bytes */
      strcpy (bd.nm, COPYRIGHT_STR);
      memcpy (bd.nm + strlen (COPYRIGHT_STR), bh.nm + 1, MP4TAG_ID_LEN - 1);
      bd.nm [MP4TAG_ID_LEN + strlen (COPYRIGHT_STR) - 1] = '\0';
    } else {
      memcpy (bd.nm, bh.nm, MP4TAG_ID_LEN);
      bd.nm [MP4TAG_ID_LEN] = '\0';
    }

    bd.data = NULL;
    bd.dlen = 0;
    skiplen = bd.len;
    needdata = false;

    if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_PRINT_FILE_STRUCTURE)) {
      fprintf (stdout, "%*s %2d %.5s: %" PRId64 " %" PRId64 " rem: %" PRId64 "\n",
          level*2, " ", level, bd.nm, bd.boxlen, bd.len,
          libmp4tag->rem_length [level]);
    }

    descend = false;

    /* hierarchies used: */
    /*   moov.trak.mdia.mdhd  (has duration) */
    /*   moov.trak.mdia.minf.stbl.stco  (offset table to update) */
    /*   moov.trak.mdia.minf.stbl.co64  (offset table to update) */
    /*   moov.udta.meta.ilst.*  (tags) */
    if (strcmp (bd.nm, boxids [MP4TAG_MOOV]) == 0 ||
        strcmp (bd.nm, boxids [MP4TAG_TRAK]) == 0 ||
        strcmp (bd.nm, boxids [MP4TAG_UDTA]) == 0 ||
        strcmp (bd.nm, boxids [MP4TAG_MDIA]) == 0 ||
        strcmp (bd.nm, boxids [MP4TAG_STBL]) == 0 ||
        strcmp (bd.nm, boxids [MP4TAG_MINF]) == 0 ||
        strcmp (bd.nm, boxids [MP4TAG_ILST]) == 0) {
      /* want to descend into this hierarchy */
      /* container only, don't need to skip over any data */
      descend = true;
      skiplen = 0;
    }
    if (strcmp (bd.nm, boxids [MP4TAG_META]) == 0) {
      /* want to descend into this hierarchy */
      /* skip the 4 bytes of flags */
      skiplen = MP4TAG_META_SZ - MP4TAG_BOXHEAD_SZ;
      libmp4tag->insert_delta += MP4TAG_META_SZ;
      descend = true;
    }

    /* save off any offsets before any processing is done */

    if (strcmp (bd.nm, boxids [MP4TAG_UDTA]) == 0) {
      /* need to save this offset in case there is no 'ilst' box */
      libmp4tag->noilst_offset = libmp4tag->offset - MP4TAG_BOXHEAD_SZ;
      libmp4tag->after_ilst_offset =
          libmp4tag->noilst_offset + MP4TAG_BOXHEAD_SZ;
      libmp4tag->insert_delta = MP4TAG_BOXHEAD_SZ;
    }

    if (strcmp (bd.nm, boxids [MP4TAG_ILST]) == 0) {
      libmp4tag->parentidx = level - 1;
      libmp4tag->taglist_offset = libmp4tag->offset;
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
        if (libmp4tag->canwrite) {
          libmp4tag->checkforfree = true;
        }
      }
    }

    if (strcmp (bd.nm, boxids [MP4TAG_STCO]) == 0) {
      libmp4tag->stco_offset = libmp4tag->offset;
      libmp4tag->stco_len = bd.len;
      if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_DUMP_CO)) {
        needdata = true;
      }
    }

    if (strcmp (bd.nm, boxids [MP4TAG_CO64]) == 0) {
      libmp4tag->co64_offset = libmp4tag->offset;
      libmp4tag->co64_len = bd.len;
      if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_DUMP_CO)) {
        needdata = true;
      }
    }

    if (libmp4tag->checkforfree) {
      if (libmp4tag->taglist_orig_data_len == 0) {
        libmp4tag->taglist_orig_data_len = libmp4tag->taglist_len;
      }
      /* note that this will also locate free boxes */
      /* trailing the 'moov' box */
      /* all free space is consolidated */
      if (strcmp (bd.nm, boxids [MP4TAG_FREE]) == 0) {
        if (level == 0) {
          libmp4tag->exterior_free_len += bd.boxlen;
        } else {
          libmp4tag->interior_free_len += bd.boxlen;
        }
        libmp4tag->taglist_len += bd.boxlen;
        libmp4tag->after_ilst_offset += bd.boxlen;
        /* continue on and see if there are more 'free' boxes to add */
      } else {
        /* if this spot was reached, there is some other */
        /* box after the 'ilst' or 'free' boxes */
        /* set check-for-free to false so that the unlimited flag */
        /* will not be set, and so that any future free space is */
        /* not added in to the available space */
        libmp4tag->checkforfree = false;
        if (! mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_PRINT_FILE_STRUCTURE)) {
          libmp4tag->parsedone = true;
          break;
        }
      }
    }

    if (descend && bd.len > 0) {
      /* only want to update the base-offsets if the 'ilst' */
      /* has not been found */
      if (level < MP4TAG_LEVEL_MAX &&
          libmp4tag->taglist_offset == 0) {
        ssize_t      offset;

        offset = libmp4tag->offset;

        libmp4tag->base_lengths [level] = bd.boxlen;
        snprintf (libmp4tag->base_name [level], sizeof (libmp4tag->base_name [level]),
            "%s", bd.nm);
        libmp4tag->base_offsets [level] = offset - MP4TAG_BOXHEAD_SZ;
        if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_OTHER)) {
          fprintf (stdout, "%*s %2d store base %s len:%" PRIu64 " offset:%08" PRIx64 "\n",
              level*2, " ", level, bd.nm, bd.len + MP4TAG_BOXHEAD_SZ, (int64_t) libmp4tag->base_offsets [level]);
        }
        libmp4tag->base_offset_count = level + 1;
      }

      if (skiplen > 0) {
        mp4tag_data_seek (libmp4tag, skiplen);
      }

      mp4tag_parse_file (libmp4tag, bd.boxlen - skiplen, level + 1);
      /* when descending, the box's data has already been skipped or read */
      skiplen = 0;
    }

    if (libmp4tag->taglist_orig_data_len == 0) {
      libmp4tag->taglist_orig_data_len = libmp4tag->taglist_len;
    }

    /* if descended into the hierarchy, now done */

    if (strcmp (bd.nm, boxids [MP4TAG_MOOV]) == 0 &&
        libmp4tag->noilst_offset == 0) {
      libmp4tag->noilst_offset = libmp4tag->offset;
      libmp4tag->after_ilst_offset = libmp4tag->noilst_offset;
    }

    /* out of 'ilst', do not process more tags */
    /* only need to check for any 'free' boxes trailing the 'ilst'.*/
    if (strcmp (bd.nm, boxids [MP4TAG_ILST]) == 0) {
      libmp4tag->processdata = false;
      if (libmp4tag->canwrite) {
        libmp4tag->checkforfree = true;    // not quite done yet
      }
    }

    /* the 'needdata' flag indicates that the data in the box needs */
    /* to be read and will be processed */
    if (strcmp (bd.nm, boxids [MP4TAG_MDHD]) == 0) {
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
      rrc = mp4tag_data_read (libmp4tag, bd.data, bd.len);
      if (rrc != MP4TAG_READ_OK) {
        free (bd.data);
        bd.data = NULL;
        break;
      }
    }
    if (! needdata && skiplen > 0) {
      rrc = mp4tag_data_seek (libmp4tag, skiplen);
      if (rrc != MP4TAG_READ_OK) {
        free (bd.data);
        bd.data = NULL;
        break;
      }
    }

    if (needdata && bd.data != NULL && bd.len > 0) {
      if (! libmp4tag->isstream &&
          mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_DUMP_CO) &&
          (strcmp (bd.nm, boxids [MP4TAG_STCO]) == 0 ||
          strcmp (bd.nm, boxids [MP4TAG_CO64]) == 0)) {
        /* debugging */
        mp4tag_dump_co (libmp4tag, bd.nm, bd.len, bd.data);
      }
      if (strcmp (bd.nm, boxids [MP4TAG_MDHD]) == 0) {
        mp4tag_process_mdhd (libmp4tag, bd.data);
      }
      if (libmp4tag->processdata) {
        if (strcmp (bd.nm, boxids [MP4TAG_COVR]) == 0) {
          mp4tag_process_covr (libmp4tag, bd.nm, bd.len, bd.data);
        } else {
          mp4tag_process_tag (libmp4tag, bd.nm, bd.len, bd.data);
        }
      }
      free (bd.data);
      bd.data = NULL;
    }

    libmp4tag->rem_length [level] -= bd.boxlen;
    if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_PRINT_FILE_STRUCTURE)) {
      fprintf (stdout, "%*s    %.5s: end: rem: %" PRId64 "\n",
          level*2, " ", bd.nm, libmp4tag->rem_length [level]);
      fflush (stdout);
    }

    /* check for version 1.3.0 bug */
    if (strcmp (bd.nm, boxids [MP4TAG_ILST]) == 0) {
      libmp4tag->ilst_remaining = libmp4tag->rem_length [level];
      libmp4tag->ilstend = true;
      /* reached the end of the 'ilst' box and there is data remaining */
      /* to be processed. */
      /* if the 'ilst' box is not processed in the normal exit below, */
      /* this is a probable indicator that the 1.3.0 bug is present */
      if (libmp4tag->ilst_remaining > 0) {
        libmp4tag->ilstremain = true;
        if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_BUG)) {
          fprintf (stdout, "ilst-rem %" PRIu64 "\n", libmp4tag->ilst_remaining);
        }
      }
    }

    if (libmp4tag->rem_length [level] <= 0 && boxlen != 0) {
      /* this is the normal exit when done with a level */

      /* checks for version 1.3.0 bug */
      if (strcmp (bd.nm, boxids [MP4TAG_ILST]) == 0) {
        libmp4tag->ilstdone = true;
        if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_BUG)) {
          fprintf (stdout, "ilst-done\n");
        }
      }
      if (strcmp (bd.nm, boxids [MP4TAG_FREE]) == 0) {
        /* only applies to first free box immediately after an 'ilst' */
        /* if the 'ilst' box finishes properly, there is no bug */
        if (libmp4tag->ilstend &&
            ! libmp4tag->ilstdone &&
            libmp4tag->rem_length [level] < 0) {
          libmp4tag->freeneg = true;
          if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_BUG)) {
            fprintf (stdout, "free-neg\n");
          }
        }
      }
      if (strcmp (bd.nm, boxids [MP4TAG_UDTA]) == 0) {
        if (libmp4tag->rem_length [level] == 0) {
          libmp4tag->udtazero = true;
          if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_BUG)) {
            fprintf (stdout, "udta-zero\n");
          }
        }

        /* if ilstdone is true, then 'ilst' was properly processed */
        if (libmp4tag->ilstremain &&
            ! libmp4tag->ilstdone &&
            libmp4tag->freeneg &&
            libmp4tag->udtazero) {
          libmp4tag->dofix = true;
          if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_BUG)) {
            fprintf (stdout, "do-fix\n");
          }
        }
      }

      libmp4tag->ilstend = false;

      return libmp4tag->mp4error;
    }

    rrc = mp4tag_data_read (libmp4tag, &bh, MP4TAG_BOXHEAD_SZ);
  }

  if (level == 0) {
    if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_PRINT_FILE_STRUCTURE)) {
      fprintf (stdout, "taglist-data-len: %d\n", libmp4tag->taglist_orig_data_len);
      fprintf (stdout, "taglist-len: %d\n", libmp4tag->taglist_len);
    }

    if (libmp4tag->checkforfree) {
      /* if checkforfree is still true, check and see if the end of file */
      /* was reached */
      mp4tag_parse_check_end (libmp4tag);
    }

    mp4tag_sort_tags (libmp4tag);

    if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_PRINT_FILE_STRUCTURE)) {
      fprintf (stdout, "interior-free: %d\n", libmp4tag->interior_free_len);
      fprintf (stdout, "exterior-free: %d\n", libmp4tag->exterior_free_len);
      fprintf (stdout, "ilst-remaining: %" PRId64 "\n", libmp4tag->ilst_remaining);
    }
  }

  /* at this time, do not look for more free boxes after the 'udta' */
  /* hierarchy.  processing gets messy.  and i don't have a sample */
  if (libmp4tag->checkforfree) {
    libmp4tag->checkforfree = false;
    libmp4tag->parsedone = true;
  }

  return libmp4tag->mp4error;
}

int
mp4tag_parse_ftyp (libmp4tag_t *libmp4tag)
{
  int         ok = 0;
  uint32_t    idx;
  uint32_t    len;
  size_t      rrc;
  boxhead_t   bh;
  char        *buff;
  char        tmp [MP4TAG_ID_LEN + 1];

  rrc = mp4tag_data_read (libmp4tag, &bh, MP4TAG_BOXHEAD_SZ);
  if (rrc != MP4TAG_READ_OK) {
    return libmp4tag->mp4error;
  }

  /* the total length includes the length and the identifier */
  len = be32toh (bh.len) - MP4TAG_BOXHEAD_SZ;
  if (memcmp (bh.nm, boxids [MP4TAG_FTYP], MP4TAG_ID_LEN) != 0) {
    libmp4tag->mp4error = MP4TAG_ERR_NOT_MP4;
    return libmp4tag->mp4error;
  }
  ++ok;

  buff = malloc (len);
  if (buff == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
    return libmp4tag->mp4error;
  }
  rrc = mp4tag_data_read (libmp4tag, buff, len);
  if (rrc != MP4TAG_READ_OK) {
    return libmp4tag->mp4error;
  }

  idx = 0;
  while (idx < len && libmp4tag->mp4error == MP4TAG_OK) {
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
        ++ok;
      }
      if (memcmp (buff + idx, "mp71", 4) == 0 ||
          memcmp (buff + idx, "mp7b", 4) == 0) {
        /* mpeg-7 meta data */
        fprintf (stdout, "== mpeg-7 meta data\n");
        libmp4tag->mp7meta = true;
      }
      if (memcmp (buff + idx, "3g2a", 4) == 0) {
        ++ok;
      }
      if (memcmp (buff + idx, "3gp4", 4) == 0) {
        ++ok;
      }
      if (memcmp (buff + idx, "3gp5", 4) == 0) {
        ++ok;
      }
      if (memcmp (buff + idx, "isom", 4) == 0) {
        /* generic iso media */
        ++ok;
      }
      /* isom, iso2, qt, avc1, 3g2, 3gp, mmp4 */
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
    /* version 0 is 32-bit */
    /* a packed version of mdhd4 is not necessary as all data is aligned */
    memcpy (&mdhd4, data, sizeof (boxmdhd4_t));
    mdhd8.flags = be32toh (mdhd4.flags);
    mdhd8.creationdate = be32toh (mdhd4.creationdate);
    mdhd8.modifieddate = be32toh (mdhd4.modifieddate);
    mdhd8.timescale = be32toh (mdhd4.timescale);
    mdhd8.duration = be32toh (mdhd4.duration);
    mdhd8.moreflags = be32toh (mdhd4.moreflags);
  } else {
    /* version 1 is 64-bit */
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
  if (mdhd8.duration > 0) {
    libmp4tag->creationdate = mdhd8.creationdate;
    libmp4tag->modifieddate = mdhd8.modifieddate;
    libmp4tag->samplerate = mdhd8.timescale;
    libmp4tag->duration = (int64_t)
        ((double) mdhd8.duration * 1000.0 / (double) mdhd8.timescale);
  }
}

static void
mp4tag_process_tag (libmp4tag_t *libmp4tag, const char *tag,
    uint32_t blen, const char *data)
{
  const char  *p;
  /* tnm must be large enough to hold any custom tag name */
  char        tnm [MP4TAG_ID_MAX];
  uint32_t    type;
  uint8_t     t8;
  uint16_t    t16;
  uint32_t    t32;
  uint64_t    t64;
  uint32_t    tlen;       /* length of data item */
  uint32_t    plen;       /* processed length */
  char        tmp [40];

  p = data;

  if (strcmp (tag, boxids [MP4TAG_FREE]) == 0) {
    return;
  }

  snprintf (tnm, sizeof (tnm), "%s", tag);
  if (strcmp (tag, boxids [MP4TAG_CUSTOM]) == 0) {
    size_t    len;        /* current length of the tag name */

    len = strlen (tnm);
    tnm [len++] = ':';
    tnm [len] = '\0';

    memcpy (&tlen, p, sizeof (uint32_t));
    tlen = be32toh (tlen);
    tlen -= MP4TAG_BOXHEAD_SZ;
    tlen -= sizeof (uint32_t);
    /* mean = something application name */
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

    /* reduce the max length by the data size, two more boxes, */
    /* and the length of the custom name */
    blen -= MP4TAG_DATA_SZ;
    blen -= MP4TAG_BOXHEAD_SZ * 2;
    blen -= sizeof (uint32_t) * 2;
    blen -= tlen;

    p += tlen;
  }

  mp4tag_process_data (p, &tlen, &type);
  p += MP4TAG_DATA_SZ;
  plen = 0;

  if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_OTHER)) {
    fprintf (stdout, "%s %03x tlen:%" PRId32 " blen:%" PRId32 "\n", tnm, type, tlen, blen);
  }

  do {
    plen += tlen;

    /* general data */
    if (type == MP4TAG_ID_DATA ||
        type == MP4TAG_ID_NUM) {

      /* 'disk' and 'trkn' must be handle as special cases. */
      /* they are marked as data (0x00). */
      if (strcmp (tnm, boxids [MP4TAG_DISK]) == 0 ||
          strcmp (tnm, boxids [MP4TAG_TRKN]) == 0) {
        t16 = 0;

        /* pair of 32 bit and 16 bit numbers */
        /* trkn has an extra 2 bytes of padding */
        memcpy (&t32, p, sizeof (uint32_t));
        t32 = be32toh (t32);

        /* apparently there exist track number boxes */
        /* that are not the full size */
        if (strcmp (tnm, boxids [MP4TAG_TRKN]) != 0 ||
            tlen >= sizeof (uint32_t) + sizeof (uint16_t)) {
          p += sizeof (uint32_t);
          memcpy (&t16, p, sizeof (uint16_t));
          t16 = be16toh (t16);
        }

        /* trkn has an additional two trailing bytes that are not used */

        if (t16 == 0) {
          snprintf (tmp, sizeof (tmp), "%" PRId32, t32);
        } else {
          snprintf (tmp, sizeof (tmp), "%" PRId32 "/%" PRId16, t32, t16);
        }
        mp4tag_add_tag (libmp4tag, tnm, tmp, MP4TAG_STRING, type, tlen, NULL);
      } else if (tlen == sizeof (uint32_t)) {
        memcpy (&t32, p, sizeof (uint32_t));
        t32 = be32toh (t32);
        snprintf (tmp, sizeof (tmp), "%" PRId32, t32);
        mp4tag_add_tag (libmp4tag, tnm, tmp, MP4TAG_STRING, type, tlen, NULL);
      } else if (tlen == sizeof (uint16_t)) {
        memcpy (&t16, p, sizeof (uint16_t));
        t16 = be16toh (t16);

        /* the 'gnre' tag is converted to '©gen' */
        /* hard-coded lists of genres are not good */
        if (strcmp (tnm, boxids [MP4TAG_GNRE]) == 0) {
          /* the itunes value is offset by 1 */
          t16 -= 1;
          if (t16 < mp4tagoldgenrelistsz) {
            /* do not use the 'gnre' identifier */
            strcpy (tnm, COPYRIGHT_STR);
            strcat (tnm, boxids [MP4TAG_GEN]);
            mp4tag_add_tag (libmp4tag, tnm, mp4tagoldgenrelist [t16],
                MP4TAG_STRING, MP4TAG_ID_STRING,
                strlen (mp4tagoldgenrelist [t16]), NULL);
          }
        } else {
          snprintf (tmp, sizeof (tmp), "%" PRId16, t16);
          mp4tag_add_tag (libmp4tag, tnm, tmp, MP4TAG_STRING, type, tlen, NULL);
        }
      } else if (tlen == sizeof (uint64_t)) {
        memcpy (&t64, p, sizeof (uint64_t));
        t64 = be64toh (t64);
        snprintf (tmp, sizeof (tmp), "%" PRId64, t64);
        mp4tag_add_tag (libmp4tag, tnm, tmp, MP4TAG_STRING, type, tlen, NULL);
      } else if (tlen == sizeof (uint8_t)) {
        memcpy (&t8, p, sizeof (uint8_t));
        snprintf (tmp, sizeof (tmp), "%" PRId8, t8);
        mp4tag_add_tag (libmp4tag, tnm, tmp, MP4TAG_STRING, type, tlen, NULL);
      } else {
        /* binary data */
        mp4tag_add_tag (libmp4tag, tnm, p, tlen, type, tlen, NULL);
      }
    }

    /* string type */
    if (type == MP4TAG_ID_STRING && tlen > 0) {
      /* pass as negative len to indicate a string that needs a terminator */
      mp4tag_add_tag (libmp4tag, tnm, p, - (ssize_t) tlen, type, tlen, NULL);
      if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_OTHER)) {
        fprintf (stdout, "add-tag %s %.*s\n", tnm, tlen, p);
      }
    }

    // fprintf (stdout, "%" PRId32 " >= %" PRId32 "\n", plen + MP4TAG_DATA_SZ, blen);
    if (plen + MP4TAG_DATA_SZ >= blen) {
      break;
    }

    p += tlen;
    plen += MP4TAG_DATA_SZ;

    mp4tag_process_data (p, &tlen, &type);
    if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_OTHER)) {
      fprintf (stdout, "%s %03x tlen:%" PRId32 " plen:%" PRId32 " blen:%" PRId32 "\n", tnm, type, tlen, plen, blen);
    }

    p += MP4TAG_DATA_SZ;
  } while (1);
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
  char        *cname = NULL;

  while (blen > 0) {
    memcpy (&tlen, p, sizeof (uint32_t));
    tlen = be32toh (tlen);
    blen -= tlen;
    if (memcmp (p + sizeof (uint32_t), boxids [MP4TAG_DATA], MP4TAG_ID_LEN) == 0) {
      if (cflag > 0 && cdata != NULL) {
        mp4tag_add_tag (libmp4tag, boxids [MP4TAG_COVR], cdata, clen, type, clen, cname);
        if (cname != NULL) {
          free (cname);
        }
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
    } else if (memcmp (p + sizeof (uint32_t), boxids [MP4TAG_NAME], MP4TAG_ID_LEN) == 0) {
      const char  *tcname = NULL;
      int         tclen;

      tcname = p + MP4TAG_BOXHEAD_SZ;
      tclen = tlen - MP4TAG_BOXHEAD_SZ;
      if (cname != NULL) {
        free (cname);
      }
      cname = malloc (tclen + 1);
      memcpy (cname, tcname, tclen);
      cname [tclen] = '\0';
    }
    p += tlen;
  }
  if (cflag > 0 && cdata != NULL) {
    mp4tag_add_tag (libmp4tag, boxids [MP4TAG_COVR], cdata, clen, type, clen, cname);
  }
  if (cname != NULL) {
    free (cname);
  }
}

static void
mp4tag_process_data (const char *p, uint32_t *plen, uint32_t *ptype)
{
  uint32_t    tlen;
  uint32_t    type;

  memcpy (&tlen, p, sizeof (uint32_t));
  tlen = be32toh (tlen);
  tlen -= MP4TAG_DATA_SZ;

  p += MP4TAG_BOXHEAD_SZ;
  memcpy (&type, p, sizeof (uint32_t));
  type = be32toh (type);
  type = type & 0x00ffffff;
  *plen = tlen;
  *ptype = type;
}

static void
mp4tag_parse_check_end (libmp4tag_t *libmp4tag)
{
  if ((size_t) libmp4tag->offset == libmp4tag->filesz) {
    libmp4tag->unlimited = true;
  }
}

static int
mp4tag_data_seek (libmp4tag_t *libmp4tag, int64_t skiplen)
{
  int     rc;

  if (! libmp4tag->isstream) {
    rc = mp4tag_fseek (libmp4tag->fh, skiplen, SEEK_CUR);
  } else {
    if (libmp4tag->seekcb == NULL) {
      libmp4tag->mp4error = MP4TAG_ERR_NO_CALLBACK;
      return MP4TAG_READ_NONE;
    }

    rc = libmp4tag->seekcb (skiplen, libmp4tag->userdata);
  }
  if (rc != 0) {
    libmp4tag->mp4error = MP4TAG_ERR_FILE_SEEK_ERROR;
    return MP4TAG_READ_NONE;
  }
  libmp4tag->offset += skiplen;

  return MP4TAG_READ_OK;
}

static int
mp4tag_data_read (libmp4tag_t *libmp4tag, void *buff, size_t sz)
{
  size_t    bwant = sz;
  size_t    totbr = 0;
  size_t    br = 0;
  time_t    tmval = 0;
  time_t    ttm = 0;
  int       rc = 0;
  char      *cbuff = buff;

  if (libmp4tag->isstream) {
    if (libmp4tag->readcb == NULL) {
      libmp4tag->mp4error = MP4TAG_ERR_NO_CALLBACK;
      return MP4TAG_READ_NONE;
    }

    tmval = mp4tag_get_time ();
    tmval += libmp4tag->timeout;
  }

  while (bwant > 0) {
    if (! libmp4tag->isstream) {
      br = fread (cbuff + totbr, 1, bwant, libmp4tag->fh);
    } else {
      br = libmp4tag->readcb (cbuff + totbr, 1, bwant, libmp4tag->userdata);
    }
    if (br > 0) {
      totbr += br;
      bwant -= br;
      libmp4tag->offset += br;
    }
    if (bwant == 0) {
      rc = MP4TAG_READ_OK;
      break;
    }
    if (! libmp4tag->isstream && bwant != 0) {
      /* probably end-of-file */
      return MP4TAG_READ_NONE;
    }

    if (libmp4tag->isstream) {
      mp4tag_sleep (MP4TAG_SLEEP_TIME);

      ttm = mp4tag_get_time ();
      if (ttm > tmval) {
        /* no more data found within timeout period */
        return MP4TAG_READ_NONE;
      }
    }
  }

  return rc;
}

static time_t
mp4tag_get_time (void)
{
  struct timeval    curr;
  time_t            s, u, m, tot;

  gettimeofday (&curr, NULL);

  s = curr.tv_sec;
  u = curr.tv_usec;
  m = u / 1000;
  tot = s * 1000 + m;
  return tot;
}


/* debugging */

static void
mp4tag_dump_co (libmp4tag_t *libmp4tag, const char *ident, size_t len, const char *data)
{
  const char    *dptr = data;
  uint32_t      numoffsets;
  int           offsetsz;
  uint32_t      t32;
  int64_t       t64 = 0;
  int64_t       origoffset = 0;

  if (libmp4tag->isstream == false) {
    /* preserve the current position */
    origoffset = mp4tag_ftell (libmp4tag->fh);
  }

  if (strcmp (ident, boxids [MP4TAG_STCO]) == 0) {
    offsetsz = sizeof (uint32_t);
  }
  if (strcmp (ident, boxids [MP4TAG_CO64]) == 0) {
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
    fprintf (stdout, "%6d: %08" PRIx64 " ", i, t64);
    mp4tag_dump_data (libmp4tag, t64);
    fprintf (stdout, " ");
    mp4tag_dump_data (libmp4tag, t64 + 200);
    fprintf (stdout, "\n");
    dptr += offsetsz;
  }

  if (libmp4tag->isstream == false) {
    /* restore the original position */
    mp4tag_fseek (libmp4tag->fh, origoffset, SEEK_SET);
  }
}

static void
mp4tag_dump_data (libmp4tag_t *libmp4tag, int64_t offset)
{
  unsigned char    buff [8];

  if (libmp4tag->isstream) {
    return;
  }

  if (mp4tag_fseek (libmp4tag->fh, offset, SEEK_SET) == 0) {
    if (fread (buff, sizeof (buff), 1, libmp4tag->fh) == 1) {
      for (size_t j = 0; j < sizeof (buff); ++j) {
        fprintf (stdout, "%02x", buff [j]);
      }
    }
  }
}

