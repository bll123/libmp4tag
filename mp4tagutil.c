/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>

#if _hdr_windows
# include <windows.h>
#endif

#include "libmp4tag.h"
#include "mp4tagint.h"

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
mp4tag_parse_cover_tag (const char *tag, int *pcoveridx)
{
  size_t    len;
  int       coveridx = -1;
  int       offset = -1;

  len = strlen (tag);
  if (len > MP4TAG_ID_LEN) {
    char    *tmp;
    char    *p;
    char    *tokstr;

    tmp = strdup (tag);
    p = strtok_r (tmp, ":", &tokstr);
    if (p != NULL) {
      p = strtok_r (NULL, ":", &tokstr);
      if (p != NULL) {
        coveridx = atoi (p);

        p = strtok_r (NULL, ":", &tokstr);
	if (p != NULL && strcmp (p, MP4TAG_NAME) == 0) {
	  offset = (int) (p - tmp);
	}
      }
    }
    free (tmp);
  }

  *pcoveridx = coveridx;
  return offset;
}

int
mp4tag_find_tag (libmp4tag_t *libmp4tag, const char *tag)
{
  mp4tag_t    key;
  mp4tag_t    *result;
  int         idx = MP4TAG_NOTFOUND;
  int         coveridx = 0;

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

  key.tag = (char *) tag;
  if (memcmp (tag, MP4TAG_COVR, MP4TAG_ID_LEN) == 0) {
    mp4tag_parse_cover_tag (tag, &coveridx);
    if (coveridx < 0) {
      coveridx = 0;
    }
    key.tag = (char *) MP4TAG_COVR;
  }
  key.coveridx = coveridx;
fprintf (stdout, "find: tag: %s cidx: %d\n", key.tag, key.coveridx);
  result = bsearch (&key, libmp4tag->tags, libmp4tag->tagcount,
      sizeof (mp4tag_t), mp4tag_compare);

  if (result != NULL) {
    idx = result->idx;
// fprintf (stdout, "ft: found at idx: %d\n", idx);
  }

  return idx;
}

mp4tagdef_t *
mp4tag_check_tag (const char *tag)
{
  mp4tagdef_t key;
  mp4tagdef_t *result;

  if (tag == NULL) {
    return NULL;
  }

  key.tag = tag;
  result = bsearch (&key, mp4taglist, mp4taglistlen,
      sizeof (mp4tagdef_t), mp4tag_compare_list);

  return result;
}

int
mp4tag_compare (const void *a, const void *b)
{
  const mp4tag_t  *ta = a;
  const mp4tag_t  *tb = b;
  int             rc;

  if (ta->tag == NULL && tb->tag == NULL) {
    return 0;
  } else if (ta->tag == NULL && tb->tag != NULL) {
    return -1;
  } else if (ta->tag != NULL && tb->tag == NULL) {
    return 1;
  }

  rc = strcmp (ta->tag, tb->tag);
  if (rc == 0) {
    if (ta->coveridx == tb->coveridx) {
      rc = 0;
    } else if (ta->coveridx < tb->coveridx) {
      rc = -1;
    } else if (ta->coveridx > tb->coveridx) {
      rc = 1;
    }
  }
  return rc;
}

int
mp4tag_compare_list (const void *a, const void *b)
{
  const mp4tagdef_t  *ta = a;
  const mp4tagdef_t  *tb = b;

  if (ta->tag == NULL && tb->tag == NULL) {
    return 0;
  }
  if (ta->tag == NULL && tb->tag != NULL) {
    return -1;
  }
  if (ta->tag != NULL && tb->tag == NULL) {
    return 1;
  }
  return strcmp (ta->tag, tb->tag);
}

void
mp4tag_add_tag (libmp4tag_t *libmp4tag, const char *tag,
    const char *data, ssize_t sz, uint32_t origflag, size_t origlen,
    const char *covername)
{
  int   tagidx;
  int   coveridx = -1;

  tagidx = libmp4tag->tagcount;
  if (tagidx >= libmp4tag->tagalloccount) {
    libmp4tag->tagalloccount += 10;
    libmp4tag->tags = realloc (libmp4tag->tags,
        sizeof (mp4tag_t) * libmp4tag->tagalloccount);
    if (libmp4tag->tags == NULL) {
      libmp4tag->errornum = MP4TAG_ERR_OUT_OF_MEMORY;
      return;
    }
  }

  libmp4tag->tags [tagidx].tag = NULL;
  libmp4tag->tags [tagidx].data = NULL;
  libmp4tag->tags [tagidx].covername = NULL;
  libmp4tag->tags [tagidx].coveridx = 0;
  libmp4tag->tags [tagidx].binary = false;
  /* save these off so that writing the tags back out is easier */
  libmp4tag->tags [tagidx].identtype = origflag;
  libmp4tag->tags [tagidx].internallen = origlen;

  libmp4tag->tags [tagidx].tag = strdup (tag);

  if (memcmp (tag, MP4TAG_COVR, MP4TAG_ID_LEN) == 0) {
// fprintf (stdout, "tag: %s\n", tag);

    mp4tag_parse_cover_tag (tag, &coveridx);
    /* make sure the base tag is set properly */
    free (libmp4tag->tags [tagidx].tag);
    libmp4tag->tags [tagidx].tag = strdup (MP4TAG_COVR);

    if (covername != NULL) {
fprintf (stdout, "tagidx: %d covername: %s\n", tagidx, covername);
      libmp4tag->tags [tagidx].covername = strdup (covername);
    }
    if (coveridx == -1) {
      libmp4tag->tags [tagidx].coveridx = libmp4tag->covercount;
      libmp4tag->covercount += 1;
// fprintf (stdout, "cidx: new\n");
    } else {
      libmp4tag->tags [tagidx].coveridx = coveridx;
// fprintf (stdout, "cidx: exist: %d\n", coveridx);
    }
fprintf (stdout, "tag: %s cidx: %d\n", tag, libmp4tag->tags [tagidx].coveridx);
  }

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
    libmp4tag->errornum = MP4TAG_ERR_NO_TAGS;
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
  libmp4tag->tags [libmp4tag->tagcount].tag = NULL;
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
    libmp4tag->errornum = MP4TAG_ERR_NO_TAGS;
    return;
  }
  if (idx < 0 || idx >= libmp4tag->tagcount) {
    return;
  }

  if (libmp4tag->tags [idx].tag != NULL) {
    free (libmp4tag->tags [idx].tag);
  }
  if (libmp4tag->tags [idx].data != NULL) {
    free (libmp4tag->tags [idx].data);
  }
  if (libmp4tag->tags [idx].covername != NULL) {
    free (libmp4tag->tags [idx].covername);
  }
}

#ifdef _WIN32

void *
mp4tag_towide (const char *buff)
{
    wchar_t     *tbuff = NULL;
    size_t      len;

    /* the documentation lies; len does not include room for the null byte */
    len = MultiByteToWideChar (CP_UTF8, 0, buff, strlen (buff), NULL, 0);
    tbuff = malloc ((len + 1) * sizeof (wchar_t));
    if (tbuff != NULL) {
      MultiByteToWideChar (CP_UTF8, 0, buff, strlen (buff), tbuff, len);
      tbuff [len] = L'\0';
    }
    return tbuff;
}

#endif
