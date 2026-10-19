#ifndef FH_IO_FAULTS_H
#define FH_IO_FAULTS_H
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stddef.h>
/* Every wrapped descriptor is real; failed close consumes it exactly once.
 * A close error here models the POSIX ambiguous/consumed descriptor case.
 * Wrappers are compiled ONLY into selected portable adapter translation units. */
enum fh_call {FH_OPEN,FH_READ,FH_WRITE,FH_SEEK,FH_SYNC,FH_CLOSE,FH_LINK,FH_UNLINK,FH_CALLS};
enum fh_mode {FH_ERROR,FH_ZERO,FH_SHORT,FH_EINTR,FH_RACE,FH_MISMATCH,FH_TRUNCATE,FH_EXTRA};
struct fh_io_state {
    size_t calls[FH_CALLS],index,triggered,open_live,open_peak;
    int active,mode,operation,deny_cleanup,short_writes;
    char retained[1600];
};
extern struct fh_io_state fh_io;
extern const char *const fh_names[FH_CALLS];
void fh_io_reset(int,size_t,int);
int fh_open(const char *,int,...);
ssize_t fh_read(int,void *,size_t);
ssize_t fh_write(int,const void *,size_t);
off_t fh_lseek(int,off_t,int);
int fh_fsync(int);
int fh_close(int);
int fh_link(const char *,const char *);
int fh_unlink(const char *);
#endif
