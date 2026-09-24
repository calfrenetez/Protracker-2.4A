#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef __amigaos__
#include <proto/dos.h>
#endif
#include <stdint.h>
#include "file_save.h"
#include "document.h"

struct output_file {const char *destination;char temporary[1536],directory[1500];int fd,owned;uint8_t buffer[4096];};
static int begin(void *ctx)
{
    struct output_file *f=ctx;unsigned i;size_t n=strlen(f->destination);
    if(n>1400)return 0;
    for(i=0;i<32;++i) {
#ifdef __amigaos__
        BPTR lock;
        if(snprintf(f->directory,sizeof(f->directory),"%s.pttmp-%lu-%u",f->destination,(unsigned long)getpid(),i)<0)return 0;
        lock=CreateDir((STRPTR)f->directory);
        if(!lock) {
            if(IoErr()==ERROR_OBJECT_EXISTS)continue;
            return 0;
        }
        UnLock(lock);f->owned=1;
        if(snprintf(f->temporary,sizeof(f->temporary),"%s/data",f->directory)<0)return 0;
        f->fd=open(f->temporary,O_CREAT|O_TRUNC|O_WRONLY,0600);
        return f->fd>=0;
#else
        if(snprintf(f->temporary,sizeof(f->temporary),"%s.pttmp-%lu-%u",f->destination,(unsigned long)getpid(),i)<0)return 0;
        f->fd=open(f->temporary,O_CREAT|O_EXCL|O_WRONLY,0600);
        if(f->fd>=0) {f->owned=1;return 1;}
        if(errno!=EEXIST)return 0;
#endif
    }
    return 0;
}
static size_t write_bytes(void *ctx,const void *p,size_t n)
{
    struct output_file *f=ctx;long done;
    if(n>32768)n=32768;
    done=(long)write(f->fd,p,n);return done>0?(size_t)done:0;
}
static int finish(void *ctx)
{
    struct output_file *f=ctx;int rc;
#ifndef __amigaos__
    if(fsync(f->fd))return 0;
#endif
    rc=close(f->fd);f->fd=-1;return rc==0;
}
static long read_retry(int fd,void *buffer,size_t n)
{
    long count;do {count=(long)read(fd,buffer,n);} while(count<0 && errno==EINTR);return count;
}
static int verify(void *ctx,const void *data,size_t n)
{
    struct output_file *f=ctx;int fd=open(f->temporary,O_RDONLY);
    const uint8_t *bytes=data;size_t pos=0;int ok=1;
    if(fd<0)return 0;
    while(pos<n) {
        size_t count=n-pos,have=0;if(count>sizeof(f->buffer))count=sizeof(f->buffer);
        while(have<count) {
            long got=read_retry(fd,f->buffer+have,count-have);
            if(got<=0) {ok=0;break;}have+=(size_t)got;
        }
        if(!ok || memcmp(bytes+pos,f->buffer,count)) {ok=0;break;}
        pos+=count;
    }
    if(ok && read_retry(fd,f->buffer,1)!=0)ok=0;
    if(close(fd))ok=0;
    return ok;
}
static int publish(void *ctx)
{
    struct output_file *f=ctx;
#ifdef __amigaos__
    /* DOS Rename fails if the destination already exists. Never delete it. */
    if(!Rename((STRPTR)f->temporary,(STRPTR)f->destination))return 0;
    if(!DeleteFile((STRPTR)f->directory))fprintf(stderr,"Staging directory retained: %s\n",f->directory);
#else
    /* Same-filesystem link gives an atomic no-replace destination on POSIX. */
    if(link(f->temporary,f->destination))return 0;
    /* Publication is already complete if temp cleanup fails. */
    if(unlink(f->temporary))fprintf(stderr,"Temporary file retained: %s\n",f->temporary);
#endif
    f->owned=0;return 1;
}
static void abort_output(void *ctx)
{
    struct output_file *f=ctx;
    if(f->fd>=0) {close(f->fd);f->fd=-1;}
    if(f->owned) {
        if(unlink(f->temporary) && errno!=ENOENT)fprintf(stderr,"Temporary file retained: %s\n",f->temporary);
#ifdef __amigaos__
        if(!DeleteFile((STRPTR)f->directory))fprintf(stderr,"Staging directory retained: %s\n",f->directory);
#endif
        f->owned=0;
    }
}
static enum pt_save_result save_new(const char *path,const void *bytes,size_t n,struct output_file *f)
{
    struct pt_save_ops ops;
    if(!path || !*path || (!bytes && n))return PT_SAVE_BEGIN;
    memset(f,0,sizeof(*f));f->destination=path;f->fd=-1;
    ops.context=f;ops.begin=begin;ops.write=write_bytes;ops.finish=finish;
    ops.verify=verify;ops.publish=publish;ops.abort=abort_output;
    return pt_safe_save(&ops,bytes,n);
}

