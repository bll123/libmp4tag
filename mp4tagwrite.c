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

#include "libmp4tag.h"
#include "mp4tagint.h"
#include "mp4tagbe.h"

static const char *MP4TAG_CUSTOM_DELIM = ":";
static const char *MP4TAG_TEMP_SUFFIX = "-mp4tag.tmp";
static const char *MP4TAG_BACKUP_SUFFIX = "-mp4tag.bak";

static int  mp4tag_write_inplace (libmp4tag_t *libmp4tag, const char *data, uint32_t datalen);
static int  mp4tag_write_rewrite (libmp4tag_t *libmp4tag, const char *data, uint32_t datalen);
static int  mp4tag_write_freebox (libmp4tag_t *libmp4tag, FILE *ofh, uint32_t freelen);
static void mp4tag_update_offsets (libmp4tag_t *libmp4tag, FILE *ofh, int32_t delta, uint64_t foffset);
static void mp4tag_update_offset_block (libmp4tag_t *libmp4tag, FILE *ofh, int32_t delta, uint64_t foffset, uint64_t boffset, uint32_t blen, int offsetsz);
static char * mp4tag_build_append (libmp4tag_t *libmp4tag, int idx, char *data, uint32_t *dlen);
static void mp4tag_parse_pair (const char *data, int *a, int *b);
static char * mp4tag_append_data (char *dptr, const char *tnm, uint32_t sz);
static char * mp4tag_append_len_8 (char *dptr, uint64_t val);
static char * mp4tag_append_len_16 (char *dptr, uint64_t val);
static char * mp4tag_append_len_32 (char *dptr, uint64_t val);
static char * mp4tag_append_len_64 (char *dptr, uint64_t val);
static void mp4tag_update_data_len (libmp4tag_t *libmp4tag, char *data, uint32_t len);
static int  mp4tag_copy_file_data (FILE *ifh, FILE *ofh, uint64_t offset, size_t len);
static void mp4tag_debug_write_vals (libmp4tag_t *libmp4tag, uint32_t datalen, int32_t delta, int32_t totdelta, int32_t freelen);

/* if there are no tags, null will be returned. */
char *
mp4tag_build_data (libmp4tag_t *libmp4tag, uint32_t *datalen)
{
  char        *data = NULL;

  *datalen = 0;

  for (int i = 0; i < libmp4tag->tagcount; ++i) {
    const mp4tagdef_t   *result = NULL;

    if (memcmp (libmp4tag->tags [i].tag, boxids [MP4TAG_CUSTOM], MP4TAG_ID_LEN) == 0) {
      libmp4tag->tags [i].priority = MP4TAG_PRI_CUSTOM;
    } else {
      result = mp4tag_check_tag (libmp4tag->tags [i].tag);
      if (result != NULL) {
        libmp4tag->tags [i].priority = result->priority;
      } else {
        /* unknown tag */
        libmp4tag->tags [i].priority = MP4TAG_PRI_CUSTOM;
      }
    }
  }

  /* mp4tag_build_append will take care of re-setting datacount and lastbox */
  libmp4tag->datacount = 0;
  libmp4tag->lastbox_offset = -1;
  *libmp4tag->lastbox_nm = '\0';
  /* tags are written by priority order, then ascii order */
  for (int pri = 0; pri < MP4TAG_PRI_MAX; ++pri) {
    for (int i = 0; i < libmp4tag->tagcount; ++i) {
      if (libmp4tag->tags [i].priority == pri) {
        data = mp4tag_build_append (libmp4tag, i, data, datalen);
      }
    }
  }

  return data;
}

int
mp4tag_write_data (libmp4tag_t *libmp4tag, const char *data,
    uint32_t datalen)
{
  int32_t  tlen = 0;

  /* tlen is the maximum size of an 'ilst' with a free block */
  /* for in-place writes */
  tlen = libmp4tag->taglist_len;
  tlen -= MP4TAG_BOXHEAD_SZ;

  libmp4tag->mp4error = MP4TAG_OK;

  /* in order to do an in-place write, the space to receive the data */
  /* a) must be exactly equal in size */
  /* b) or must have room for the data and a free space block */
  /* c) or the 'ilst/free' is at the end of the file */
  if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_WRITE)) {
    fprintf (stdout, "-- check in-place\n");
    fprintf (stdout, "  offset: %ld\n", (long) libmp4tag->taglist_offset);
    fprintf (stdout, "  taglist-len: %ld\n", (long) libmp4tag->taglist_len);
    fprintf (stdout, "  tlen (max): %ld\n", (long) tlen);
    fprintf (stdout, "  datalen: %ld\n", (long) datalen);
    fprintf (stdout, "  unlimited: %ld\n", (long) libmp4tag->unlimited);
  }
  if (libmp4tag->taglist_offset != 0 &&
      (libmp4tag->unlimited ||
      datalen == libmp4tag->taglist_len ||
      (tlen >= 0 && datalen < (uint32_t) tlen))) {
    if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_WRITE)) {
      fprintf (stdout, "-- write: in-place\n");
    }
    mp4tag_write_inplace (libmp4tag, data, datalen);
  } else {
    if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_WRITE)) {
      fprintf (stdout, "-- write: rewrite\n");
    }
    mp4tag_write_rewrite (libmp4tag, data, datalen);
  }

  return libmp4tag->mp4error;
}

