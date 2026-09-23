#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "file_load.h"
static long read_retry(int fd,void *buffer,size_t n)
{
    long got;do {got=(long)read(fd,buffer,n);} while(got<0 && errno==EINTR);
    return got;
}
enum pt_load_result pt_file_load(const char *path,size_t limit,
    const struct pt_allocator *a,uint8_t **bytes,size_t *size)
{
    int fd;off_t end;size_t n,pos=0;uint8_t *data=NULL,extra;
    enum pt_load_result result=PT_LOAD_IO;
    if(!path || !bytes || !size || !a || !a->allocate || !a->release)return PT_LOAD_INVALID;
    fd=open(path,O_RDONLY);if(fd<0)return PT_LOAD_IO;
    end=lseek(fd,0,SEEK_END);if(end<0)goto done;
    if((uintmax_t)end>(uintmax_t)limit) {result=PT_LOAD_LIMIT;goto done;}
    n=(size_t)end;
    if(lseek(fd,0,SEEK_SET)!=0)goto done;
    data=a->allocate(a->context,n?n:1);
    if(!data) {result=PT_LOAD_MEMORY;goto done;}
    while(pos<n) {
        size_t count=n-pos;long got;if(count>32768)count=32768;
        got=read_retry(fd,data+pos,count);if(got<=0)goto done;
        pos+=(size_t)got;
    }
    if(read_retry(fd,&extra,1)!=0)goto done;
    result=PT_LOAD_OK;
done:
    if(close(fd))result=PT_LOAD_IO;
    if(result==PT_LOAD_OK) {*bytes=data;*size=n;}
    else if(data)a->release(a->context,data);
    return result;
}
