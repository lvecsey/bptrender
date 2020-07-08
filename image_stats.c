
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "image_stats.h"

int image_stats(double *drgb, long int num_pixels, image_statpack *fill_pack) {

  long int pixelno;

  double rmax, gmax, bmax;
  double rmin, gmin, bmin;

  double rsum, gsum, bsum;
  
  double ravg, gavg, bavg;
  
  rmin = drgb[3*0+0];
  gmin = drgb[3*0+1];
  bmin = drgb[3*0+2];  

  rmax = drgb[3*0+0];
  gmax = drgb[3*0+1];
  bmax = drgb[3*0+2];  
  
  rsum = drgb[3*0+0];
  gsum = drgb[3*0+1];
  bsum = drgb[3*0+2];  
  
  for (pixelno = 1; pixelno < num_pixels; pixelno++) {

    if (drgb[3*pixelno+0] < rmin) {
      rmin = drgb[3*pixelno+0];
    }

    if (drgb[3*pixelno+1] < gmin) {
      gmin = drgb[3*pixelno+1];
    }

    if (drgb[3*pixelno+2] < bmin) {
      bmin = drgb[3*pixelno+2];
    }

    if (drgb[3*pixelno+0] > rmax) {
      rmax = drgb[3*pixelno+0];
    }

    if (drgb[3*pixelno+1] > gmax) {
      gmax = drgb[3*pixelno+1];
    }

    if (drgb[3*pixelno+2] > bmax) {
      bmax = drgb[3*pixelno+2];
    }

    rsum += drgb[3*pixelno+0];
    gsum += drgb[3*pixelno+1];
    bsum += drgb[3*pixelno+2];    
    
  }

  ravg = rsum / num_pixels;
  gavg = gsum / num_pixels;
  bavg = bsum / num_pixels;  
  
  fprintf(stderr, "image: min %g %g %g, avg %g %g %g, max %g %g %g\n", rmin, gmin, bmin, ravg, gavg, bavg, rmax, gmax, bmax);

  if (fill_pack != NULL) {

    fill_pack->rmin = rmin;
    fill_pack->gmin = gmin;
    fill_pack->bmin = bmin;    

    fill_pack->rmax = rmax;
    fill_pack->gmax = gmax;
    fill_pack->bmax = bmax;    

    fill_pack->ravg = ravg;
    fill_pack->gavg = gavg;
    fill_pack->bavg = bavg;    
    
  }
    
  return 0;
  
}
