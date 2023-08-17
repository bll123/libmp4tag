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

#include "libmp4tag.h"

static void setTagName (const char *tag, char *buff, size_t sz);

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
  bool          binary = false;
  bool          clean = false;
  int           fnidx = -1;

  static struct option mp4tagcli_options [] = {
    { "binary",         no_argument,        NULL,   'b' },
    { "clean",          no_argument,        NULL,   'c' },
    { "display",        required_argument,  NULL,   'd' },
    { "dump",           no_argument,        NULL,   'D' },
    { "duration",       no_argument,        NULL,   'u' },
    { NULL,             0,                  NULL,   0 }
  };

  *tagname = '\0';

  while ((c = getopt_long_only (argc, argv, "cd:Du",
      mp4tagcli_options, &option_index)) != -1) {
    switch (c) {
      case 'b': {
        binary = true;
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

  libmp4tag = mp4tag_open (argv [fnidx]);
  if (libmp4tag == NULL) {
    fprintf (stderr, "unable to open %s\n", argv [1]);
    exit (1);
  }

  mp4tag_parse (libmp4tag);

  if (display) {
    int     rc;

    rc = mp4tag_get_tag_by_name (libmp4tag, tagname, &mp4tagpub);
    if (rc != MP4TAG_OK) {
      fprintf (stdout, "%s not found\n", tagname);
    }
    if (rc == MP4TAG_OK &&
        ! mp4tagpub.binary &&
        mp4tagpub.name != NULL &&
        mp4tagpub.data != NULL) {
      fprintf (stdout, "%s=%s\n", mp4tagpub.name, mp4tagpub.data);
    }
    if (rc == MP4TAG_OK &&
        ! dump &&
        mp4tagpub.binary &&
        mp4tagpub.name != NULL) {
      fprintf (stdout, "%s (binary data: %" PRId64 ")\n",
          mp4tagpub.name, (uint64_t) mp4tagpub.datalen);
    }
    if (rc == MP4TAG_OK &&
        dump &&
        mp4tagpub.binary &&
        mp4tagpub.name != NULL) {
      fwrite (mp4tagpub.data, mp4tagpub.datalen, 1, stdout);
    }
  }
  if (clean) {
    mp4tag_clean_tags (libmp4tag);
  }

  for (int i = fnidx + 1; i < argc; ++i) {
    char    *tstr;
    char    *tokstr;
    char    *p;

    tstr = strdup (argv [i]);
    if (tstr != NULL) {
      p = strtok_r (tstr, "=", &tokstr);
      setTagName (tstr, tagname, sizeof (tagname));
      if (p != NULL) {
        p = strtok_r (NULL, "=", &tokstr);
        if (p == NULL) {
          mp4tag_delete_tag (libmp4tag, tagname);
	}
        if (p != NULL) {
          if (! binary) {
            setTagName (tstr, tagname, sizeof (tagname));
            mp4tag_set_tag_str (libmp4tag, tagname, p);
          }
        }
      }
      free (tstr);
    }
  }

  if (duration) {
    fprintf (stdout, "%" PRId64 "\n", mp4tag_duration (libmp4tag));
  }
  if (! display && ! duration && ! clean) {
    fprintf (stdout, "duration=%" PRId64 "\n", mp4tag_duration (libmp4tag));

    mp4tag_iterate_init (libmp4tag);
    while (mp4tag_iterate (libmp4tag, &mp4tagpub) == MP4TAG_OK) {
      if (! mp4tagpub.binary &&
          mp4tagpub.name != NULL &&
          mp4tagpub.data != NULL) {
        fprintf (stdout, "%s=%s\n", mp4tagpub.name, mp4tagpub.data);
      }
    }
  }

  mp4tag_free (libmp4tag);
  exit (0);
}

static void
setTagName (const char *tag, char *buff, size_t sz)
{
  if (strlen (tag) == 3) {
    snprintf (buff, sz, "%s%s", COPYRIGHT_STR, tag);
  } else {
    strcpy (buff, tag);
  }
}
