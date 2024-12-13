/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 *
 * Resources:
 *    https://xhelmboyx.tripod.com/formats/mp4-layout.txt
 *      stco information appears to be incorrect. like co64, it also
 *      has a 32-bit version/flags.
 *    https://github.com/quodlibet/mutagen/blob/master/mutagen/mp4/__init__.py
 *    https://picard-docs.musicbrainz.org/en/appendices/tag_mapping.html
 *    https://docs.mp3tag.de/mapping/
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#if _sys_resource
# include <sys/resource.h>
#endif

#include "libmp4tag.h"
#include "mp4tagint.h"

const char *PREFIX_STR = "\xc2\xa9";   /* copyright symbol */

/* these error strings are only for debugging purposes, and */
/* do not need to be translated */
static const char *mp4tagerrmsgs [] = {
  [MP4TAG_OK] = "ok",
  [MP4TAG_FINISH] = "finish",
  [MP4TAG_ERR_BAD_STRUCT] = "bad structure",
  [MP4TAG_ERR_OUT_OF_MEMORY] = "out of memory",
  [MP4TAG_ERR_NOT_MP4] = "not mp4",
  [MP4TAG_ERR_NOT_OPEN] = "not open",
  [MP4TAG_ERR_NULL_VALUE] = "null value",
  [MP4TAG_ERR_NO_TAGS] = "no tags",
  [MP4TAG_ERR_MISMATCH] = "mismatch",
  [MP4TAG_ERR_TAG_NOT_FOUND] = "not found",
  [MP4TAG_ERR_NOT_IMPLEMENTED] = "not implemented",
  [MP4TAG_ERR_FILE_NOT_FOUND] = "file not found",
  [MP4TAG_ERR_FILE_READ_ERROR] = "file read error",
  [MP4TAG_ERR_FILE_WRITE_ERROR] = "file write error",
  [MP4TAG_ERR_FILE_SEEK_ERROR] = "file seek error",
  [MP4TAG_ERR_FILE_TELL_ERROR] = "file tell error",
  [MP4TAG_ERR_UNABLE_TO_PROCESS] = "unable to process",
  [MP4TAG_ERR_NOT_PARSED] = "not parsed",
  [MP4TAG_ERR_CANNOT_WRITE] = "cannot write",
};

typedef struct libmp4tagpreserve {
  mp4tag_t  *tags;
  int       tagcount;
} libmp4tagpreserve_t;

static libmp4tag_t *mp4tag_alloc (int *mp4error);
static void mp4tag_free_tags (libmp4tag_t *libmp4tag);
static void mp4tag_copy_to_pub (mp4tagpub_t *mp4tagpub, mp4tag_t *mp4tag);
static void mp4tag_init_tags (libmp4tag_t *libmp4tag);
#if LIBMP4TAG_DEBUG
static void enable_core_dump (void);
#endif

libmp4tag_t *
mp4tag_open (const char *fn, int *mp4error)
{
  libmp4tag_t *libmp4tag = NULL;
  int         rc;

#if LIBMP4TAG_DEBUG
  enable_core_dump ();
#endif
  if (fn == NULL) {
    *mp4error = MP4TAG_ERR_NULL_VALUE;
    return NULL;
  }

  *mp4error = MP4TAG_OK;
  libmp4tag = mp4tag_alloc (mp4error);
  if (*mp4error != MP4TAG_OK) {
    return NULL;
  }

  libmp4tag->fh = mp4tag_fopen (fn, "rb+");
  if (libmp4tag->fh == NULL) {
    /* if the file cannot be opened, try opening w/o write capabilities */
    libmp4tag->fh = mp4tag_fopen (fn, "rb");
    if (libmp4tag->fh == NULL) {
      *mp4error = MP4TAG_ERR_FILE_NOT_FOUND;
      libmp4tag->libmp4tagident = 0;
      free (libmp4tag);
      return NULL;
    }
    libmp4tag->canwrite = false;
  }

  /* needed for parse, write */
  libmp4tag->filesz = mp4tag_file_size (fn);
  libmp4tag->offset = 0;

  rc = mp4tag_parse_ftyp (libmp4tag);
  if (rc != MP4TAG_OK) {
    *mp4error = rc;
    fclose (libmp4tag->fh);
    libmp4tag->libmp4tagident = 0;
    free (libmp4tag);
    return NULL;
  }

  libmp4tag->fn = strdup (fn);
  if (libmp4tag->fn == NULL) {
    *mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
    fclose (libmp4tag->fh);
    libmp4tag->libmp4tagident = 0;
    free (libmp4tag);
    return NULL;
  }

  /* for debugging only */
#if LIBMP4TAG_DEBUG_STREAM
  libmp4tag->isstream = true;
  libmp4tag->canwrite = false;
  libmp4tag->offset = ftell (libmp4tag->fh);
  libmp4tag->timeout = 10;
#endif

  return libmp4tag;
}

