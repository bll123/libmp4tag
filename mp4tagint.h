/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */

#ifndef INC_MP4TAGINT_H
#define INC_MP4TAGINT_H

#include "config.h"

#include <stdint.h>

#include "libmp4tag.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#define LIBMP4TAG_DEBUG 1

enum {
  /* various idents that libmp4tag needs to descend into or use */
  MP4TAG_CO64,
  MP4TAG_FREE,
  MP4TAG_FTYP,
  MP4TAG_HDLR,
  MP4TAG_ILST,
  MP4TAG_MDHD,
  MP4TAG_MDIA,
  MP4TAG_META,
  MP4TAG_MINF,
  MP4TAG_MOOV,
  MP4TAG_STBL,
  MP4TAG_STCO,
  MP4TAG_TRAK,
  MP4TAG_UDTA,
  /* tags that have special handling */
  MP4TAG_COVR,
  MP4TAG_CUSTOM,
  MP4TAG_DISK,
  MP4TAG_GEN,
  MP4TAG_GNRE,
  MP4TAG_TRKN,
  /* internal mp4 values used by tag idents */
  MP4TAG_DATA,
  MP4TAG_MEAN,
  MP4TAG_NAME,
};

enum {
  MP4TAG_NOTFOUND = -1,     // returned by find-tag
  MP4TAG_ID_LEN = 4,
  MP4TAG_PREFIX_CHAR = 0xa9,
  /* this length has to be at least 2 + 3 + 1 bytes */
  /* room enough to hold the copyright symbol, three more chars and eol */
  MP4TAG_ID_DISP_LEN = 6,
  MP4TAG_STRING = 0,
  /* internal MP4 ID values */
  MP4TAG_ID_BOOL = 0x15,
  MP4TAG_ID_STRING = 0x01,
  MP4TAG_ID_DATA = 0x00,
  MP4TAG_ID_NUM = 0x15,
  MP4TAG_ID_JPG = 0x0d,
  MP4TAG_ID_PNG = 0x0e,
  /* internal MP4 sizes */
  MP4TAG_BOXHEAD_SZ = sizeof (uint32_t) + MP4TAG_ID_LEN,
  /* data-len + ident + flags + reserved */
  MP4TAG_DATA_SZ = MP4TAG_ID_LEN + sizeof (uint32_t) * 3,
  MP4TAG_HDLR_SZ = MP4TAG_BOXHEAD_SZ + sizeof (uint32_t) * 6 + sizeof (uint8_t),
  MP4TAG_META_SZ = MP4TAG_BOXHEAD_SZ + sizeof (uint32_t),
  /* priorities */
  MP4TAG_PRI_CUSTOM = 8,
  MP4TAG_PRI_NOWRITE = -1,
  MP4TAG_PRI_MAX = 20,
  MP4TAG_LEVEL_MAX = 15,
  /* the copy size seems to make little difference in speed */
  MP4TAG_COPY_SIZE = 5 * 1024 * 1024,       // 5 mibibytes
  MP4TAG_FREE_SPACE_SZ = 2048,
  MP4TAG_NO_FILESZ = -3,
  MP4TAG_READ_OK = 1,
  MP4TAG_READ_NONE = 0,
  MP4TAG_SLEEP_TIME = 2,
  TEMP_NM_SZ = 300,
};

enum {
  MP4TAG_DBG_NONE                   = 0,
  /* only prints file structure that is relevant to libmp4tag */
  MP4TAG_DBG_PRINT_FILE_STRUCTURE   = (1 << 0),
  MP4TAG_DBG_WRITE                  = (1 << 1),
  MP4TAG_DBG_DUMP_CO                = (1 << 2),
  MP4TAG_DBG_OTHER                  = (1 << 3),
  MP4TAG_DBG_BUG                    = (1 << 4),
};

enum {
  MP4TAG_IDENT = 0x6c69626d70347467,
};

