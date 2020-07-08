#ifndef COMPRESS_ZLIB_H
#define COMPRESS_ZLIB_H

#include <stdlib.h>

#define CHUNK 4096

int compress_zlib(void *src_buf, size_t src_len, void *dst_buf, size_t dst_len, int level, size_t *compressed_len);

#endif
