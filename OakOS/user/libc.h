#ifndef OAK_USER_LIBC_H
#define OAK_USER_LIBC_H

long oak_getpid(void);
long oak_fork(void);
long oak_waitpid(unsigned long pid, int *status);
void oak_exit(int status);
long oak_write(const char *text, unsigned long length);
long oak_mkdir(const char *path);
long oak_create(const char *path);
long oak_write_file(const char *path, const void *data, unsigned long length);
long oak_read_file(const char *path, void *data, unsigned long capacity);

#endif