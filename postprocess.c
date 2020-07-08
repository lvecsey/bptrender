
#include <stdio.h>

#include "mini_gxkit.h"

#include "image_stats.h"

int postprocess(double *drgb, image_t *img) {

  image_statpack istat;

  long int num_pixels;

  num_pixels = img->xres * img->yres;
  
  fprintf(stderr, "%s: Collecting stats on frame.\n", __FUNCTION__);
      
  image_stats(drgb, num_pixels, &istat);

  fprintf(stderr, "%s: Final fill of image output buffer.\n", __FUNCTION__);
      
  {
    long int pixelno;

    double factor;
    
    double v;

    factor = 0.1;
    
    v = 0.25;
    
    for (pixelno = 0; pixelno < num_pixels; pixelno++) {
      img->rgb[pixelno].r = 65535.0 * (drgb[3*pixelno+0] / (factor * ((1.0 - v) * istat.ravg + v * istat.rmax)));
      img->rgb[pixelno].g = 65535.0 * (drgb[3*pixelno+1] / (factor * ((1.0 - v) * istat.gavg + v * istat.gmax)));
      img->rgb[pixelno].b = 65535.0 * (drgb[3*pixelno+2] / (factor * ((1.0 - v) * istat.bavg + v * istat.bmax)));
    }
  }
  
  return 0;
  
}