static int
mp4tag_write_inplace (libmp4tag_t *libmp4tag, const char *data,
    uint32_t datalen)
{
  int32_t   delta;      /* change in 'ilst' size */
  int32_t   freelen;
  int32_t   totdelta;   /* change in delta + freelen */

  if (libmp4tag->fh == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_NOT_OPEN;
    return libmp4tag->mp4error;
  }

  if ((libmp4tag->options & MP4TAG_OPTION_KEEP_BACKUP) == MP4TAG_OPTION_KEEP_BACKUP) {
    char    ofn [2048];
    FILE    *ofh = NULL;
    int     rc;

    snprintf (ofn, sizeof (ofn), "%s%s", libmp4tag->fn, MP4TAG_BACKUP_SUFFIX);
    ofh = mp4tag_fopen (ofn, "wb+");
    if (ofh == NULL) {
      libmp4tag->mp4error = MP4TAG_ERR_NOT_OPEN;
      return libmp4tag->mp4error;
    }

    rc = mp4tag_copy_file_data (libmp4tag->fh, ofh, 0, libmp4tag->filesz);
    if (rc != MP4TAG_OK) {
      libmp4tag->mp4error = rc;
      return libmp4tag->mp4error;
    }

    mp4tag_copy_file_times (libmp4tag->fh, ofh);
    fclose (ofh);
  }

  if (fseek (libmp4tag->fh, libmp4tag->taglist_offset, SEEK_SET) != 0) {
    libmp4tag->mp4error = MP4TAG_ERR_FILE_SEEK_ERROR;
    return libmp4tag->mp4error;
  }

  if (datalen > 0) {
    if (fwrite (data, datalen, 1, libmp4tag->fh) != 1) {
      libmp4tag->mp4error = MP4TAG_ERR_FILE_WRITE_ERROR;
      return libmp4tag->mp4error;
    }
  }

  freelen = 0;
  freelen += libmp4tag->interior_free_len;
  freelen += libmp4tag->exterior_free_len;

  /* calculate change in 'ilst' length */
  /* want a signed value */
  delta = (int32_t) datalen - (int32_t) libmp4tag->taglist_orig_data_len;
  if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_WRITE)) {
    fprintf (stdout, "-- datalen: %d orig-data-len %d\n", datalen, libmp4tag->taglist_orig_data_len);
    fprintf (stdout, "-- chg in ilst len: %d\n", delta);
  }
  if (freelen > 0) {
    /* only if there is free space available */
    freelen -= delta;
  }
  totdelta = delta;
  if (libmp4tag->exterior_free_len != 0 || libmp4tag->unlimited) {
    /* if the interior free box and the exterior free box are */
    /* contiguous, the delta excludes the interior free box length */
    /* the free box will be written at hierarchy level 0 */
    /* also do this if the unlimited flag is set */
    delta -= libmp4tag->interior_free_len;
    totdelta = delta;
  }

  if (libmp4tag->exterior_free_len == 0 && ! libmp4tag->unlimited) {
    /* if there is only an interior free box */
    /* the total delta change for the parent boxes */
    /* must include the free box */
    /* but not in the case if the unlimited flag is set (handled above) */
    totdelta = libmp4tag->taglist_len - (datalen + freelen);
  }

  mp4tag_debug_write_vals (libmp4tag, datalen, delta, totdelta, freelen);

  /* there are various cases possible */
  /* in these three cases, the interior and exterior free spaces */
  /* will be merged if there is no intervening data */
  /*   int > 0, ext > 0, unlimited = 0 */
  /*   int > 0, ext = 0, unlimited = 0 */
  /*   int = 0, ext > 0, unlimited = 0 */
  /* unlimited is true */
  /*   there may be interior/exterior space that can be used */

  if (freelen != 0 || libmp4tag->unlimited) {
    /* the free-box is placed at hierarchy level 0 if possible */

    if (libmp4tag->unlimited && freelen < libmp4tag->freespacesz) {
      /* this will also handle the situation where the 'ilst' shrinks */
      /* and there is not enough room for a free box */
      freelen = MP4TAG_BOXHEAD_SZ + libmp4tag->freespacesz;
    }
    if (freelen > 8) {
      int     rc;

      rc = mp4tag_write_freebox (libmp4tag, libmp4tag->fh, freelen);
      if (rc != MP4TAG_OK) {
        libmp4tag->mp4error = rc;
        return libmp4tag->mp4error;
      }
    }
  } /* if a free box needs to be written */

  /* re-write the taglist length */
  if (delta != 0) {
    uint32_t    t32;
    int         rc;

    rc = fseek (libmp4tag->fh, libmp4tag->taglist_base_offset, SEEK_SET);
    if (rc != 0) {
      libmp4tag->mp4error = MP4TAG_ERR_FILE_SEEK_ERROR;
      return libmp4tag->mp4error;
    }

    t32 = datalen + MP4TAG_BOXHEAD_SZ;
    if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_WRITE)) {
      fprintf (stdout, "update taglist len: %d\n", t32);
    }
    t32 = htobe32 (t32);
    if (fwrite (&t32, sizeof (uint32_t), 1, libmp4tag->fh) != 1) {
      libmp4tag->mp4error = MP4TAG_ERR_FILE_WRITE_ERROR;
    }
  }

  /* if the 'ilst' box has changed in size, */
  /* the parent offsets must be updated */
  /* totdelta includes any free box */
  if (totdelta != 0) {
    mp4tag_update_parent_lengths (libmp4tag, libmp4tag->fh, delta);
  }

  return libmp4tag->mp4error;
}