libmp4tag_t *
mp4tag_openstream (FILE *fh, size_t offset, long timeout, int *mp4error)
{
  libmp4tag_t *libmp4tag = NULL;

  *mp4error = MP4TAG_OK;
  libmp4tag = mp4tag_alloc (mp4error);
  if (*mp4error != MP4TAG_OK) {
    return NULL;
  }

  libmp4tag->fh = fh;
  if (libmp4tag->fh == NULL) {
    *mp4error = MP4TAG_ERR_FILE_NOT_FOUND;
    libmp4tag->libmp4tagident = 0;
    free (libmp4tag);
    return NULL;
  }

  /* needed for parse, write */
  libmp4tag->filesz = MP4TAG_NO_FILESZ;

  /* the assumption is made that the stream being passed in is known */
  /* to be an mp4 */

  libmp4tag->isstream = true;
  libmp4tag->canwrite = false;
  libmp4tag->timeout = timeout;
  libmp4tag->offset = offset;

  return libmp4tag;
}

libmp4tag_t *
mp4tag_openstreamfd (int fd, size_t offset, long timeout, int *mp4error)
{
  libmp4tag_t *libmp4tag = NULL;

  *mp4error = MP4TAG_OK;
  libmp4tag = mp4tag_alloc (mp4error);
  if (*mp4error != MP4TAG_OK) {
    return NULL;
  }

  libmp4tag->fh = fdopen (fd, "rb");
  if (libmp4tag->fh == NULL) {
    *mp4error = MP4TAG_ERR_FILE_NOT_FOUND;
    libmp4tag->libmp4tagident = 0;
    free (libmp4tag);
    return NULL;
  }

  /* needed for parse, write */
  libmp4tag->filesz = MP4TAG_NO_FILESZ;

  /* the assumption is made that the stream being passed in is known */
  /* to be an mp4 */

  libmp4tag->isstream = true;
  libmp4tag->canwrite = false;
  libmp4tag->timeout = timeout;
  libmp4tag->offset = offset;

  return libmp4tag;
}

int
mp4tag_parse (libmp4tag_t *libmp4tag)
{
  size_t    offset;

  if (libmp4tag == NULL || libmp4tag->libmp4tagident != MP4TAG_IDENT) {
    return MP4TAG_ERR_BAD_STRUCT;
  }

  libmp4tag->mp4error = MP4TAG_OK;

  /* parsing requires an open file */
  if (libmp4tag->fh == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_NOT_OPEN;
    return libmp4tag->mp4error;
  }

  offset = ftell (libmp4tag->fh);
  mp4tag_parse_file (libmp4tag, 0, 0);

  if (libmp4tag->mp4error == MP4TAG_OK) {
    if (libmp4tag->canwrite && libmp4tag->ilst_remaining != 0) {
      /* version 1.3.x would not calculate the correct lengths */
      /* for the containers if two free boxes got combined */
      mp4tag_update_parent_lengths (libmp4tag, libmp4tag->fh, - libmp4tag->ilst_remaining);
      if (fseek (libmp4tag->fh, offset, SEEK_SET) == 0) {
        mp4tag_free_tags (libmp4tag);
        mp4tag_init_tags (libmp4tag);
        mp4tag_parse_file (libmp4tag, 0, 0);
      }
    }
    libmp4tag->parsed = true;
  }
  return libmp4tag->mp4error;
}

