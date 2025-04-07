#pragma once

// Windows toolchain is set up to 32bit x86
// which is little endian

#define htobe16(x) htons(x)
#define htole16(x) (x)
#define be16toh(x) ntohs(x)
#define le16toh(x) (x)

#define htobe32(x) htonl(x)
#define htole32(x) (x)
#define be32toh(x) ntohl(x)
#define le32toh(x) (x)

#define htobe64(x) htonll(x)
#define htole64(x) (x)
#define be64toh(x) ntohll(x)
#define le64toh(x) (x)

#ifndef __BYTE_ORDER
#define __BYTE_ORDER    BYTE_ORDER
#endif

#ifndef __BIG_ENDIAN
#define __BIG_ENDIAN    BIG_ENDIAN
#endif

#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN LITTLE_ENDIAN
#endif

#ifndef __PDP_ENDIAN
#define __PDP_ENDIAN    PDP_ENDIAN
#endif