/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 *
 * Resources:
 *    https://picard-docs.musicbrainz.org/en/appendices/tag_mapping.html
 *    https://xhelmboyx.tripod.com/formats/mp4-layout.txt
 *    https://docs.mp3tag.de/mapping/
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <wchar.h>

#if _hdr_windows
# include <windows.h>
#endif

#include "libmp4tag.h"
#include "libmp4tagint.h"

static void mp4tag_free_tags (libmp4tag_t *libmp4tag);
int         mp4tag_check_covr (const char *tag, const char *fn);
#ifdef _WIN32
static void * mp4tag_towide (const char *buff);
#endif

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
  libmp4tag->taglist_begin = 0;
  libmp4tag->taglist_len = 0;
  libmp4tag->free_begin = 0;
  libmp4tag->free_len = 0;
  libmp4tag->tagcount = 0;
  libmp4tag->tagalloccount = 0;
  libmp4tag->iterator = 0;
  libmp4tag->maintype [0] = '\0';
  libmp4tag->mp4version [0] = '\0';
  libmp4tag->mp7meta = false;

  libmp4tag->fh = mp4tag_fopen (fn, "rb");
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
mp4tag_close (libmp4tag_t *libmp4tag)
{
  if (libmp4tag == NULL) {
    return;
  }
  if (libmp4tag->fh == NULL) {
    libmp4tag->errornum = MP4TAG_OK;
    return;
  }
  fclose (libmp4tag->fh);
  libmp4tag->fh = NULL;
  libmp4tag->errornum = MP4TAG_OK;
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
    mp4tagpub->name = mp4tag->name;
    mp4tagpub->data = mp4tag->data;
    mp4tagpub->datalen = mp4tag->datalen;
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

  mp4tagpub->name = libmp4tag->tags [libmp4tag->iterator].name;
  mp4tagpub->data = libmp4tag->tags [libmp4tag->iterator].data;
  mp4tagpub->datalen = libmp4tag->tags [libmp4tag->iterator].datalen;
  mp4tagpub->binary = libmp4tag->tags [libmp4tag->iterator].binary;
  ++libmp4tag->iterator;

  libmp4tag->errornum = MP4TAG_OK;
  return libmp4tag->errornum;
}

int
mp4tag_set_tag_str (libmp4tag_t *libmp4tag, const char *tag, const char *data)
{
  int       idx = -1;

  if (libmp4tag == NULL) {
    return MP4TAG_ERR_BAD_STRUCT;
  }
  if (tag == NULL) {
    libmp4tag->errornum = MP4TAG_ERR_NULL_VALUE;
    return libmp4tag->errornum;
  }
  if (data == NULL) {
    libmp4tag->errornum = MP4TAG_ERR_NULL_VALUE;
    return libmp4tag->errornum;
  }

  idx = mp4tag_find_tag (libmp4tag, tag);
  if (idx >= 0 && idx < libmp4tag->tagcount) {
    mp4tag_t  *mp4tag;

    mp4tag = &libmp4tag->tags [idx];
    if (mp4tag->binary) {
      libmp4tag->errornum = MP4TAG_ERR_MISMATCH;
      return libmp4tag->errornum;
    }
    if (mp4tag->data != NULL) {
      free (mp4tag->data);
    }
    mp4tag->data = strdup (data);
    libmp4tag->errornum = MP4TAG_OK;
    if (mp4tag->data == NULL) {
      libmp4tag->errornum = MP4TAG_ERR_OUT_OF_MEMORY;
    }
    mp4tag->datalen = strlen (data);
  } else {
    const mp4tagdef_t *tagdef = NULL;
    int               ok = false;

    /* custom tags are always valid */
    if (memcmp (tag, MP4TAG_CUSTOM, MP4TAG_ID_LEN) == 0) {
      ok = true;
    }
    if ((tagdef = mp4tag_check_tag (tag)) != NULL) {
      ok = true;
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
	  libmp4tag->errornum = MP4TAG_ERR_MISMATCH;
          return libmp4tag->errornum;
	}
      }
      if (tflag == MP4TAG_ID_STRING) {
        tlen = strlen (data);
      }

      mp4tag_add_tag (libmp4tag, tag, data, strlen (data), tflag, tlen);
      mp4tag_sort_tags (libmp4tag);
      libmp4tag->errornum = MP4TAG_OK;
    } else {
      libmp4tag->errornum = MP4TAG_ERR_NOT_FOUND;
    }
  }

  return libmp4tag->errornum;
}

