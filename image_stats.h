#ifndef IMAGE_STATS_H
#define IMAGE_STATS_H

typedef struct {

  double rmax, gmax, bmax;
  double rmin, gmin, bmin;

  double ravg, gavg, bavg;
  
} image_statpack;

int image_stats(double *drgb, long int num_pixels, image_statpack *fill_pack);

#endif
