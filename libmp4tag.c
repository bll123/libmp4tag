/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 *
 * Resources:
 *    https://xhelmboyx.tripod.com/formats/mp4-layout.txt
 *    https://github.com/quodlibet/mutagen/blob/master/mutagen/mp4/__init__.py
 *    https://picard-docs.musicbrainz.org/en/appendices/tag_mapping.html
 *    https://docs.mp3tag.de/mapping/
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#if _hdr_windows
# include <windows.h>
#endif

#include "libmp4tag.h"
#include "mp4tagint.h"

const char *mp4tagerrmsgs [] = {
  [MP4TAG_OK] = "ok",
  [MP4TAG_FINISH] = "finish",
  [MP4TAG_ERR_BAD_STRUCT] = "bad structure",
  [MP4TAG_ERR_OUT_OF_MEMORY] = "out of memory",
  [MP4TAG_ERR_UNABLE_TO_OPEN] = "unable to open",
  [MP4TAG_ERR_NOT_MP4] = "not mp4",
  [MP4TAG_ERR_NOT_OPEN] = "not open",
  [MP4TAG_ERR_NULL_VALUE] = "null value",
  [MP4TAG_ERR_NO_TAGS] = "no tags",
  [MP4TAG_ERR_MISMATCH] = "mismatch",
  [MP4TAG_ERR_NOT_FOUND] = "not found",
  [MP4TAG_ERR_NOT_IMPLEMENTED] = "not implemented",
};

static void mp4tag_free_tags (libmp4tag_t *libmp4tag);

typedef struct libmp4tagpreserve {
  mp4tag_t  *tags;
  int       tagcount;
} libmp4tagpreserve_t;

libmp4tag_t *
mp4tag_open (const char *fn, int *errornum)
{
  libmp4tag_t *libmp4tag = NULL;
  int         rc;

  if (fn == NULL) {
    *errornum = MP4TAG_ERR_NULL_VALUE;
    return NULL;
  }

  libmp4tag = malloc (sizeof (libmp4tag_t));
  if (libmp4tag == NULL) {
    *errornum = MP4TAG_ERR_OUT_OF_MEMORY;
    return NULL;
  }

  libmp4tag->tags = NULL;
  libmp4tag->creationdate = 0;
  libmp4tag->modifieddate = 0;
  libmp4tag->duration = 0;
  libmp4tag->samplerate = 0;
  for (int i = 0; i < MP4TAG_BASE_OFF_MAX; ++i) {
    libmp4tag->base_offsets [i] = 0;
  }
  libmp4tag->base_offset_count = 0;
  libmp4tag->taglist_offset = 0;
  libmp4tag->coverstart_offset = -1;
  libmp4tag->taglist_len = 0;
  libmp4tag->tagcount = 0;
  libmp4tag->tagalloccount = 0;
  libmp4tag->iterator = 0;
  libmp4tag->mp7meta = false;
  libmp4tag->unlimited = false;
  libmp4tag->covercount = 0;
  libmp4tag->errornum = MP4TAG_OK;

  libmp4tag->fh = mp4tag_fopen (fn, "rb+");
  if (libmp4tag->fh == NULL) {
    *errornum = MP4TAG_ERR_UNABLE_TO_OPEN;
    free (libmp4tag);
    return NULL;
  }

  rc = mp4tag_parse_ftyp (libmp4tag);
  if (rc < 0) {
    *errornum = MP4TAG_ERR_NOT_MP4;
    fclose (libmp4tag->fh);
    free (libmp4tag);
    return NULL;
  }

  libmp4tag->fn = strdup (fn);
  if (libmp4tag->fn == NULL) {
    *errornum = MP4TAG_ERR_OUT_OF_MEMORY;
    fclose (libmp4tag->fh);
    free (libmp4tag);
    return NULL;
  }

  return libmp4tag;
}

void
mp4tag_free (libmp4tag_t *libmp4tag)
{
  if (libmp4tag == NULL) {
    return;
  }

  if (libmp4tag->fn != NULL) {
    free (libmp4tag->fn);
    libmp4tag->fn = NULL;
  }
  if (libmp4tag->fh != NULL) {
    fclose (libmp4tag->fh);
    libmp4tag->fh = NULL;
  }
  mp4tag_free_tags (libmp4tag);
  free (libmp4tag);
}

