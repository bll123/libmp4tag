/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */

#ifndef INC_LIBMP4TAGINT_H
#define INC_LIBMP4TAGINT_H

#include <stdint.h>

#include "libmp4tag.h"

enum {
  MP4TAG_STRING = 0,
  MP4TAG_ID_BOOL = 0x15,
  MP4TAG_ID_STRING = 0x01,
  MP4TAG_ID_DATA = 0x00,
  MP4TAG_ID_NUM = 0x15,
  MP4TAG_ID_JPG = 0x0d,
  MP4TAG_ID_PNG = 0x0e,
};

typedef struct mp4tag {
  char    *name;
  char    *data;
  size_t  datalen;
  int     idx;
  /* internalflags is the flag value from the original data */
  int     internalflags;
  /* internallen is the length of the original data */
  int     internallen;
  bool    binary;
} mp4tag_t;

typedef struct libmp4tag {
  char      *fn;
  FILE      *fh;
  mp4tag_t  *tags;
  int64_t   creationdate;
  int64_t   modifieddate;
  int64_t   duration;
  int32_t   samplerate;
  off_t     taglist_begin;
  size_t    taglist_len;
  off_t     free_begin;
  size_t    free_len;
  int       tagcount;
  int       tagalloccount;
  int       iterator;
} libmp4tag_t;

/* tagdef.c */

typedef struct {
  const char  *name;
  int         identtype;
  int         len;
} mp4tagdef_t;

extern const mp4tagdef_t mp4taglist [];
extern const int mp4taglistlen;

/* mp4tagparse.c */

void mp4tag_parse_file (libmp4tag_t *libmp4tag);

/* mp4tagutil.c */

void mp4tag_sort_tags (libmp4tag_t *libmp4tag);
int  mp4tag_find_tag (libmp4tag_t *libmp4tag, const char *tag);
bool mp4tag_check_tag (libmp4tag_t *libmp4tag, const char *tag);
int  mp4tag_compare (const void *a, const void *b);
int  mp4tag_compare_list (const void *a, const void *b);
void mp4tag_add_tag (libmp4tag_t *libmp4tag, const char *nm, const char *data, ssize_t sz, uint32_t origflag, size_t origlen);
void mp4tag_del_tag (libmp4tag_t *libmp4tag, int idx);
void mp4tag_free_tag_by_idx (libmp4tag_t *libmp4tag, int idx);

#endif /* INC_LIBMP4TAGINT_H */
