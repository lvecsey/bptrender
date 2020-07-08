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

#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/mman.h>
#include <errno.h>

#include <endian.h>

#include "recvfile.h"
#include "sendfile.h"

#include "mini_gxkit.h"

#include "uncompress_zlib.h"

#include "fih_core.h"

#include "readfile.h"
#include "writefile.h"

#include "sendack.h"

#include "imgfill_pack.h"

#include "postprocess.h"

int imgfill_func(void *data, size_t len, void *extra) {

  imgfill_pack *pack;

  pack = (imgfill_pack*) extra;

  memcpy(pack->drgb_start, data, len);

  (*pack->slice_set)++;
  
  return 0;

}

int main(int argc, char *argv[]) {

  int in_fd;

  int out_fd;

  uint64_t len;

  char *imghalf_fn;

  int imghalf_fd;

  long int localport;

  long int num_iters;

  void *compressed_rgb;

  size_t drgb_sz;
  
  image_t img;

  long int num_pixels;
  size_t img_sz;

  long int input_xres, input_yres;

  int retval;
  
  imghalf_fn = argc>1 ? argv[1] : NULL;

  retval = argc>2 ? sscanf(argv[2],"%ldx%ld",&input_xres,&input_yres) : -1;

  fprintf(stderr, "%s: Full frame size %ldx%ld, half frame %ldx%ld\n", __FUNCTION__, input_xres, input_yres, input_xres, input_yres >> 1);
  
  compressed_rgb = malloc(4096);
  if (compressed_rgb == NULL) {
    perror("malloc");
    return -1;
  }

  img.xres = input_xres;
  img.yres = input_yres >> 1;
  num_pixels = img.xres * img.yres;
  img_sz = num_pixels * sizeof(pixel_t);

  drgb_sz = num_pixels * sizeof(double) * 3;
  
  img.rgb = malloc(img_sz);
  if (img.rgb == NULL) {
    perror("malloc");
    return -1;
  }
  
  {

    ssize_t bytes_read;
    ssize_t bytes_written;

    size_t fsize;

    mode_t mode;

    void *mem;

    int retval;

    char *env_PROTO;

    char *env_TCPLOCALPORT;

    fih_state fh;

    long int xres, yres;

    long int image_splices = 0;

    long int results;
    
    mem = malloc(4096);
    if (mem == NULL) {
      perror("malloc");
      return -1;
    }
    
    mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    env_PROTO = getenv("PROTO");

    if (env_PROTO != NULL && !strncmp(env_PROTO, "TCP", 3)) {
      in_fd = 6;
      out_fd = 7;
    }
    else {
      in_fd = 0;
      out_fd = 1;
    }
    
    env_TCPLOCALPORT = getenv("TCPLOCALPORT");

    localport = (env_TCPLOCALPORT != NULL) ? strtol(env_TCPLOCALPORT, NULL, 10) : 0;

    
    if (imghalf_fn[0] == '-' && imghalf_fn[1] == 0) {
      imghalf_fd = 1;
    }

    else {

      fprintf(stderr, "%s: Saving frames to %s\n", __FUNCTION__, imghalf_fn);
      
      imghalf_fd = open(imghalf_fn, O_CREAT | O_TRUNC | O_WRONLY, mode);
      if (imghalf_fd == -1) {
	fprintf(stderr, "%s: Failed to open output file %s\n", __FUNCTION__, imghalf_fn);
	perror("open");
	return -1;
      }

    }

    memset(&fh, 0, sizeof(fih_state));
    fh.fd = in_fd;
    fh.xres = input_xres;
    fh.yres = (input_yres >> 1);
    fh.len.req_bytes = 8;
    fh.len.data = malloc(fh.len.req_bytes);
    if (fh.len.data == NULL) {
      perror("malloc");
      return -1;
    }
    fh.compressed_drgb.req_bytes = 4096;
    fh.compressed_drgb.data = malloc(fh.compressed_drgb.req_bytes);
    if (fh.compressed_drgb.data == NULL) {
      perror("malloc");
      return -1;
    }
    fh.prot_state = FILLING_LEN;
    fh.unpacked_drgb = malloc(drgb_sz);
    if (fh.unpacked_drgb == NULL) {
      perror("malloc");
      return -1;
    }

    if (imghalf_fn[0] != '-' && imghalf_fn[1] != 0) {
      imghalf_fd = open(imghalf_fn, O_CREAT | O_TRUNC | O_WRONLY, mode);
      if (imghalf_fd == -1) {
	fprintf(stderr, "%s: Failed to open output file %s\n", __FUNCTION__, imghalf_fn);
	perror("open");
	return -1;
      }
    }
      
    fh.output_func = NULL;
    fh.output_extra = &imghalf_fd;
    
    fh.active = 1;
    
    while (fh.active) {

      results = 0;
      
      retval = fih_core(&fh, FC_VERBOSE, &results);
      if (retval == -1) {
	printf("%s: Trouble with call to fih_core.\n", __FUNCTION__);
	return -1;
      }

      if (retval == 100) {
	fh.active = 0;
      }
      
      if (results == FCR_OUTPUT) {

	fprintf(stderr, "%s: Completed.\n", __FUNCTION__);

	retval = postprocess(fh.unpacked_drgb, &img);
	if (retval == -1) {
	  fprintf(stderr, "%s: Trouble calling postprocess from drgb to rgb.\n", __FUNCTION__);
	  return -1;
	}
	
	bytes_written = writefile(imghalf_fd, img.rgb, img_sz);
	if (bytes_written != img_sz) {
	  perror("write");
	  return -1;
	}

	retval = sendack_restart(&fh);
	if (retval == -1) {
	  fprintf(stderr, "%s: Trouble sending an ack and restart. fih1\n", __FUNCTION__);
	}

      }      
      
    }
    
    retval = close(imghalf_fd);
    if (retval == -1) {
      perror("close");
      return -1;
    }

    free(mem);

    free(fh.len.data);
    free(fh.compressed_drgb.data);
    free(fh.unpacked_drgb);
    
  }

  free(img.rgb);
  
  free(compressed_rgb);
  
  return 0;

}
  