void
mp4tag_free (libmp4tag_t *libmp4tag)
{
  if (libmp4tag == NULL || libmp4tag->libmp4tagident != MP4TAG_IDENT) {
    return;
  }

  if (libmp4tag->isstream == false && libmp4tag->fh != NULL) {
    fclose (libmp4tag->fh);
  }
  libmp4tag->fh = NULL;

  if (libmp4tag->fn != NULL) {
    free (libmp4tag->fn);
    libmp4tag->fn = NULL;
  }

  mp4tag_free_tags (libmp4tag);

  libmp4tag->libmp4tagident = 0;
  free (libmp4tag);
}

int64_t
mp4tag_duration (libmp4tag_t *libmp4tag)
{
  if (libmp4tag == NULL || libmp4tag->libmp4tagident != MP4TAG_IDENT) {
    return 0;
  }

  if (! libmp4tag->parsed) {
    libmp4tag->mp4error = MP4TAG_ERR_NOT_PARSED;
    return libmp4tag->mp4error;
  }

  libmp4tag->mp4error = MP4TAG_OK;
  return libmp4tag->duration;
}

int
mp4tag_get_tag_by_name (libmp4tag_t *libmp4tag, const char *tag,
    mp4tagpub_t *mp4tagpub)
{
  int     idx = -1;
  int     dataidx;
  char    *ttag;

  if (libmp4tag == NULL || libmp4tag->libmp4tagident != MP4TAG_IDENT) {
    return MP4TAG_ERR_BAD_STRUCT;
  }

  if (! libmp4tag->parsed) {
    libmp4tag->mp4error = MP4TAG_ERR_NOT_PARSED;
    return libmp4tag->mp4error;
  }

  libmp4tag->mp4error = MP4TAG_OK;

  if (mp4tagpub == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_NULL_VALUE;
    return libmp4tag->mp4error;
  }
  if (libmp4tag->tags == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_NO_TAGS;
    return libmp4tag->mp4error;
  }

  ttag = strdup (tag);
  if (ttag == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
    return libmp4tag->mp4error;
  }

  mp4tag_parse_tagname (ttag, &dataidx);
  idx = mp4tag_find_tag (libmp4tag, ttag, dataidx);
  if (idx >= 0 && idx < libmp4tag->tagcount) {
    mp4tag_t    *mp4tag;

    mp4tag = &libmp4tag->tags [idx];
    mp4tag_copy_to_pub (mp4tagpub, mp4tag);
  } else {
    libmp4tag->mp4error = MP4TAG_ERR_TAG_NOT_FOUND;
  }

  free (ttag);
  return libmp4tag->mp4error;
}

int
mp4tag_iterate_init (libmp4tag_t *libmp4tag)
{
  if (libmp4tag == NULL || libmp4tag->libmp4tagident != MP4TAG_IDENT) {
    return MP4TAG_ERR_BAD_STRUCT;
  }

  if (! libmp4tag->parsed) {
    libmp4tag->mp4error = MP4TAG_ERR_NOT_PARSED;
    return libmp4tag->mp4error;
  }

  libmp4tag->iterator = 0;
  return MP4TAG_OK;
}

int
mp4tag_iterate (libmp4tag_t *libmp4tag, mp4tagpub_t *mp4tagpub)
{
  if (libmp4tag == NULL || libmp4tag->libmp4tagident != MP4TAG_IDENT) {
    return MP4TAG_ERR_BAD_STRUCT;
  }

  if (! libmp4tag->parsed) {
    libmp4tag->mp4error = MP4TAG_ERR_NOT_PARSED;
    return libmp4tag->mp4error;
  }

  libmp4tag->mp4error = MP4TAG_OK;

  if (mp4tagpub == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_NULL_VALUE;
    return libmp4tag->mp4error;
  }

  if (libmp4tag->tags == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_NO_TAGS;
    return libmp4tag->mp4error;
  }

  if (libmp4tag->iterator >= libmp4tag->tagcount) {
    libmp4tag->mp4error = MP4TAG_OK;
    return MP4TAG_FINISH;
  }

  mp4tag_copy_to_pub (mp4tagpub, &libmp4tag->tags [libmp4tag->iterator]);
  ++libmp4tag->iterator;

  return libmp4tag->mp4error;
}

