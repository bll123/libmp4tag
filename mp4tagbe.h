/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_MP4TAGBE_H
#define INC_MP4TAGBE_H

/* endianess */

#if _hdr_endian
# include <endian.h>
#endif
#if ! _hdr_endian && _hdr_arpa_inet
# include <arpa/inet.h>
# define be32toh ntohl
# define be16toh ntohs
# define be64toh ntohll
# define htobe32 htonl
# define htobe16 htons
# define htobe64 htonll
#endif
#if ! _hdr_endian && _hdr_winsock2
# include <winsock2.h>
# define be32toh ntohl
# define be16toh ntohs
/* it appears that the msys2 winsock2 header file */
/* does not define ntohll or htonll */
/* but this will get all mucked up if ntohll is actually defined */
static inline uint64_t
ntohll (uint64_t v)
{
  return (((uint64_t) ntohl((uint32_t) (v & 0xFFFFFFFFUL))) << 32) |
      (uint64_t) ntohl ((uint32_t) (v >> 32));
}
# define be64toh ntohll
# define htobe32 htonl
# define htobe16 htons
static inline uint64_t
htonll (uint64_t v)
{
  return (((uint64_t) htonl((uint32_t) (v & 0xFFFFFFFFUL))) << 32) |
      (uint64_t) htonl ((uint32_t) (v >> 32));
}
# define htobe64 htonll
#endif

#endif /* INC_MP4TAGBE_H */
