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

#define COPYRIGHT_STR   "\xc2\xa9"

typedef struct libmp4tag libmp4tag_t;
typedef struct libmp4tagpreserve libmp4tagpreserve_t;

/* libmp4tag.c */

typedef struct {
  const char  *name;
  const char  *data;
  size_t      datalen;
  bool        binary;
} mp4tagpub_t;

/* return values for the iterator */
enum {
  MP4TAG_OK,
  MP4TAG_ERROR,
  MP4TAG_FINISH,
};

enum {
  MP4TAG_NOTFOUND = -1,
  MP4TAG_ID_MAX = 255,
};

libmp4tag_t   * mp4tag_open (const char *fn);
void            mp4tag_close (libmp4tag_t *libmp4tag);
void            mp4tag_free (libmp4tag_t *libmp4tag);
void            mp4tag_parse (libmp4tag_t *libmp4tag);
int64_t         mp4tag_duration (libmp4tag_t *libmp4tag);
int             mp4tag_get_tag_by_name (libmp4tag_t *libmp4tag, const char *tag, mp4tagpub_t *mp4tagpub);
void            mp4tag_iterate_init (libmp4tag_t *libmp4tag);
int             mp4tag_iterate (libmp4tag_t *libmp4tag, mp4tagpub_t *mp4tagpub);
int             mp4tag_set_tag_str (libmp4tag_t *libmp4tag, const char *name, const char *data);
int             mp4tag_set_tag_binary (libmp4tag_t *libmp4tag, const char *name, const char *data, size_t sz);
int             mp4tag_delete_tag (libmp4tag_t *libmp4tag, const char *name);
int             mp4tag_write_tags (libmp4tag_t *libmp4tag);
int             mp4tag_clean_tags (libmp4tag_t *libmp4tag);
libmp4tagpreserve_t *mp4tag_preserve_tags (libmp4tag_t *libmp4tag);
int             mp4tag_restore_tags (libmp4tag_t *libmp4tag, libmp4tagpreserve_t *preserve);
int             mp4tag_preserve_free (libmp4tagpreserve_t *preserve);
const char    * mp4tag_version (void);

/* versioning */

/* Being a library, the major number will reflect the api version. */
/* The major value will (hopefully) stay at version 1. */
/* The minor value will be updated when major functionality is implemented */
/* or there are additions to the api. */
/* The revision value will change for bug fixes/cleanup/documentation. */

#define LIBMP4TAG_VERS_MAJOR 1
#define LIBMP4TAG_VERS_MINOR 0
#define LIBMP4TAG_VERS_REVISION 1
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
