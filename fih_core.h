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

#ifndef FIH_CORE_H
#define FIH_CORE_H

enum { FILLING_LEN, FILLING_IMG };

#include "fillpack.h"

#include "mini_gxkit.h"

typedef struct {

  int fd;
  
  fillpack len, compressed_drgb;

  long int prot_state;

  long int xres, yres;
  
  double *unpacked_drgb;

  int (*output_func)(void *data, size_t len, void *extra);

  void *output_extra;
  
  int active : 1;
  
} fih_state;

#define FC_QUIET 0x0
#define FC_VERBOSE 0x1

int fih_writeout(void *data, size_t len, void *extra);

#define FCR_OUTPUT 0x2000

int fih_core(fih_state *fh, int flags, long int *results);

int sendack_restart(fih_state *fh);

#endif
