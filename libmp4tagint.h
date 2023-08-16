#ifndef INC_LIBMP4TAGINT_H
#define INC_LIBMP4TAGINT_H

#include "libmp4tag.h"

typedef struct libmp4tag {
  FILE      *fh;
} libmp4tag_t;

/* tagdef.c */

enum {
  MP4TAG_TYPE_STR,
  MP4TAG_TYPE_PIC,
  MP4TAG_TYPE_BOOL,
  MP4TAG_TYPE_OTHER,   // anything that isn't handled
};

typedef struct {
  const char  *nm;
  const char  *vorbisname;
  int         type;
} mp4tag_t;

extern const mp4tag_t mp4tags [];

/* parsemp4.c */

void parsemp4 (libmp4tag_t *libmp4tag);

#endif /* INC_LIBMP4TAGINT_H */