static int
mp4tag_write_rewrite (libmp4tag_t *libmp4tag, const char *data,
    uint32_t datalen)
{
  FILE      *ofh;
  char      ofn [2048];
  int       rc;
  uint64_t  offset;
  size_t    wlen;
  int32_t   freelen;
  int32_t   delta;

  libmp4tag->mp4error = MP4TAG_OK;

  if (libmp4tag->fh == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_NOT_OPEN;
    return libmp4tag->mp4error;
  }

  if (libmp4tag->taglist_offset == 0 &&
      libmp4tag->noilst_offset == 0) {
    libmp4tag->mp4error = MP4TAG_ERR_UNABLE_TO_PROCESS;
    return libmp4tag->mp4error;
  }

  snprintf (ofn, sizeof (ofn), "%s%s", libmp4tag->fn, MP4TAG_TEMP_SUFFIX);
  ofh = mp4tag_fopen (ofn, "wb+");
  if (ofh == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_NOT_OPEN;
    return libmp4tag->mp4error;
  }

  offset = libmp4tag->taglist_offset;
  if (libmp4tag->taglist_offset != 0) {
    /* copy up to the taglist head, the 'ilst' head will be re-written */
    offset -= MP4TAG_BOXHEAD_SZ;
  }
  if (offset == 0 && libmp4tag->noilst_offset != 0) {
    offset = libmp4tag->noilst_offset;
  }

  /* the following code will set 'rc' to the mp4-error, */
  /* and libmp4tag->mp4error will be set to 'rc' on failure */
  /* once this code is complete */

  if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_WRITE)) {
    fprintf (stdout, "  copy-data: length:%ld\n", (long) offset);
  }
  rc = mp4tag_copy_file_data (libmp4tag->fh, ofh, 0, offset);

  if (rc == MP4TAG_OK && libmp4tag->taglist_offset == 0) {
    char    *buff;
    size_t  len;
    size_t  alloclen;

    if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_WRITE)) {
      fprintf (stdout, "  no ilst, insert boxes\n");
    }

    /* there is no 'ilst' block in the mp4 file */

    /* only update the 'moov' box length */
    libmp4tag->parentidx = 0;

    /* allocation size: udta + meta + hdlr + ilst-box */
    alloclen = MP4TAG_BOXHEAD_SZ;
    alloclen += MP4TAG_META_SZ + MP4TAG_HDLR_SZ + MP4TAG_BOXHEAD_SZ;

    buff = malloc (alloclen);
    if (buff == NULL) {
      rc = MP4TAG_ERR_OUT_OF_MEMORY;
    }

    /* for the length field, include the hierarchy, datalen */
    /* and the size of the free block that will be added */
    len = alloclen;
    len += datalen;
    len += MP4TAG_BOXHEAD_SZ + libmp4tag->freespacesz;

    if (rc == MP4TAG_OK) {
      uint32_t    h32;
      char        *dptr;

      dptr = buff;

      /* udta */
      dptr = mp4tag_append_len_32 (dptr, len);
      dptr = mp4tag_append_data (dptr, boxids [MP4TAG_UDTA], MP4TAG_ID_LEN);

      /* meta */
      len -= MP4TAG_BOXHEAD_SZ;
      dptr = mp4tag_append_len_32 (dptr, len);
      dptr = mp4tag_append_data (dptr, boxids [MP4TAG_META], MP4TAG_ID_LEN);
      /* meta version+flags */
      dptr = mp4tag_append_len_32 (dptr, 0);

      /* hdlr */
      h32 = MP4TAG_HDLR_SZ;
      dptr = mp4tag_append_len_32 (dptr, h32);
      dptr = mp4tag_append_data (dptr, boxids [MP4TAG_HDLR], MP4TAG_ID_LEN);
      dptr = mp4tag_append_len_32 (dptr, 0);  // version+flags
      dptr = mp4tag_append_len_32 (dptr, 0);  // quicktime type
      dptr = mp4tag_append_data (dptr, "mdir", MP4TAG_ID_LEN);
      dptr = mp4tag_append_data (dptr, "appl", MP4TAG_ID_LEN);
      dptr = mp4tag_append_len_32 (dptr, 0);  // flags
      dptr = mp4tag_append_len_32 (dptr, 0);  // mask
      /* zero-length string */
      dptr = mp4tag_append_len_8 (dptr, 0);   // trailing byte

      /* ilst */
      len -= MP4TAG_META_SZ;
      len -= MP4TAG_HDLR_SZ;
      len -= MP4TAG_BOXHEAD_SZ + libmp4tag->freespacesz;
      dptr = mp4tag_append_len_32 (dptr, len);
      dptr = mp4tag_append_data (dptr, boxids [MP4TAG_ILST], MP4TAG_ID_LEN);

      if (fwrite (buff, alloclen, 1, ofh) != 1) {
        rc = MP4TAG_ERR_FILE_WRITE_ERROR;
      }
      free (buff);
    }
  }

  if (rc == MP4TAG_OK && libmp4tag->taglist_offset != 0) {
    uint32_t    t32;

    t32 = datalen + MP4TAG_BOXHEAD_SZ;
    if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_WRITE)) {
      fprintf (stdout, "  ilst size w/head: %d\n", t32);
    }
    t32 = htobe32 (t32);
    if (fwrite (&t32, sizeof (uint32_t), 1, ofh) != 1) {
      rc = MP4TAG_ERR_FILE_WRITE_ERROR;
    }
    if (fwrite (boxids [MP4TAG_ILST], MP4TAG_ID_LEN, 1, ofh) != 1) {
      rc = MP4TAG_ERR_FILE_WRITE_ERROR;
    }
  }

  if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_WRITE)) {
    fprintf (stdout, "  data-offset: %ld\n", ftell (ofh));
    fprintf (stdout, "  tags: %ld\n", (long) datalen);
  }
  if (rc == MP4TAG_OK && fwrite (data, datalen, 1, ofh) != 1) {
    rc = MP4TAG_ERR_FILE_WRITE_ERROR;
  }

  if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_WRITE)) {
    fprintf (stdout, "  free-box: %d\n", MP4TAG_BOXHEAD_SZ + libmp4tag->freespacesz);
  }

  freelen = MP4TAG_BOXHEAD_SZ + libmp4tag->freespacesz;
  rc = mp4tag_write_freebox (libmp4tag, ofh, freelen);

  offset = libmp4tag->after_ilst_offset;
  wlen = libmp4tag->filesz - offset;
  if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_WRITE)) {
    fprintf (stdout, "  file-size: %ld\n", (long) libmp4tag->filesz);
    fprintf (stdout, "  copy-final-data: i-offset:%ld length:%ld\n", (long) offset, (long) wlen);
  }
  if (rc == MP4TAG_OK) {
    rc = mp4tag_copy_file_data (libmp4tag->fh, ofh, offset, wlen);
  }

  /* want a signed value */
  delta = (int32_t) datalen - (int32_t) libmp4tag->taglist_len;
  delta += freelen;
  /* the delta does not include the interior free space */
  delta -= libmp4tag->interior_free_len;

  if (libmp4tag->taglist_offset == 0) {
    /* if the udta & etc. were inserted, adjust the delta size */
    delta += MP4TAG_BOXHEAD_SZ;
    delta += MP4TAG_META_SZ + MP4TAG_HDLR_SZ + MP4TAG_BOXHEAD_SZ;
    delta -= libmp4tag->insert_delta;
  }
  mp4tag_debug_write_vals (libmp4tag, datalen, delta, -1, freelen);

  if (rc == MP4TAG_OK) {
    mp4tag_update_parent_lengths (libmp4tag, ofh, delta);
    mp4tag_update_offsets (libmp4tag, ofh, delta, offset);
  }

  fclose (ofh);

  if (rc == MP4TAG_OK) {
    char    tfn [2048];

    snprintf (tfn, sizeof (tfn), "%s%s", libmp4tag->fn, MP4TAG_BACKUP_SUFFIX);
    /* windows will not allow an open file to be removed, */
    /* and the original file is still open */

    if (libmp4tag->fh != NULL) {
      fclose (libmp4tag->fh);
      libmp4tag->fh = NULL;
    }

    mp4tag_file_move (libmp4tag->fn, tfn);
    mp4tag_file_move (ofn, libmp4tag->fn);
    if ((libmp4tag->options & MP4TAG_OPTION_KEEP_BACKUP) != MP4TAG_OPTION_KEEP_BACKUP) {
      mp4tag_file_delete (tfn);
    }

    /* and re-open the file */
    libmp4tag->fh = mp4tag_fopen (libmp4tag->fn, "rb+");
  } else {
    mp4tag_file_delete (ofn);
  }

  if (rc != MP4TAG_OK) {
    libmp4tag->mp4error = rc;
  }

  return libmp4tag->mp4error;
}

