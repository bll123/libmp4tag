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
mp4tag_sort_tags (libmp4tag_t *libmp4tag)
{
  if (libmp4tag == NULL) {
    return;
  }
  if (libmp4tag->tags == NULL) {
    return;
  }

  /* it's available in libc ... */
  qsort (libmp4tag->tags, libmp4tag->tagcount, sizeof (mp4tag_t),
      mp4tag_compare);
  for (int i = 0; i < libmp4tag->tagcount; ++i) {
    libmp4tag->tags [i].idx = i;
  }
}

int
mp4tag_find_tag (libmp4tag_t *libmp4tag, const char *tag)
{
  mp4tag_t    key;
  mp4tag_t    *result;
  int         idx = MP4TAG_NOTFOUND;

  if (libmp4tag == NULL) {
    return -1;
  }
  if (tag == NULL) {
    return -1;
  }
  if (libmp4tag->tagcount == 0) {
    return -1;
  }
  if (libmp4tag->tags == NULL) {
    return -1;
  }

  key.name = (char *) tag;
  result = bsearch (&key, libmp4tag->tags, libmp4tag->tagcount,
      sizeof (mp4tag_t), mp4tag_compare);

  if (result != NULL) {
    idx = result->idx;
  }

  return idx;
}

bool
mp4tag_check_tag (libmp4tag_t *libmp4tag, const char *tag)
{
  mp4tagdef_t key;
  mp4tagdef_t *result;
  bool        rc = false;

  if (libmp4tag == NULL) {
    return rc;
  }
  if (tag == NULL) {
    return rc;
  }

  /* custom tags are always valid */
  if (strncmp (tag, "----", 4) == 0) {
    return true;
  }

  key.name = tag;
  result = bsearch (&key, mp4taglist, mp4taglistlen,
      sizeof (mp4tagdef_t), mp4tag_compare_list);

  if (result != NULL) {
    rc = true;
  }

  return rc;
}

int
mp4tag_compare (const void *a, const void *b)
{
  const mp4tag_t  *ta = a;
  const mp4tag_t  *tb = b;

  if (ta->name == NULL && tb->name == NULL) {
    return 0;
  }
  if (ta->name == NULL && tb->name != NULL) {
    return -1;
  }
  if (ta->name != NULL && tb->name == NULL) {
    return 1;
  }
  return strcmp (ta->name, tb->name);
}

int
mp4tag_compare_list (const void *a, const void *b)
{
  const mp4tagdef_t  *ta = a;
  const mp4tagdef_t  *tb = b;

  if (ta->name == NULL && tb->name == NULL) {
    return 0;
  }
  if (ta->name == NULL && tb->name != NULL) {
    return -1;
  }
  if (ta->name != NULL && tb->name == NULL) {
    return 1;
  }
  return strcmp (ta->name, tb->name);
}

void
mp4tag_add_tag (libmp4tag_t *libmp4tag, const char *nm,
    const char *data, ssize_t sz, uint32_t origflag, size_t origlen)
{
  int   tagidx;

  tagidx = libmp4tag->tagcount;
  if (tagidx >= libmp4tag->tagalloccount) {
    libmp4tag->tagalloccount += 10;
    libmp4tag->tags = realloc (libmp4tag->tags,
        sizeof (mp4tag_t) * libmp4tag->tagalloccount);
    if (libmp4tag->tags == NULL) {
      return;
    }
  }
  libmp4tag->tags [tagidx].name = NULL;
  libmp4tag->tags [tagidx].data = NULL;
  libmp4tag->tags [tagidx].binary = false;
  /* save these off so that writing the tags back out is easier */
  libmp4tag->tags [tagidx].internalflags = origflag;
  libmp4tag->tags [tagidx].internallen = origlen;

  libmp4tag->tags [tagidx].name = strdup (nm);
  if (sz == MP4TAG_STRING) {
    /* string with null terminator */
    libmp4tag->tags [tagidx].data = strdup (data);
    libmp4tag->tags [tagidx].datalen = strlen (data);
  } else if (sz < 0) {
    /* string w/o null terminator */
    sz = - sz;
    libmp4tag->tags [tagidx].data = malloc (sz + 1);
    if (libmp4tag->tags [tagidx].data != NULL) {
      memcpy (libmp4tag->tags [tagidx].data, data, sz);
    }
    libmp4tag->tags [tagidx].data [sz] = '\0';
    libmp4tag->tags [tagidx].datalen = sz;
  } else {
    /* binary data */
    libmp4tag->tags [tagidx].data = malloc (sz);
    if (libmp4tag->tags [tagidx].data != NULL) {
      memcpy (libmp4tag->tags [tagidx].data, data, sz);
    }
    libmp4tag->tags [tagidx].binary = true;
    libmp4tag->tags [tagidx].datalen = sz;
  }
  libmp4tag->tagcount += 1;
}

void
mp4tag_del_tag (libmp4tag_t *libmp4tag, int idx)
{
  if (libmp4tag == NULL) {
    return;
  }
  if (idx < 0 || idx >= libmp4tag->tagcount) {
    return;
  }

  mp4tag_free_tag_by_idx (libmp4tag, idx);

  for (int i = idx; i < libmp4tag->tagcount; ++i) {
    int     next;

    next = i + 1;
    if (next < libmp4tag->tagcount) {
      libmp4tag->tags [i] = libmp4tag->tags [next];
    }
  }

  libmp4tag->tagcount -= 1;
  libmp4tag->tags [libmp4tag->tagcount].name = NULL;
  libmp4tag->tags [libmp4tag->tagcount].data = NULL;
  for (int i = 0; i < libmp4tag->tagcount; ++i) {
    libmp4tag->tags [i].idx = i;
  }
}

void
mp4tag_free_tag_by_idx (libmp4tag_t *libmp4tag, int idx)
{
  if (libmp4tag == NULL) {
    return;
  }
  if (libmp4tag->tags == NULL) {
    return;
  }
  if (idx < 0 || idx >= libmp4tag->tagcount) {
    return;
  }

  if (libmp4tag->tags [idx].name != NULL) {
    free (libmp4tag->tags [idx].name);
  }
  if (libmp4tag->tags [idx].data != NULL) {
    free (libmp4tag->tags [idx].data);
  }
}
