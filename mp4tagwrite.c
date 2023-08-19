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
#include "libmp4tagint.h"

static char * mp4tag_build_append (libmp4tag_t *libmp4tag, int idx, char *data, uint32_t *dlen);
static void mp4tag_parse_pair (const char *data, int *a, int *b);

char *
mp4tag_build_data (libmp4tag_t *libmp4tag, uint32_t *datalen)
{
  char      *data = NULL;
  uint32_t  dlen = 0;

  *datalen = 0;

  if (libmp4tag == NULL) {
    return NULL;
  }

  for (int i = 0; i < libmp4tag->tagcount; ++i) {
    const mp4tagdef_t   *result = NULL;

    if (memcmp (libmp4tag->tags [i].name, MP4TAG_CUSTOM, MP4TAG_ID_LEN) == 0) {
      libmp4tag->tags [i].priority = MP4TAG_PRI_CUSTOM;
    } else {
      result = mp4tag_check_tag (libmp4tag->tags [i].name);
      if (result != NULL) {
        libmp4tag->tags [i].priority = result->priority;
      }
    }
  }

  for (int pri = 0; pri < MP4TAG_PRI_MAX; ++pri) {
    for (int i = 0; i < libmp4tag->tagcount; ++i) {
      if (libmp4tag->tags [i].priority == pri) {
        data = mp4tag_build_append (libmp4tag, i, data, &dlen);
      }
    }
  }

  *datalen = dlen;
  return data;
}

int
mp4tag_write_data (libmp4tag_t *libmp4tag, const char *data,
    uint32_t datalen, int flags)
{
  uint32_t  tlen = 0;
  int       rc;

  tlen = libmp4tag->taglist_len;
  tlen -= sizeof (uint32_t) - MP4TAG_ID_LEN;

  /* in order to do an in-place write, the space to receive the data */
  /* must be exactly equal in size, or must have room for the data and */
  /* a free space block, or the 'ilst' is at the end of the file. */
fprintf (stdout, "offset: %ld\n", (long) libmp4tag->taglist_offset);
fprintf (stdout, "taglist-len: %ld\n", (long) libmp4tag->taglist_len);
fprintf (stdout, "datalen: %ld\n", (long) datalen);
fprintf (stdout, "tlen: %ld\n", (long) tlen);
fprintf (stdout, "unlimited: %ld\n", (long) libmp4tag->unlimited);
  if (libmp4tag->taglist_offset != 0 &&
      (libmp4tag->unlimited ||
      datalen == libmp4tag->taglist_len ||
      datalen < tlen)) {
    FILE *fh;
fprintf (stdout, "  ok to write\n");

    fh = mp4tag_fopen (libmp4tag->fn, "rb+");
    if (fh != NULL) {
fprintf (stdout, "  open ok\n");
      if (fseek (fh, libmp4tag->taglist_offset, SEEK_SET) == 0) {
fprintf (stdout, "  seek ok\n");
        fwrite (data, datalen, 1, fh);

        if (datalen < libmp4tag->taglist_len) {
          int     freelen;
          char    *buff;

          freelen = libmp4tag->taglist_len - datalen;
          if (freelen > 8) {
            buff = malloc (freelen);
            if (buff != NULL) {
              uint32_t    t32;

              memset (buff, '\0', freelen);
              t32 = htobe32 (freelen);
              memcpy (buff, &t32, sizeof (uint32_t));
              memcpy (buff + sizeof (uint32_t), MP4TAG_FREE, MP4TAG_ID_LEN);
              fwrite (buff, freelen, 1, fh);
              free (buff);
            }
          }
        }
      }
      fclose (fh);
    }
    rc = MP4TAG_OK;
  } else {
    rc = MP4TAG_ERR_NOT_IMPLEMENTED;
  }

{
  FILE *fh;
  fh = fopen ("out.dat", "wb");
  fwrite (data, datalen, 1, fh);
  fclose (fh);
}

  return rc;
}

