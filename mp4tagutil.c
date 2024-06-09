/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#if _hdr_windows
# define WIN32_LEAN_AND_MEAN 1
# include <windows.h>
#endif

#include "libmp4tag.h"
#include "mp4tagint.h"

static int  mp4tag_check_covr (const char *tag, const char *fn);

void
mp4tag_sort_tags (libmp4tag_t *libmp4tag)
{
  if (libmp4tag == NULL || libmp4tag->libmp4tagident != MP4TAG_IDENT) {
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
    if (tmp == NULL) {
      return offset;
    }
    p = strtok_r (tmp, MP4TAG_COVER_DELIM, &tokstr);
    if (p != NULL) {
      p = strtok_r (NULL, MP4TAG_COVER_DELIM, &tokstr);
      if (p != NULL) {
        coveridx = atoi (p);

        p = strtok_r (NULL, MP4TAG_COVER_DELIM, &tokstr);
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

  if (libmp4tag == NULL || libmp4tag->libmp4tagident != MP4TAG_IDENT) {
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
  if (strncmp (tag, MP4TAG_COVR, MP4TAG_ID_LEN) == 0) {
    mp4tag_parse_cover_tag (tag, &coveridx);
    if (coveridx < 0) {
      coveridx = 0;
    }
    key.tag = (char *) MP4TAG_COVR;
  }
  key.coveridx = coveridx;
  result = bsearch (&key, libmp4tag->tags, libmp4tag->tagcount,
      sizeof (mp4tag_t), mp4tag_compare);

  if (result != NULL) {
    idx = result->idx;
  }

  return idx;
}

mp4tagdef_t *
mp4tag_check_tag (const char *tag)
{
  mp4tagdef_t key;
  mp4tagdef_t *result;
  char        tmp [MP4TAG_ID_DISP_LEN];

  if (tag == NULL) {
    return NULL;
  }

  /* note that custom tags have already been handled */
  /* but the tag could still be "covr:1:name" */
  /* be careful with copying here */

  snprintf (tmp, sizeof (tmp), "%s", tag);
  if (strncmp (tag, MP4TAG_COVR, MP4TAG_ID_LEN) == 0) {
    memcpy (tmp, tag, MP4TAG_ID_LEN);
    tmp [MP4TAG_ID_LEN] = '\0';
  }

  key.tag = tmp;
  result = bsearch (&key, mp4taglist, mp4taglistlen,
      sizeof (mp4tagdef_t), mp4tag_compare_list);

  return result;
}

/* for comparison within the list of parsed tags */
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
    /* sort by both the name and the cover index */
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

/* for comparison within the list of valid tags */
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
      libmp4tag->mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
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
  if (libmp4tag->tags [tagidx].tag == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
    return;
  }

  if (strncmp (tag, MP4TAG_COVR, MP4TAG_ID_LEN) == 0) {
// fprintf (stdout, "tag: %s\n", tag);

    mp4tag_parse_cover_tag (tag, &coveridx);
    /* make sure the base tag is set properly */
    if (libmp4tag->tags [tagidx].tag != NULL) {
      free (libmp4tag->tags [tagidx].tag);
    }
    libmp4tag->tags [tagidx].tag = strdup (MP4TAG_COVR);
    if (libmp4tag->tags [tagidx].tag == NULL) {
      libmp4tag->mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
      return;
    }

    if (covername != NULL) {
      libmp4tag->tags [tagidx].covername = strdup (covername);
      if (libmp4tag->tags [tagidx].covername == NULL) {
        libmp4tag->mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
        return;
      }
    }

    if (coveridx == -1) {
      libmp4tag->tags [tagidx].coveridx = libmp4tag->covercount;
      libmp4tag->covercount += 1;
    } else {
      libmp4tag->tags [tagidx].coveridx = coveridx;
    }
  }

  if (sz == MP4TAG_STRING) {
    /* string with null terminator */
    libmp4tag->tags [tagidx].data = strdup (data);
    if (libmp4tag->tags [tagidx].data == NULL) {
      libmp4tag->mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
      return;
    }
    libmp4tag->tags [tagidx].datalen = strlen (data);
  } else if (sz < 0) {
    /* string w/o null terminator, cannot use strdup */
    sz = - sz;
    libmp4tag->tags [tagidx].data = malloc (sz + 1);
    if (libmp4tag->tags [tagidx].data == NULL) {
      libmp4tag->mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
      return;
    }
    memcpy (libmp4tag->tags [tagidx].data, data, sz);
    libmp4tag->tags [tagidx].data [sz] = '\0';
    libmp4tag->tags [tagidx].datalen = sz;
  } else {
    /* binary data */
    if (sz > 0) {
      libmp4tag->tags [tagidx].data = malloc (sz);
      if (libmp4tag->tags [tagidx].data == NULL) {
        libmp4tag->mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
        return;
      }
      memcpy (libmp4tag->tags [tagidx].data, data, sz);
    }
    libmp4tag->tags [tagidx].binary = true;
    libmp4tag->tags [tagidx].datalen = sz;
  }
  libmp4tag->tagcount += 1;
}

int
mp4tag_set_tag_string (libmp4tag_t *libmp4tag, const char *tag,
    int idx, const char *data)
{
  if (libmp4tag == NULL || libmp4tag->libmp4tagident != MP4TAG_IDENT) {
    return MP4TAG_ERR_BAD_STRUCT;
  }
  if (tag == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_NULL_VALUE;
    return libmp4tag->mp4error;
  }
  if (data == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_NULL_VALUE;
    return libmp4tag->mp4error;
  }

  if (idx >= 0 && idx < libmp4tag->tagcount) {
    mp4tag_t  *mp4tag;

    /* an existing tag is being updated. */
    /* use the information read from the audio file. */

    mp4tag = &libmp4tag->tags [idx];

    if (strncmp (tag, MP4TAG_COVR, MP4TAG_ID_LEN) == 0) {
      int     offset;
      int     coveridx;

      /* handle cover tags separately */
      /* only cover filenames are allowed for set-tag-str */

      offset = mp4tag_parse_cover_tag (tag, &coveridx);
      if (offset > 0) {
        if (mp4tag->covername != NULL) {
          free (mp4tag->covername);
        }
        mp4tag->covername = strdup (data);
        if (mp4tag->covername == NULL) {
          libmp4tag->mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
        }
      } else {
        libmp4tag->mp4error = MP4TAG_ERR_MISMATCH;
        return libmp4tag->mp4error;
      }
    } else {

      /* existing: not a cover image tag */

      if (mp4tag->binary) {
        libmp4tag->mp4error = MP4TAG_ERR_MISMATCH;
        return libmp4tag->mp4error;
      }

      if (mp4tag->data != NULL) {
        free (mp4tag->data);
      }
      mp4tag->data = strdup (data);
      if (mp4tag->data == NULL) {
        libmp4tag->mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
        return libmp4tag->mp4error;
      } else {
        mp4tag->datalen = strlen (data);
      }
    }
  } else {
    const mp4tagdef_t *tagdef = NULL;
    int               ok = false;

    /* a new tag is being added. */
    /* in this case, check to make sure it is in the valid list. */

    /* custom tags are always valid */
    if (memcmp (tag, MP4TAG_CUSTOM, MP4TAG_ID_LEN) == 0) {
      ok = true;
    }
    if ((tagdef = mp4tag_check_tag (tag)) != NULL) {
      ok = true;
      if (memcmp (tag, MP4TAG_COVR, MP4TAG_ID_LEN) == 0) {
        int     coveridx;

        /* trying to set the cover name, but no associated cover tag was found */
        if (mp4tag_parse_cover_tag (tag, &coveridx) > 0) {
          ok = false;
        }
      }
    }

    if (ok) {
      uint32_t    tflag;
      uint32_t    tlen;

      tflag = MP4TAG_ID_STRING;
      if (tagdef != NULL) {
        int     rc;

        tflag = tagdef->identtype;
        tlen = tagdef->len;

        /* valid: strings, numerics have string representation, trkn, disk */
        rc = tflag == MP4TAG_ID_STRING ||
            tflag == MP4TAG_ID_NUM ||
            (tflag == MP4TAG_ID_DATA &&
            (strcmp (tag, MP4TAG_TRKN) == 0 || strcmp (tag, MP4TAG_DISK) == 0));
        if (! rc) {
          libmp4tag->mp4error = MP4TAG_ERR_MISMATCH;
          return libmp4tag->mp4error;
        }
      }
      if (tflag == MP4TAG_ID_STRING) {
        tlen = strlen (data);
      }

      mp4tag_add_tag (libmp4tag, tag, data, MP4TAG_STRING, tflag, tlen, NULL);
      mp4tag_sort_tags (libmp4tag);
    } else {
      libmp4tag->mp4error = MP4TAG_ERR_TAG_NOT_FOUND;
    }
  }

  return libmp4tag->mp4error;
}

int
mp4tag_set_tag_binary (libmp4tag_t *libmp4tag,
    const char *tag, int idx, const char *data, size_t sz, const char *fn)
{
  int       identtype = MP4TAG_ID_DATA;

  if (libmp4tag == NULL || libmp4tag->libmp4tagident != MP4TAG_IDENT) {
    return MP4TAG_ERR_BAD_STRUCT;
  }
  if (tag == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_NULL_VALUE;
    return libmp4tag->mp4error;
  }
  if (data == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_NULL_VALUE;
    return libmp4tag->mp4error;
  }

  if (idx >= 0 && idx < libmp4tag->tagcount) {
    mp4tag_t  *mp4tag;

    /* an existing binary data tag */

    mp4tag = &libmp4tag->tags [idx];
    if (! mp4tag->binary) {
      libmp4tag->mp4error = MP4TAG_ERR_MISMATCH;
      return libmp4tag->mp4error;
    }
    if (mp4tag->data != NULL) {
      free (mp4tag->data);
    }
    mp4tag->data = malloc (sz);
    if (mp4tag->data == NULL) {
      libmp4tag->mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
      return libmp4tag->mp4error;
    }

    memcpy (mp4tag->data, data, sz);
    mp4tag->datalen = sz;
    mp4tag->internallen = sz;
    identtype = mp4tag_check_covr (tag, fn);
    mp4tag->identtype = identtype;
  } else {
    mp4tagdef_t *tagdef = NULL;
    bool        ok = false;

    /* a new binary data tag */
    /* check to make sure this tag exists in the valid list */

    /* custom tags are always valid */
    if (memcmp (tag, MP4TAG_CUSTOM, MP4TAG_ID_LEN) == 0) {
      ok = true;
    } else {
      tagdef = mp4tag_check_tag (tag);
      if (tagdef == NULL) {
        libmp4tag->mp4error = MP4TAG_ERR_TAG_NOT_FOUND;
        return libmp4tag->mp4error;
      }
      ok = true;

      if (tagdef->identtype != MP4TAG_ID_DATA &&
          tagdef->identtype != MP4TAG_ID_JPG &&
          tagdef->identtype != MP4TAG_ID_PNG) {
        libmp4tag->mp4error = MP4TAG_ERR_MISMATCH;
        ok = false;
      }
    }

    if (strcmp (tag, MP4TAG_TRKN) == 0 ||
        strcmp (tag, MP4TAG_DISK) == 0) {
      libmp4tag->mp4error = MP4TAG_ERR_MISMATCH;
      ok = false;
    }

    if (ok) {
      identtype = mp4tag_check_covr (tag, fn);
      mp4tag_add_tag (libmp4tag, tag, data, sz, identtype, sz, NULL);
      mp4tag_sort_tags (libmp4tag);
    }
  }

  return libmp4tag->mp4error;
}

void
mp4tag_del_tag (libmp4tag_t *libmp4tag, int idx)
{
  if (libmp4tag == NULL || libmp4tag->libmp4tagident != MP4TAG_IDENT) {
    return;
  }
  if (libmp4tag->tags == NULL) {
    return;
  }
  if (idx < 0 || idx >= libmp4tag->tagcount) {
    libmp4tag->mp4error = MP4TAG_ERR_NO_TAGS;
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
  if (libmp4tag == NULL || libmp4tag->libmp4tagident != MP4TAG_IDENT) {
    return;
  }
  if (libmp4tag->tags == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_NO_TAGS;
    return;
  }
  if (idx < 0 || idx >= libmp4tag->tagcount) {
    return;
  }

  mp4tag_free_tag (&libmp4tag->tags [idx]);
}

void
mp4tag_free_tag (mp4tag_t *mp4tag)
{
  if (mp4tag->tag != NULL) {
    free (mp4tag->tag);
    mp4tag->tag = NULL;
  }
  if (mp4tag->data != NULL) {
    free (mp4tag->data);
    mp4tag->data = NULL;
    mp4tag->datalen = 0;
  }
  if (mp4tag->covername != NULL) {
    free (mp4tag->covername);
    mp4tag->covername = NULL;
  }
}

void
mp4tag_clone_tag (libmp4tag_t *libmp4tag, mp4tag_t *target, mp4tag_t *source)
{
  target->tag = NULL;
  if (source->tag != NULL) {
    target->tag = strdup (source->tag);
    if (target->tag == NULL) {
      libmp4tag->mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
    }
  }

  target->datalen = source->datalen;

  target->data = NULL;
  if (source->datalen > 0 && source->data != NULL) {
    size_t      len;

    len = source->datalen;
    if (! source->binary) {
      /* string null terminator */
      ++len;
    }
    target->data = malloc (len);
    if (target->data == NULL) {
      libmp4tag->mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
    } else {
      memcpy (target->data, source->data, len);
    }
  }

  target->covername = NULL;
  if (source->covername != NULL) {
    target->covername = strdup (source->covername);
    if (target->covername == NULL) {
      libmp4tag->mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
    }
  }

  target->coveridx = source->coveridx;
  target->idx = source->idx;
  target->identtype = source->identtype;
  target->internallen = source->internallen;
  target->priority = source->priority;
  target->binary = source->binary;
}

void
mp4tag_sleep (uint32_t ms)
{
/* windows seems to have a very large amount of overhead when calling */
/* nanosleep() or Sleep(). */
/* macos seems to have a minor amount of overhead when calling nanosleep() */

/* on windows, nanosleep is within the libwinpthread msys2 library which */
/* is not wanted for bdj4se &etc. So use the Windows API Sleep() function */
#if _lib_Sleep
  Sleep ((DWORD) ms);
#endif
#if ! _lib_Sleep && _lib_nanosleep
  struct timespec   ts;
  struct timespec   rem;

  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms - (ts.tv_sec * 1000)) * 1000 * 1000;
  while (ts.tv_sec > 0 || ts.tv_nsec > 0) {
    /* rc = */ nanosleep (&ts, &rem);
    ts.tv_sec = 0;
    ts.tv_nsec = 0;
    /* remainder is only valid when EINTR is returned */
    /* most of the time, an interrupt is caused by a control-c while testing */
    /* just let an interrupt stop the sleep */
  }
#endif
}

/* internal routines */

/* returns MP4TAG_ID_DATA if not a 'covr' tag */
static int
mp4tag_check_covr (const char *tag, const char *fn)
{
  int     identtype = MP4TAG_ID_DATA;
  char    *p;
  char    ext [10];

  if (strncmp (tag, MP4TAG_COVR, MP4TAG_ID_LEN) != 0) {
    return identtype;
  }

  if (fn == NULL) {
    return identtype;
  }

  p = strrchr (fn, '.');
  if (p == NULL) {
    /* file type is unknown, assume jpg */
    identtype = MP4TAG_ID_JPG;
    return identtype;
  }

  /* it really isn't important what the full extension is, */
  /* the check is only for a few known extensions */
  /* and the extensions are ascii */
  strncpy (ext, p, sizeof (ext));
  ext [sizeof (ext) - 1] = '\0';
  for (size_t i = 0; i < strlen (ext); ++i) {
    ext [i] = tolower (ext [i]);
  }

  if (strcmp (ext, ".png") == 0) {
    identtype = MP4TAG_ID_PNG;
  } else if (strcmp (ext, ".jpg") == 0) {
    identtype = MP4TAG_ID_JPG;
  } else if (strcmp (ext, ".jpeg") == 0) {
    identtype = MP4TAG_ID_JPG;
  }

  return identtype;
}
