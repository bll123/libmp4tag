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

FILE *
mp4tag_fopen (const char *fname, const char *mode)
{
  FILE          *fh = NULL;

#ifdef _WIN32
  {
    wchar_t       *tfname = NULL;
    wchar_t       *tmode = NULL;

    tfname = mp4tag_towide (fname);
    tmode = mp4tag_towide (mode);
    if (tfname != NULL && tmode != NULL) {
      fh = _wfopen (tfname, tmode);
      free (tfname);
      free (tmode);
    }
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

#if _lib__wstat64
  {
    struct __stat64  statbuf;
    wchar_t       *tfname = NULL;
    int           rc;

    tfname = mp4tag_towide (fname);
    if (tfname != NULL) {
      rc = _wstat64 (tfname, &statbuf);
      if (rc == 0) {
        sz = statbuf.st_size;
      }
      free (tfname);
    }
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
mp4tag_read_file (const char *fn, size_t *sz, int *mp4error)
{
  char    *fdata = NULL;
  int     rc = -1;
  FILE    *fh;
  ssize_t fsz;

  *mp4error = MP4TAG_OK;

  *sz = 0;
  fsz = mp4tag_file_size (fn);
  if (fsz < 0) {
    *mp4error = MP4TAG_ERR_FILE_NOT_FOUND;
    return NULL;
  }

  fdata = malloc (fsz);
  if (fdata == NULL) {
    *mp4error = MP4TAG_ERR_OUT_OF_MEMORY;
    return NULL;
  }

  fh = mp4tag_fopen (fn, "rb");
  if (fh == NULL) {
    *mp4error = MP4TAG_ERR_FILE_NOT_FOUND;
  } else {
    rc = fread (fdata, fsz, 1, fh);
    if (rc != 1) {
      *mp4error = MP4TAG_ERR_FILE_READ_ERROR;
    }
  }
  fclose (fh);

  if (rc != 1 && fdata != NULL) {
    free (fdata);
    fdata = NULL;
  }

  *sz = fsz;
  return fdata;
}

int
mp4tag_file_delete (const char *fname)
{
  int     rc = -1;
#if _lib__wunlink
  wchar_t *tname;
#endif

  if (fname == NULL) {
    return 0;
  }

#if _lib__wunlink
  tname = mp4tag_towide (fname);
  if (tname != NULL) {
    rc = _wunlink (tname);
    free (tname);
  }
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
    if (wfname != NULL && wnfn != NULL) {
      rc = _wrename (wfname, wnfn);
      free (wfname);
      free (wnfn);
    }
  }
#else
  rc = rename (fname, nfn);
#endif
  return rc;
}

#ifdef _WIN32

wchar_t *
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

char *
mp4tag_fromwide (const wchar_t *buff)
{
  char        *tbuff = NULL;
  size_t      len;

  /* the documentation lies; len does not include room for the null byte */
  len = WideCharToMultiByte (CP_UTF8, 0, buff, -1, NULL, 0, NULL, NULL);
  tbuff = malloc (len + 1);
  if (tbuff != NULL) {
    WideCharToMultiByte (CP_UTF8, 0, buff, -1, tbuff, len, NULL, NULL);
    tbuff [len] = '\0';
  }
  return tbuff;
}

#endif

