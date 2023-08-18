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

char *
mp4tag_build_data (libmp4tag_t *libmp4tag)
{
  char      *data = NULL;
  uint32_t  dlen = 0;

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

{
  FILE *fh;
  fh = fopen ("out.dat", "wb");
  fwrite (data, dlen, 1, fh);
  fclose (fh);
}

  return data;
}

static char *
mp4tag_build_append (libmp4tag_t *libmp4tag, int idx, char *data, uint32_t *dlen)
{
  mp4tag_t    *mp4tag;
  uint32_t    tlen;
  uint64_t    t64;
  uint32_t    t32;
  uint16_t    t16;
  uint8_t     t8;
  char        *dptr;
  char        tnm [MP4TAG_ID_LEN + 1];

  mp4tag = &libmp4tag->tags [idx];

  if (mp4tag->name == NULL) {
    return data;
  }

// ### FIX ---- has to be handled differently

  /* idlen, ident, dlen, data-ident, data-flags, data-reserved, data */
  tlen = sizeof (uint32_t) * 6 + mp4tag->internallen;
fprintf (stderr, "name: %s\n", mp4tag->name);
fprintf (stderr, "datalen: %d\n", mp4tag->datalen);
fprintf (stderr, "dlen: %d tlen: %d new: %d\n", *dlen, tlen, *dlen + tlen);
  data = realloc (data, *dlen + tlen);
  dptr = data + *dlen;
fprintf (stderr, "data: %p\n", data);
fprintf (stderr, "dptr: %p\n", dptr);
  *dlen += tlen;
fprintf (stderr, "new dlen: %d\n", *dlen);

  t32 = htobe32 (tlen);
  /* box length */
  memcpy (dptr, &t32, sizeof (uint32_t));
  dptr += sizeof (uint32_t);

  /* ident */
  if (memcmp (mp4tag->name, COPYRIGHT_STR, strlen (COPYRIGHT_STR)) == 0) {
    tnm [0] = '\xa9';
    memcpy (tnm + 1, mp4tag->name + strlen (COPYRIGHT_STR), 3);
    tnm [MP4TAG_ID_LEN] = '\0';
  } else {
    strcpy (tnm, mp4tag->name);
  }
  memcpy (dptr, tnm, MP4TAG_ID_LEN);
  dptr += MP4TAG_ID_LEN;

  /* data length */
  tlen -= sizeof (uint32_t) * 2;
  t32 = htobe32 (tlen);
  memcpy (dptr, &t32, sizeof (uint32_t));
  dptr += sizeof (uint32_t);

  /* data ident */
  memcpy (dptr, MP4TAG_DATA, MP4TAG_ID_LEN);
  dptr += MP4TAG_ID_LEN;

  /* data flags */
  t32 = htobe32 (mp4tag->internalflags);
  memcpy (dptr, &t32, sizeof (uint32_t));
  dptr += sizeof (uint32_t);

  if ((mp4tag->internalflags & 0x00FFFFFF) == MP4TAG_ID_STRING) {
    memcpy (dptr, mp4tag->data, mp4tag->datalen);
    dptr += mp4tag->datalen;
  }
  if ((mp4tag->internalflags & 0x00FFFFFF) == MP4TAG_ID_NUM) {
    t64 = 0;
    if (mp4tag->data != NULL) {
      t64 = atoll (mp4tag->data);
    }
    if (tlen == 1) {
      t8 = (uint8_t) t64;
      memcpy (dptr, &t8, sizeof (uint8_t));
      dptr += sizeof (uint8_t);
    }
    if (tlen == 2) {
      t16 = (uint16_t) t64;
      t16 = htobe16 (t16);
      memcpy (dptr, &t16, sizeof (uint16_t));
      dptr += sizeof (uint16_t);
    }
    if (tlen == 4) {
      t32 = (uint32_t) t64;
      t32 = htobe16 (t32);
      memcpy (dptr, &t32, sizeof (uint32_t));
      dptr += sizeof (uint32_t);
    }
    if (tlen == 8) {
      t64 = htobe64 (t64);
      memcpy (dptr, &t64, sizeof (uint64_t));
      dptr += sizeof (uint64_t);
    }
  }
  if ((mp4tag->internalflags & 0x00FFFFFF) == MP4TAG_ID_DATA) {
    if (strcmp (mp4tag->name, MP4TAG_TRKN) == 0) {
    } else if (strcmp (mp4tag->name, MP4TAG_DISK) == 0) {
    } else {
fprintf (stderr, "dptr: %p\n", dptr);
fprintf (stderr, "b: dptr-data: %ld\n", (long) (dptr-data));
fprintf (stderr, "b: datalen: %d\n", mp4tag->datalen);
fprintf (stderr, "b: tlen: %d\n", tlen);
      memcpy (dptr, mp4tag->data, mp4tag->datalen);
      dptr += mp4tag->datalen;
    }
  }
  if ((mp4tag->internalflags & 0x00FFFFFF) == MP4TAG_ID_JPG ||
      (mp4tag->internalflags & 0x00FFFFFF) == MP4TAG_ID_PNG) {
    memcpy (dptr, mp4tag->data, mp4tag->datalen);
    dptr += mp4tag->datalen;
  }

  return data;
}