enum pt_save_result pt_file_save_new(const char *path,const void *bytes,size_t n)
{
    struct output_file f;return save_new(path,bytes,n,&f);
}
enum pt_save_result pt_file_save_new_allocated(const char *path,const void *bytes,size_t n,const struct pt_allocator *a)
{
    struct output_file *f;enum pt_save_result result;
    if(!a)return pt_file_save_new(path,bytes,n);
    if(!path || !*path || strlen(path)>1400 || (!bytes && n) || !a->allocate || !a->release)return PT_SAVE_INVALID;
    f=a->allocate(a->context,sizeof(*f));if(!f)return PT_SAVE_MEMORY;
    result=save_new(path,bytes,n,f);a->release(a->context,f);return result;
}

struct generated_file {struct output_file file;uint8_t expected[4096];};
enum pt_save_result pt_file_save_generated(const char *path,size_t total,
    pt_file_generate generate,void *context,const struct pt_allocator *a)
{
    struct generated_file *g;struct output_file *f;size_t pos,n,have;
    enum pt_save_result result;int reader=-1;
    if(!path || !*path || strlen(path)>1400 || !generate || !a || !a->allocate || !a->release)
        return PT_SAVE_INVALID;
    g=a->allocate(a->context,sizeof(*g));if(!g)return PT_SAVE_MEMORY;
    memset(g,0,sizeof(*g));f=&g->file;f->destination=path;f->fd=-1;
    result=PT_SAVE_BEGIN;if(!begin(f))goto done;
    result=PT_SAVE_WRITE;
    for(pos=0;pos<total;pos+=n) {
        n=total-pos;if(n>sizeof(g->expected))n=sizeof(g->expected);
        if(generate(context,pos,g->expected,n)!=1)goto done;
        for(have=0;have<n;) {
            size_t written=write_bytes(f,g->expected+have,n-have);
            if(!written)goto done;
            have+=written;
        }
    }
    result=PT_SAVE_FINISH;if(!finish(f))goto done;
    result=PT_SAVE_VERIFY;reader=open(f->temporary,O_RDONLY);if(reader<0)goto done;
    for(pos=0;pos<total;pos+=n) {
        n=total-pos;if(n>sizeof(g->expected))n=sizeof(g->expected);
        if(generate(context,pos,g->expected,n)!=1)goto done;
        for(have=0;have<n;) {
            long count=read_retry(reader,f->buffer+have,n-have);
            if(count<=0)goto done;
            have+=(size_t)count;
        }
        if(memcmp(f->buffer,g->expected,n))goto done;
    }
    if(read_retry(reader,f->buffer,1)!=0)goto done;
    {int rc=close(reader);reader=-1;if(rc)goto done;}
    result=PT_SAVE_PUBLISH;if(!publish(f))goto done;
    result=PT_SAVE_OK;
done:
    if(reader>=0)close(reader);
    abort_output(f);a->release(a->context,g);return result;
}
