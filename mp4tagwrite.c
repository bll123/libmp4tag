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

#include "libmp4tag.h"
#include "mp4tagint.h"
#include "mp4tagbe.h"

static int  mp4tag_write_inplace (libmp4tag_t *libmp4tag, const char *data, uint32_t datalen);
static int  mp4tag_write_rewrite (libmp4tag_t *libmp4tag, const char *data, uint32_t datalen);
static int  mp4tag_write_freebox (FILE *ofh, uint32_t freelen);
static void mp4tag_update_parent_lengths (libmp4tag_t *libmp4tag, FILE *ofh, int32_t delta);
static void mp4tag_update_offsets (libmp4tag_t *libmp4tag, FILE *ofh, int32_t delta, size_t foffset);
static void mp4tag_update_offset_block (libmp4tag_t *libmp4tag, FILE *ofh, int32_t delta, size_t foffset, uint32_t boffset, uint32_t blen, int offsetsz);
static char * mp4tag_build_append (libmp4tag_t *libmp4tag, int idx, char *data, uint32_t *dlen);
static void mp4tag_parse_pair (const char *data, int *a, int *b);
static char * mp4tag_append_data (char *dptr, const char *tnm, uint32_t sz);
static char * mp4tag_append_len_8 (char *dptr, uint64_t val);
static char * mp4tag_append_len_16 (char *dptr, uint64_t val);
static char * mp4tag_append_len_32 (char *dptr, uint64_t val);
static char * mp4tag_append_len_64 (char *dptr, uint64_t val);
static void mp4tag_update_cover_len (libmp4tag_t *libmp4tag, char *data, uint32_t len);
static int  mp4tag_copy_file_data (FILE *ifh, FILE *ofh, size_t offset, size_t len);