int
mp4tag_set_tag (libmp4tag_t *libmp4tag, const char *tag,
    const char *data, bool forcebinary)
{
  int       idx = -1;
  bool      binary = false;
  int       offset;
  int       dataidx;
  char      *ttag;

  if (libmp4tag == NULL || libmp4tag->libmp4tagident != MP4TAG_IDENT) {
    return MP4TAG_ERR_BAD_STRUCT;
  }

  if (! libmp4tag->parsed) {
    libmp4tag->mp4error = MP4TAG_ERR_NOT_PARSED;
    return libmp4tag->mp4error;
  }

  libmp4tag->mp4error = MP4TAG_OK;

  if (tag == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_NULL_VALUE;
    return libmp4tag->mp4error;
  }

  if (data == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_NULL_VALUE;
    return libmp4tag->mp4error;
  }

  ttag = strdup (tag);
  if (ttag == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
    return libmp4tag->mp4error;
  }

  offset = mp4tag_parse_tagname (ttag, &dataidx);
// fprintf (stderr, "set-tag %s %s %d %d\n", tag, ttag, dataidx, offset);
  idx = mp4tag_find_tag (libmp4tag, ttag, dataidx);
  if (idx >= 0 && idx < libmp4tag->tagcount) {
    mp4tag_t  *mp4tag;

    mp4tag = &libmp4tag->tags [idx];
    binary = mp4tag->binary;
  }

  if (memcmp (ttag, boxids [MP4TAG_COVR], MP4TAG_ID_LEN) == 0) {
    binary = true;
    if (offset > 0) {
      /* cover name */
      binary = false;
    }
  }

  if (forcebinary) {
    binary = true;
  }

  if (binary) {
    char      *fdata;
    size_t    sz;
    int       mp4err;

    fdata = mp4tag_read_file (data, &sz, &mp4err);
    if (mp4err != MP4TAG_OK) {
      libmp4tag->mp4error = mp4err;
    }
    if (fdata != NULL) {
      mp4tag_set_tag_binary (libmp4tag, tag, idx, fdata, sz, data);
      free (fdata);
    }
  } else {
    mp4tag_set_tag_string (libmp4tag, tag, idx, data);
  }

  free (ttag);
  return libmp4tag->mp4error;
}

/* this will not work for cover images as the api is currently defined */
int
mp4tag_set_binary_tag (libmp4tag_t *libmp4tag, const char *tag,
    const char *data, size_t datalen)
{
  int       idx = -1;
  bool      binary = false;
  int       offset;
  int       dataidx;
  char      *ttag;

  if (libmp4tag == NULL || libmp4tag->libmp4tagident != MP4TAG_IDENT) {
    return MP4TAG_ERR_BAD_STRUCT;
  }

  if (! libmp4tag->parsed) {
    libmp4tag->mp4error = MP4TAG_ERR_NOT_PARSED;
    return libmp4tag->mp4error;
  }

  if (tag == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_NULL_VALUE;
    return libmp4tag->mp4error;
  }

  if (data == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_NULL_VALUE;
    return libmp4tag->mp4error;
  }

  libmp4tag->mp4error = MP4TAG_OK;

  ttag = strdup (tag);
  if (ttag == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
    return libmp4tag->mp4error;
  }

  offset = mp4tag_parse_tagname (ttag, &dataidx);
  idx = mp4tag_find_tag (libmp4tag, ttag, dataidx);
  if (idx >= 0 && idx < libmp4tag->tagcount) {
    mp4tag_t  *mp4tag;

    mp4tag = &libmp4tag->tags [idx];
    binary = mp4tag->binary;
  } else {
    /* assume the application knows what it is doing */
    binary = true;
  }

  if (memcmp (ttag, boxids [MP4TAG_COVR], MP4TAG_ID_LEN) == 0) {
    binary = true;
    if (offset > 0) {
      binary = false;
    }
  }

  if (! binary) {
    libmp4tag->mp4error = MP4TAG_ERR_MISMATCH;
    return libmp4tag->mp4error;
  }

  mp4tag_set_tag_binary (libmp4tag, tag, idx, data, datalen, NULL);

  free (ttag);
  return libmp4tag->mp4error;
}

