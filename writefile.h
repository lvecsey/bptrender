#ifndef WRITEFILE_H
#define WRITEFILE_H

#include <sys/types.h>

ssize_t writefile(int fd, void *buf, size_t len);

#endif
