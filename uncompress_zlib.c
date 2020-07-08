
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <assert.h>

#include <zlib.h>

#define CHUNK 4096

int uncompress_zlib(void *src_buf, size_t src_len, void *dst_buf, size_t dst_len, size_t *uncompressed_len) {

    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    size_t amt;

    size_t remaining;
    
    unsigned char *adv_p;

    unsigned char *wptr;
    
    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK) {
      printf("Trouble with inflateInit.\n");
      return -1;
    }

    adv_p = src_buf;

    wptr = dst_buf;
    
    do {

      amt = CHUNK;
      if (remaining < amt) {
	amt = remaining;
      }
      
      strm.avail_in = amt;
      if (strm.avail_in == 0)
	break;
      strm.next_in = adv_p;

        /* run inflate() on input until output buffer not full */
        do {

            strm.avail_out = CHUNK;
            strm.next_out = wptr;

            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
		printf("Z_MEM_ERROR\n");
		return -1;
            }

            have = CHUNK - strm.avail_out;
	    wptr += have;

        } while (strm.avail_out == 0);

	adv_p += amt;
	remaining -= amt;
	
        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);

    if (uncompressed_len != NULL) {
      uncompressed_len[0] = (wptr - ((unsigned char*)dst_buf));
    }
    
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

