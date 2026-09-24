#include "raw_import.h"
#include "../core/wav.h"
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
struct input {int fd;size_t length,bytes;};
static long read_retry(int fd,void *buffer,size_t n)
{long got;do {got=read(fd,buffer,n);}while(got<0 && errno==EINTR);return got;}
static int fill(void *context,int32_t *data,uint32_t frames,const struct pt_raw_format *format)
{
    struct input *in=context;uint8_t block[3072];size_t pos=0;unsigned align=format->channels*(format->bits/8);
    (void)frames;
    while(pos<in->bytes) {
        size_t n=in->bytes-pos,have=0;struct pt_pcm pcm;
        if(n>sizeof(block))n=sizeof(block); /* divisible by every supported frame width */
        while(have<n) {long got=read_retry(in->fd,block+have,n-have);if(got<=0)return 0;have+=(size_t)got;}
        pcm=(struct pt_pcm){data+(pos/align)*format->channels,(n/align)*format->channels,(uint32_t)(n/align),format->rate,format->channels,format->bits};
        if(pt_raw_decode(block,n,format,&pcm)!=PT_RAW_OK)return 0;
        pos+=n;
    }
    if(lseek(in->fd,0,SEEK_END)!=(off_t)in->length)return 0;
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
    in.length=in.bytes=(size_t)end;if(lseek(in.fd,0,SEEK_SET)!=0)goto done;
    result=pt_sampler_import_raw_fill(s,p,h,slot,in.length,name,format,fill,&in);
done:
    if(in.fd>=0)close(in.fd);
    return result;
}

static int read_at(void *context,size_t offset,uint8_t *out,size_t n)
{
    struct input *in=context;size_t have=0;
    if(offset>in->length || n>in->length-offset || lseek(in->fd,(off_t)offset,SEEK_SET)!=(off_t)offset)return 0;
    while(have<n) {long got=read_retry(in->fd,out+have,n-have);if(got<=0)return 0;have+=(size_t)got;}
    return 1;
}
int pt_wav_file_candidate(const char *path)
{
    struct input in;uint8_t header[12];int ok;
    if(!path)return 0;
    in.fd=open(path,O_RDONLY);if(in.fd<0)return 0;in.length=12;
    ok=read_at(&in,0,header,12) && !memcmp(header,"RIFF",4) && !memcmp(header+8,"WAVE",4);
    if(close(in.fd))return 0;
    return ok;
}
enum pt_edit_result pt_wav_file_import(const char *path,size_t limit,struct pt_sampler *s,struct pt_project *p,struct pt_pattern_history *h,unsigned slot,const char *name)
{
    struct input in;off_t end;struct pt_wav_info info;struct pt_raw_format format;
    enum pt_edit_result result=PT_EDIT_INVALID;
    if(!path)return result;
    in.fd=open(path,O_RDONLY);if(in.fd<0)return result;
    end=lseek(in.fd,0,SEEK_END);if(end<0)goto done;
    if((uintmax_t)end>(uintmax_t)limit) {result=PT_EDIT_CAPACITY;goto done;}
    in.length=(size_t)end;
    if(pt_wav_inspect_reader(read_at,&in,in.length,&info)!=PT_WAV_OK || !info.frames) {result=PT_EDIT_UNSUPPORTED;goto done;}
    in.bytes=info.data_bytes;
    if(lseek(in.fd,(off_t)info.data_offset,SEEK_SET)!=(off_t)info.data_offset)goto done;
    format=(struct pt_raw_format){info.rate,info.bits,info.channels,1,info.bits==8};
    result=pt_sampler_import_raw_fill(s,p,h,slot,in.bytes,name?name:"IMPORTED SAMPLE",&format,fill,&in);
done:
    if(in.fd>=0)close(in.fd);
    return result;
}

int pt_svx_file_candidate(const char *path)
{
    struct input in;uint8_t header[12];int ok;
    if(!path)return 0;
    in.fd=open(path,O_RDONLY);if(in.fd<0)return 0;in.length=12;
    ok=read_at(&in,0,header,12) && !memcmp(header,"FORM",4) && !memcmp(header+8,"8SVX",4);
    if(close(in.fd))return 0;
    return ok;
}
static int svx_fill(void *context,int32_t *data,uint32_t frames,const struct pt_raw_format *format)
{
    struct input *in=context;uint8_t extra;struct pt_pcm pcm={data,frames,frames,format->rate,1,8};
    if(pt_svx_decode_reader(read_at,in,in->length,&pcm)!=PT_SVX_OK)return 0;
    if(lseek(in->fd,0,SEEK_END)!=(off_t)in->length || read_retry(in->fd,&extra,1)!=0)return 0;
    {int rc=close(in->fd);in->fd=-1;return rc==0;}
}
enum pt_edit_result pt_svx_file_import(const char *path,size_t limit,struct pt_sampler *s,struct pt_project *p,struct pt_pattern_history *h,unsigned slot,const char *name)
{
    struct input in;off_t end;struct pt_svx_info info;enum pt_edit_result result=PT_EDIT_INVALID;
    if(!path)return result;
    in.fd=open(path,O_RDONLY);if(in.fd<0)return result;
    end=lseek(in.fd,0,SEEK_END);if(end<0)goto done;
    if((uintmax_t)end>(uintmax_t)limit) {result=PT_EDIT_CAPACITY;goto done;}
    in.length=(size_t)end;
    if(pt_svx_inspect_reader(read_at,&in,in.length,&info)!=PT_SVX_OK || !info.frames) {result=PT_EDIT_UNSUPPORTED;goto done;}
    result=pt_sampler_import_svx_fill(s,p,h,slot,&info,name,svx_fill,&in);
done:
    if(in.fd>=0)close(in.fd);
    return result;
}