static int
mp4tag_write_freebox (libmp4tag_t *libmp4tag, FILE *ofh, uint32_t freelen)
{
  char        *buff;
  uint32_t    t32;
  int         rc = MP4TAG_OK;

  /* the freelen argument already includes the boxhead size */
  buff = malloc (freelen);
  if (buff == NULL) {
    return MP4TAG_ERR_OUT_OF_MEMORY;
  }

  memset (buff, '\0', freelen);
  t32 = htobe32 (freelen);
  memcpy (buff, &t32, sizeof (uint32_t));
  memcpy (buff + sizeof (uint32_t), boxids [MP4TAG_FREE], MP4TAG_ID_LEN);
  if (fwrite (buff, freelen, 1, ofh) != 1) {
    rc = MP4TAG_ERR_FILE_WRITE_ERROR;
  }
  free (buff);
  return rc;
}

static void
mp4tag_update_offsets (libmp4tag_t *libmp4tag, FILE *ofh,
    int32_t delta, uint64_t foffset)
{
  /* stco and co64 have different offsets sizes, */
  /* otherwise seem to be the same */

  if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_WRITE)) {
    if (libmp4tag->stco_offset != 0 || libmp4tag->co64_offset != 0) {
      fprintf (stdout, "  update-offsets\n");
      fprintf (stdout, "    delta: %d\n", delta);
    }
  }

  mp4tag_update_offset_block (libmp4tag, ofh, delta, foffset,
      libmp4tag->stco_offset, libmp4tag->stco_len, sizeof (uint32_t));
  mp4tag_update_offset_block (libmp4tag, ofh, delta, foffset,
      libmp4tag->co64_offset, libmp4tag->co64_len, sizeof (uint64_t));
}

