#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libmp4tag.h"

int
main (int argc, const char *argv [])
{
  libmp4tag_t   *libmp4tag;

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
  mp4tag_free (libmp4tag);
  exit (0);
}