int
mp4tag_delete_tag (libmp4tag_t *libmp4tag, const char *tag)
{
  int       idx = -1;
  int       offset;
  int       dataidx;
  char      *ttag;

  if (libmp4tag == NULL || libmp4tag->libmp4tagident != MP4TAG_IDENT) {
    return MP4TAG_ERR_BAD_STRUCT;
  }

  if (! libmp4tag->parsed) {
    libmp4tag->mp4error = MP4TAG_ERR_NOT_PARSED;
    return libmp4tag->mp4error;
  }

  libmp4tag->mp4error = MP4TAG_OK;

  if (tag == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_NULL_VALUE;
    return libmp4tag->mp4error;
  }
  if (libmp4tag->tags == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_TAG_NOT_FOUND;
    return MP4TAG_ERR_TAG_NOT_FOUND;
  }

  ttag = strdup (tag);
  if (ttag == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
    return libmp4tag->mp4error;
  }

  offset = mp4tag_parse_tagname (ttag, &dataidx);
// fprintf (stderr, "del-tag %s %s %d %d\n", tag, ttag, dataidx, offset);
  idx = mp4tag_find_tag (libmp4tag, ttag, dataidx);
  if (idx >= 0 && idx < libmp4tag->tagcount) {
    if (memcmp (ttag, boxids [MP4TAG_COVR], MP4TAG_ID_LEN) == 0) {
      if (offset > 0) {
        mp4tag_t    *mp4tag;

        mp4tag = &libmp4tag->tags [idx];
        if (mp4tag->covername != NULL) {
          free (mp4tag->covername);
          mp4tag->covername = NULL;
        }
        libmp4tag->mp4error = MP4TAG_OK;
        free (ttag);
        return libmp4tag->mp4error;
      }
    }

    mp4tag_del_tag (libmp4tag, idx);
  } else {
    /* deleting a non-existent tag is ok */
    libmp4tag->mp4error = MP4TAG_ERR_TAG_NOT_FOUND;
  }

  free (ttag);
  return libmp4tag->mp4error;
}

int
mp4tag_clean_tags (libmp4tag_t *libmp4tag)
{
  if (libmp4tag == NULL || libmp4tag->libmp4tagident != MP4TAG_IDENT) {
    return MP4TAG_ERR_BAD_STRUCT;
  }

  if (! libmp4tag->parsed) {
    libmp4tag->mp4error = MP4TAG_ERR_NOT_PARSED;
    return libmp4tag->mp4error;
  }

  libmp4tag->mp4error = MP4TAG_OK;

  if (libmp4tag->tags == NULL) {
    /* already clean */
    return MP4TAG_OK;
  }

  mp4tag_free_tags (libmp4tag);
  return libmp4tag->mp4error;
}

int
mp4tag_write_tags (libmp4tag_t *libmp4tag)
{
  char      *data = NULL;
  uint32_t  dlen = 0;
  int       rc;

  if (libmp4tag == NULL || libmp4tag->libmp4tagident != MP4TAG_IDENT) {
    libmp4tag->mp4error = MP4TAG_ERR_BAD_STRUCT;
    return libmp4tag->mp4error;
  }

  if (libmp4tag->canwrite == false) {
    libmp4tag->mp4error = MP4TAG_ERR_CANNOT_WRITE;
    return libmp4tag->mp4error;
  }

  if (! libmp4tag->parsed) {
    libmp4tag->mp4error = MP4TAG_ERR_NOT_PARSED;
    return libmp4tag->mp4error;
  }

  libmp4tag->mp4error = MP4TAG_OK;

  data = mp4tag_build_data (libmp4tag, &dlen);
  if (libmp4tag->mp4error != MP4TAG_OK) {
    return libmp4tag->mp4error;
  }

  /* if data is null and dlen == 0 , it is a complete clean of the tags */
  rc = mp4tag_write_data (libmp4tag, data, dlen);
  if (data != NULL) {
    free (data);
  }
  return rc;
}

