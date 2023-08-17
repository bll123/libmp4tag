/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "libmp4tag.h"
#include "libmp4tagint.h"

void
mp4tag_add_tag (libmp4tag_t *libmp4tag, const char *nm, const char *data, ssize_t sz)
{
  int   tagidx;

  tagidx = libmp4tag->tagcount;
  if (tagidx >= libmp4tag->tagalloccount) {
    libmp4tag->tagalloccount += 10;
    libmp4tag->tags = realloc (libmp4tag->tags,
        sizeof (mp4tag_t) * libmp4tag->tagalloccount);
  }
  libmp4tag->tags [tagidx].name = NULL;
  libmp4tag->tags [tagidx].data = NULL;
  libmp4tag->tags [tagidx].binary = false;

  libmp4tag->tags [tagidx].name = strdup (nm);
  if (sz == MP4TAG_STRING) {
    /* string with null terminator */
    libmp4tag->tags [tagidx].data = strdup (data);
  } else if (sz < 0) {
    /* string w/o null terminator */
    sz = - sz;
    libmp4tag->tags [tagidx].data = malloc (sz + 1);
    memcpy (libmp4tag->tags [tagidx].data, data, sz);
    libmp4tag->tags [tagidx].data [sz] = '\0';
  } else {
    /* binary data */
    libmp4tag->tags [tagidx].data = malloc (sz);
    memcpy (libmp4tag->tags [tagidx].data, data, sz);
    libmp4tag->tags [tagidx].binary = true;
  }
  libmp4tag->tagcount += 1;
}
