#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "aesdsocket.h"

int file_create(char *pathname, ...);

int file_write(int fd, const char *buffer, int size);

int file_size(int fd);

int file_read(int fd, char *buffer, int size);

void file_delete(char *file);