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
#include <wchar.h>

#include "libmp4tag.h"
#include "libmp4tagint.h"

static FILE * mp4tag_fopen (const char *fn, const char *mode);
#ifdef _WIN32
static void * mp4tag_towide (const char *buff);
#endif


libmp4tag_t *
mp4tag_open (const char *fn)
{
  libmp4tag_t *libmp4tag = NULL;

  if (fn == NULL) {
    return NULL;
  }

  libmp4tag = malloc (sizeof (libmp4tag_t));
  libmp4tag->fn = strdup (fn);
  libmp4tag->fh = mp4tag_fopen (fn, "rb");
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

  return libmp4tag;
}

void
mp4tag_close (libmp4tag_t *libmp4tag)
{
  if (libmp4tag == NULL) {
    return;
  }
  if (libmp4tag->fh == NULL) {
    return;
  }
  fclose (libmp4tag->fh);
  libmp4tag->fh = NULL;
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
  if (libmp4tag->tags != NULL) {
    for (int i = 0; i < libmp4tag->tagcount; ++i) {
      if (libmp4tag->tags [i].name != NULL) {
        free (libmp4tag->tags [i].name);
      }
      if (libmp4tag->tags [i].data != NULL) {
        free (libmp4tag->tags [i].data);
      }
    }
    free (libmp4tag->tags);
    libmp4tag->tags = NULL;
    libmp4tag->tagcount = 0;
    libmp4tag->tagalloccount = 0;
  }
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
    return;
  }

  mp4tag_parse_file (libmp4tag);
}

int64_t
mp4tag_duration (libmp4tag_t *libmp4tag)
{
  if (libmp4tag == NULL) {
    return 0;
  }

  return libmp4tag->duration;
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
mp4tag_iterate (libmp4tag_t *libmp4tag, mp4tagdisp_t *mp4tagdisp)
{
  if (libmp4tag == NULL) {
    return MP4TAG_ERROR;
  }
  if (mp4tagdisp == NULL) {
    return MP4TAG_ERROR;
  }

  if (libmp4tag->iterator >= libmp4tag->tagcount) {
    return MP4TAG_FINISH;
  }
  if (libmp4tag->tags == NULL) {
    return MP4TAG_ERROR;
  }

  mp4tagdisp->name = libmp4tag->tags [libmp4tag->iterator].name;
  mp4tagdisp->data = libmp4tag->tags [libmp4tag->iterator].data;
  mp4tagdisp->binary = libmp4tag->tags [libmp4tag->iterator].binary;
  ++libmp4tag->iterator;

  return MP4TAG_OK;
}

const char *
mp4tag_version (void)
{
  return LIBMP4TAG_VERSION;
}

const char *
mp4tag_api_version (void)
{
  return LIBMP4TAG_API_VERSION;
}

static FILE *
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

#ifdef _WIN32
static void *
mp4tag_towide (const char *buff)
{
    wchar_t     *tbuff = NULL;
    size_t      len;

    /* the documentation lies; len does not include room for the null byte */
    len = MultiByteToWideChar (CP_UTF8, 0, buff, strlen (buff), NULL, 0);
    tbuff = malloc ((len + 1) * sizeof (wchar_t));
    MultiByteToWideChar (CP_UTF8, 0, buff, strlen (buff), tbuff, len);
    tbuff [len] = L'\0';
    return tbuff;
}
#endif
