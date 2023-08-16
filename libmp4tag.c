/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */

#include "config.h"

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

