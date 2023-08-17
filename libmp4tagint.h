/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */

#ifndef INC_LIBMP4TAGINT_H
#define INC_LIBMP4TAGINT_H

#include <stdint.h>

#include "libmp4tag.h"

enum {
  MP4TAG_STRING = 0,
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
  const char  *nm;
} mp4tagdef_t;

extern const mp4tagdef_t mp4tags [];

/* mp4tagparse.c */

void mp4tag_parse_file (libmp4tag_t *libmp4tag);

/* mp4tagutil.c */

void mp4tag_sort_tags (libmp4tag_t *libmp4tag);
int  mp4tag_find_tag (libmp4tag_t *libmp4tag, const char *nm);
int  mp4tag_compare (const void *a, const void *b);
void mp4tag_add_tag (libmp4tag_t *libmp4tag, const char *nm, const char *data, ssize_t sz, uint32_t origflag, size_t origlen);

#endif /* INC_LIBMP4TAGINT_H */
