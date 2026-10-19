#define _POSIX_C_SOURCE 200809L
#include "io_faults.h"
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
struct fh_io_state fh_io;
const char *const fh_names[FH_CALLS]={"open","read","write","lseek","fsync","close","link","unlink"};
static int tracked[32];
void fh_io_reset(int operation,size_t index,int mode)
{
    assert(!fh_io.open_live);memset(&fh_io,0,sizeof(fh_io));memset(tracked,0,sizeof(tracked));
    fh_io.active=index!=0;fh_io.operation=operation;fh_io.index=index;fh_io.mode=mode;
}
static int hit(int op)
{
    ++fh_io.calls[op];
    if(fh_io.active && fh_io.operation==op && fh_io.calls[op]==fh_io.index) {++fh_io.triggered;return 1;}
    return 0;
}
static void remember(int fd)
{
    size_t i;if(fd<0)return;
    for(i=0;i<32 && tracked[i];++i) {}assert(i<32);tracked[i]=fd+1;
    ++fh_io.open_live;if(fh_io.open_live>fh_io.open_peak)fh_io.open_peak=fh_io.open_live;
}
static void forget(int fd)
{
    size_t i;for(i=0;i<32 && tracked[i]!=fd+1;++i) {}assert(i<32);
    tracked[i]=0;assert(fh_io.open_live);--fh_io.open_live;
}
int fh_open(const char *path,int flags,...)
{
    mode_t mode=0;int fd,h;
    if(flags&O_CREAT) {va_list ap;va_start(ap,flags);mode=(mode_t)va_arg(ap,int);va_end(ap);}
    h=hit(FH_OPEN);
    if(h && fh_io.mode==FH_ERROR) {errno=EACCES;return -1;}
    if(h && fh_io.mode>=FH_MISMATCH) {
        int f=open(path,O_WRONLY);assert(f>=0);
        if(fh_io.mode==FH_TRUNCATE)assert(ftruncate(f,7)==0);
        else if(fh_io.mode==FH_EXTRA) {assert(lseek(f,0,SEEK_END)>=0);assert(write(f,"!",1)==1);}
        else assert(write(f,"!",1)==1);
        assert(!close(f));
    }
    fd=(flags&O_CREAT)?open(path,flags,mode):open(path,flags);remember(fd);return fd;
}
ssize_t fh_read(int fd,void *p,size_t n)
{
    if(hit(FH_READ)) {
        if(fh_io.mode==FH_ZERO)return 0;
        if(fh_io.mode==FH_SHORT) {if(n>7)n=7;}
        else {errno=fh_io.mode==FH_EINTR?EINTR:EIO;return -1;}
    }
    return read(fd,p,n);
}
ssize_t fh_write(int fd,const void *p,size_t n)
{
    if(hit(FH_WRITE)) {
        if(fh_io.mode==FH_ZERO)return 0;
        if(fh_io.mode==FH_SHORT) {if(n>7)n=7;}
        else {errno=fh_io.mode==FH_EINTR?EINTR:ENOSPC;return -1;}
    }
    if(fh_io.short_writes && n>7)n=7;
    return write(fd,p,n);
}
off_t fh_lseek(int fd,off_t off,int whence)
{if(hit(FH_SEEK)) {errno=EIO;return -1;}return lseek(fd,off,whence);}
int fh_fsync(int fd)
{if(hit(FH_SYNC)) {errno=EIO;return -1;}return fsync(fd);}
int fh_close(int fd)
{
    int h=hit(FH_CLOSE),rc;forget(fd);rc=close(fd);
    if(h) {assert(!rc);errno=EIO;return -1;}return rc;
}
int fh_link(const char *from,const char *to)
{
    if(hit(FH_LINK)) {
        if(fh_io.mode==FH_RACE) {int fd=open(to,O_CREAT|O_EXCL|O_WRONLY,0600);assert(fd>=0);assert(write(fd,"RACE",4)==4 && !close(fd));}
        else {errno=EIO;return -1;}
    }
    return link(from,to);
}
int fh_unlink(const char *path)
{
    int h=hit(FH_UNLINK);
    if(h || fh_io.deny_cleanup) {
        assert(strlen(path)<sizeof(fh_io.retained));strcpy(fh_io.retained,path);errno=EACCES;return -1;
    }
    return unlink(path);
}
