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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mini_gxkit.h"

#include "readfile.h"
#include "writefile.h"

int main(int argc, char *argv[]) {

  image_t img;

  long int num_pixels;

  size_t img_sz;

  struct stat buf1, buf2;

  int retval;

  char *rgb_fn1;

  char *rgb_fn2;
  
  img.xres = 7680;
  img.yres = 4320;

  num_pixels = img.xres * img.yres;
  img_sz = num_pixels * sizeof(pixel_t);

  img.rgb = malloc(img_sz);
  if (img.rgb == NULL) {
    perror("malloc");
    return -1;
  }    

  rgb_fn1 = argc>1 ? argv[1] : NULL;

  rgb_fn2 = argc>2 ? argv[2] : NULL;  
  
  {

    int fd1, fd2;
    
    ssize_t bytes_read;
    ssize_t bytes_written;

    int out_fd;

    out_fd = 1;

    fprintf(stderr, "%s: Opening rgb_fn1 %s for input.\n", __FUNCTION__, rgb_fn1);
    
    fd1 = open(rgb_fn1, O_RDONLY);
    if (fd1 == -1) {
      perror("open");
      return -1;
    }

    fprintf(stderr, "%s: Opening rgb_fn2 %s for input.\n", __FUNCTION__, rgb_fn2);
    
    fd2 = open(rgb_fn2, O_RDONLY);    
    if (fd2 == -1) {
      perror("open");
      return -1;
    }

    retval = fstat(fd1, &buf1);
    if (retval == -1) {
      perror("fstat");
      return -1;
    }

    retval = fstat(fd2, &buf2);
    if (retval == -1) {
      perror("fstat");
      return -1;
    }

    while (buf1.st_size > 0 && buf2.st_size > 0) {
    
      bytes_read = readfile(fd1, img.rgb, img_sz >> 1);
      if (bytes_read != img_sz >> 1) {
	fprintf(stderr, "%s: Trouble reading from fd1 %d\n", __FUNCTION__, fd1);
	perror("read");
	return -1;
      }

      buf1.st_size -= bytes_read;
      
      bytes_read = readfile(fd2, img.rgb + (num_pixels >> 1), img_sz >> 1);    
      if (bytes_read != img_sz >> 1) {
	fprintf(stderr, "%s: Trouble reading from fd2 %d\n", __FUNCTION__, fd2);
	perror("read");
	return -1;
      }

      buf2.st_size -= bytes_read;
      
      bytes_written = writefile(out_fd, img.rgb, img_sz);
      if (bytes_written != img_sz) {
	perror("write");
	return -1;
      }

    }
      
  }
  
  free(img.rgb);
  
  return 0;

}
