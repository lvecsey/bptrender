
#include "region.h"

char *regstr(long int region) {

  switch(region) {

  case RTOP: return "TOP";
  case RBOTTOM: return "BOTTOM";
  default: return "NONE";

  }

  return 0;

}
