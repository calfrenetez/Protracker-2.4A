#include "mod_import.h"
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
struct input {int fd;size_t length;};
static long read_retry(int fd,void *data,size_t n)
{long got;do {got=read(fd,data,n);}while(got<0 && errno==EINTR);return got;}
static int read_at(void *context,size_t pos,uint8_t *out,size_t n)
{
    struct input *in=context;size_t have=0;
    if(pos>in->length || n>in->length-pos || lseek(in->fd,(off_t)pos,SEEK_SET)!=(off_t)pos)return 0;
    while(have<n) {long got=read_retry(in->fd,out+have,n-have);if(got<=0)return 0;have+=(size_t)got;}
    return 1;
}
static int finish(void *context)
{
    struct input *in=context;uint8_t extra;int rc;
    if(lseek(in->fd,0,SEEK_END)!=(off_t)in->length || read_retry(in->fd,&extra,1)!=0)return 0;
    rc=close(in->fd);in->fd=-1;return rc==0;
}
int pt_mod_file_candidate(const char *path)
{
    struct input in;uint8_t tag[8];int ok;
    if(!path)return 0;
    in.fd=open(path,O_RDONLY);if(in.fd<0)return 0;in.length=1084;
    if(!read_at(&in,0,tag,8) || !memcmp(tag,"PT24G\r\n\032",8) || !memcmp(tag,"PP20",4) || !memcmp(tag,"PX20",4)) {close(in.fd);return 0;}
    ok=read_at(&in,1080,tag,4) && tag[0]=='M' && tag[2]=='K' &&
       ((tag[1]=='.' && tag[3]=='.') || (tag[1]=='!' && tag[3]=='!'));
    if(close(in.fd))return 0;
    return ok;
}
enum pt_project_result pt_mod_file_load(struct pt_document *d,const char *path,size_t limit,size_t budget)
{
    struct input in;off_t end;enum pt_project_result r=PT_PROJECT_INVALID;
    if(!path)return r;
    in.fd=open(path,O_RDONLY);if(in.fd<0)return r;
    end=lseek(in.fd,0,SEEK_END);if(end<0)goto done;
    if((uintmax_t)end>(uintmax_t)limit) {r=PT_PROJECT_CAPACITY;goto done;}
    in.length=(size_t)end;r=pt_document_load_mod_reader(d,read_at,&in,in.length,budget,finish);
done:
    if(in.fd>=0)close(in.fd);
    return r;
}
