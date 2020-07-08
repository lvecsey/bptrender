/*
    Render a billion points across a cluster to an image or movie.
    Copyright (C) 2020  Lester Vecsey

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mini_gxkit.h"

#include "fih_core.h"

#include "recvfile.h"

#include "writefile.h"

#include "uncompress_zlib.h"

#include "sendack.h"

int fih_writeout(void *data, size_t len, void *extra) {

  int *out_fd;

  ssize_t bytes_written;
  
  out_fd = (int*) extra;
  
  bytes_written = writefile(out_fd[0], data, len);
  if (bytes_written != len) {
    fprintf(stderr, "%s: Trouble writing to out_fd %d\n", __FUNCTION__, out_fd[0]);
    perror("write");
    return -1;
  }

  return 0;
  
}

int sendack_restart(fih_state *fh) {

  ssize_t bytes_written;
  
  bytes_written = sendack(fh->fd, 0x100);
  if (bytes_written != sizeof(uint64_t)) {
    fprintf(stderr, "%s: Trouble with call to sendack.\n", __FUNCTION__);
    perror("write");
    return -1;
  }

  fh->len.cur_bytes = 0;

  fh->compressed_drgb.cur_bytes = 0;
      
  fh->prot_state = FILLING_LEN;

  return 0;
  
}

int fih_core(fih_state *fh, int flags, long int *results) {

  ssize_t bytes_read;

  ssize_t bytes_written;
  
  uint64_t *len;

  size_t remaining;

  size_t fsize;

  long int num_iters;

  unsigned char *wptr;

  int retval;
  
  len = fh->len.data;
  
  switch(fh->prot_state) {

  case FILLING_LEN:

    remaining = (fh->len.req_bytes - fh->len.cur_bytes);

    num_iters = 0;

    wptr = ((unsigned char*) fh->len.data) + fh->len.cur_bytes;
    
    bytes_read = recvfile(fh->fd, wptr, remaining, &num_iters);
    if (bytes_read == -1) {
      fprintf(stderr, "%s: Failed to read flie length from server.\n", __FUNCTION__);
      perror("read");
      return -1;
    }

    fprintf(stderr, "%s: (%d) recvfile num_iters %ld\n", __FUNCTION__, fh->fd, num_iters);
      
    if (bytes_read == 0) {
      fprintf(stderr, "%s: recvfile returned nothing for fsize length.\n", __FUNCTION__);
      return 100;
    }

    fh->len.cur_bytes += bytes_read;
    
    if (fh->len.cur_bytes == fh->len.req_bytes) {

      fsize = be64toh(*len);

      if (!fsize) {
	fprintf(stderr, "%s: No fsize.\n", __FUNCTION__);
	break;
      }

      fh->compressed_drgb.req_bytes = fsize;
      
      fh->prot_state = FILLING_IMG;
      
    }
    
    break;

  case FILLING_IMG:

    {
    
      long int num_pixels;
      size_t drgb_sz;

      num_pixels = fh->xres * fh->yres;
      drgb_sz = num_pixels * sizeof(double) * 3;

      fsize = be64toh(*len);
    
      fprintf(stderr, "%s: (%d) Allocating / Reallocating to %lu bytes.\n", __FUNCTION__, fh->fd, fsize);
      
      fh->compressed_drgb.data = realloc(fh->compressed_drgb.data, fsize);
      if (fh->compressed_drgb.data == NULL) {
	fprintf(stderr, "%s: (%d) Trouble reallocating memory to fsize %lu.\n", __FUNCTION__, fh->fd, fsize);
	perror("realloc");
	return -1;
      }

      fprintf(stderr, "%s: (%d) Receiving file.\n", __FUNCTION__, fh->fd);

      num_iters = 0;

      remaining = (fh->compressed_drgb.req_bytes - fh->compressed_drgb.cur_bytes);

      wptr = ((unsigned char*) fh->compressed_drgb.data) + fh->compressed_drgb.cur_bytes;
      
      bytes_read = recvfile(fh->fd, wptr, remaining, &num_iters);
      if (bytes_read <= 0) {
	fprintf(stderr, "%s: (%d) Trouble with recvfile, bytes_read %ld\n", __FUNCTION__, fh->fd, bytes_read);
	return -1;
      }

      fprintf(stderr, "%s: (%d) recvfile num_iters %ld\n", __FUNCTION__, fh->fd, num_iters);
      
      if (bytes_read != fsize) {
	fprintf(stderr, "%s: (%d) Trouble reading from network into memory.\n", __FUNCTION__, fh->fd);
	perror("read");
	return -1;
      }

      fprintf(stderr, "%s: (%d) Uncompressing image.\n", __FUNCTION__, fh->fd);
    
      {

	size_t uncompressed_len;
	uncompressed_len = 0;
	retval = uncompress_zlib(fh->compressed_drgb.data, fsize, fh->unpacked_drgb, drgb_sz, &uncompressed_len);
	if (retval == -1) {
	  fprintf(stderr, "%s: (%d) Trouble with call to uncompress_zlib.\n", __FUNCTION__, fh->fd);
	  return -1;
	}

	if (drgb_sz != uncompressed_len) {
	  fprintf(stderr, "%s: (%d) Mismatch drgb_sz %lu uncompressed_len %lu.\n", __FUNCTION__, fh->fd, drgb_sz, uncompressed_len);
	  return -1;
	}
	
      }

      fprintf(stderr, "%s: (%d) Writing or appending to output.\n", __FUNCTION__, fh->fd);
      
      if (fh->output_func != NULL) {
	
	retval = fh->output_func(fh->unpacked_drgb, drgb_sz, fh->output_extra);
	if (retval == -1) {
	  fprintf(stderr, "%s: Trouble calling output_func.\n", __FUNCTION__);
	  bytes_written = sendack(fh->fd, 0xFA);
	  return -1;
	}
	
      }

      if (results != NULL) {
	results[0] |= FCR_OUTPUT;
      }
      
      fprintf(stderr, "%s: (%d) File received OK, need to send ACK.\n", __FUNCTION__, fh->fd);
      
    }
      
    break;

  }
    
  return 0;

}
