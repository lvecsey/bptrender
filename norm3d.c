
#include "vector.h"

#include "norm3d.h"

#include <math.h>

int norm3d(vec3d *vec) {

  double magnitude = sqrt( ((*vec)[0] * (*vec)[0]) + ((*vec)[1] * (*vec)[1]) + ((*vec)[2] * (*vec)[2]));

  (*vec)[0] /= magnitude;
  (*vec)[1] /= magnitude;
  (*vec)[2] /= magnitude;  

  return 0;

}
