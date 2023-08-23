/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */

#ifndef INC_MP4TAGINT_H
#define INC_MP4TAGINT_H

#include "config.h"

#include <stdint.h>

#include "libmp4tag.h"

/* idents that libmp4tag needs to descend into or use */
#define MP4TAG_CO64   "co64"
#define MP4TAG_FREE   "free"
#define MP4TAG_FTYP   "ftyp"
#define MP4TAG_HDLR   "hdlr"
#define MP4TAG_ILST   "ilst"
#define MP4TAG_MDHD   "mdhd"
#define MP4TAG_MDIA   "mdia"
#define MP4TAG_META   "meta"
#define MP4TAG_MINF   "minf"
#define MP4TAG_MOOV   "moov"
#define MP4TAG_STBL   "stbl"
#define MP4TAG_STCO   "stco"
#define MP4TAG_TRAK   "trak"
#define MP4TAG_UDTA   "udta"
/* tags */
#define MP4TAG_COVR   "covr"
#define MP4TAG_CUSTOM "----"
#define MP4TAG_DISK   "disk"
#define MP4TAG_GEN    "gen"
#define MP4TAG_GNRE   "gnre"
#define MP4TAG_TRKN   "trkn"
/* used by tag idents */
#define MP4TAG_DATA   "data"
#define MP4TAG_MEAN   "mean"
#define MP4TAG_NAME   "name"

#define MP4TAG_CUSTOM_DELIM ":"
#define MP4TAG_COVER_DELIM ":"

enum {
  MP4TAG_ID_LEN = 4,
  MP4TAG_STRING = 0,
  MP4TAG_ID_BOOL = 0x15,
  MP4TAG_ID_STRING = 0x01,
  MP4TAG_ID_DATA = 0x00,
  MP4TAG_ID_NUM = 0x15,
  MP4TAG_ID_JPG = 0x0d,
  MP4TAG_ID_PNG = 0x0e,
  MP4TAG_PRI_CUSTOM = 8,
  MP4TAG_PRI_NOWRITE = -1,
  MP4TAG_PRI_MAX = 20,
  MP4TAG_BASE_OFF_MAX = 10,
  MP4TAG_BOXHEAD_SZ = sizeof (uint32_t) + MP4TAG_ID_LEN,
  /* data-len + ident + flags + reserved */
  MP4TAG_DATA_SZ = MP4TAG_ID_LEN + sizeof (uint32_t) * 3,
  MP4TAG_HDLR_SZ = MP4TAG_BOXHEAD_SZ + sizeof (uint32_t) * 6 + sizeof (uint8_t),
  MP4TAG_META_SZ = MP4TAG_BOXHEAD_SZ + sizeof (uint32_t),
  MP4TAG_COPY_SIZE = 5 * 1024 * 1024,
  MP4TAG_FREE_SPACE_SZ = 256,
};

typedef struct mp4tag {
  char      *tag;
  char      *data;
  char      *covername;
  uint32_t  datalen;
  int       coveridx;
  int       idx;
  /* identtype is the flag value from the original data */
  int       identtype;
  /* internallen is the length of the original data */
  int       internallen;
  /* priority is used to order the tags for writing */
  int       priority;
  bool      binary;
} mp4tag_t;

typedef struct libmp4tag {
  char      *fn;
  size_t    filesz;
  FILE      *fh;
  mp4tag_t  *tags;
  int64_t   creationdate;
  int64_t   modifieddate;
  int64_t   duration;
  int32_t   samplerate;
  /* used by the parser and writer */
  uint32_t  base_lengths [MP4TAG_BASE_OFF_MAX];
  ssize_t   base_offsets [MP4TAG_BASE_OFF_MAX];
  /* for debugging, otherwise not needed */
  char      base_name [MP4TAG_BASE_OFF_MAX][MP4TAG_ID_LEN + 1];
  int       base_offset_count;
  ssize_t   taglist_base_offset;
  ssize_t   taglist_offset;
  uint32_t  taglist_len;
  int       parentidx;
  ssize_t   udta_offset;
  int       covercount;
  /* coverstart is a temporary variable used by the write process */
  int32_t   coverstart_offset;
  /* tag list */
  int       tagcount;
  int       tagalloccount;
  int       iterator;
  int       mp4error;
  bool      mp7meta : 1;
  bool      unlimited : 1;
} libmp4tag_t;

/* mp4const.c */

typedef struct {
  int         priority;
  const char  *tag;
  int         identtype;
  int         len;
} mp4tagdef_t;

extern const mp4tagdef_t mp4taglist [];
extern const int mp4taglistlen;
extern const char *mp4tagoldgenrelist [];
extern const int mp4tagoldgenrelistsz;

/* mp4tagparse.c */

int  mp4tag_parse_file (libmp4tag_t *libmp4tag);
int  mp4tag_parse_ftyp (libmp4tag_t *libmp4tag);

/* mp4tagwrite.c */

char  * mp4tag_build_data (libmp4tag_t *libmp4tag, uint32_t *dlen);
int   mp4tag_write_data (libmp4tag_t *libmp4tag, const char *data, uint32_t datalen);

/* mp4tagutil.c */

void mp4tag_sort_tags (libmp4tag_t *libmp4tag);
int  mp4tag_find_tag (libmp4tag_t *libmp4tag, const char *tag);
int  mp4tag_parse_cover_tag (const char *tag, int *coveridx);
mp4tagdef_t *mp4tag_check_tag (const char *tag);
int  mp4tag_compare (const void *a, const void *b);
int  mp4tag_compare_list (const void *a, const void *b);
void mp4tag_add_tag (libmp4tag_t *libmp4tag, const char *tag, const char *data, ssize_t sz, uint32_t origflag, size_t origlen, const char *covername);
int  mp4tag_set_tag_str (libmp4tag_t *libmp4tag, const char *name, int idx, const char *data);
int  mp4tag_set_tag_binary (libmp4tag_t *libmp4tag, const char *name, int idx, const char *data, size_t sz, const char *fn);
void mp4tag_del_tag (libmp4tag_t *libmp4tag, int idx);
void mp4tag_free_tag_by_idx (libmp4tag_t *libmp4tag, int idx);

#endif /* INC_MP4TAGINT_H */
