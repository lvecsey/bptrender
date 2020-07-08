
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <assert.h>

#include <string.h>

#include <zlib.h>

#include "compress_zlib.h"

int compress_zlib(void *src_buf, size_t src_len, void *dst_buf, size_t dst_len, int level, size_t *compressed_len) {

  int ret, flush;
  unsigned have;
  z_stream strm;

  size_t amt;

  size_t remaining;

  unsigned char *adv_p;

  unsigned char *wptr;
  
  /* allocate deflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  ret = deflateInit(&strm, level);
  if (ret != Z_OK)
    return ret;

  remaining = src_len;

  adv_p = src_buf;

  wptr = dst_buf;
  
  do {

    amt = CHUNK;
    if (remaining < amt) {
      amt = remaining;
    }
    
    strm.avail_in = amt;
    flush = (amt < CHUNK) ? Z_FINISH : Z_NO_FLUSH;
    strm.next_in = adv_p;

    do {

      strm.avail_out = CHUNK;
      strm.next_out = wptr;

      ret = deflate(&strm, flush);    /* no bad return value */
      assert(ret != Z_STREAM_ERROR);  /* state not clobbered */

      have = CHUNK - strm.avail_out;
      wptr += have;
      
    } while (strm.avail_out == 0);
    assert(strm.avail_in == 0);     /* all input will be used */

    adv_p += amt;
    remaining -= amt;
    
  } while (flush != Z_FINISH);
  assert(ret == Z_STREAM_END);        /* stream will be complete */

  (void)deflateEnd(&strm);

  if (compressed_len != NULL) {
    compressed_len[0] = (wptr - ((unsigned char*) dst_buf));
  }
    
  return Z_OK;
  
}