void
mp4tag_parse (libmp4tag_t *libmp4tag)
{
  if (libmp4tag == NULL) {
    return;
  }
  /* parsing requires an open file */
  if (libmp4tag->fh == NULL) {
    libmp4tag->errornum = MP4TAG_ERR_NOT_OPEN;
    return;
  }

  mp4tag_parse_file (libmp4tag);
  libmp4tag->errornum = MP4TAG_OK;
}

int64_t
mp4tag_duration (libmp4tag_t *libmp4tag)
{
  if (libmp4tag == NULL) {
    return 0;
  }

  libmp4tag->errornum = MP4TAG_OK;
  return libmp4tag->duration;
}

int
mp4tag_get_tag_by_name (libmp4tag_t *libmp4tag, const char *tag,
    mp4tagpub_t *mp4tagpub)
{
  int     idx = -1;

  if (libmp4tag == NULL) {
    return MP4TAG_ERR_BAD_STRUCT;
  }
  if (mp4tagpub == NULL) {
    libmp4tag->errornum = MP4TAG_ERR_NULL_VALUE;
    return libmp4tag->errornum;
  }
  if (libmp4tag->tags == NULL) {
    libmp4tag->errornum = MP4TAG_ERR_NO_TAGS;
    return libmp4tag->errornum;
  }

  idx = mp4tag_find_tag (libmp4tag, tag);
  if (idx >= 0 && idx < libmp4tag->tagcount) {
    mp4tag_t    *mp4tag;

    mp4tag = &libmp4tag->tags [idx];
    mp4tagpub->tag = mp4tag->tag;
    mp4tagpub->data = mp4tag->data;
    mp4tagpub->datalen = mp4tag->datalen;
    mp4tagpub->covername = mp4tag->covername;
    mp4tagpub->coveridx = mp4tag->coveridx;
    mp4tagpub->binary = mp4tag->binary;
    libmp4tag->errornum = MP4TAG_OK;
  } else {
    libmp4tag->errornum = MP4TAG_ERR_NOT_FOUND;
  }

  return libmp4tag->errornum;
}

void
mp4tag_iterate_init (libmp4tag_t *libmp4tag)
{
  if (libmp4tag == NULL) {
    return;
  }

  libmp4tag->iterator = 0;
}

int
mp4tag_iterate (libmp4tag_t *libmp4tag, mp4tagpub_t *mp4tagpub)
{
  if (libmp4tag == NULL) {
    return MP4TAG_ERR_BAD_STRUCT;
  }
  if (mp4tagpub == NULL) {
    libmp4tag->errornum = MP4TAG_ERR_NULL_VALUE;
    return libmp4tag->errornum;
  }

  if (libmp4tag->iterator >= libmp4tag->tagcount) {
    libmp4tag->errornum = MP4TAG_OK;
    return MP4TAG_FINISH;
  }
  if (libmp4tag->tags == NULL) {
    libmp4tag->errornum = MP4TAG_ERR_NO_TAGS;
    return libmp4tag->errornum;
  }

  mp4tagpub->tag = libmp4tag->tags [libmp4tag->iterator].tag;
  mp4tagpub->data = libmp4tag->tags [libmp4tag->iterator].data;
  mp4tagpub->datalen = libmp4tag->tags [libmp4tag->iterator].datalen;
  mp4tagpub->covername = libmp4tag->tags [libmp4tag->iterator].covername;
  mp4tagpub->coveridx = libmp4tag->tags [libmp4tag->iterator].coveridx;
  mp4tagpub->binary = libmp4tag->tags [libmp4tag->iterator].binary;
  ++libmp4tag->iterator;

  libmp4tag->errornum = MP4TAG_OK;
  return libmp4tag->errornum;
}

