#ifndef RECVFILE_H
#define RECVFILE_H

#include <sys/types.h>

ssize_t recvfile(int fd, void *buf, size_t len, long int *num_iters);

#endif
