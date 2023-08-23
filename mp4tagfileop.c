/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#if _hdr_windows
# include <windows.h>
#endif

#include "libmp4tag.h"
#include "mp4tagint.h"

#ifdef _WIN32
static void * mp4tag_towide (const char *buff);
#endif

FILE *
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

ssize_t
mp4tag_file_size (const char *fname)
{
  ssize_t       sz = -1;

#if _lib__wstat
  {
    struct _stat  statbuf;
    wchar_t       *tfname = NULL;
    int           rc;

    tfname = mp4tag_towide (fname);
    rc = _wstat (tfname, &statbuf);
    if (rc == 0) {
      sz = statbuf.st_size;
    }
    mdfree (tfname);
  }
#else
  {
    int rc;
    struct stat statbuf;

    rc = stat (fname, &statbuf);
    if (rc == 0) {
      sz = statbuf.st_size;
    }
  }
#endif
  return sz;
}

char *
mp4tag_read_file (libmp4tag_t *libmp4tag, const char *fn, size_t *sz)
{
  char    *fdata = NULL;
  int     rc = -1;

  *sz = mp4tag_file_size (fn);
  if (*sz > 0) {
    FILE    *fh;

    fdata = malloc (*sz);
    if (fdata == NULL) {
      return NULL;
    }

    fh = mp4tag_fopen (fn, "rb");
    if (fh != NULL) {
      rc = fread (fdata, *sz, 1, fh);
      fclose (fh);
    }
  } /* file has a valid size */

  if (rc != 1 && fdata != NULL) {
    free (fdata);
    fdata = NULL;
  }

  return fdata;
}

int
mp4tag_file_delete (const char *fname)
{
  int     rc;
#if _lib__wunlink
  wchar_t *tname;
#endif

  if (fname == NULL) {
    return 0;
  }

#if _lib__wunlink
  tname = mp4tag_towide (fname);
  rc = _wunlink (tname);
  mdfree (tname);
#else
  rc = unlink (fname);
#endif
  return rc;
}

int
mp4tag_file_move (const char *fname, const char *nfn)
{
  int       rc = -1;

#if _lib__wrename
  /*
   * Windows won't rename to an existing file, but does
   * not return an error.
   */
  mp4tag_file_delete (nfn);
  {
    wchar_t   *wfname;
    wchar_t   *wnfn;

    wfname = mp4tag_towide (fname);
    wnfn = mp4tag_towide (nfn);
    rc = _wrename (wfname, wnfn);
    mdfree (wfname);
    mdfree (wnfn);
  }
#else
fprintf (stdout, "rename %s %s\n", fname, nfn);
  rc = rename (fname, nfn);
#endif
  return rc;
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
    if (tbuff != NULL) {
      MultiByteToWideChar (CP_UTF8, 0, buff, strlen (buff), tbuff, len);
      tbuff [len] = L'\0';
    }
    return tbuff;
}

#endif