int
mp4tag_set_tag (libmp4tag_t *libmp4tag, const char *tag,
    const char *data, bool forcebinary)
{
  int       idx = -1;
  bool      binary = false;
  int       rc = MP4TAG_ERR_NOT_FOUND;

  idx = mp4tag_find_tag (libmp4tag, tag);
  if (idx >= 0 && idx < libmp4tag->tagcount) {
    mp4tag_t  *mp4tag;

    mp4tag = &libmp4tag->tags [idx];
    binary = mp4tag->binary;
  }

  if (memcmp (tag, MP4TAG_COVR, MP4TAG_ID_LEN) == 0) {
    int     offset;
    int     coveridx;

    binary = true;
    offset = mp4tag_parse_cover_tag (tag, &coveridx);
    if (offset > 0) {
      binary = false;
    }
  }

  if (forcebinary) {
    binary = true;
  }

  if (binary) {
    char      *fdata;
    size_t    sz;

    fdata = mp4tag_read_file (libmp4tag, data, &sz);
    if (fdata != NULL) {
      rc = mp4tag_set_tag_binary (libmp4tag, tag, idx, fdata, sz, data);
      free (fdata);
    }
  } else {
    rc = mp4tag_set_tag_str (libmp4tag, tag, idx, data);
  }

  return rc;
}

int
mp4tag_delete_tag (libmp4tag_t *libmp4tag, const char *tag)
{
  int       idx = -1;

  if (libmp4tag == NULL) {
    return MP4TAG_ERR_BAD_STRUCT;
  }
  if (tag == NULL) {
    libmp4tag->errornum = MP4TAG_ERR_NULL_VALUE;
    return libmp4tag->errornum;
  }
  if (libmp4tag->tags == NULL) {
    libmp4tag->errornum = MP4TAG_ERR_NO_TAGS;
    return MP4TAG_OK;
  }

  idx = mp4tag_find_tag (libmp4tag, tag);
  if (idx >= 0 && idx < libmp4tag->tagcount) {
    if (strncmp (tag, MP4TAG_COVR, MP4TAG_ID_LEN) == 0) {
      int   coveridx;

      if (mp4tag_parse_cover_tag (tag, &coveridx) > 0) {
        mp4tag_t    *mp4tag;

        mp4tag = &libmp4tag->tags [idx];
	if (mp4tag->covername != NULL) {
	  free (mp4tag->covername);
	  mp4tag->covername = NULL;
	}
        libmp4tag->errornum = MP4TAG_OK;
	return libmp4tag->errornum;
      }
    }

    mp4tag_del_tag (libmp4tag, idx);
    libmp4tag->errornum = MP4TAG_OK;
  } else {
    libmp4tag->errornum = MP4TAG_ERR_NOT_FOUND;
  }

  return libmp4tag->errornum;
}

int
mp4tag_write_tags (libmp4tag_t *libmp4tag)
{
  char      *data = NULL;
  uint32_t  dlen = 0;
  int       rc;

  data = mp4tag_build_data (libmp4tag, &dlen);
  rc = mp4tag_write_data (libmp4tag, data, dlen);
  return rc;
}

int
mp4tag_clean_tags (libmp4tag_t *libmp4tag)
{
  if (libmp4tag == NULL) {
    return MP4TAG_ERR_BAD_STRUCT;
  }
  if (libmp4tag->tags == NULL) {
    libmp4tag->errornum = MP4TAG_ERR_NO_TAGS;
    return MP4TAG_OK;
  }

  mp4tag_free_tags (libmp4tag);
  return MP4TAG_OK;
}

libmp4tagpreserve_t *
mp4tag_preserve_tags (libmp4tag_t *libmp4tag)
{
  libmp4tagpreserve_t *preserve = NULL;

  if (libmp4tag == NULL) {
    return NULL;
  }
  if (libmp4tag->tags == NULL) {
    libmp4tag->errornum = MP4TAG_ERR_NO_TAGS;
    return NULL;
  }

  preserve = malloc (sizeof (libmp4tagpreserve_t));
  if (preserve == NULL) {
    libmp4tag->errornum = MP4TAG_ERR_OUT_OF_MEMORY;
    return NULL;
  }

  preserve->tagcount = libmp4tag->tagcount;
  preserve->tags = malloc (sizeof (mp4tag_t) * preserve->tagcount);
  if (preserve->tags == NULL) {
    libmp4tag->errornum = MP4TAG_ERR_OUT_OF_MEMORY;
    free (preserve);
    return NULL;
  }

  libmp4tag->errornum = MP4TAG_OK;

  for (int i = 0; i < preserve->tagcount; ++i) {
    preserve->tags [i].tag = strdup (libmp4tag->tags [i].tag);
    preserve->tags [i].datalen = libmp4tag->tags [i].datalen;
    preserve->tags [i].data = malloc (libmp4tag->tags [i].datalen);
    if (preserve->tags [i].data != NULL) {
      memcpy (preserve->tags [i].data, libmp4tag->tags [i].data,
          libmp4tag->tags [i].datalen);
    } else {
      libmp4tag->errornum = MP4TAG_ERR_OUT_OF_MEMORY;
    }
  }

  return preserve;
}

