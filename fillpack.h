#ifndef FILLPACK_H
#define FILLPACK_H

#include <unistd.h>

typedef struct {

  void *data;
  size_t req_bytes;
  size_t cur_bytes;

} fillpack;

#endif