static void
mp4tag_update_offset_block (libmp4tag_t *libmp4tag, FILE *ofh, int32_t delta,
    uint64_t foffset, uint64_t boffset, uint32_t blen, int offsetsz)
{
  int       rc;
  char      *buff;
  char      *dptr;
  uint32_t  t32;
  uint64_t  t64;
  uint32_t  numoffsets;

  /* stco has 4 bytes version/flags, 4 bytes number of offsets */
  /* and 32-bit offsets */
  /* co64 has 4 bytes version/flags, 4 bytes number of offsets */
  /* and 64-bit offsets */

  if (boffset == 0) {
    /* block does not exist */
    return;
  }

  if (boffset >= foffset) {
    if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_WRITE)) {
      fprintf (stdout, "    boffset-b4: %" PRId64 "\n", boffset);
    }
    boffset += delta;
  }

  if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_WRITE)) {
    fprintf (stdout, "    offsetsz: %d %s\n", offsetsz, offsetsz == sizeof (uint32_t) ? "stco" : "co64");
    fprintf (stdout, "    foffset: %ld\n", (long) foffset);
    fprintf (stdout, "    boffset: %" PRId64 "\n", boffset);
    fprintf (stdout, "    blen: %d\n", blen);
  }
  rc = fseek (ofh, boffset, SEEK_SET);
  if (rc != 0) {
    libmp4tag->mp4error = MP4TAG_ERR_FILE_SEEK_ERROR;
    return;
  }

  buff = malloc (blen);
  if (buff == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
    return;
  }

  if (fread (buff, blen, 1, ofh) != 1) {
    libmp4tag->mp4error = MP4TAG_ERR_FILE_READ_ERROR;
    return;
  }

  dptr = buff;
  /* appears that the documentation I am using is incorrect about stco */
  /* stco does have a version/flags value */
  memcpy (&t32, dptr, sizeof (uint32_t));
  dptr += sizeof (uint32_t);
  memcpy (&t32, dptr, sizeof (uint32_t));
  dptr += sizeof (uint32_t);
  numoffsets = be32toh (t32);
  if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_WRITE)) {
    fprintf (stdout, "    num-offsets: %d\n", numoffsets);
  }

  for (uint32_t i = 0; i < numoffsets; ++i) {
    if (offsetsz == sizeof (uint32_t)) {
      memcpy (&t32, dptr, sizeof (uint32_t));
      t32 = be32toh (t32);
      if (t32 > foffset) {
        t32 += delta;
        t32 = htobe32 (t32);
        memcpy (dptr, &t32, sizeof (uint32_t));
      }
    }
    if (offsetsz == sizeof (uint64_t)) {
      memcpy (&t64, dptr, sizeof (uint64_t));
      t64 = be64toh (t64);
      if (t64 > foffset) {
        t64 += delta;
        t64 = htobe64 (t64);
        memcpy (dptr, &t64, sizeof (uint64_t));
      }
    }
    dptr += offsetsz;
  }

  rc = fseek (ofh, boffset, SEEK_SET);
  if (rc != 0) {
    libmp4tag->mp4error = MP4TAG_ERR_FILE_SEEK_ERROR;
    return;
  }

  if (fwrite (buff, blen, 1, ofh) != 1) {
    libmp4tag->mp4error = MP4TAG_ERR_FILE_WRITE_ERROR;
    return;
  }

  free (buff);
}

