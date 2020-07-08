
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
    for (pixelno = 0; pixelno < num_pixels; pixelno++) {
      img->rgb[pixelno].r = 65535.0 * (drgb[3*pixelno+0] / (1.5 * (istat.rmax - istat.ravg)));
      img->rgb[pixelno].g = 65535.0 * (drgb[3*pixelno+1] / (1.5 * (istat.gmax - istat.gavg)));
      img->rgb[pixelno].b = 65535.0 * (drgb[3*pixelno+2] / (1.5 * (istat.bmax - istat.bavg)));
    }
  }
  
  return 0;
  
}
