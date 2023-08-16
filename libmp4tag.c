/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 *
 * Resources:
 *    https://picard-docs.musicbrainz.org/en/appendices/tag_mapping.html
 *    https://xhelmboyx.tripod.com/formats/mp4-layout.txt
 *    https://docs.mp3tag.de/mapping/
 */

// #include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libmp4tag.h"
#include "libmp4tagint.h"

libmp4tag_t *
mp4tag_open (const char *fn)
{
  libmp4tag_t *libmp4tag = NULL;

  if (fn == NULL) {
    return NULL;
  }

  libmp4tag = malloc (sizeof (libmp4tag_t));
  libmp4tag->fn = strdup (fn);
  libmp4tag->fh = fopen (fn, "rb");
  libmp4tag->tags = NULL;
  libmp4tag->tagcount = 0;

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
  }
  if (libmp4tag->fh != NULL) {
    fclose (libmp4tag->fh);
  }
  free (libmp4tag);
}

void
mp4tag_parse (libmp4tag_t *libmp4tag)
{
  if (libmp4tag == NULL) {
    return;
  }
  if (libmp4tag->fh == NULL) {
    return;
  }

  parsemp4 (libmp4tag);
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