static char *
mp4tag_build_append (libmp4tag_t *libmp4tag, int idx,
    char *data, uint32_t *dlen)
{
  mp4tag_t    *mp4tag;
  uint32_t    tlen;
  uint32_t    savelen;
  uint64_t    t64;
  char        *dptr;
  char        tnm [MP4TAG_ID_LEN + 1];
  bool        iscustom = false;
  char        *appname = NULL;
  size_t      appnamelen;
  char        *customname = NULL;
  size_t      customnamelen;
  char        *custom = NULL;
  char        tempnm [TEMP_NM_SZ];
  size_t      nmlen;


  mp4tag = &libmp4tag->tags [idx];

  if (mp4tag->tag == NULL) {
    return data;
  }

  if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_WRITE)) {
    fprintf (stdout, "name: %s type: %02x dataidx: %d\n", mp4tag->tag, mp4tag->identtype, mp4tag->dataidx);
    fprintf (stdout, "  int-len: %d data-len: %d\n", mp4tag->internallen, mp4tag->datalen);
    fflush (stdout);
  }

  savelen = mp4tag->internallen;
  if (mp4tag->identtype == MP4TAG_ID_STRING) {
    savelen = mp4tag->datalen;
  }
  if (strcmp (mp4tag->tag, boxids [MP4TAG_TRKN]) == 0) {
    /* track number may have been a short variant, and the */
    /* internal length is incorrect in that case. */
    savelen = sizeof (uint32_t) + sizeof (uint16_t) * 2;
  }
  if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_WRITE)) {
    fprintf (stdout, "  save-len: %d\n", savelen);
  }

  /* boxhead: idlen + ident */
  /* data: dlen + data-ident + data-flags + data-reserved */
  tlen = MP4TAG_BOXHEAD_SZ + MP4TAG_DATA_SZ + savelen;

  if (memcmp (mp4tag->tag, boxids [MP4TAG_CUSTOM], MP4TAG_ID_LEN) == 0) {
    char    *p;
    char    *tokstr;

    iscustom = true;
    custom = strdup (mp4tag->tag);

    /* the ident, don't need to save this, handled below */
    p = strtok_r (custom, MP4TAG_CUSTOM_DELIM, &tokstr);
    if (p == NULL) {
      free (custom);
      return data;
    }

    /* the appname string */
    p = strtok_r (NULL, MP4TAG_CUSTOM_DELIM, &tokstr);
    if (p == NULL) {
      free (custom);
      return data;
    }
    appname = p;

    /* the custom name */
    /* do not parse here, as there may be a : in the name */
    p += strlen (appname) + 1;
    if (p == NULL) {
      free (custom);
      return data;
    }
    customname = p;

    appnamelen = strlen (appname);
    customnamelen = strlen (customname);

    /* 'mean' len, 'mean' id, flags */
    tlen += sizeof (uint32_t) * 2 + MP4TAG_ID_LEN;
    tlen += appnamelen;
    /* 'name' len, 'name' id, flags */
    tlen += sizeof (uint32_t) * 2 + MP4TAG_ID_LEN;
    tlen += customnamelen;
  }

  /* the name is needed to check for arrays */

  /* ident */
  if (memcmp (mp4tag->tag, COPYRIGHT_STR, strlen (COPYRIGHT_STR)) == 0) {
    tnm [0] = (char) MP4TAG_PREFIX_CHAR;
    memcpy (tnm + 1, mp4tag->tag + strlen (COPYRIGHT_STR), 3);
    tnm [MP4TAG_ID_LEN] = '\0';
  } else {
    /* mp4tag->tag may be custom and much longer than 4 chars */
    memcpy (tnm, mp4tag->tag, MP4TAG_ID_LEN);
  }
  tnm [MP4TAG_ID_LEN] = '\0';
  strcpy (tempnm, tnm);
  nmlen = MP4TAG_ID_LEN;
  if (iscustom) {
    snprintf (tempnm + nmlen, sizeof (tempnm) - nmlen,
        "%s%.*s", MP4TAG_CUSTOM_DELIM, (int) appnamelen, appname);
    nmlen += appnamelen + strlen (MP4TAG_CUSTOM_DELIM);
    snprintf (tempnm + nmlen, sizeof (tempnm) - nmlen,
        "%s%.*s", MP4TAG_CUSTOM_DELIM, (int) customnamelen, customname);
  }

  /* array processing */
  /* save off the last name processed so that the datacount */
  /* can be reset when the name changes */
  if (strcmp (tempnm, libmp4tag->lastbox_nm) != 0) {
    libmp4tag->datacount = 0;
    libmp4tag->lastbox_offset = -1;
    strcpy (libmp4tag->lastbox_nm, tempnm);
  }

  if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_WRITE)) {
    fprintf (stdout, "  datacount: %d\n", libmp4tag->datacount);
    fprintf (stdout, "  lastbox offset %d\n", libmp4tag->lastbox_offset);
  }

  if (libmp4tag->datacount > 0) {
    /* if processing a second cover or array item, */
    /* do not allocate extra space for the */
    /* ident-len and ident */
    tlen -= MP4TAG_BOXHEAD_SZ;
    if (iscustom) {
      /* mean */
      tlen -= sizeof (uint32_t) * 2 + MP4TAG_ID_LEN;
      tlen -= appnamelen;
      /* name */
      tlen -= sizeof (uint32_t) * 2 + MP4TAG_ID_LEN;
      tlen -= customnamelen;
    }
  }

  if (mp4tag->covername != NULL) {
    /* if a cover name is present, include that length for the realloc */
    tlen += MP4TAG_BOXHEAD_SZ + strlen (mp4tag->covername);
  }

  data = realloc (data, *dlen + tlen);
  if (data == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
    return NULL;
  }
  dptr = data + *dlen;
  *dlen += tlen;

  if (mp4tag->covername != NULL) {
    /* back out the cover name size change */
    /* as that size is not part of the data box */
    tlen -= MP4TAG_BOXHEAD_SZ;
    tlen -= strlen (mp4tag->covername);
  }

  if (libmp4tag->lastbox_offset == -1) {
    /* save the offset for the start of the last box */
    /* it is needed for multiple covers/cover names and */
    /* for arrays of strings */
    libmp4tag->lastbox_offset = (int32_t) (dptr - data);
  }

  /* if this is a second cover, or an array, */
  /* the identifier has already been processed */
  if (libmp4tag->datacount == 0) {
    /* box length */
    dptr = mp4tag_append_len_32 (dptr, tlen);

    dptr = mp4tag_append_data (dptr, tnm, MP4TAG_ID_LEN);

    if (iscustom) {
      size_t      tmplen;

      /* update tlen to remove 'mean' */
      tlen -= sizeof (uint32_t) * 2 + MP4TAG_ID_LEN;
      tlen -= appnamelen;

      tmplen = sizeof (uint32_t) * 2 + MP4TAG_ID_LEN + appnamelen;
      dptr = mp4tag_append_len_32 (dptr, tmplen);
      dptr = mp4tag_append_data (dptr, boxids [MP4TAG_MEAN], MP4TAG_ID_LEN);
      dptr = mp4tag_append_len_32 (dptr, 0);
      dptr = mp4tag_append_data (dptr, appname, appnamelen);

      /* update tlen to remove 'name' */
      tlen -= sizeof (uint32_t) * 2 + MP4TAG_ID_LEN;
      tlen -= customnamelen;

      tmplen = sizeof (uint32_t) * 2 + MP4TAG_ID_LEN + customnamelen;
      dptr = mp4tag_append_len_32 (dptr, tmplen);
      dptr = mp4tag_append_data (dptr, boxids [MP4TAG_NAME], MP4TAG_ID_LEN);
      dptr = mp4tag_append_len_32 (dptr, 0);
      dptr = mp4tag_append_data (dptr, customname, customnamelen);
    }

    /* data length does not include ident len and ident */
    tlen -= MP4TAG_BOXHEAD_SZ;
  }

  dptr = mp4tag_append_len_32 (dptr, tlen);
  dptr = mp4tag_append_data (dptr, boxids [MP4TAG_DATA], MP4TAG_ID_LEN);

  /* data flags */
  dptr = mp4tag_append_len_32 (dptr, mp4tag->identtype);
  /* data reserved */
  dptr = mp4tag_append_len_32 (dptr, 0);

  if (mp4tag->identtype == MP4TAG_ID_STRING) {
    if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_WRITE)) {
      fprintf (stdout, "  string %.*s\n", (int) mp4tag->datalen, mp4tag->data);
    }
    dptr = mp4tag_append_data (dptr, mp4tag->data, mp4tag->datalen);
    if (libmp4tag->datacount > 0 && libmp4tag->lastbox_offset != -1) {
      mp4tag_update_data_len (libmp4tag, data, MP4TAG_DATA_SZ + mp4tag->datalen);
    }
  }
  if (mp4tag->identtype == MP4TAG_ID_NUM) {
    if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_WRITE)) {
      fprintf (stdout, "  numeric %s\n", mp4tag->data);
    }
    t64 = 0;
    if (mp4tag->data != NULL) {
      t64 = atoll (mp4tag->data);
    }
    if (mp4tag->internallen == sizeof (uint8_t)) {
      dptr = mp4tag_append_len_8 (dptr, t64);
    }
    if (mp4tag->internallen == sizeof (uint16_t)) {
      dptr = mp4tag_append_len_16 (dptr, t64);
    }
    if (mp4tag->internallen == sizeof (uint32_t)) {
      dptr = mp4tag_append_len_32 (dptr, t64);
    }
    if (mp4tag->internallen == sizeof (uint64_t)) {
      dptr = mp4tag_append_len_64 (dptr, t64);
    }
  }
  if (mp4tag->identtype == MP4TAG_ID_DATA) {
    int   ta = 0;
    int   tb = 0;

    if (strcmp (mp4tag->tag, boxids [MP4TAG_TRKN]) == 0) {
      mp4tag_parse_pair (mp4tag->data, &ta, &tb);
      if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_WRITE)) {
        fprintf (stdout, "  trkn %s %d %d\n", mp4tag->data, ta, tb);
      }
      dptr = mp4tag_append_len_32 (dptr, ta);
      dptr = mp4tag_append_len_16 (dptr, tb);
      /* trkn has an extra two bytes padding */
      dptr = mp4tag_append_len_16 (dptr, 0);
    } else if (strcmp (mp4tag->tag, boxids [MP4TAG_DISK]) == 0) {
      mp4tag_parse_pair (mp4tag->data, &ta, &tb);
      if (mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_WRITE)) {
        fprintf (stdout, "  disk %s %d %d\n", mp4tag->data, ta, tb);
      }
      dptr = mp4tag_append_len_32 (dptr, ta);
      dptr = mp4tag_append_len_16 (dptr, tb);
    } else {
      dptr = mp4tag_append_data (dptr, mp4tag->data, mp4tag->datalen);
    }
  }

  if (mp4tag->identtype == MP4TAG_ID_JPG ||
      mp4tag->identtype == MP4TAG_ID_PNG) {
    dptr = mp4tag_append_data (dptr, mp4tag->data, mp4tag->datalen);

    if (libmp4tag->datacount > 0 && libmp4tag->lastbox_offset != -1) {
      /* datalen + size of a data box */
      mp4tag_update_data_len (libmp4tag, data,
          MP4TAG_DATA_SZ + mp4tag->datalen);
    }
    if (mp4tag->covername != NULL && *mp4tag->covername) {
      uint32_t    cnmlen;
      uint32_t    tcnmlen;

      cnmlen = strlen (mp4tag->covername);
      tcnmlen = MP4TAG_BOXHEAD_SZ + cnmlen;
      dptr = mp4tag_append_len_32 (dptr, tcnmlen);
      dptr = mp4tag_append_data (dptr, boxids [MP4TAG_NAME], MP4TAG_ID_LEN);
      dptr = mp4tag_append_data (dptr, mp4tag->covername, cnmlen);

      mp4tag_update_data_len (libmp4tag, data, tcnmlen);
    }
  }

  libmp4tag->datacount += 1;

  if (iscustom) {
    free (custom);
  }

  return data;
}

