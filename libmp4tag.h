/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */

#ifndef INC_LIBMP4TAG_H
#define INC_LIBMP4TAG_H

#include <stdbool.h>
#include <stdint.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct libmp4tag libmp4tag_t;
typedef struct libmp4tagpreserve libmp4tagpreserve_t;
typedef size_t (*mp4tag_readcb_t)(char *buff, size_t sz, size_t nmemb, void *udata);
typedef int (*mp4tag_seekcb_t)(size_t offset, void *udata);

/* libmp4tag.c */

typedef struct {
  const char  *tag;
  const char  *data;
  const char  *language;
  const char  *covername;
  size_t      datalen;
  int         dataidx;
  int         covertype;
  bool        binary;
} mp4tagpub_t;

/* iTunes 'stik' media types */
enum {
  MP4TAG_MEDIA_TYPE_MOVIE_OLD = 0,
  MP4TAG_MEDIA_TYPE_MUSIC = 1,
  MP4TAG_MEDIA_TYPE_AUDIOBOOK = 2,
  MP4TAG_MEDIA_TYPE_WHACKED_BOOKMARK = 5,
  MP4TAG_MEDIA_TYPE_MUSIC_VIDEO = 6,
  MP4TAG_MEDIA_TYPE_MOVIE = 9,
  MP4TAG_MEDIA_TYPE_TV_SHOW = 10,
  MP4TAG_MEDIA_TYPE_BOOKLET = 11,
  MP4TAG_MEDIA_TYPE_RINGTONE = 14,
  MP4TAG_MEDIA_TYPE_PODCAST = 21,
  MP4TAG_MEDIA_TYPE_ITUNES_U = 23,
};

/* libmp4tag error codes */
enum {
  /* the iterator returns the first three values */
  /* other routines return MP4TAG_OK/MP4TAG_ERR_... */
  MP4TAG_OK,
  MP4TAG_FINISH,
  MP4TAG_ERR_BAD_STRUCT,    // null structure or already freed
  MP4TAG_ERR_OUT_OF_MEMORY,
  MP4TAG_ERR_NOT_MP4,
  MP4TAG_ERR_NOT_OPEN,
  MP4TAG_ERR_NOT_PARSED,
  MP4TAG_ERR_NULL_VALUE,
  MP4TAG_ERR_NO_TAGS,
  MP4TAG_ERR_MISMATCH,      // mismatch between string and binary
  MP4TAG_ERR_TAG_NOT_FOUND,
  MP4TAG_ERR_NOT_IMPLEMENTED,
  MP4TAG_ERR_FILE_NOT_FOUND,
  MP4TAG_ERR_FILE_SEEK_ERROR,
  MP4TAG_ERR_FILE_TELL_ERROR,
  MP4TAG_ERR_FILE_READ_ERROR,
  MP4TAG_ERR_FILE_WRITE_ERROR,
  MP4TAG_ERR_UNABLE_TO_PROCESS,
  MP4TAG_ERR_NO_CALLBACK,
  MP4TAG_ERR_CANNOT_WRITE,  // stream or read-only file
};

enum {
  MP4TAG_OPTION_NONE          = 0,
  MP4TAG_OPTION_KEEP_BACKUP   = (1 << 0),
};

enum {
  MP4TAG_ID_MAX = 255,
  /* iTunes internal JPG and PNG codes */
  MP4TAG_COVER_JPG = 0x0d,
  MP4TAG_COVER_PNG = 0x0e,
};

libmp4tag_t * mp4tag_open (const char *fn, int *mp4error);
libmp4tag_t * mp4tag_openstream (mp4tag_readcb_t readcb, mp4tag_seekcb_t seekcb, void *userdata, uint32_t timeout, int *mp4error);
int       mp4tag_parse (libmp4tag_t *libmp4tag);
void      mp4tag_free (libmp4tag_t *libmp4tag);

int64_t   mp4tag_duration (libmp4tag_t *libmp4tag);
int       mp4tag_get_tag_by_name (libmp4tag_t *libmp4tag, const char *tag, mp4tagpub_t *mp4tagpub);
int       mp4tag_iterate_init (libmp4tag_t *libmp4tag);
int       mp4tag_iterate (libmp4tag_t *libmp4tag, mp4tagpub_t *mp4tagpub);

int       mp4tag_set_tag (libmp4tag_t *libmp4tag, const char *tag, const char *data, bool forcebinary);
int       mp4tag_set_binary_tag (libmp4tag_t *libmp4tag, const char *tag, const char *data, size_t datalen);
int       mp4tag_delete_tag (libmp4tag_t *libmp4tag, const char *tag);
int       mp4tag_clean_tags (libmp4tag_t *libmp4tag);

int       mp4tag_write_tags (libmp4tag_t *libmp4tag);

libmp4tagpreserve_t *mp4tag_preserve_tags (libmp4tag_t *libmp4tag);
int       mp4tag_restore_tags (libmp4tag_t *libmp4tag, libmp4tagpreserve_t *preserve);
int       mp4tag_preserve_free (libmp4tagpreserve_t *preserve);

int   mp4tag_error (libmp4tag_t *libmp4tag);
const char * mp4tag_version (void);
const char * mp4tag_error_str (libmp4tag_t *libmp4tag);
void  mp4tag_set_debug_flags (libmp4tag_t *libmp4tag, int dbgflags);
void  mp4tag_set_free_space (libmp4tag_t *libmp4tag, int32_t freespacesz);
void  mp4tag_set_option (libmp4tag_t *libmp4tag, int option);

/* mp4const.c */

extern const char *COPYRIGHT_STR;

/* mp4tagfileop.c */
/* public file interface helper routines */
/* these routines are useful for the application */

FILE    * mp4tag_fopen (const char *fn, const char *mode);
ssize_t mp4tag_file_size (const char *fn);
char    * mp4tag_read_file (const char *fn, size_t *sz, int *mp4error);
int     mp4tag_file_delete (const char *fname);
int     mp4tag_file_move (const char *fname, const char *nfn);
void    mp4tag_copy_file_times (FILE *ifh, FILE *ofh);
#ifdef _WIN32
wchar_t * mp4tag_towide (const char *buff);
char    * mp4tag_fromwide (const wchar_t *buff);
#endif

/* versioning */

/* Being a library, the major number will reflect the api version. */
/* The major value will change when the API or ABI changes */
/* The minor value will be updated when major functionality is implemented */
/* or there are additions to the api. */
/* The revision value will change for bug fixes/cleanup/documentation. */

#define LIBMP4TAG_VERS_MAJOR 2
#define LIBMP4TAG_VERS_MINOR 0
#define LIBMP4TAG_VERS_REVISION 1
#define LIBMP4TAG_RELEASE_STATE "production"
#define CPP_STR(x) #x
#define LIBMP4TAG_VERSION_STR(maj,min,rev) \
   CPP_STR(maj) "." \
   CPP_STR(min) "." \
   CPP_STR(rev)
#define LIBMP4TAG_VERSION \
   LIBMP4TAG_VERSION_STR(LIBMP4TAG_VERS_MAJOR,LIBMP4TAG_VERS_MINOR,LIBMP4TAG_VERS_REVISION)

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_LIBMP4TAG_H */
