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

#include "mini_gxkit.h"

#include <math.h>

#include <vector.h>

#include "pointcol.h"

#include "render.h"

#include "render_work.h"

#include "dot.h"

#include "ga.h"

#include "stereographic.h"

#include "region.h"

void *render(void *extra) {

  void *ret;

  render_work *rw;

  point3d_t pnta;

  point3d_t pntb;

  point3d_t *pcp;
  
  point3d_t pntc;
  
  double *matrix;

  long int pointno;

  long int sampleno;

  point_t aud_pt;

  double phi, theta;

  double rho;

  point_t pt;
  long int xpos, ypos;

  double depth;

  double aspect;

  double vp;

  double vf;
  
  long int num_splits;

  long int num_pixelupdate;
  
  rw = (render_work*) extra;
  
  ret = NULL;

  num_splits = 2;
  
  rho = 1.0;

  aspect = ((double) rw->xres) / rw->yres;

  depth = 1.75;

  matrix = rw->matrix;
  
  vf = rw->vf;

  num_pixelupdate = 0;
  
  for (pointno = 0; pointno < rw->num_pointcols; pointno++) {

    vp = pointno; vp /= (rw->num_pointcols - 1);
    
    sampleno = (pointno * rw->num_samples) / rw->num_pointcols;

    aud_pt = (point_t) { .x = rw->audio_samples[sampleno].left, .y = rw->audio_samples[sampleno].right };

    stereographic(aud_pt, M_PI, 0.0, 1.0, &phi, &theta);

    pnta = (point3d_t) { .x = cos(phi) * cos(theta) * 1.0, .y = cos(phi) * sin(theta) * rho, .z = sin(phi) };

    pcp = &(rw->pc[pointno].pnta);
    
    pntb = (point3d_t) { .x = (1.0 - vf) * pnta.x + vf * pcp->x, .y = (1.0 - vf) * pnta.y + vf * pcp->y, .z = (1.0 - vf) * pnta.z + vf * pcp->z };

    pntc.x = (pntb.x) * matrix[0] + (pntb.y) * matrix[1] + (pntb.z) * matrix[2] + 1.0 * matrix[3];
    pntc.y = (pntb.x) * matrix[4] + (pntb.y) * matrix[5] + (pntb.z) * matrix[6] + 1.0 * matrix[7]; 
    pntc.z = (pntb.x) * matrix[8] + (pntb.y) * matrix[9] + (pntb.z) * matrix[10] + 1.0 * matrix[11];  

    pt.x = pntc.x / (pntc.z + depth);
    pt.y = pntc.y / (pntc.z + depth);    

    pt.x /= aspect;
    pt.y *= -1.0;
    
    xpos = pt.x * (rw->xres >> 1); xpos += rw->xres >> 1;
    ypos = pt.y * ( (num_splits * rw->yres) >> 1); ypos += (num_splits * rw->yres) >> 1;    

    if (xpos < 0 || xpos >= rw->xres) {
      continue;
    }

    switch(rw->region) {
    case RTOP:
      if (ypos >= ((num_splits * rw->yres) >> 1)) {
	continue;
      }
      break;
    case RBOTTOM:
      if (ypos < ((num_splits * rw->yres) >> 1)) {
	continue;
      }
      ypos -= ((num_splits * rw->yres) >> 1);
      break;
    }
    
    if (ypos >= 0 && ypos < rw->yres) {
      rw->drgb[ypos * rw->xres * 3 + xpos * 3 + 0] += rw->pc[pointno].color.r;
      rw->drgb[ypos * rw->xres * 3 + xpos * 3 + 1] += rw->pc[pointno].color.g;
      rw->drgb[ypos * rw->xres * 3 + xpos * 3 + 2] += rw->pc[pointno].color.b;
      num_pixelupdate++;
    }
    
  }

  fprintf(stderr, "%s: Updated %ld pixels.\n", __FUNCTION__, num_pixelupdate);
  
  return ret;

}
