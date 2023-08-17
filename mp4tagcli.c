/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#include "libmp4tag.h"

int
main (int argc, const char *argv [])
{
  libmp4tag_t   *libmp4tag;
  mp4tagdisp_t  mp4tagdisp;

  if (argc < 2) {
    fprintf (stderr, "no file specified %d\n", argc);
    exit (1);
  }

  libmp4tag = mp4tag_open (argv [1]);
  if (libmp4tag == NULL) {
    fprintf (stderr, "unable to open %s\n", argv [1]);
    exit (1);
  }

  mp4tag_parse (libmp4tag);

  fprintf (stdout, "duration=%" PRId64 "\n", mp4tag_duration (libmp4tag));

  mp4tag_iterate_init (libmp4tag);
  while (mp4tag_iterate (libmp4tag, &mp4tagdisp) == MP4TAG_OK) {
    if (! mp4tagdisp.binary &&
        mp4tagdisp.name != NULL &&
        mp4tagdisp.data != NULL) {
      fprintf (stdout, "%s=%s\n", mp4tagdisp.name, mp4tagdisp.data);
    }
  }

  mp4tag_free (libmp4tag);
  exit (0);
}

