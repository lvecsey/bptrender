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

#include <math.h>

#include "pointcol.h"

#include <sys/mman.h>
#include <errno.h>

const long int def_numpointcols = 3e9;

int main(int argc, char *argv[]) {

  long int pointno;

  long int num_pointcols;

  char *filename;

  mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

  int fd;

  size_t fsize;

  int retval;

  double radians;

  pixel_t white = { 65535, 65535, 65535 };

  pointcol pc;

  off_t offset;
  
  filename = argc>1 ? argv[1] : "/tmp/pointcols.dat";
  
  fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, mode);
  if (fd == -1) {
    perror("open");
    return -1;
  }

  num_pointcols = argc>2 ? strtod(argv[2], NULL) : def_numpointcols;
  
  fsize = num_pointcols;
  fsize *= sizeof(pointcol);

  retval = ftruncate(fd, fsize);
  if (retval == -1) {
    perror("ftruncate");
    return -1;
  }

  for (pointno = 0; pointno < num_pointcols; pointno++) {

    radians = 2.0 * M_PI * pointno / (num_pointcols - 1);
    
    pc = (pointcol) {
      .pnta = { -1.0 + 2.0 * pointno / (num_pointcols - 1), .y = cos(radians), .z = 0.0 },
      .color = white
    };

    offset = pointno;
    offset *= sizeof(pointcol);
    
    retval = pwrite(fd, &pc, sizeof(pointcol), offset);
    if (retval == -1) {
      perror("pwrite");
      return -1;
    }
    
  }
  
  retval = close(fd);
  if (retval == -1) {
    perror("close");
    return -1;
  }
  
  return 0;

}
