/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

#if _hdr_windows
# define WIN32_LEAN_AND_MEAN 1
# include <windows.h>
#endif

#include "libmp4tag.h"

typedef struct {
  int       nargc;
  char      **utf8argv;
} argcopy_t;

static libmp4tag_t * openparse (const char *fname, int dbgflags, int options);
static void setTagName (const char *tag, char *buff, size_t sz);
static void displayTag (mp4tagpub_t *mp4tagpub);
static void cleanargs (argcopy_t *argcopy);

int
main (int argc, char *argv [])
{
  argcopy_t     argcopy;
  libmp4tag_t   *libmp4tag;
  mp4tagpub_t   mp4tagpub;
  int           c;
  int           option_index;
  char          tagname [MP4TAG_ID_MAX];
  const         char *infname = NULL;
  const         char *copyto = NULL;
  const         char *dumpfn = NULL;
  bool          display = false;
  bool          dump = false;
  bool          duration = false;
  bool          forcebinary = false;
  bool          clean = false;
  bool          write = false;
  bool          copy = false;
  bool          testbin = false;
  int           fnidx = -1;
  int           dbgflags = 0;
  int           options = 0;
  int           rc = MP4TAG_OK;
  char          *targ;
#if _lib_GetCommandLineW
  wchar_t       **wargv;
  int           targc;
#endif

  static struct option mp4tagcli_options [] = {
    { "binary",         no_argument,        NULL,   'b' },
    { "clean",          no_argument,        NULL,   'c' },
    { "copyfrom",       required_argument,  NULL,   'f' },
    { "backup",         no_argument,        NULL,   'k' },
    { "copyto",         required_argument,  NULL,   't' },
    { "debug",          required_argument,  NULL,   'x' },
    { "display",        required_argument,  NULL,   'd' },
    { "dump",           required_argument,  NULL,   'D' },
    { "duration",       no_argument,        NULL,   'u' },
    { "testbin",        no_argument,        NULL,   'B' },
    { "version",        no_argument,        NULL,   'v' },
    { NULL,             0,                  NULL,   0 }
  };

  /* msys2 does not convert argv to utf8 on windows */
  argcopy.nargc = argc;
  argcopy.utf8argv = NULL;
  if (argc > 0) {
    argcopy.utf8argv = malloc (sizeof (char *) * argc);
  }
#if _lib_GetCommandLineW
  wargv = CommandLineToArgvW (GetCommandLineW(), &targc);
  for (int i = 0; i < argc; ++i) {
    argcopy.utf8argv [i] = mp4tag_fromwide (wargv [i]);
  }
  LocalFree (wargv);
#else
  for (int i = 0; i < argc; ++i) {
    argcopy.utf8argv [i] = strdup (argv [i]);
  }
#endif

  *tagname = '\0';

  /* do not specify the 'c' clean short argument */
  while ((c = getopt_long_only (argc, argcopy.utf8argv, "d:D:f:kFt:ux:",
      mp4tagcli_options, &option_index)) != -1) {
    switch (c) {
      case 'b': {
        forcebinary = true;
        break;
      }
      case 'B': {
        testbin = true;
        break;
      }
      case 'c': {
        clean = true;
        break;
      }
      case 'D': {
        dump = true;
        if (optarg != NULL) {
          dumpfn = argcopy.utf8argv [optind - 1];
        }
        break;
      }
      case 'd': {
        display = true;
        if (optarg != NULL) {
          targ = argcopy.utf8argv [optind - 1];
          setTagName (targ, tagname, sizeof (tagname));
        }
        break;
      }
      case 'f': {
        if (optarg != NULL) {
          targ = argcopy.utf8argv [optind - 1];
          infname = targ;
        }
        break;
      }
      case 'k': {
        options |= MP4TAG_OPTION_KEEP_BACKUP;
        break;
      }
      case 't': {
        if (optarg != NULL) {
          targ = argcopy.utf8argv [optind - 1];
          copyto = targ;
        }
        break;
      }
      case 'u': {
        duration = true;
        break;
      }
      case 'v': {
        fprintf (stdout, "mp4tagcli: version %s\n", mp4tag_version ());
        exit (0);
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

  if (infname != NULL && copyto != NULL) {
    copy = true;
  }

  if (! copy) {
    fnidx = optind;
    if (fnidx <= 0 || fnidx >= argc) {
      fprintf (stderr, "no file specified\n");
      cleanargs (&argcopy);
      exit (1);
    }
    infname = argcopy.utf8argv [fnidx];
  }

  libmp4tag = openparse (infname, dbgflags, options);

  if (copy) {
    libmp4tagpreserve_t   *preserve;

    preserve = mp4tag_preserve_tags (libmp4tag);
    mp4tag_free (libmp4tag);
    libmp4tag = openparse (copyto, dbgflags, options);
    mp4tag_restore_tags (libmp4tag, preserve);
    write = true;
  }

  if (clean && ! copy) {
    if (mp4tag_clean_tags (libmp4tag) != MP4TAG_OK) {
      fprintf (stderr, "Unable to clean tags (%s)\n", mp4tag_error_str (libmp4tag));
      rc = mp4tag_error (libmp4tag);
    }
    write = true;
  }

  if (rc == MP4TAG_OK && ! clean && ! copy) {
    for (int i = fnidx + 1; i < argc; ++i) {
      char    *tstr;
      char    *tokstr;
      char    *p;

      tstr = strdup (argcopy.utf8argv [i]);
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
            rc = mp4tag_error (libmp4tag);
          }
        }
        if (p != NULL) {
          if (testbin) {
            char    *data = NULL;
            size_t  sz;
            int     mp4err;

            data = mp4tag_read_file (p, &sz, &mp4err);
            if (mp4err == MP4TAG_OK && data != NULL) {
              if (mp4tag_set_binary_tag (libmp4tag, tagname, data, sz) == MP4TAG_OK) {
                write = true;
              } else {
                fprintf (stderr, "Unable to set tag: %s (%s)\n", tagname, mp4tag_error_str (libmp4tag));
                rc = mp4tag_error (libmp4tag);
              }
            }
          } else {
            if (mp4tag_set_tag (libmp4tag, tagname, p, forcebinary) == MP4TAG_OK) {
              write = true;
            } else {
              fprintf (stderr, "Unable to set tag: %s (%s)\n", tagname, mp4tag_error_str (libmp4tag));
              rc = mp4tag_error (libmp4tag);
            }
          }
        } /* data is being set for the tag */
      } /* there is a tag name */

      free (tstr);

    } /* for each argument on the command line */
  } /* not clean */

  if (rc == MP4TAG_OK && write) {
    if (mp4tag_write_tags (libmp4tag) != MP4TAG_OK) {
      fprintf (stderr, "Unable to write tags (%s)\n", mp4tag_error_str (libmp4tag));
    }
  }

  if (rc == MP4TAG_OK && display && ! clean && ! copy) {
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
        FILE    *fh;

        fh = fopen (dumpfn, "wb");
        if (fh != NULL) {
          fwrite (mp4tagpub.data, mp4tagpub.datalen, 1, fh);
          fclose (fh);
        }
      }
    }
  }

  if (rc == MP4TAG_OK && ! write && duration) {
    fprintf (stdout, "%" PRId64 "\n", mp4tag_duration (libmp4tag));
  }

  if (rc == MP4TAG_OK && ! write && ! display && ! duration && ! clean) {
    fprintf (stdout, "duration=%" PRId64 "\n", mp4tag_duration (libmp4tag));

    mp4tag_iterate_init (libmp4tag);
    while (mp4tag_iterate (libmp4tag, &mp4tagpub) == MP4TAG_OK) {
      displayTag (&mp4tagpub);
    }
  }

  mp4tag_free (libmp4tag);
  cleanargs (&argcopy);
  return rc;
}

