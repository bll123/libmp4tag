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

/**
 * Public tag data.
 * Returned by mp4tag_get_tag_by_name() and by mp4tag_iterate()
 */
typedef struct {
  const char  *tag;         /** The name of the tag. */
  const char  *data;        /** The value of the tag. */
  const char  *covername;   /** For 'covr' tags, the cover name, or NULL */
  size_t      datalen;      /** The length of the tag value */
  int         coveridx;     /** Which 'covr'. */
  bool        binary;       /** If true, the tag value is binary data */
} mp4tagpub_t;

/**
 * Possible error returns.
 */
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
};

enum {
  MP4TAG_ID_MAX = 255,
};

/**
 * Open an MP4 file.
 * The file is checked for a valid 'ftyp' header.
 * The MP4 file is not yet parsed.
 * @param[in] fn The filename to process.
 * @return libmp4tag_t * Pointer to libmp4tag structure or NULL on error
 */
libmp4tag_t * mp4tag_open (const char *fn, int *mp4error);

/**
 * Frees a libmp4tag_t * structure.
 * Any open files are closed and all associated data is freed.
 * @param[in] libmp4tag Pointer to libmp4tag structure.
 */
void      mp4tag_free (libmp4tag_t *libmp4tag);

/**
 * Parses the open file.
 * @param[in] libmp4tag Pointer to libmp4tag structure.
 * @return MP4TAG_OK or error return.
 */
int       mp4tag_parse (libmp4tag_t *libmp4tag);

/**
 * Returns the duration in millseconds.
 * @param[in] libmp4tag Pointer to libmp4tag structure.
 * @return duration in milliseconds or 0 on error.
 */
int64_t   mp4tag_duration (libmp4tag_t *libmp4tag);

/**
 * Get the data for a particular tag.
 * @param[in] libmp4tag Pointer to libmp4tag structure.
 * @param[in] tag The tag name to process.
 * @param[out] mp4tagpub Pointer to public mp4tagpub_t structure to be filled in.
 * @return MP4TAG_OK or error return.
 */
int       mp4tag_get_tag_by_name (libmp4tag_t *libmp4tag, const char *tag, mp4tagpub_t *mp4tagpub);

/**
 * Initialize a tag iterator.
 * @param[in] libmp4tag Pointer to libmp4tag structure.
 * @return MP4TAG_OK or error return.
 */
int       mp4tag_iterate_init (libmp4tag_t *libmp4tag);

/**
 * Iterate through the tags.
 * @param[in] libmp4tag Pointer to libmp4tag structure.
 * @param[out] mp4tagpub Pointer to public mp4tagpub_t structure to be filled in.
 * @return MP4TAG_OK, MP4TAG_FINISH or error return.
 */
int       mp4tag_iterate (libmp4tag_t *libmp4tag, mp4tagpub_t *mp4tagpub);

/**
 * Set a tag to a value.
 * @param[in] libmp4tag Pointer to libmp4tag structure.
 * @param[in] tag The name of the tag to set.
 * @param[in] data The value that the tag will be set to, or in the
 *    case of binary data, the filename to read the data from.
 * @param[in] forcebinary Set to true for unknown tags that are being set to binary data.
 * @return MP4TAG_OK or error return.
 */
int       mp4tag_set_tag (libmp4tag_t *libmp4tag, const char *tag, const char *data, bool forcebinary);

/**
 * Delete a tag.
 * @param[in] libmp4tag Pointer to libmp4tag structure.
 * @param[in] tag The name of the tag to delete.
 * @return MP4TAG_OK or error return.
 */
int       mp4tag_delete_tag (libmp4tag_t *libmp4tag, const char *name);

/**
 * Write the tags out to the MP4 file.
 * @param[in] libmp4tag Pointer to libmp4tag structure.
 * @return MP4TAG_OK or error return.
 */
int       mp4tag_write_tags (libmp4tag_t *libmp4tag);

/**
 * Remove all tags.
 * mp4tag_write_tags() must still be called.
 * @param[in] libmp4tag Pointer to libmp4tag structure.
 * @return MP4TAG_OK or error return.
 */
int       mp4tag_clean_tags (libmp4tag_t *libmp4tag);

