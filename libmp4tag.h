#ifndef INC_LIBMP4TAG_H
#define INC_LIBMP4TAG_H

typedef struct libmp4tag libmp4tag_t;

/* libmp4tag.c */

libmp4tag_t * mp4tag_open (const char *fn);
void          mp4tag_close (libmp4tag_t *libmp4tag);
void          mp4tag_free (libmp4tag_t *libmp4tag);
void          mp4tag_parse (libmp4tag_t *libmp4tag);
const char  * mp4tag_version (void);
const char  * mp4tag_api_version (void);

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
