/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */

#ifndef INC_LIBMP4TAG_H
#define INC_LIBMP4TAG_H

#include <stdbool.h>
#include <stdint.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#define PREFIX_STR   "\xc2\xa9"

typedef struct libmp4tag libmp4tag_t;
typedef struct libmp4tagpreserve libmp4tagpreserve_t;

/* libmp4tag.c */

typedef struct {
  const char  *tag;
  const char  *data;
  const char  *covername;
  size_t      datalen;
  int         coveridx;
  bool        binary;
} mp4tagpub_t;

/* possible errors */
enum {
  /* the iterator returns the first three values */
  /* other routines return MP4TAG_OK/MP4TAG_ERROR */
  MP4TAG_OK,
  MP4TAG_FINISH,
  MP4TAG_ERR_BAD_STRUCT,    // null structure
  MP4TAG_ERR_OUT_OF_MEMORY,
  MP4TAG_ERR_UNABLE_TO_OPEN,
  MP4TAG_ERR_NOT_MP4,
  MP4TAG_ERR_NOT_OPEN,
  MP4TAG_ERR_NULL_VALUE,
  MP4TAG_ERR_NO_TAGS,
  MP4TAG_ERR_MISMATCH,      // mismatch between string and binary
  MP4TAG_ERR_NOT_FOUND,
  MP4TAG_ERR_NOT_IMPLEMENTED,
  MP4TAG_ERR_FILE_SEEK_ERROR,
  MP4TAG_ERR_FILE_TELL_ERROR,
  MP4TAG_ERR_FILE_READ_ERROR,
  MP4TAG_ERR_FILE_WRITE_ERROR,
  MP4TAG_ERR_UNABLE_TO_PROCESS,
};

enum {
  MP4TAG_NOTFOUND = -1,
  MP4TAG_ID_MAX = 255,
};

libmp4tag_t * mp4tag_open (const char *fn, int *mp4error);
void      mp4tag_free (libmp4tag_t *libmp4tag);
int       mp4tag_parse (libmp4tag_t *libmp4tag);
int64_t   mp4tag_duration (libmp4tag_t *libmp4tag);
int       mp4tag_get_tag_by_name (libmp4tag_t *libmp4tag, const char *tag, mp4tagpub_t *mp4tagpub);
int       mp4tag_iterate_init (libmp4tag_t *libmp4tag);
int       mp4tag_iterate (libmp4tag_t *libmp4tag, mp4tagpub_t *mp4tagpub);
int       mp4tag_set_tag (libmp4tag_t *libmp4tag, const char *tag, const char *data, bool forcebinary);
int       mp4tag_delete_tag (libmp4tag_t *libmp4tag, const char *name);
int       mp4tag_write_tags (libmp4tag_t *libmp4tag);
int       mp4tag_clean_tags (libmp4tag_t *libmp4tag);
libmp4tagpreserve_t *mp4tag_preserve_tags (libmp4tag_t *libmp4tag);
int       mp4tag_restore_tags (libmp4tag_t *libmp4tag, libmp4tagpreserve_t *preserve);
int       mp4tag_preserve_free (libmp4tagpreserve_t *preserve);
int       mp4tag_error (libmp4tag_t *libmp4tag);
const char  * mp4tag_version (void);
const char  * mp4tag_error_str (libmp4tag_t *libmp4tag);
void      mp4tag_set_debug_flags (libmp4tag_t *libmp4tag, int dbgflags);
/* these routines are useful for the application */

/* mp4tagfileop.c */
/* public file interface helper routines */

FILE    * mp4tag_fopen (const char *fn, const char *mode);
ssize_t mp4tag_file_size (const char *fn);
char    * mp4tag_read_file (libmp4tag_t *libmp4tag, const char *fn, size_t *sz);
int     mp4tag_file_delete (const char *fname);
int     mp4tag_file_move (const char *fname, const char *nfn);

/* versioning */

/* Being a library, the major number will reflect the api version. */
/* The major value will (hopefully) stay at version 1. */
/* The minor value will be updated when major functionality is implemented */
/* or there are additions to the api. */
/* The revision value will change for bug fixes/cleanup/documentation. */

#define LIBMP4TAG_VERS_MAJOR 1
#define LIBMP4TAG_VERS_MINOR 0
#define LIBMP4TAG_VERS_REVISION 5
#define CPP_STR(x) #x
#define LIBMP4TAG_VERSION_STR(maj,min,rev) \
   CPP_STR(maj) "." \
   CPP_STR(min) "." \
   CPP_STR(rev)
#define LIBMP4TAG_VERSION \
   LIBMP4TAG_VERSION_STR(LIBMP4TAG_VERS_MAJOR,LIBMP4TAG_VERS_MINOR,LIBMP4TAG_VERS_REVISION)
#define LIBMP4TAG_RELEASE_STATE "alpha"

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_LIBMP4TAG_H */