/**
 * Preserve the tags in the audio file.
 * @param[in] libmp4tag Pointer to libmp4tag structure.
 * @return libmp4tagpreserve_t * or NULL on error.
 */
libmp4tagpreserve_t *mp4tag_preserve_tags (libmp4tag_t *libmp4tag);

/**
 * Restore the tags to the audio file.
 * The mp4tag_write_tags() routine must be called afterwards.
 * Not yet implemented.
 * @param[in] libmp4tag Pointer to libmp4tag structure.
 * @param[in] preserve Pointer to libmp4tagpreserve_t structure returned
 *    from a mp4tag_preserve_tags call.
 * @return MP4TAG_OK or error return.
 */
int       mp4tag_restore_tags (libmp4tag_t *libmp4tag, libmp4tagpreserve_t *preserve);

/**
 * Free a libmp4tagpreserve_t structure returned by mp4tag_preserve_tags().
 * @param[in] preserve Pointer to libmp4tagpreserve_t structure.
 * @return MP4TAG_OK or error return.
 */
int       mp4tag_preserve_free (libmp4tagpreserve_t *preserve);

/**
 * Return the last error code.
 * @param[in] libmp4tag Pointer to libmp4tag structure.
 * @return last error encountered.
 */
int       mp4tag_error (libmp4tag_t *libmp4tag);

/**
 * Get the libmp4tag version string.
 * @return libmp4tag version string.
 */
const char  * mp4tag_version (void);

/**
 * Return a brief readable error string corresponding to the last
 * error.  This routine is intended more for debugging purposes.
 * @return error string.
 */
const char  * mp4tag_error_str (libmp4tag_t *libmp4tag);

/**
 * @param[in] libmp4tag Pointer to libmp4tag structure.
 * Set the debug flags.
 */
void      mp4tag_set_debug_flags (libmp4tag_t *libmp4tag, int dbgflags);

/* these routines are useful for the application */

/* mp4tagfileop.c */
/* public file interface helper routines */

/**
 * Open a file.
 * Works for both posix and windows.
 * @param[in] fn The file to open.
 * @param[in] mode Mode to use for opening the file.
 * @return FILE * pointer or NULL on error.
 */
FILE    * mp4tag_fopen (const char *fn, const char *mode);

/**
 * Return a file's size.
 * Works for both posix and windows.
 * @param[in] fn The file to check.
 * @return Size of the file or -1 on error.
 */
ssize_t mp4tag_file_size (const char *fn);

/**
 * Read all data from a file and return the data.
 * Works for both posix and windows.
 * @param[in] libmp4tag Pointer to libmp4tag_t structure.  Use to track errors.
 * @param[in] fn The file to read.
 * @param[out] sz The file to read.
 * @return allocated value or NULL on error
 */
char    * mp4tag_read_file (libmp4tag_t *libmp4tag, const char *fn, size_t *sz);

/**
 * Delete a file.
 * Works for both posix and windows.
 * @param[in] fn The file to delete.
 * @return 0 on success.
 */
int     mp4tag_file_delete (const char *fname);

/**
 * Move a file from one name to another.
 * Works for both posix and windows.
 * @param[in] fname The old filename.
 * @param[in] nfn The new filename.
 * @return 0 on success.
 */
int     mp4tag_file_move (const char *fname, const char *nfn);

/* versioning */

/* Being a library, the major number will reflect the api version. */
/* The major value will (hopefully) stay at version 1. */
/* The minor value will be updated when major functionality is implemented */
/* or there are additions to the api. */
/* The revision value will change for bug fixes/cleanup/documentation. */

#define LIBMP4TAG_VERS_MAJOR 1
#define LIBMP4TAG_VERS_MINOR 1
#define LIBMP4TAG_VERS_REVISION 1
#define CPP_STR(x) #x
#define LIBMP4TAG_VERSION_STR(maj,min,rev) \
   CPP_STR(maj) "." \
   CPP_STR(min) "." \
   CPP_STR(rev)
#define LIBMP4TAG_VERSION \
   LIBMP4TAG_VERSION_STR(LIBMP4TAG_VERS_MAJOR,LIBMP4TAG_VERS_MINOR,LIBMP4TAG_VERS_REVISION)
#define LIBMP4TAG_RELEASE_STATE "beta"

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_LIBMP4TAG_H */
