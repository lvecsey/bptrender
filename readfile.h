#ifndef READFILE_H
#define READFILE_H

#include <sys/types.h>

ssize_t readfile(int fd, void *buf, size_t len);

#endif