static void
setTagName (const char *tag, char *buff, size_t sz)
{
  if (strcmp (tag, "art") == 0) {
    tag = "ART";
  }
  if (strcmp (tag, "aart") == 0) {
    tag = "aART";
  }
  if (strlen (tag) == 3 || (strlen (tag) >= 4 && tag [3] == ':')) {
    snprintf (buff, sz, "%s%s", PREFIX_STR, tag);
  } else {
    snprintf (buff, sz, "%s", tag);
  }
}

static void
displayTag (mp4tagpub_t *mp4tagpub)
{
  if (! mp4tagpub->binary &&
      mp4tagpub->tag != NULL &&
      mp4tagpub->data != NULL) {
    if (mp4tagpub->dataidx > 0) {
      fprintf (stdout, "%s:%d=%s\n",
          mp4tagpub->tag, mp4tagpub->dataidx, mp4tagpub->data);
    } else {
      fprintf (stdout, "%s=%s\n", mp4tagpub->tag, mp4tagpub->data);
    }
  }
  if (mp4tagpub->binary &&
      mp4tagpub->tag != NULL) {
    const char  *covertypedisp = "";

    if (strcmp (mp4tagpub->tag, "covr") == 0) {
      covertypedisp = "jpg ";
      if (mp4tagpub->covertype == MP4TAG_COVER_PNG) {
        covertypedisp = "png ";
      }
    }
    if (mp4tagpub->dataidx > 0) {
      fprintf (stdout, "%s:%d=(data: %s%" PRId64 " bytes)\n",
          mp4tagpub->tag, mp4tagpub->dataidx, covertypedisp, (uint64_t) mp4tagpub->datalen);
    } else {
      fprintf (stdout, "%s=(data: %s%" PRId64 " bytes)\n",
          mp4tagpub->tag, covertypedisp, (uint64_t) mp4tagpub->datalen);
    }
    if (mp4tagpub->covername != NULL &&
        *mp4tagpub->covername) {
      /* cover name */
      fprintf (stdout, "%s:%d:name=%s\n",
          mp4tagpub->tag, mp4tagpub->dataidx, mp4tagpub->covername);
    }
  }
}

static libmp4tag_t *
openparse (const char *fname, int dbgflags, int options)
{
  libmp4tag_t   *libmp4tag = NULL;
  int           mp4error;

  libmp4tag = mp4tag_open (fname, &mp4error);
  if (libmp4tag == NULL) {
    fprintf (stderr, "unable to open %s\n", fname);
    exit (1);
  }

  mp4tag_set_option (libmp4tag, options);
  if (dbgflags != 0) {
    mp4tag_set_debug_flags (libmp4tag, dbgflags);
  }

  mp4tag_parse (libmp4tag);
  return libmp4tag;
}

static void
cleanargs (argcopy_t *argcopy)
{
  if (argcopy->utf8argv != NULL) {
    for (int i = 0; i < argcopy->nargc; ++i) {
      if (argcopy->utf8argv [i] != NULL) {
        free (argcopy->utf8argv [i]);
      }
    }

    free (argcopy->utf8argv);
  }
}