static void
mp4tag_parse_pair (const char *data, int *a, int *b)
{
  int   rc;

  rc = sscanf (data, "%d/%d", a, b);
  if (rc == 1 || rc == 2) {
    return;
  }
  /* try using mutagen output format */
  rc = sscanf (data, "(%d,%d)", a, b);
}

static char *
mp4tag_append_data (char *dptr, const char *tnm, uint32_t sz)
{
  memcpy (dptr, tnm, sz);
  dptr += sz;
  return dptr;
}

static char *
mp4tag_append_len_8 (char *dptr, uint64_t val)
{
  uint8_t    t8;

  t8 = (uint8_t) val;
  memcpy (dptr, &t8, sizeof (uint8_t));
  dptr += sizeof (uint8_t);
  return dptr;
}

static char *
mp4tag_append_len_16 (char *dptr, uint64_t val)
{
  uint16_t    t16;

  t16 = htobe16 (val);
  memcpy (dptr, &t16, sizeof (uint16_t));
  dptr += sizeof (uint16_t);
  return dptr;
}

static char *
mp4tag_append_len_32 (char *dptr, uint64_t val)
{
  uint32_t    t32;

  t32 = htobe32 (val);
  memcpy (dptr, &t32, sizeof (uint32_t));
  dptr += sizeof (uint32_t);
  return dptr;
}

