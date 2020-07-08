
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "readfile.h"

ssize_t readfile(int fd, void *buf, size_t len) {

  size_t remaining;
  unsigned char *adv_p;
  ssize_t bytes_read;

  size_t chunk_sz;

  size_t amt;
  
  chunk_sz = 4096;
  
  adv_p = buf;
  
  remaining = len;
  while (remaining > 0) {

    amt = chunk_sz;
    if (remaining < amt) {
      amt = remaining;
    }
    
    bytes_read = read(fd, adv_p + (len - remaining), amt);
    if (!bytes_read) break;
    if (bytes_read < 0) {
      return -1;
    }
    remaining -= bytes_read;
  }

  return (len - remaining);
  
}
    
  
  
