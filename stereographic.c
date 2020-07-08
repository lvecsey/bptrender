
#include <math.h>

#include "mini_gxkit.h"

#include "stereographic.h"

// https://mathworld.wolfram.com/StereographicProjection.html

int inv_stereo(double phi, double phi0, double theta, double theta1, double radius) {

  double k;

  point_t pt;

  k = 2.0 * radius / (1 + sin(theta1) * sin(theta) + cos(theta1) * cos(theta) * cos(phi - phi0));
  
  pt.x = k * cos(theta) * sin(phi - phi0);

  pt.y = k * (cos(theta) * sin(theta) - sin(theta) * cos(theta) * cos(phi - phi0));
  
  return 0;

}
  
int stereographic(point_t pt, double phi0, double theta1, double radius, double *phi, double *theta) {

  double p;

  double c;

  p = sqrt(pt.x * pt.x + pt.y * pt.y);

  c = 2.0 * atan(p / (2.0 * radius));

  theta[0] = asin(cos(c) * sin(theta1) + (pt.y * sin(c) * cos(theta1) / p));

  phi[0] = phi0 + atan(pt.x * sin(c) / (p * cos(theta1) * cos(c) - pt.y * sin(theta1) * sin(c)));
	   
  return 0;
  
}
