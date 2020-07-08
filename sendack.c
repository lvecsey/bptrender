
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <stdint.h>

#include <endian.h>

#include <errno.h>

#include "sendfile.h"

#include "sendack.h"

ssize_t sendack(int out_fd, uint64_t val) {

  ssize_t bytes_written;
  
  {
    uint64_t saved;

    saved = htobe64(0x100);
      
    bytes_written = sendfile(out_fd, &saved, sizeof(uint64_t));
    if (bytes_written <= 0) {
      perror("write");
      return -1;
    }
    if (bytes_written != sizeof(uint64_t)) {
      perror("write");
      return -1;
    }
  }

  return bytes_written;

}
