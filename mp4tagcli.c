#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libmp4tag.h"

int
main (int argc, const char *argv [])
{
  FILE        *fh;

  if (argc < 2) {
    fprintf (stderr, "no file specified %d\n", argc);
    exit (1);
  }

  fh = fopen (argv [1], "rb");
  if (fh == NULL) {
    fprintf (stderr, "unable to open %s\n", argv [1]);
    exit (1);
  }

  parsemp4 (fh);

  fclose (fh);
  exit (0);
}