libmp4tagpreserve_t *
mp4tag_preserve_tags (libmp4tag_t *libmp4tag)
{
  libmp4tagpreserve_t *preserve = NULL;

  if (libmp4tag == NULL) {
    return NULL;
  }

  if (! libmp4tag->parsed) {
    libmp4tag->mp4error = MP4TAG_ERR_NOT_PARSED;
    return NULL;
  }

  if (libmp4tag->tags == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_NO_TAGS;
    return NULL;
  }

  libmp4tag->mp4error = MP4TAG_OK;

  preserve = malloc (sizeof (libmp4tagpreserve_t));
  if (preserve == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
    return NULL;
  }

  preserve->tagcount = libmp4tag->tagcount;
  preserve->tags = malloc (sizeof (mp4tag_t) * preserve->tagcount);
  if (preserve->tags == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
    free (preserve);
    return NULL;
  }

  libmp4tag->mp4error = MP4TAG_OK;

  for (int i = 0; i < preserve->tagcount; ++i) {
    mp4tag_clone_tag (libmp4tag, &preserve->tags [i], &libmp4tag->tags [i]);
  }

  return preserve;
}

int
mp4tag_restore_tags (libmp4tag_t *libmp4tag, libmp4tagpreserve_t *preserve)
{
  if (libmp4tag == NULL || libmp4tag->libmp4tagident != MP4TAG_IDENT) {
    return MP4TAG_ERR_BAD_STRUCT;
  }

  if (! libmp4tag->parsed) {
    libmp4tag->mp4error = MP4TAG_ERR_NOT_PARSED;
    return libmp4tag->mp4error;
  }

  if (preserve == NULL) {
    libmp4tag->mp4error = MP4TAG_ERR_NULL_VALUE;
    return libmp4tag->mp4error;
  }

  libmp4tag->mp4error = MP4TAG_OK;

  mp4tag_free_tags (libmp4tag);

  libmp4tag->tagcount = preserve->tagcount;
  if (libmp4tag->tagcount > libmp4tag->tagalloccount) {
    libmp4tag->tagalloccount = libmp4tag->tagcount;
    libmp4tag->tags = realloc (libmp4tag->tags,
        sizeof (mp4tag_t) * libmp4tag->tagcount);
    if (libmp4tag->tags == NULL) {
      libmp4tag->mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
      return libmp4tag->mp4error;
    }
  }

  for (int i = 0; i < preserve->tagcount; ++i) {
    mp4tag_clone_tag (libmp4tag, &libmp4tag->tags [i], &preserve->tags [i]);
  }

  return libmp4tag->mp4error;
}

int
mp4tag_preserve_free (libmp4tagpreserve_t *preserve)
{
  int     rc = MP4TAG_OK;

  if (preserve == NULL) {
    return MP4TAG_ERR_NULL_VALUE;
  }

  if (preserve->tags != NULL) {
    for (int i = 0; i < preserve->tagcount; ++i) {
      mp4tag_free_tag (&preserve->tags [i]);
    }
    free (preserve->tags);
    preserve->tags = NULL;
    preserve->tagcount = 0;
  }
  free (preserve);

  return rc;
}

int
mp4tag_error (libmp4tag_t *libmp4tag)
{
  if (libmp4tag == NULL || libmp4tag->libmp4tagident != MP4TAG_IDENT) {
    return MP4TAG_ERR_BAD_STRUCT;
  }

  return libmp4tag->mp4error;
}

const char *
mp4tag_version (void)
{
  return LIBMP4TAG_VERSION;
}

const char *
mp4tag_error_str (libmp4tag_t *libmp4tag)
{
  if (libmp4tag == NULL || libmp4tag->libmp4tagident != MP4TAG_IDENT) {
    return mp4tagerrmsgs [MP4TAG_ERR_BAD_STRUCT];
  }

  return mp4tagerrmsgs [libmp4tag->mp4error];
}

void
mp4tag_set_debug_flags (libmp4tag_t *libmp4tag, int dbgflags)
{
  if (libmp4tag == NULL || libmp4tag->libmp4tagident != MP4TAG_IDENT) {
    return;
  }

  libmp4tag->dbgflags = dbgflags;
}

