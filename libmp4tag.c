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

const char *
mp4tag_version (char *vbuff, size_t sz)
{
  return LIBMP4TAG_VERSION;
}

const char *
mp4tag_api_version (void)
{
  return LIBMP4TAG_API_VERSION;
}

