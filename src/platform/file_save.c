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

struct output_file {const char *destination;char temporary[1536],directory[1500];int fd,owned;};
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
static int verify(void *ctx,const void *data,size_t n)
{
    struct output_file *f=ctx;FILE *file=fopen(f->temporary,"rb");uint8_t buffer[4096];
    const uint8_t *bytes=data;size_t pos=0;int ok=1;
    if(!file)return 0;
    while(pos<n) {
        size_t count=n-pos;if(count>sizeof(buffer))count=sizeof(buffer);
        if(fread(buffer,1,count,file)!=count || memcmp(bytes+pos,buffer,count)) {ok=0;break;}
        pos+=count;
    }
    if(ok && (fgetc(file)!=EOF || ferror(file)))ok=0;
    if(fclose(file))ok=0;
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
enum pt_save_result pt_file_save_new(const char *path,const void *bytes,size_t n)
{
    struct output_file f;struct pt_save_ops ops;
    if(!path || !*path || (!bytes && n))return PT_SAVE_BEGIN;
    memset(&f,0,sizeof(f));f.destination=path;f.fd=-1;
    ops.context=&f;ops.begin=begin;ops.write=write_bytes;ops.finish=finish;
    ops.verify=verify;ops.publish=publish;ops.abort=abort_output;
    return pt_safe_save(&ops,bytes,n);
}
