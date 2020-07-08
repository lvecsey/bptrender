#ifndef UNCOMPRESS_ZLIB_H
#define UNCOMPRESS_ZLIB_H

#include <stdlib.h>

#define CHUNK 4096

int uncompress_zlib(void *src_buf, size_t src_len, void *dst_buf, size_t dst_len, size_t *uncompressed_len);

#endif
