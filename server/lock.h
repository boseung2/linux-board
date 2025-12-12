#ifndef LOCK_H
#define LOCK_H

#include <fcntl.h>
#include <unistd.h>

// READ LOCK 설정
static inline int file_read_lock(int fd) {
    struct flock fl;
    fl.l_type   = F_RDLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = 0;
    fl.l_len    = 0; // to EOF

    return fcntl(fd, F_SETLKW, &fl);
}

// WRITE LOCK 설정
static inline int file_write_lock(int fd) {
    struct flock fl;
    fl.l_type   = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = 0;
    fl.l_len    = 0; // to EOF

    return fcntl(fd, F_SETLKW, &fl);
}

// LOCK 해제
static inline int file_unlock(int fd) {
    struct flock fl;
    fl.l_type   = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = 0;
    fl.l_len    = 0; // to EOF

    return fcntl(fd, F_SETLK, &fl);
}

#endif