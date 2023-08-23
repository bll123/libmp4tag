/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

#include "libmp4tag.h"

static void setTagName (const char *tag, char *buff, size_t sz);
static void displayTag (mp4tagpub_t *mp4tagpub);

int
main (int argc, char *argv [])
{
  libmp4tag_t   *libmp4tag;
  mp4tagpub_t   mp4tagpub;
  int           c;
  int           option_index;
  char          tagname [MP4TAG_ID_MAX];
  bool          display = false;
  bool          dump = false;
  bool          duration = false;
  bool          forcebinary = false;
  bool          clean = false;
  bool          write = false;
  int           fnidx = -1;
  int           mp4error;
  int           dbgflags = 0;

  static struct option mp4tagcli_options [] = {
    { "binary",         no_argument,        NULL,   'b' },
    { "clean",          no_argument,        NULL,   'c' },
    { "debug",          required_argument,  NULL,   'x' },
    { "display",        required_argument,  NULL,   'd' },
    { "dump",           no_argument,        NULL,   'D' },
    { "duration",       no_argument,        NULL,   'u' },
    { NULL,             0,                  NULL,   0 }
  };

  *tagname = '\0';

  while ((c = getopt_long_only (argc, argv, "cd:Dux:",
      mp4tagcli_options, &option_index)) != -1) {
    switch (c) {
      case 'b': {
        forcebinary = true;
        break;
      }
      case 'c': {
        clean = true;
        break;
      }
      case 'D': {
        dump = true;
        break;
      }
      case 'd': {
        display = true;
        if (optarg != NULL) {
          setTagName (optarg, tagname, sizeof (tagname));
        }
        break;
      }
      case 'u': {
        duration = true;
        break;
      }
      case 'x': {
        dbgflags = atoi (optarg);
	break;
      }
      default: {
        break;
      }
    }
  }

  fnidx = optind;
  if (fnidx <= 0 || fnidx >= argc) {
    fprintf (stderr, "no file specified\n");
    exit (1);
  }

  libmp4tag = mp4tag_open (argv [fnidx], &mp4error);
  if (libmp4tag == NULL) {
    fprintf (stderr, "unable to open %s\n", argv [1]);
    exit (1);
  }

  if (dbgflags != 0) {
    mp4tag_set_debug_flags (libmp4tag, dbgflags);
  }

  mp4tag_parse (libmp4tag);

  if (clean) {
    mp4tag_clean_tags (libmp4tag);
    write = true;
  }

  if (! clean) {
    for (int i = fnidx + 1; i < argc; ++i) {
      char    *tstr;
      char    *tokstr;
      char    *p;

      tstr = strdup (argv [i]);
      if (tstr == NULL) {
        continue;
      }

      p = strtok_r (tstr, "=", &tokstr);
      setTagName (tstr, tagname, sizeof (tagname));
      if (p != NULL) {
        p = strtok_r (NULL, "=", &tokstr);
        if (p == NULL) {
          if (mp4tag_delete_tag (libmp4tag, tagname) == MP4TAG_OK) {
            write = true;
          } else {
            fprintf (stderr, "Unable to delete tag: %s (%s)\n", tagname, mp4tag_error_str (libmp4tag));
          }
        }
        if (p != NULL) {
          if (mp4tag_set_tag (libmp4tag, tagname, p, forcebinary) == MP4TAG_OK) {
            write = true;
          } else {
            fprintf (stderr, "Unable to set tag: %s (%s)\n", tagname, mp4tag_error_str (libmp4tag));
          }
        } /* data is being set for the tag */
      } /* there is a tag name */

      free (tstr);

    } /* for each argument on the command line */
  } /* not clean */

  if (write) {
    if (mp4tag_write_tags (libmp4tag) != MP4TAG_OK) {
      fprintf (stderr, "Unable to write tags (%s)\n", mp4tag_error_str (libmp4tag));
    }
  }

  if (display && ! clean) {
    int     rc;

    rc = mp4tag_get_tag_by_name (libmp4tag, tagname, &mp4tagpub);
    if (rc != MP4TAG_OK) {
      fprintf (stdout, "%s not found\n", tagname);
    }
    if (rc == MP4TAG_OK) {
      if (! dump) {
        displayTag (&mp4tagpub);
      }
      if (dump &&
          mp4tagpub.binary &&
          mp4tagpub.tag != NULL) {
        fwrite (mp4tagpub.data, mp4tagpub.datalen, 1, stdout);
      }
    }
  }

  if (! write && duration) {
    fprintf (stdout, "%" PRId64 "\n", mp4tag_duration (libmp4tag));
  }

  if (! write && ! display && ! duration && ! clean) {
    fprintf (stdout, "duration=%" PRId64 "\n", mp4tag_duration (libmp4tag));

    mp4tag_iterate_init (libmp4tag);
    while (mp4tag_iterate (libmp4tag, &mp4tagpub) == MP4TAG_OK) {
      displayTag (&mp4tagpub);
    }
  }

  mp4tag_free (libmp4tag);
  exit (0);
}

static void
setTagName (const char *tag, char *buff, size_t sz)
{
  if (strlen (tag) == 3) {
    snprintf (buff, sz, "%s%s", PREFIX_STR, tag);
  } else {
    strcpy (buff, tag);
  }
}

static void
displayTag (mp4tagpub_t *mp4tagpub)
{
  if (! mp4tagpub->binary &&
      mp4tagpub->tag != NULL &&
      mp4tagpub->data != NULL) {
    fprintf (stdout, "%s=%s\n", mp4tagpub->tag, mp4tagpub->data);
  }
  if (mp4tagpub->binary &&
      mp4tagpub->tag != NULL) {
    if (mp4tagpub->coveridx > 0) {
      fprintf (stdout, "%s:%d=(data: %" PRId64 " bytes)\n",
          mp4tagpub->tag, mp4tagpub->coveridx, (uint64_t) mp4tagpub->datalen);
    } else {
      fprintf (stdout, "%s=(data: %" PRId64 " bytes)\n",
          mp4tagpub->tag, (uint64_t) mp4tagpub->datalen);
    }
    if (mp4tagpub->covername != NULL &&
        *mp4tagpub->covername) {
      /* cover name */
      fprintf (stdout, "%s:%d:name=%s\n",
          mp4tagpub->tag, mp4tagpub->coveridx, mp4tagpub->covername);
    }
  }
}