static char *
mp4tag_append_len_64 (char *dptr, uint64_t val)
{
  uint64_t    t64;

  t64 = htobe64 (val);
  memcpy (dptr, &t64, sizeof (uint64_t));
  dptr += sizeof (uint64_t);
  return dptr;
}

static void
mp4tag_update_data_len (libmp4tag_t *libmp4tag, char *data, uint32_t len)
{
  uint32_t    t32;
  char        *coverstart;

  if (libmp4tag->lastbox_offset == -1) {
    return;
  }

  coverstart = data + libmp4tag->lastbox_offset;
  memcpy (&t32, coverstart, sizeof (uint32_t));
  t32 = be32toh (t32);
  t32 += len;
  t32 = htobe32 (t32);
  memcpy (coverstart, &t32, sizeof (uint32_t));
}

static int
mp4tag_copy_file_data (FILE *ifh, FILE *ofh, uint64_t offset, size_t len)
{
  char    *data;
  size_t  rlen = 0;
  size_t  bread = 0;
  size_t  bwrite = 0;
  size_t  bremain = len;
  size_t  totwrite = 0;
  int     rc = MP4TAG_OK;

  if (fseek (ifh, offset, SEEK_SET) != 0) {
    return MP4TAG_ERR_FILE_SEEK_ERROR;
  }

  data = malloc (MP4TAG_COPY_SIZE);
  if (data == NULL) {
    rc = MP4TAG_ERR_OUT_OF_MEMORY;
    return rc;
  }

  while (totwrite < len) {
    rlen = MP4TAG_COPY_SIZE;
    if (bremain < rlen) {
      rlen = bremain;
    }
    bread = fread (data, 1, rlen, ifh);
    if (bread <= 0) {
      break;
    }
    bwrite = fwrite (data, 1, bread, ofh);
    if (bwrite != bread) {
      rc = MP4TAG_ERR_FILE_WRITE_ERROR;
      return rc;
    }
    totwrite += bwrite;
    bremain -= bwrite;
  }
  if (totwrite != len) {
    rc = MP4TAG_ERR_FILE_WRITE_ERROR;
  }

  free (data);

  return rc;
}

static void
mp4tag_debug_write_vals (libmp4tag_t *libmp4tag, uint32_t datalen,
    int32_t delta, int32_t totdelta, int32_t freelen)
{
  if (! mp4tag_chk_dbg (libmp4tag, MP4TAG_DBG_WRITE)) {
    return;
  }

  fprintf (stdout, " taglist-orig-len: %d\n", libmp4tag->taglist_orig_len);
  fprintf (stdout, "      taglist-len: %d\n", libmp4tag->taglist_len);
  fprintf (stdout, "          datalen: %d\n", datalen);
  fprintf (stdout, "            delta: %d\n", delta);
  if (totdelta != -1) {
    fprintf (stdout, "         totdelta: %d\n", totdelta);
  }
  fprintf (stdout, "interior-free-len: %d\n", libmp4tag->interior_free_len);
  fprintf (stdout, "exterior-free-len: %d\n", libmp4tag->exterior_free_len);
  fprintf (stdout, "          freelen: %d\n", freelen);
}