static char *
mp4tag_build_append (libmp4tag_t *libmp4tag, int idx,
    char *data, uint32_t *dlen)
{
  mp4tag_t    *mp4tag;
  uint32_t    tlen;
  uint32_t    savelen;
  uint64_t    t64;
  uint32_t    t32;
  uint16_t    t16;
  uint8_t     t8;
  char        *dptr;
  char        tnm [MP4TAG_ID_LEN + 1];
  bool        iscustom = false;
  char        *vendor = NULL;
  size_t      vendorlen;
  char        *customname = NULL;
  size_t      customnamelen;
  char        *custom = NULL;

  mp4tag = &libmp4tag->tags [idx];

  if (mp4tag->name == NULL) {
    return data;
  }

  /* idlen + ident + dlen + data-ident + data-flags + data-reserved = 6 */
// fprintf (stdout, "name: %s type: %02x\n", mp4tag->name, mp4tag->identtype);
// fprintf (stdout, " int-len: %d\n", mp4tag->internallen);
// fprintf (stdout, " data-len: %d\n", mp4tag->datalen);
  savelen = mp4tag->internallen;
  if (mp4tag->identtype == MP4TAG_ID_STRING) {
    savelen = mp4tag->datalen;
  }
  tlen = sizeof (uint32_t) * 6 + savelen;

  if (memcmp (mp4tag->name, MP4TAG_CUSTOM, MP4TAG_ID_LEN) == 0) {
    char    *p;
    char    *tokstr;

    iscustom = true;
    custom = strdup (mp4tag->name);

    /* the ident, don't need to save this */
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

    tlen += sizeof (uint32_t) * 3;    /* 'mean' len, 'mean' id, flags */
    vendorlen = strlen (vendor);
    tlen += vendorlen;
    tlen += sizeof (uint32_t) * 3;    /* 'name' len, 'name' id, flags */
    customnamelen = strlen (customname);
    tlen += customnamelen;
  }

  data = realloc (data, *dlen + tlen);
  if (data == NULL) {
    libmp4tag->errornum = MP4TAG_ERR_OUT_OF_MEMORY;
    return NULL;
  }
  dptr = data + *dlen;
  *dlen += tlen;

  t32 = htobe32 (tlen);
  /* box length */
  memcpy (dptr, &t32, sizeof (uint32_t));
  dptr += sizeof (uint32_t);

  /* ident */
  if (memcmp (mp4tag->name, PREFIX_STR, strlen (PREFIX_STR)) == 0) {
    tnm [0] = '\xa9';
    memcpy (tnm + 1, mp4tag->name + strlen (PREFIX_STR), 3);
    tnm [MP4TAG_ID_LEN] = '\0';
  } else {
    /* mp4tag->name may be custom and much longer than 4 chars */
    memcpy (tnm, mp4tag->name, MP4TAG_ID_LEN);
  }
  tnm [MP4TAG_ID_LEN] = '\0';
  memcpy (dptr, tnm, MP4TAG_ID_LEN);
  dptr += MP4TAG_ID_LEN;

  if (iscustom) {
    size_t      tmplen;

    /* update tlen to remove 'mean' */
    tlen -= sizeof (uint32_t) * 3;
    tlen -= vendorlen;

    tmplen = sizeof (uint32_t) * 3 + vendorlen;
    t32 = htobe32 (tmplen);
    memcpy (dptr, &t32, sizeof (uint32_t));
    dptr += sizeof (uint32_t);
    memcpy (dptr, MP4TAG_MEAN, MP4TAG_ID_LEN);
    dptr += sizeof (uint32_t);
    t32 = 0;
    memcpy (dptr, &t32, sizeof (uint32_t));
    dptr += sizeof (uint32_t);
    memcpy (dptr, vendor, vendorlen);
    dptr += vendorlen;

    /* update tlen to remove 'name' */
    tlen -= sizeof (uint32_t) * 3;
    tlen -= customnamelen;

    tmplen = sizeof (uint32_t) * 3 + customnamelen;
    t32 = htobe32 (tmplen);
    memcpy (dptr, &t32, sizeof (uint32_t));
    dptr += sizeof (uint32_t);
    memcpy (dptr, MP4TAG_NAME, MP4TAG_ID_LEN);
    dptr += sizeof (uint32_t);
    t32 = 0;
    memcpy (dptr, &t32, sizeof (uint32_t));
    dptr += sizeof (uint32_t);
    memcpy (dptr, customname, customnamelen);
    dptr += customnamelen;
  }

  /* data length does not include ident len and ident */
  tlen -= sizeof (uint32_t) * 2;
  t32 = htobe32 (tlen);
  memcpy (dptr, &t32, sizeof (uint32_t));
  dptr += sizeof (uint32_t);

  /* data ident */
  memcpy (dptr, MP4TAG_DATA, MP4TAG_ID_LEN);
  dptr += MP4TAG_ID_LEN;

  /* data flags */
  t32 = htobe32 (mp4tag->identtype);
  memcpy (dptr, &t32, sizeof (uint32_t));
  dptr += sizeof (uint32_t);

  /* data reserved */
  t32 = 0;
  memcpy (dptr, &t32, sizeof (uint32_t));
  dptr += sizeof (uint32_t);

  if (mp4tag->identtype == MP4TAG_ID_STRING) {
    memcpy (dptr, mp4tag->data, mp4tag->datalen);
    dptr += mp4tag->datalen;
  }
  if (mp4tag->identtype == MP4TAG_ID_NUM) {
    t64 = 0;
    if (mp4tag->data != NULL) {
      t64 = atoll (mp4tag->data);
    }
    if (mp4tag->internallen == 1) {
      t8 = (uint8_t) t64;
      memcpy (dptr, &t8, sizeof (uint8_t));
      dptr += sizeof (uint8_t);
    }
    if (mp4tag->internallen == 2) {
      t16 = (uint16_t) t64;
      t16 = htobe16 (t16);
      memcpy (dptr, &t16, sizeof (uint16_t));
      dptr += sizeof (uint16_t);
    }
    if (mp4tag->internallen == 4) {
      t32 = (uint32_t) t64;
      t32 = htobe16 (t32);
      memcpy (dptr, &t32, sizeof (uint32_t));
      dptr += sizeof (uint32_t);
    }
    if (mp4tag->internallen == 8) {
      t64 = htobe64 (t64);
      memcpy (dptr, &t64, sizeof (uint64_t));
      dptr += sizeof (uint64_t);
    }
  }
  if (mp4tag->identtype == MP4TAG_ID_DATA) {
    int   ta = 0;
    int   tb = 0;

    if (strcmp (mp4tag->name, MP4TAG_TRKN) == 0) {
      mp4tag_parse_pair (mp4tag->data, &ta, &tb);
      t32 = (uint32_t) ta;
      t32 = htobe32 (t32);
      memcpy (dptr, &t32, sizeof (uint32_t));
      dptr += sizeof (uint32_t);

      t16 = (uint16_t) tb;
      t16 = htobe16 (t16);
      memcpy (dptr, &t16, sizeof (uint16_t));
      dptr += sizeof (uint16_t);

      /* trkn has an extra two bytes */
      t16 = 0;
      memcpy (dptr, &t16, sizeof (uint16_t));
      dptr += sizeof (uint16_t);
    } else if (strcmp (mp4tag->name, MP4TAG_DISK) == 0) {
      mp4tag_parse_pair (mp4tag->data, &ta, &tb);

      t32 = (uint32_t) ta;
      t32 = htobe32 (t32);
      memcpy (dptr, &t32, sizeof (uint32_t));
      dptr += sizeof (uint32_t);

      t16 = (uint16_t) tb;
      t16 = htobe16 (t16);
      memcpy (dptr, &t16, sizeof (uint16_t));
      dptr += sizeof (uint16_t);
    } else {
      memcpy (dptr, mp4tag->data, mp4tag->datalen);
      dptr += mp4tag->datalen;
    }
  }
  if (mp4tag->identtype == MP4TAG_ID_JPG ||
      mp4tag->identtype == MP4TAG_ID_PNG) {
    memcpy (dptr, mp4tag->data, mp4tag->datalen);
    dptr += mp4tag->datalen;
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

  rc = sscanf (data, "(%d,%d)", a, b);
  if (rc == 2) {
    return;
  }
  /* try with a slash */
  rc = sscanf (data, "%d/%d", a, b);
}
