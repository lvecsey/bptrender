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

int project_point(double *drgb, uint64_t *perpixelcounts, long int xres, long int yres, long int region, point3d_t *pntc, double depth, double aspect, pixel_t fill_color, long int *num_pixelupdate) {
  
  point_t pt;

  long int xpos, ypos;

  long int num_splits;
  
  num_splits = 2;
  
  pt.x = pntc->x / (pntc->z + depth);
  pt.y = pntc->y / (pntc->z + depth);    

  pt.x /= aspect;
  pt.y *= -1.0;
    
  xpos = pt.x * (xres >> 1); xpos += xres >> 1;
  ypos = pt.y * ( (num_splits * yres) >> 1); ypos += (num_splits * yres) >> 1;    

  if (xpos < 0 || xpos >= xres) {
    return 0;
  }

  switch(region) {
  case RTOP:
    if (ypos >= ((num_splits * yres) >> 1)) {
      return 0;
    }
    break;
  case RBOTTOM:
    if (ypos < ((num_splits * yres) >> 1)) {
      return 0;
    }
    ypos -= ((num_splits * yres) >> 1);
    break;
  }
    
  if (ypos >= 0 && ypos < yres) {
    drgb[ypos * xres * 3 + xpos * 3 + 0] += fill_color.r / 65535.0;
    drgb[ypos * xres * 3 + xpos * 3 + 1] += fill_color.g / 65535.0;
    drgb[ypos * xres * 3 + xpos * 3 + 2] += fill_color.b / 65535.0;
    num_pixelupdate[0]++;
  }

  return 0;

}

void *render(void *extra) {

  void *ret;

  render_work *rw;

  point3d_t pnta;

  point3d_t pntb;

  point3d_t *pcp;
  
  point3d_t pntc;

  point3d_t pntd;
  
  double *matrix;

  long int pointno;

  long int sampleno;

  point_t aud_pt;

  double phi, theta;

  double rho;

  double depth;

  double aspect;

  double vp;

  double vf;
  
  long int num_pixelupdate;

  long int range;

  double vol;
  
  double radius;

  pixel_t red = { .r = 65535, .g = 0, .b = 0 };

  pixel_t blue = { .r = 0, .g = 0, .b = 65535 };
  
  rw = (render_work*) extra;
  
  ret = NULL;
  
  rho = 1.0;

  aspect = ((double) rw->xres) / rw->yres;

  depth = 1.75;

  matrix = rw->matrix;
  
  vf = rw->vf;

  num_pixelupdate = 0;
  
  for (pointno = 0; pointno < rw->num_pointcols; pointno++) {

    vp = pointno; vp /= (rw->num_pointcols - 1);

    range = (rw->num_samples / rw->num_frames);
    
    sampleno = vf * (rw->num_samples - range) + vp * range;

    aud_pt = (point_t) { .x = rw->audio_samples[sampleno].left, .y = rw->audio_samples[sampleno].right };
    
    stereographic(aud_pt, 0.0, 0.0, 1.0, &phi, &theta);

    phi *= 2.0;
    theta *= 2.0;
    
    vol = 0.5 * (fabs(aud_pt.x) + fabs(aud_pt.y));

    pnta = (point3d_t) { .x = cos(phi) * cos(theta) * 1.0, .y = cos(phi) * sin(theta) * rho, .z = sin(phi) };

    pntc.x = (pnta.x) * matrix[0] + (pnta.y) * matrix[1] + (pnta.z) * matrix[2] + 1.0 * matrix[3];
    pntc.y = (pnta.x) * matrix[4] + (pnta.y) * matrix[5] + (pnta.z) * matrix[6] + 1.0 * matrix[7]; 
    pntc.z = (pnta.x) * matrix[8] + (pnta.y) * matrix[9] + (pnta.z) * matrix[10] + 1.0 * matrix[11];  

    project_point(rw->drgb, rw->perpixelcounts, rw->xres, rw->yres, rw->region, &pntc, depth, aspect, red, &num_pixelupdate);
    
    pcp = &(rw->pc[pointno].pnta);

    pntb = (point3d_t) { .x = (1.0 - vp) * pnta.x + vp * pcp->x, .y = (1.0 - vp) * pnta.y + vp * pcp->y, .z = (1.0 - vp) * pnta.z + vp * pcp->z };
    
    pntd.x = (pntb.x) * matrix[0] + (pntb.y) * matrix[1] + (pntb.z) * matrix[2] + 1.0 * matrix[3];
    pntd.y = (pntb.x) * matrix[4] + (pntb.y) * matrix[5] + (pntb.z) * matrix[6] + 1.0 * matrix[7]; 
    pntd.z = (pntb.x) * matrix[8] + (pntb.y) * matrix[9] + (pntb.z) * matrix[10] + 1.0 * matrix[11];  

    project_point(rw->drgb, rw->perpixelcounts, rw->xres, rw->yres, rw->region, &pntd, depth, aspect, rw->pc[pointno].color, &num_pixelupdate);
    
  }

  fprintf(stderr, "%s: Updated %ld pixels.\n", __FUNCTION__, num_pixelupdate);
  
  return ret;

}