int
mp4tag_restore_tags (libmp4tag_t *libmp4tag, libmp4tagpreserve_t *preserve)
{
  if (libmp4tag == NULL) {
    return MP4TAG_ERR_BAD_STRUCT;
  }
  if (preserve == NULL) {
    libmp4tag->errornum = MP4TAG_ERR_NULL_VALUE;
    return libmp4tag->errornum;
  }

  return MP4TAG_ERR_NOT_IMPLEMENTED;
}

int
mp4tag_preserve_free (libmp4tagpreserve_t *preserve)
{
  if (preserve == NULL) {
    return MP4TAG_ERR_NULL_VALUE;
  }

  if (preserve->tags != NULL) {
    for (int i = 0; i < preserve->tagcount; ++i) {
      if (preserve->tags [i].tag != NULL) {
        free (preserve->tags [i].tag);
      }
      if (preserve->tags [i].data != NULL) {
        free (preserve->tags [i].data);
      }
    }
    free (preserve->tags);
    preserve->tags = NULL;
    preserve->tagcount = 0;
  }
  free (preserve);

  return MP4TAG_OK;
}

int
mp4tag_error (libmp4tag_t *libmp4tag)
{
  if (libmp4tag == NULL) {
    return MP4TAG_ERR_BAD_STRUCT;
  }

  return libmp4tag->errornum;
}

const char *
mp4tag_version (void)
{
  return LIBMP4TAG_VERSION;
}

const char *
mp4tag_error_str (libmp4tag_t *libmp4tag)
{
  if (libmp4tag == NULL) {
    return mp4tagerrmsgs [MP4TAG_ERR_BAD_STRUCT];
  }

  return mp4tagerrmsgs [libmp4tag->errornum];
}

FILE *
mp4tag_fopen (const char *fname, const char *mode)
{
  FILE          *fh;

#ifdef _WIN32
  {
    wchar_t       *tfname = NULL;
    wchar_t       *tmode = NULL;

    tfname = mp4tag_towide (fname);
    tmode = mp4tag_towide (mode);
    fh = _wfopen (tfname, tmode);
    free (tfname);
    free (tmode);
  }
#else
  {
    fh = fopen (fname, mode);
  }
#endif
  return fh;
}

ssize_t
mp4tag_file_size (const char *fname)
{
  ssize_t       sz = -1;

#if _lib__wstat
  {
    struct _stat  statbuf;
    wchar_t       *tfname = NULL;
    int           rc;

    tfname = mp4tag_towide (fname);
    rc = _wstat (tfname, &statbuf);
    if (rc == 0) {
      sz = statbuf.st_size;
    }
    mdfree (tfname);
  }
#else
  {
    int rc;
    struct stat statbuf;

    rc = stat (fname, &statbuf);
    if (rc == 0) {
      sz = statbuf.st_size;
    }
  }
#endif
  return sz;
}

/* internal routines */

static void
mp4tag_free_tags (libmp4tag_t *libmp4tag)
{
  if (libmp4tag == NULL) {
    return;
  }
  if (libmp4tag->tags == NULL) {
    return;
  }

  for (int i = 0; i < libmp4tag->tagcount; ++i) {
    mp4tag_free_tag_by_idx (libmp4tag, i);
  }
  free (libmp4tag->tags);
  libmp4tag->tags = NULL;
  libmp4tag->tagcount = 0;
  libmp4tag->tagalloccount = 0;
}