void
mp4tag_set_option (libmp4tag_t *libmp4tag, int option)
{
  if (libmp4tag == NULL || libmp4tag->libmp4tagident != MP4TAG_IDENT) {
    return;
  }

  libmp4tag->options |= option;
}


/* internal routines */

static libmp4tag_t *
mp4tag_alloc (int *mp4error)
{
  libmp4tag_t   *libmp4tag = NULL;

  libmp4tag = malloc (sizeof (libmp4tag_t));
  if (libmp4tag == NULL) {
    *mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
    return NULL;
  }

  libmp4tag->libmp4tagident = MP4TAG_IDENT;
  libmp4tag->fn = NULL;
  libmp4tag->filesz = MP4TAG_NO_FILESZ;
  libmp4tag->fh = NULL;

  mp4tag_init_tags (libmp4tag);

  libmp4tag->isstream = false;
  libmp4tag->canwrite = true;

  return libmp4tag;
}


static void
mp4tag_free_tags (libmp4tag_t *libmp4tag)
{
  if (libmp4tag == NULL || libmp4tag->libmp4tagident != MP4TAG_IDENT) {
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

static void
mp4tag_copy_to_pub (mp4tagpub_t *mp4tagpub, mp4tag_t *mp4tag)
{
  mp4tagpub->tag = mp4tag->tag;
  mp4tagpub->data = mp4tag->data;
  mp4tagpub->datalen = mp4tag->datalen;
  mp4tagpub->covername = mp4tag->covername;
  mp4tagpub->dataidx = mp4tag->dataidx;
  mp4tagpub->covertype = mp4tag->identtype;
  mp4tagpub->binary = mp4tag->binary;
}

static void
mp4tag_init_tags (libmp4tag_t *libmp4tag)
{
  libmp4tag->tags = NULL;
  libmp4tag->offset = 0;
  libmp4tag->creationdate = 0;
  libmp4tag->modifieddate = 0;
  libmp4tag->duration = 0;
  libmp4tag->samplerate = 0;
  libmp4tag->timeout = 0;
  for (int i = 0; i < MP4TAG_LEVEL_MAX; ++i) {
    libmp4tag->base_lengths [i] = 0;
    libmp4tag->base_offsets [i] = 0;
    libmp4tag->base_name [i][0] = '\0';
    libmp4tag->calc_length [i] = 0;
    libmp4tag->rem_length [i] = 0;
  }
  libmp4tag->base_offset_count = 0;
  libmp4tag->taglist_base_offset = 0;
  libmp4tag->taglist_offset = 0;
  libmp4tag->taglist_orig_len = 0;
  libmp4tag->taglist_len = 0;
  libmp4tag->interior_free_len = 0;
  libmp4tag->exterior_free_len = 0;
  libmp4tag->taglist_orig_data_len = 0;     // debugging
  libmp4tag->parentidx = -1;
  libmp4tag->noilst_offset = 0;
  libmp4tag->after_ilst_offset = 0;
  libmp4tag->insert_delta = 0;
  libmp4tag->stco_offset = 0;
  libmp4tag->stco_len = 0;
  libmp4tag->co64_offset = 0;
  libmp4tag->co64_len = 0;
  libmp4tag->datacount = 0;
  libmp4tag->lastbox_offset = -1;
  libmp4tag->tagcount = 0;
  libmp4tag->tagalloccount = 0;
  libmp4tag->iterator = 0;
  libmp4tag->mp4error = MP4TAG_OK;
  libmp4tag->dbgflags = 0;
  libmp4tag->options = MP4TAG_OPTION_NONE;
  libmp4tag->mp7meta = false;
  libmp4tag->unlimited = false;
  libmp4tag->datacount = 0;
  libmp4tag->parsed = false;
  libmp4tag->processdata = false;
  libmp4tag->checkforfree = false;
  libmp4tag->parsedone = false;
}

#if LIBMP4TAG_DEBUG
static void
enable_core_dump (void)
{
#if _lib_setrlimit
  struct rlimit corelim;

  corelim.rlim_cur = RLIM_INFINITY;
  corelim.rlim_max = RLIM_INFINITY;

  setrlimit (RLIMIT_CORE, &corelim);
#endif
  return;
}
#endif
