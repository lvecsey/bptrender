#ifndef SENDFILE_H
#define SENDFILE_H

#include <sys/types.h>

int sendfile(int fd, void *buf, size_t len);

#endif
