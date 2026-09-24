#include "raw_import.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
struct input {int fd;size_t length;};
static long read_retry(int fd,void *buffer,size_t n)
{long got;do {got=read(fd,buffer,n);}while(got<0 && errno==EINTR);return got;}
static int fill(void *context,int32_t *data,uint32_t frames,const struct pt_raw_format *format)
{
    struct input *in=context;uint8_t block[3072];size_t pos=0;unsigned align=format->channels*(format->bits/8);
    (void)frames;
    while(pos<in->length) {
        size_t n=in->length-pos,have=0;struct pt_pcm pcm;
        if(n>sizeof(block))n=sizeof(block); /* divisible by every supported frame width */
        while(have<n) {long got=read_retry(in->fd,block+have,n-have);if(got<=0)return 0;have+=(size_t)got;}
        pcm=(struct pt_pcm){data+(pos/align)*format->channels,(n/align)*format->channels,(uint32_t)(n/align),format->rate,format->channels,format->bits};
        if(pt_raw_decode(block,n,format,&pcm)!=PT_RAW_OK)return 0;
        pos+=n;
    }
    if(read_retry(in->fd,block,1)!=0)return 0;
    {int rc=close(in->fd);in->fd=-1;return rc==0;}
}
enum pt_edit_result pt_raw_file_import(const char *path,size_t limit,struct pt_sampler *s,struct pt_project *p,struct pt_pattern_history *h,unsigned slot,const char *name,const struct pt_raw_format *format)
{
    struct input in;off_t end;enum pt_edit_result result=PT_EDIT_INVALID;
    if(!path)return result;
    in.fd=open(path,O_RDONLY);if(in.fd<0)return result;
    end=lseek(in.fd,0,SEEK_END);
    if(end<0)goto done;
    if((uintmax_t)end>(uintmax_t)limit) {result=PT_EDIT_CAPACITY;goto done;}
    in.length=(size_t)end;if(lseek(in.fd,0,SEEK_SET)!=0)goto done;
    result=pt_sampler_import_raw_fill(s,p,h,slot,in.length,name,format,fill,&in);
done:
    if(in.fd>=0)close(in.fd);
    return result;
}