int
mp4tag_set_tag_binary (libmp4tag_t *libmp4tag,
    const char *tag, const char *data, size_t sz, const char *fn)
{
  int       idx = -1;
  int       identtype = MP4TAG_ID_DATA;

  if (libmp4tag == NULL) {
    return MP4TAG_ERR_BAD_STRUCT;
  }
  if (tag == NULL) {
    libmp4tag->errornum = MP4TAG_ERR_NULL_VALUE;
    return libmp4tag->errornum;
  }
  if (data == NULL) {
    libmp4tag->errornum = MP4TAG_ERR_NULL_VALUE;
    return libmp4tag->errornum;
  }

  idx = mp4tag_find_tag (libmp4tag, tag);
  if (idx >= 0 && idx < libmp4tag->tagcount) {
    mp4tag_t  *mp4tag;

    mp4tag = &libmp4tag->tags [idx];
    if (! mp4tag->binary) {
      libmp4tag->errornum = MP4TAG_ERR_MISMATCH;
      return libmp4tag->errornum;
    }
    if (mp4tag->data != NULL) {
      free (mp4tag->data);
    }
    mp4tag->data = malloc (sz);
    if (mp4tag->data == NULL) {
      libmp4tag->errornum = MP4TAG_ERR_OUT_OF_MEMORY;
      return libmp4tag->errornum;
    }
    memcpy (mp4tag->data, data, sz);
    mp4tag->datalen = sz;
    mp4tag->internallen = sz;
    identtype = mp4tag_check_covr (tag, fn);
    mp4tag->identtype = identtype;
    libmp4tag->errornum = MP4TAG_OK;
  } else {
    mp4tagdef_t *tagdef = NULL;

    tagdef = mp4tag_check_tag (tag);
    if (tagdef == NULL) {
      libmp4tag->errornum = MP4TAG_ERR_NOT_FOUND;
      return libmp4tag->errornum;
    }

    if (tagdef->identtype != MP4TAG_ID_DATA &&
        tagdef->identtype != MP4TAG_ID_JPG &&
        tagdef->identtype != MP4TAG_ID_PNG) {
      libmp4tag->errornum = MP4TAG_ERR_MISMATCH;
      return libmp4tag->errornum;
    }
    if (strcmp (tag, MP4TAG_TRKN) == 0 ||
        strcmp (tag, MP4TAG_DISK) == 0) {
      libmp4tag->errornum = MP4TAG_ERR_MISMATCH;
      return libmp4tag->errornum;
    }

    identtype = mp4tag_check_covr (tag, fn);
    mp4tag_add_tag (libmp4tag, tag, data, sz, identtype, sz);
    mp4tag_sort_tags (libmp4tag);
    libmp4tag->errornum = MP4TAG_OK;
  }

  return libmp4tag->errornum;
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
  mp4tag_build_data (libmp4tag);
  /* a stub for future development */
  return MP4TAG_ERR_NOT_IMPLEMENTED;
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

  return mp4tag_write_tags (libmp4tag);
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
    preserve->tags [i].name = strdup (libmp4tag->tags [i].name);
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
      if (preserve->tags [i].name != NULL) {
        free (preserve->tags [i].name);
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

    tfname = osToWideChar (fname);
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

/* returns MP4TAG_ID_DATA if not a 'covr' tag */
int
mp4tag_check_covr (const char *tag, const char *fn)
{
  int     identtype = MP4TAG_ID_DATA;
  size_t  len;

  if (strcmp (tag, MP4TAG_COVR) != 0) {
    return identtype;
  }

  if (fn == NULL) {
    return identtype;
  }

  len = strlen (fn);
  if (strcmp (fn + len - 4, ".png") == 0) {
    identtype = MP4TAG_ID_PNG;
  } else if (strcmp (fn + len - 4, ".jpg") == 0) {
    identtype = MP4TAG_ID_JPG;
  } else if (strcmp (fn + len - 5, ".jpeg") == 0) {
    identtype = MP4TAG_ID_JPG;
  }

  return identtype;
}


#ifdef _WIN32

static void *
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