/* any changes to this structure must be reflected in mp4tag_clone_tag() */
typedef struct mp4tag {
  char      *tag;
  char      *data;
  char      *covername;
  uint32_t  datalen;
  int       dataidx;
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
  int64_t         libmp4tagident;
  FILE            *fh;
  char            *fn;
  mp4tag_readcb_t readcb;
  mp4tag_seekcb_t seekcb;
  void            *userdata;
  mp4tag_t        *tags;
  size_t          filesz;
  size_t          offset;
  int64_t         creationdate;
  int64_t         modifieddate;
  int64_t         duration;
  int32_t         samplerate;
  uint32_t        timeout;
  /* used by the parser and writer */
  uint32_t        base_lengths [MP4TAG_LEVEL_MAX];
  ssize_t         base_offsets [MP4TAG_LEVEL_MAX];
  int64_t         rem_length [MP4TAG_LEVEL_MAX];
  uint64_t        ilst_remaining;     /* for 1.3.0 bug */
  /* for debugging, otherwise not needed */
  char            base_name [MP4TAG_LEVEL_MAX][MP4TAG_ID_DISP_LEN + 1];
  uint32_t        taglist_orig_data_len;    /* for debugging */
  int             base_offset_count;
  ssize_t         taglist_base_offset;
  ssize_t         taglist_offset;
  uint32_t        taglist_orig_len;
  uint32_t        taglist_len;
  uint32_t        interior_free_len;
  uint32_t        exterior_free_len;
  int             parentidx;
  ssize_t         noilst_offset;
  ssize_t         after_ilst_offset;
  uint32_t        insert_delta;
  ssize_t         stco_offset;
  uint32_t        stco_len;
  ssize_t         co64_offset;
  uint32_t        co64_len;
  /* datacount is a temporary variable used by both add-tag */
  /* and the write process */
  int             datacount;
  /* temporary variable used by the write process */
  char            lastbox_nm [TEMP_NM_SZ];
  int32_t         lastbox_offset;
  /* tag list */
  int             tagcount;
  int             tagalloccount;
  int             iterator;
  int             mp4error;
  int             dbgflags;
  int             options;
  bool            mp7meta;
  bool            unlimited;
  bool            parsed;
  /* used by the parser */
  bool            processdata;
  bool            checkforfree;
  bool            parsedone;
  /* used by the parser to track 1.3.0 bug */
  bool            ilstremain;
  bool            ilstend;
  bool            ilstdone;
  bool            freeneg;
  bool            udtazero;
  bool            dofix;
  /* streams */
  bool            isstream;
  bool            canwrite;
} libmp4tag_t;

/* mp4const.c */

typedef struct {
  int         priority;
  const char  *tag;
  int         identtype;
  int         len;
} mp4tagdef_t;

extern const char *boxids [];
extern const mp4tagdef_t mp4taglist [];
extern const int mp4taglistlen;
extern const char *mp4tagoldgenrelist [];
extern const int mp4tagoldgenrelistsz;

/* mp4tagparse.c */

int  mp4tag_parse_file (libmp4tag_t *libmp4tag, uint32_t boxlen, int level);
int  mp4tag_parse_ftyp (libmp4tag_t *libmp4tag);

/* mp4tagwrite.c */

char  * mp4tag_build_data (libmp4tag_t *libmp4tag, uint32_t *dlen);
int   mp4tag_write_data (libmp4tag_t *libmp4tag, const char *data, uint32_t datalen);


/* mp4writeutil.c */
void mp4tag_update_parent_lengths (libmp4tag_t *libmp4tag, FILE *ofh, int32_t delta);

/* mp4tagutil.c */

extern const char *MP4TAG_INPUT_DELIM;
void mp4tag_sort_tags (libmp4tag_t *libmp4tag);
int  mp4tag_find_tag (libmp4tag_t *libmp4tag, const char *tag, int dataidx);
int  mp4tag_parse_tagname (char *tag, int *dataidx);
mp4tagdef_t *mp4tag_check_tag (const char *tag);
int  mp4tag_compare (const void *a, const void *b);
int  mp4tag_compare_list (const void *a, const void *b);
int  mp4tag_add_tag (libmp4tag_t *libmp4tag, const char *tag, const char *data, ssize_t sz, uint32_t origflag, size_t origlen, const char *covername);
int  mp4tag_set_tag_string (libmp4tag_t *libmp4tag, const char *name, int idx, const char *data);
int  mp4tag_set_tag_binary (libmp4tag_t *libmp4tag, const char *name, int idx, const char *data, size_t sz, const char *fn);
void mp4tag_del_tag (libmp4tag_t *libmp4tag, int idx);
void mp4tag_free_tag_by_idx (libmp4tag_t *libmp4tag, int idx);
void mp4tag_free_tag (mp4tag_t *mp4tag);
void mp4tag_clone_tag (libmp4tag_t *libmp4tag, mp4tag_t *target, mp4tag_t *source);
void mp4tag_sleep (uint32_t ms);
bool mp4tag_chk_dbg (libmp4tag_t *libmp4tag, int dbg);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_MP4TAGINT_H */