char *
mp4tag_build_data (libmp4tag_t *libmp4tag, uint32_t *datalen)
{
  char      *data = NULL;

  *datalen = 0;

  if (libmp4tag == NULL) {
    return NULL;
  }

  for (int i = 0; i < libmp4tag->tagcount; ++i) {
    const mp4tagdef_t   *result = NULL;

    if (memcmp (libmp4tag->tags [i].tag, MP4TAG_CUSTOM, MP4TAG_ID_LEN) == 0) {
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

  libmp4tag->coverstart_offset = -1;
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
  uint32_t  tlen = 0;

  /* tlen is the maximum size of an 'ilst' with a free block */
  /* for in-place writes */
  tlen = libmp4tag->taglist_len;
  tlen -= MP4TAG_BOXHEAD_SZ;

  libmp4tag->mp4error = MP4TAG_OK;

  /* in order to do an in-place write, the space to receive the data */
  /* a) must be exactly equal in size */
  /* b) or must have room for the data and a free space block */
  /* c) or the 'ilst/free' is at the end of the file */
  if (libmp4tag->dbgflags & MP4TAG_DBG_WRITE) {
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
      datalen < tlen)) {
    if (libmp4tag->dbgflags & MP4TAG_DBG_WRITE) {
      fprintf (stdout, "-- write: in-place\n");
    }
    mp4tag_write_inplace (libmp4tag, data, datalen);
  } else {
    if (libmp4tag->dbgflags & MP4TAG_DBG_WRITE) {
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
  int32_t   delta;
  int32_t   freelen;

  if (libmp4tag->fh == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_NOT_OPEN;
    return libmp4tag->mp4error;
  }

  if (fseek (libmp4tag->fh, libmp4tag->taglist_offset, SEEK_SET) != 0) {
    libmp4tag->mp4error = MP4TAG_ERR_FILE_SEEK_ERROR;
    return libmp4tag->mp4error;
  }

  if (fwrite (data, datalen, 1, libmp4tag->fh) != 1) {
    libmp4tag->mp4error = MP4TAG_ERR_FILE_WRITE_ERROR;
    return libmp4tag->mp4error;
  }

  freelen = 0;
  delta = (int32_t) datalen - (int32_t) libmp4tag->taglist_len;
  if (libmp4tag->dbgflags & MP4TAG_DBG_WRITE) {
    fprintf (stdout, "  taglist-len: %d\n", libmp4tag->taglist_len);
    fprintf (stdout, "      datalen: %d\n", datalen);
    fprintf (stdout, "        delta: %d\n", delta);
  }
  if (delta < 0) {
    freelen = libmp4tag->taglist_len - datalen;
    if (libmp4tag->dbgflags & MP4TAG_DBG_WRITE) {
      fprintf (stdout, "      freelen: %d\n", freelen);
      fprintf (stdout, "      delta-f: %d\n", delta);
    }

    if (libmp4tag->unlimited && freelen < MP4TAG_FREE_SPACE_SZ) {
      /* this will also handle the situation where the 'ilst' shrinks */
      /* and there is not enough room for a free box */
      freelen = MP4TAG_BOXHEAD_SZ + MP4TAG_FREE_SPACE_SZ;
    }
    if (freelen > 8) {
      int     rc;

      rc = mp4tag_write_freebox (libmp4tag->fh, freelen);
      if (rc != MP4TAG_OK) {
        libmp4tag->mp4error = rc;
        return libmp4tag->mp4error;
      }
    }
  } /* if a free box needs to be added */

  if (delta != 0) {
    uint32_t    t32;
    int         rc;

// fprintf (stdout, "taglist-base-offset: %08lx\n", libmp4tag->taglist_base_offset);
    rc = fseek (libmp4tag->fh, libmp4tag->taglist_base_offset, SEEK_SET);
    if (rc != 0) {
      libmp4tag->mp4error = MP4TAG_ERR_FILE_SEEK_ERROR;
      return libmp4tag->mp4error;
    }

t32 = libmp4tag->taglist_len + MP4TAG_BOXHEAD_SZ;
// fprintf (stdout, "old-ilst-len: %d\n", t32);
    t32 = datalen + MP4TAG_BOXHEAD_SZ;
// fprintf (stdout, "    upd-ilst: %d\n", t32);
    t32 = htobe32 (t32);
    if (fwrite (&t32, sizeof (uint32_t), 1, libmp4tag->fh) != 1) {
      libmp4tag->mp4error = MP4TAG_ERR_FILE_WRITE_ERROR;
    }
  }

  /* if the 'ilst' box + 'free' box has changed in size, */
  /* the parent offsets must be updated */
  if (delta + freelen > 0) {
    mp4tag_update_parent_lengths (libmp4tag, libmp4tag->fh, delta);
  }

  return libmp4tag->mp4error;
}

static int
mp4tag_write_rewrite (libmp4tag_t *libmp4tag, const char *data,
    uint32_t datalen)
{
  FILE    *ofh;
  char    ofn [2048];
  int     rc;
  size_t  offset;
  size_t  wlen;
  int32_t delta;

  libmp4tag->mp4error = MP4TAG_OK;

  if (libmp4tag->fh == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_NOT_OPEN;
    return libmp4tag->mp4error;
  }

  if (libmp4tag->taglist_offset == 0 && libmp4tag->udta_offset == 0) {
    libmp4tag->mp4error = MP4TAG_ERR_UNABLE_TO_PROCESS;
    return libmp4tag->mp4error;
  }

  snprintf (ofn, sizeof (ofn), "%s.tmp", libmp4tag->fn);
  ofh = mp4tag_fopen (ofn, "wb+");
  if (ofh == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_NOT_OPEN;
    return libmp4tag->mp4error;
  }

  offset = libmp4tag->taglist_offset;
  if (offset == 0) {
    offset = libmp4tag->udta_offset + MP4TAG_BOXHEAD_SZ;
  }
  /* this following code will set 'rc' to the mp4-error, */
  /* and libmp4tag->mp4error will be set to 'rc' on failure */
  /* once this code is complete */

  if (libmp4tag->dbgflags & MP4TAG_DBG_WRITE) {
    fprintf (stdout, "  copy-data: length:%ld\n", (long) offset);
  }
  rc = mp4tag_copy_file_data (libmp4tag->fh, ofh, 0, offset);

  if (rc == MP4TAG_OK &&
      libmp4tag->taglist_offset == 0 && libmp4tag->udta_offset != 0) {
    char    *buff;
    size_t  len;
    size_t  alloclen;
    int     trc;

    if (libmp4tag->dbgflags & MP4TAG_DBG_WRITE) {
      fprintf (stdout, "  no ilst, have udta, insert boxes\n");
    }
    /* there is no 'ilst' block in the audio file */
    /* a udta block is present */

    /* allocation size: meta + hdlr + ilst-box */
    alloclen = MP4TAG_META_SZ + MP4TAG_HDLR_SZ + MP4TAG_BOXHEAD_SZ;

    buff = malloc (alloclen);
    if (buff == NULL) {
      rc = MP4TAG_ERR_OUT_OF_MEMORY;
    }

    /* add udta and datalen to total length */
    /* and the size of the free block that will be added */
    len = alloclen + MP4TAG_BOXHEAD_SZ + datalen;
    len += MP4TAG_BOXHEAD_SZ + MP4TAG_FREE_SPACE_SZ;

    /* update the udta length */
    trc = fseek (ofh, libmp4tag->udta_offset, SEEK_SET);
    if (trc != 0) {
      rc = MP4TAG_ERR_FILE_SEEK_ERROR;
    }
    if (rc == MP4TAG_OK) {
      uint32_t    t32;

      t32 = htobe32 (len);
      trc = fwrite (&t32, sizeof (uint32_t), 1, ofh);
      if (trc != 1) {
        rc = MP4TAG_ERR_FILE_WRITE_ERROR;
      }
      if (rc == MP4TAG_OK) {
        trc = fseek (ofh, 0, SEEK_END);
        if (trc != 0) {
          rc = MP4TAG_ERR_FILE_SEEK_ERROR;
        }
      }
    }

    if (rc == MP4TAG_OK) {
      uint32_t    h32;
      char        *dptr;

      dptr = buff;

      /* meta */
      len -= MP4TAG_BOXHEAD_SZ;
      dptr = mp4tag_append_len_32 (dptr, len);
      dptr = mp4tag_append_data (dptr, MP4TAG_META, MP4TAG_ID_LEN);
      /* meta version+flags */
      dptr = mp4tag_append_len_32 (dptr, 0);

      /* hdlr */
      h32 = MP4TAG_HDLR_SZ;
      dptr = mp4tag_append_len_32 (dptr, h32);
      dptr = mp4tag_append_data (dptr, MP4TAG_HDLR, MP4TAG_ID_LEN);
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
      len -= MP4TAG_BOXHEAD_SZ + MP4TAG_FREE_SPACE_SZ;
      dptr = mp4tag_append_len_32 (dptr, len);
      dptr = mp4tag_append_data (dptr, MP4TAG_ILST, MP4TAG_ID_LEN);

      if (fwrite (buff, alloclen, 1, ofh) != 1) {
        rc = MP4TAG_ERR_FILE_WRITE_ERROR;
      }
      free (buff);
    }
  }

  if (libmp4tag->dbgflags & MP4TAG_DBG_WRITE) {
    fprintf (stdout, "  data-offset: %ld\n", ftell (ofh));
    fprintf (stdout, "  tags: %ld\n", (long) datalen);
  }
  if (rc == MP4TAG_OK && fwrite (data, datalen, 1, ofh) != 1) {
    rc = MP4TAG_ERR_FILE_WRITE_ERROR;
  }

  if (libmp4tag->dbgflags & MP4TAG_DBG_WRITE) {
    fprintf (stdout, "  free-box: %ld\n", (long) (MP4TAG_BOXHEAD_SZ + MP4TAG_FREE_SPACE_SZ));
  }
  rc = mp4tag_write_freebox (ofh, MP4TAG_BOXHEAD_SZ + MP4TAG_FREE_SPACE_SZ);

  offset = libmp4tag->taglist_offset + libmp4tag->taglist_len;
  if (libmp4tag->taglist_offset == 0) {
    offset = libmp4tag->udta_offset + MP4TAG_BOXHEAD_SZ;
  }

  wlen = libmp4tag->filesz - offset;
  if (libmp4tag->dbgflags & MP4TAG_DBG_WRITE) {
    fprintf (stdout, "  file-size: %ld\n", (long) libmp4tag->filesz);
    fprintf (stdout, "  copy-final-data: offset:%ld length:%ld\n", (long) offset, (long) wlen);
  }
  if (rc == MP4TAG_OK) {
    rc = mp4tag_copy_file_data (libmp4tag->fh, ofh, offset, wlen);
  }

  /* will this work with large uints? */
  delta = (int32_t) datalen - (int32_t) libmp4tag->taglist_len;
  delta += MP4TAG_BOXHEAD_SZ + MP4TAG_FREE_SPACE_SZ;
  if (libmp4tag->dbgflags & MP4TAG_DBG_WRITE) {
    fprintf (stdout, "  delta: %d\n", delta);
  }

  /* if it is not an insert, update the parent offsets */
  if (rc == MP4TAG_OK && libmp4tag->taglist_offset != 0) {
    mp4tag_update_parent_lengths (libmp4tag, ofh, delta);
  }

  if (rc == MP4TAG_OK) {
    mp4tag_update_offsets (libmp4tag, ofh, delta, offset);
  }

  fclose (ofh);

  if (rc == MP4TAG_OK) {
    mp4tag_file_move (ofn, libmp4tag->fn);
  } else {
    mp4tag_file_delete (ofn);
  }

  if (rc != MP4TAG_OK) {
    libmp4tag->mp4error = rc;
  }

  return libmp4tag->mp4error;
}

static int
mp4tag_write_freebox (FILE *ofh, uint32_t freelen)
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
  memcpy (buff + sizeof (uint32_t), MP4TAG_FREE, MP4TAG_ID_LEN);
  if (fwrite (buff, freelen, 1, ofh) != 1) {
    rc = MP4TAG_ERR_FILE_WRITE_ERROR;
  }
  free (buff);
  return rc;
}

static void
mp4tag_update_parent_lengths (libmp4tag_t *libmp4tag, FILE *ofh, int32_t delta)
{
  int     idx;
  int     rc;

  idx = libmp4tag->parentidx;

  if (idx >= 0 && libmp4tag->dbgflags & MP4TAG_DBG_WRITE) {
    fprintf (stdout, "  update-parent-lengths\n");
    fprintf (stdout, "    delta: %d\n", delta);
  }

  while (idx >= 0) {
    uint32_t    t32;

    rc = fseek (ofh, libmp4tag->base_offsets [idx], SEEK_SET);
    if (rc != 0) {
      libmp4tag->mp4error = MP4TAG_ERR_FILE_SEEK_ERROR;
      return;
    }

    t32 = libmp4tag->base_lengths [idx] + delta;
    if (libmp4tag->dbgflags & MP4TAG_DBG_WRITE) {
      fprintf (stdout, "    update-parent: idx: %d %s len: %d / %d\n", idx, libmp4tag->base_name [idx], libmp4tag->base_lengths [idx], t32);
    }
    t32 = htobe32 (t32);
    if (fwrite (&t32, sizeof (uint32_t), 1, ofh) != 1) {
      libmp4tag->mp4error = MP4TAG_ERR_FILE_WRITE_ERROR;
    }
    --idx;
  }
}

static void
mp4tag_update_offsets (libmp4tag_t *libmp4tag, FILE *ofh,
    int32_t delta, size_t foffset)
{
  /* stco and co64 have different offsets sizes, */
  /* otherwise seem to be the same */

  if (libmp4tag->dbgflags & MP4TAG_DBG_WRITE) {
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
    size_t foffset, uint32_t boffset, uint32_t blen, int offsetsz)
{
  int       rc;
  char      *buff;
  char      *dptr;
  uint32_t  t32;
  uint64_t  t64;
  uint32_t  numoffsets;

  /* stco has 4 bytes number of offsets and 32-bit offsets */
  /* co64 has 4 bytes version/flags, 4 bytes number of offsets */
  /* and 64-bit offsets */

  if (boffset == 0) {
    /* block does not exist */
    return;
  }

  if (boffset >= foffset) {
    if (libmp4tag->dbgflags & MP4TAG_DBG_WRITE) {
      fprintf (stdout, "    boffset-b4: %d\n", boffset);
    }
    boffset += delta;
  }

  if (libmp4tag->dbgflags & MP4TAG_DBG_WRITE) {
    fprintf (stdout, "    foffset: %ld\n", (long) foffset);
    fprintf (stdout, "    boffset: %d\n", boffset);
    fprintf (stdout, "    blen: %d\n", blen);
    fprintf (stdout, "    offsetsz: %d\n", offsetsz);
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
  dptr += sizeof (uint32_t);
  memcpy (&t32, dptr, sizeof (uint32_t));
  dptr += sizeof (uint32_t);
  numoffsets = be32toh (t32);
  if (libmp4tag->dbgflags & MP4TAG_DBG_WRITE) {
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
      dptr += sizeof (uint32_t);
    }
    if (offsetsz == sizeof (uint64_t)) {
      memcpy (&t64, dptr, sizeof (uint64_t));
      t64 = be64toh (t64);
      if (t64 > foffset) {
        t64 += delta;
        t64 = htobe32 (t64);
        memcpy (dptr, &t64, sizeof (uint64_t));
      }
      dptr += sizeof (uint64_t);
    }
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
  char        *vendor = NULL;
  size_t      vendorlen;
  char        *customname = NULL;
  size_t      customnamelen;
  char        *custom = NULL;

  mp4tag = &libmp4tag->tags [idx];

  if (mp4tag->tag == NULL) {
    return data;
  }

// fprintf (stdout, "name: %s type: %02x\n", mp4tag->tag, mp4tag->identtype);
// fprintf (stdout, " int-len: %d\n", mp4tag->internallen);
// fprintf (stdout, " data-len: %d\n", mp4tag->datalen);
  savelen = mp4tag->internallen;
  if (mp4tag->identtype == MP4TAG_ID_STRING) {
    savelen = mp4tag->datalen;
  }
  /* boxhead: idlen + ident */
  /* data: dlen + data-ident + data-flags + data-reserved */
  tlen = MP4TAG_BOXHEAD_SZ + MP4TAG_DATA_SZ + savelen;

  if (memcmp (mp4tag->tag, MP4TAG_CUSTOM, MP4TAG_ID_LEN) == 0) {
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

    /* the vendor (my name) string */
    p = strtok_r (NULL, MP4TAG_CUSTOM_DELIM, &tokstr);
    if (p == NULL) {
      free (custom);
      return data;
    }
    vendor = p;

    /* the custom name */
    /* do not parse here, as there may be a : in the name */
    p += strlen (vendor) + 1;
    if (p == NULL) {
      free (custom);
      return data;
    }
    customname = p;

    /* 'mean' len, 'mean' id, flags */
    tlen += sizeof (uint32_t) * 2 + MP4TAG_ID_LEN;
    vendorlen = strlen (vendor);
    tlen += vendorlen;
    /* 'name' len, 'name' id, flags */
    tlen += sizeof (uint32_t) * 2 + MP4TAG_ID_LEN;
    customnamelen = strlen (customname);
    tlen += customnamelen;
  }

  if (mp4tag->coveridx > 0) {
    /* if processing a second cover, do not allocate extra space for the */
    /* ident-len and ident */
    tlen -= MP4TAG_BOXHEAD_SZ;
  }

  if (mp4tag->covername != NULL) {
    /* if a cover name is present, include that length for the realloc */
    tlen += MP4TAG_BOXHEAD_SZ + strlen (mp4tag->covername);
  }

fprintf (stdout, "build-append: add: %s %d %d\n", mp4tag->tag, *dlen, tlen);
  data = realloc (data, *dlen + tlen);
  if (data == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
    return NULL;
  }
  dptr = data + *dlen;
  *dlen += tlen;
// fprintf (stdout, "tot-datalen: %d\n", *dlen);

  if (mp4tag->covername != NULL) {
    /* back out the cover name size change */
    /* as that size is not part of the data box */
    tlen -= MP4TAG_BOXHEAD_SZ;
    tlen -= strlen (mp4tag->covername);
  }

  if (memcmp (mp4tag->tag, MP4TAG_COVR, MP4TAG_ID_LEN) == 0 &&
      libmp4tag->coverstart_offset == -1) {
    /* save the offset for the start of the cover box */
    /* if there are multiple covers or cover names, it is needed */
    libmp4tag->coverstart_offset = (int32_t) (dptr - data);
// fprintf (stdout, "save cover-start: %ld\n", (long) libmp4tag->coverstart_offset);
  }
if (memcmp (mp4tag->tag, MP4TAG_COVR, MP4TAG_ID_LEN) == 0) {
// fprintf (stdout, "cover-idx: %d\n", mp4tag->coveridx);
}

  /* if this is a second cover, the identifier is not added */
  if (mp4tag->coveridx == 0) {
    /* box length */
    dptr = mp4tag_append_len_32 (dptr, tlen);

    /* ident */
    if (memcmp (mp4tag->tag, PREFIX_STR, strlen (PREFIX_STR)) == 0) {
      tnm [0] = '\xa9';
      memcpy (tnm + 1, mp4tag->tag + strlen (PREFIX_STR), 3);
      tnm [MP4TAG_ID_LEN] = '\0';
    } else {
      /* mp4tag->tag may be custom and much longer than 4 chars */
      memcpy (tnm, mp4tag->tag, MP4TAG_ID_LEN);
    }
    tnm [MP4TAG_ID_LEN] = '\0';
    dptr = mp4tag_append_data (dptr, tnm, MP4TAG_ID_LEN);

    if (iscustom) {
      size_t      tmplen;

      /* update tlen to remove 'mean' */
      tlen -= sizeof (uint32_t) * 3;
      tlen -= vendorlen;

      tmplen = sizeof (uint32_t) * 3 + vendorlen;
      dptr = mp4tag_append_len_32 (dptr, tmplen);
      dptr = mp4tag_append_data (dptr, MP4TAG_MEAN, MP4TAG_ID_LEN);
      dptr = mp4tag_append_len_32 (dptr, 0);
      dptr = mp4tag_append_data (dptr, vendor, vendorlen);

      /* update tlen to remove 'name' */
      tlen -= sizeof (uint32_t) * 3;
      tlen -= customnamelen;

      tmplen = sizeof (uint32_t) * 3 + customnamelen;
      dptr = mp4tag_append_len_32 (dptr, tmplen);
      dptr = mp4tag_append_data (dptr, MP4TAG_NAME, MP4TAG_ID_LEN);
      dptr = mp4tag_append_len_32 (dptr, 0);
      dptr = mp4tag_append_data (dptr, customname, customnamelen);
    }

    /* data length does not include ident len and ident */
    tlen -= MP4TAG_BOXHEAD_SZ;
  }

  dptr = mp4tag_append_len_32 (dptr, tlen);
  dptr = mp4tag_append_data (dptr, MP4TAG_DATA, MP4TAG_ID_LEN);

  /* data flags */
  dptr = mp4tag_append_len_32 (dptr, mp4tag->identtype);
  /* data reserved */
  dptr = mp4tag_append_len_32 (dptr, 0);

  if (mp4tag->identtype == MP4TAG_ID_STRING) {
    dptr = mp4tag_append_data (dptr, mp4tag->data, mp4tag->datalen);
  }
  if (mp4tag->identtype == MP4TAG_ID_NUM) {
    t64 = 0;
    if (mp4tag->data != NULL) {
      t64 = atoll (mp4tag->data);
    }
    if (mp4tag->internallen == 1) {
      dptr = mp4tag_append_len_8 (dptr, t64);
    }
    if (mp4tag->internallen == 2) {
      dptr = mp4tag_append_len_16 (dptr, t64);
    }
    if (mp4tag->internallen == 4) {
      dptr = mp4tag_append_len_16 (dptr, t64);
    }
    if (mp4tag->internallen == 8) {
      dptr = mp4tag_append_len_64 (dptr, t64);
    }
  }
  if (mp4tag->identtype == MP4TAG_ID_DATA) {
    int   ta = 0;
    int   tb = 0;

    if (strcmp (mp4tag->tag, MP4TAG_TRKN) == 0) {
      mp4tag_parse_pair (mp4tag->data, &ta, &tb);
      dptr = mp4tag_append_len_32 (dptr, ta);
      dptr = mp4tag_append_len_16 (dptr, tb);
      /* trkn has an extra two bytes */
      dptr = mp4tag_append_len_16 (dptr, 0);
    } else if (strcmp (mp4tag->tag, MP4TAG_DISK) == 0) {
      mp4tag_parse_pair (mp4tag->data, &ta, &tb);

      dptr = mp4tag_append_len_32 (dptr, ta);
      dptr = mp4tag_append_len_16 (dptr, tb);
    } else {
      dptr = mp4tag_append_data (dptr, mp4tag->data, mp4tag->datalen);
    }
  }
  if (mp4tag->identtype == MP4TAG_ID_JPG ||
      mp4tag->identtype == MP4TAG_ID_PNG) {
    dptr = mp4tag_append_data (dptr, mp4tag->data, mp4tag->datalen);
    if (mp4tag->coveridx > 0 && libmp4tag->coverstart_offset != -1) {
      /* datalen + size of a data box */
      mp4tag_update_cover_len (libmp4tag, data,
          MP4TAG_DATA_SZ + mp4tag->datalen);
    }
    if (mp4tag->covername != NULL && *mp4tag->covername) {
      uint32_t    cnlen;
      uint32_t    tcnlen;

      cnlen = strlen (mp4tag->covername);
      tcnlen = MP4TAG_BOXHEAD_SZ + cnlen;
      dptr = mp4tag_append_len_32 (dptr, tcnlen);
      dptr = mp4tag_append_data (dptr, MP4TAG_NAME, MP4TAG_ID_LEN);
      dptr = mp4tag_append_data (dptr, mp4tag->covername, cnlen);

      mp4tag_update_cover_len (libmp4tag, data, tcnlen);
    }
  }

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
  if (rc == 2) {
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
mp4tag_update_cover_len (libmp4tag_t *libmp4tag, char *data, uint32_t len)
{
  uint32_t    t32;
  char        *coverstart;

  if (libmp4tag->coverstart_offset == -1) {
    return;
  }

  coverstart = data + libmp4tag->coverstart_offset;
  memcpy (&t32, coverstart, sizeof (uint32_t));
  t32 = be32toh (t32);
  t32 += len;
  t32 = htobe32 (t32);
  memcpy (coverstart, &t32, sizeof (uint32_t));
}

static int
mp4tag_copy_file_data (FILE *ifh, FILE *ofh, size_t offset, size_t len)
{
  char    *data;
  size_t  rlen = 0;
  size_t  bread = 0;
  size_t  bwrite = 0;
  size_t  bremain = len;
  size_t  totwrite = 0;
  int     rc = MP4TAG_OK;

  if (fseek (ifh, offset, SEEK_SET) == 0) {
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
  }

  return rc;
}
