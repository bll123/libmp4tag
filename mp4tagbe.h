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
# define be32toh ntohl
# define be16toh ntohs
# define be64toh ntohll
# define htobe32 htonl
# define htobe16 htons
# define htobe64 htonll
#endif

