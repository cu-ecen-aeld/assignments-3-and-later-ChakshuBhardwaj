#include <stdarg.h>
#include "manipulate_file.h"

int file_write(int fd, const char *buffer, int size) {
    if (fd < 0) {
        fprintf(stderr, "Invalid file descriptor\n");
        return -1;
    }

    // Write to the file
    ssize_t written = write(fd, buffer, size);
    if (written == -1) {
        perror("Failed to write to file");
        return -1;
    }

    return written;
}

int file_size(int fd)
{
    int front, end;
    
    end = lseek(fd, 0, SEEK_END);
    front = lseek(fd, 0, SEEK_SET);
    
    return end - front;
}


int file_read(int fd, char *buffer, int size) {
    char buf[1024] = {0};

    // Validate input parameters
    if (fd < 0) {
        fprintf(stderr, "Invalid file descriptor\n");
        return -1;
    }

    if (!buffer || size <= 0) {
        fprintf(stderr, "Invalid buffer or size\n");
        return -1;
    }
    ssize_t bytes_read = 0;
    do {
        // Read from the file
        bytes_read = read(fd, buf, size);
        if (bytes_read == -1) {
            perror("Failed to read from file");
            return -1; // Return an error code
        } else if(bytes_read > 0) {
            strcat(buffer, buf);
            size -= bytes_read;
            memset(buf, 0, sizeof(buf));
        }
        
    }while(bytes_read);
   
    
    return (int)bytes_read; // Return the number of bytes read
}

void file_delete(char *file)
{
    int rc = 0;
    struct stat status;
    memset(&status, 0, sizeof(struct stat));

    rc = stat(file, &status);
    if (rc != -1) {
        remove(file);
    }
}
