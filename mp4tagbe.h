/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

/* endianess */

#if __has_include (<endian.h>)
# include <endian.h>
#endif
#if ! __has_include (<endian.h>) && __has_include (<arpa/inet.h>)
# include <arpa/inet.h>
# define be32toh ntohl
# define be16toh ntohs
# define be64toh ntohll
# define htobe32 htonl
# define htobe16 htons
# define htobe64 htonll
#endif
#if ! __has_include (<endian.h>) && __has_include (<winsock2.h>)
# include <winsock2.h>

# if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
# endif

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

# if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
# endif

#endif

