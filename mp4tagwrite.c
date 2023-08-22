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

static void mp4tag_write_inplace (libmp4tag_t *libmp4tag, const char *data, uint32_t datalen);
static void mp4tag_write_rewrite (libmp4tag_t *libmp4tag, const char *data, uint32_t datalen);
static void mp4tag_update_parent_offsets (libmp4tag_t *libmp4tag, int32_t delta, int idx);
static char * mp4tag_build_append (libmp4tag_t *libmp4tag, int idx, char *data, uint32_t *dlen);
static void mp4tag_parse_pair (const char *data, int *a, int *b);
static char * mp4tag_append_data (char *dptr, const char *tnm, uint32_t sz);
static char * mp4tag_append_len_8 (char *dptr, uint64_t val);
static char * mp4tag_append_len_16 (char *dptr, uint64_t val);
static char * mp4tag_append_len_32 (char *dptr, uint64_t val);
static char * mp4tag_append_len_64 (char *dptr, uint64_t val);
static void mp4tag_update_cover_len (libmp4tag_t *libmp4tag, char *data, uint32_t len);

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

  tlen = libmp4tag->taglist_len;
  tlen -= sizeof (uint32_t) - MP4TAG_ID_LEN;

  libmp4tag->errornum = MP4TAG_ERR_NOT_IMPLEMENTED;

  /* in order to do an in-place write, the space to receive the data */
  /* must be exactly equal in size, or must have room for the data and */
  /* a free space block, or the 'ilst/free' is at the end of the file. */
// fprintf (stdout, "offset: %08lx\n", (long) libmp4tag->taglist_offset);
// fprintf (stdout, "taglist-len: %ld\n", (long) libmp4tag->taglist_len);
// fprintf (stdout, "datalen: %ld\n", (long) datalen);
// fprintf (stdout, "tlen: %ld\n", (long) tlen);
// fprintf (stdout, "unlimited: %ld\n", (long) libmp4tag->unlimited);
  if (libmp4tag->taglist_offset != 0 &&
      (libmp4tag->unlimited ||
      datalen == libmp4tag->taglist_len ||
      datalen < tlen)) {
// fprintf (stdout, "  -- in-place\n");
    mp4tag_write_inplace (libmp4tag, data, datalen);
    libmp4tag->errornum = MP4TAG_OK;
  } else {
// fprintf (stdout, "  -- rewrite\n");
    mp4tag_write_rewrite (libmp4tag, data, datalen);
    libmp4tag->errornum = MP4TAG_ERR_NOT_IMPLEMENTED;
  }

#if 0 // DEBUGGING
{
  FILE *fh;
  fh = fopen ("out.dat", "wb");
  fwrite (data, datalen, 1, fh);
  fclose (fh);
}
#endif

  return libmp4tag->errornum;
}

static void
mp4tag_write_inplace (libmp4tag_t *libmp4tag, const char *data,
    uint32_t datalen)
{
  if (libmp4tag->fh != NULL) {
    if (fseek (libmp4tag->fh, libmp4tag->taglist_offset, SEEK_SET) == 0) {
      int32_t   delta;
      int       idx;

      fwrite (data, datalen, 1, libmp4tag->fh);

      delta = (int32_t) datalen - (int32_t) libmp4tag->taglist_len;
// fprintf (stdout, "taglist-len: %d\n", libmp4tag->taglist_len);
// fprintf (stdout, "    datalen: %d\n", datalen);
// fprintf (stdout, "      delta: %d\n", delta);
      if (delta < 0) {
        int     freelen;
        char    *buff;

        freelen = libmp4tag->taglist_len - datalen;
// fprintf (stdout, "    freelen: %d\n", freelen);
        delta += freelen;
// fprintf (stdout, "    delta-f: %d\n", delta);
        if (freelen > 8) {
          buff = malloc (freelen);
          if (buff != NULL) {
            uint32_t    t32;

            memset (buff, '\0', freelen);
            t32 = htobe32 (freelen);
            memcpy (buff, &t32, sizeof (uint32_t));
            memcpy (buff + sizeof (uint32_t), MP4TAG_FREE, MP4TAG_ID_LEN);
            fwrite (buff, freelen, 1, libmp4tag->fh);
            free (buff);
          }
        }
      } /* if a free box needs to be added */

      if (delta != 0) {
        /* the base offset count is currently pointing at a tag, one level */
        /* higher than the ilst tag. */
// fprintf (stdout, "taglist-base-offset: %08lx\n", libmp4tag->taglist_base_offset);
        if (fseek (libmp4tag->fh, libmp4tag->taglist_base_offset, SEEK_SET) == 0) {
          uint32_t    t32;

t32 = libmp4tag->taglist_len + sizeof (uint32_t) + MP4TAG_ID_LEN;
// fprintf (stdout, "old-ilst-len: %d\n", t32);
          t32 = datalen + sizeof (uint32_t) + MP4TAG_ID_LEN;
// fprintf (stdout, "    upd-ilst: %d\n", t32);
          t32 = htobe32 (t32);
          fwrite (&t32, sizeof (uint32_t), 1, libmp4tag->fh);
        }
      }

      /* if the 'ilst' has grown in size, the parent offsets must be updated */
      if (delta > 0) {
        idx = libmp4tag->base_offset_count - 2;
        mp4tag_update_parent_offsets (libmp4tag, delta, idx);
      }

    } /* fseek is ok */
  }
}

