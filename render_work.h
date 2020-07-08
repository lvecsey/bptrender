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

#ifndef RENDER_WORK_H
#define RENDER_WORK_H

#include "mini_gxkit.h"

#include "pointcol.h"

#include "audio_sample.h"

#include "ga.h"

typedef struct {

  long int xres, yres;
  
  double *drgb;
  
  long int region;
  
  long int ypos_start, ypos_end;
  
  pointcol *pc;

  long int num_pointcols;

  audio_sample *audio_samples;

  long int num_samples;

  double *matrix;
  
  double vf;
  
} render_work;

#endif
