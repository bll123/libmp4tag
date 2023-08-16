#ifndef INC_LIBMP4TAG_H
#define INC_LIBMP4TAG_H

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

/* versioning */

#define CPP_STR(x) #x
#define LIBMP4TAG_VERS_MAJOR 1
#define LIBMP4TAG_VERS_MINOR 0
#define LIBMP4TAG_VERS_REVISION 0
#define LIBMP4TAG_VERS_EXTRA 0
#define LIBMP4TAG_VERSION_STR(maj,min,rev,extra) \
   CPP_STR(maj) "." \
   CPP_STR(min) "." \
   CPP_STR(rev) "." \
   CPP_STR(extra)
#define LIBMP4TAG_VERSION \
   LIBMP4TAG_VERSION_STR(LIBMP4TAG_VERS_MAJOR,LIBMP4TAG_VERS_MINOR,LIBMP4TAG_VERS_REVISION,LIBMP4TAG_VERS_EXTRA)
#define LIBMP4TAG_API_VERS 1
#define LIBMP4TAG_API_VERS_STR(maj) CPP_STR(maj)
#define LIBMP4TAG_API_VERSION LIBMP4TAG_API_VERS_STR(LIBMP4TAG_API_VERS)

#endif /* INC_LIBMP4TAG_H */