static void
mp4tag_write_rewrite (libmp4tag_t *libmp4tag, const char *data,
    uint32_t datalen)
{
}

static void
mp4tag_update_parent_offsets (libmp4tag_t *libmp4tag, int32_t delta, int idx)
{
  while (idx >= 0) {
    if (fseek (libmp4tag->fh, libmp4tag->base_offsets [idx], SEEK_SET) == 0) {
      uint32_t    t32;

// fprintf (stdout, "p: idx: %d %s old-len: %d\n", idx, libmp4tag->base_name [idx], libmp4tag->base_lengths [idx]);
      t32 = libmp4tag->base_lengths [idx] + delta;
// fprintf (stdout, "p: idx: %d %s new-len: %d\n", idx, libmp4tag->base_name [idx], t32);
      t32 = htobe32 (t32);
      fwrite (&t32, sizeof (uint32_t), 1, libmp4tag->fh);
    }
    --idx;
  }
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

  /* idlen + ident + dlen + data-ident + data-flags + data-reserved = 6 */
fprintf (stdout, "name: %s type: %02x\n", mp4tag->tag, mp4tag->identtype);
// fprintf (stdout, " int-len: %d\n", mp4tag->internallen);
// fprintf (stdout, " data-len: %d\n", mp4tag->datalen);
  savelen = mp4tag->internallen;
  if (mp4tag->identtype == MP4TAG_ID_STRING) {
    savelen = mp4tag->datalen;
  }
  tlen = sizeof (uint32_t) * 4 + MP4TAG_ID_LEN * 2 + savelen;

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
    p = strtok_r (NULL, MP4TAG_CUSTOM_DELIM, &tokstr);
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
    tlen -= sizeof (uint32_t);
    tlen -= MP4TAG_ID_LEN;
  }

  if (mp4tag->covername != NULL) {
    /* if a cover name is present, include that length for the realloc */
    tlen += strlen (mp4tag->covername) + sizeof (uint32_t) + MP4TAG_ID_LEN;
  }

  data = realloc (data, *dlen + tlen);
  if (data == NULL) {
    libmp4tag->errornum = MP4TAG_ERR_OUT_OF_MEMORY;
    return NULL;
  }
  dptr = data + *dlen;
  *dlen += tlen;
// fprintf (stdout, "tot-datalen: %d\n", *dlen);

  if (mp4tag->covername != NULL) {
    /* back out the cover name size change */
    /* as that size is not part of the data box */
    tlen -= strlen (mp4tag->covername) + sizeof (uint32_t) + MP4TAG_ID_LEN;
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
    tlen -= sizeof (uint32_t);
    tlen -= MP4TAG_ID_LEN;
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
          mp4tag->datalen + sizeof (uint32_t) * 3 + MP4TAG_ID_LEN);
    }
    if (mp4tag->covername != NULL && *mp4tag->covername) {
      uint32_t    cnlen;
      uint32_t    tcnlen;

      cnlen = strlen (mp4tag->covername);
      tcnlen = cnlen + sizeof (uint32_t) + MP4TAG_ID_LEN;
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
