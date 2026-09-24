#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef __amigaos__
#include <proto/dos.h>
#endif
#include "render_file.h"
#include "document.h"
struct file {
    const char *path;char temporary[1536],directory[1500];int fd,owned,verifying;
    int reader;uint64_t frames;uint32_t rate;uint8_t bits;
    pt_render_progress progress;void *progress_ctx;
    uint8_t encoded[1536],check[1536],wav[44];
};
static void le16(uint8_t *b,unsigned value) {b[0]=(uint8_t)value;b[1]=(uint8_t)(value>>8);}
static void le32(uint8_t *b,uint32_t value) {le16(b,value&65535);le16(b+2,value>>16);}
static void header(uint8_t out[44],const struct pt_render_options *o,uint32_t frames)
{
    uint32_t bytes=frames*(o->bits/4);memset(out,0,44);memcpy(out,"RIFF",4);le32(out+4,bytes+36);
    memcpy(out+8,"WAVEfmt ",8);le32(out+16,16);le16(out+20,1);le16(out+22,2);
    le32(out+24,o->rate);le32(out+28,o->rate*(o->bits/4));le16(out+32,o->bits/4);le16(out+34,o->bits);
    memcpy(out+36,"data",4);le32(out+40,bytes);
}
static int begin(struct file *f)
{
    unsigned i;
    if(access(f->path,F_OK)==0)return 0;
    for(i=0;i<32;++i) {
#ifdef __amigaos__
        BPTR lock;
        snprintf(f->directory,sizeof(f->directory),"%s.pttmp-%lu-%u",f->path,(unsigned long)getpid(),i);
        lock=CreateDir((STRPTR)f->directory);
        if(!lock) {if(IoErr()==ERROR_OBJECT_EXISTS)continue;return 0;}
        UnLock(lock);f->owned=1;snprintf(f->temporary,sizeof(f->temporary),"%s/data",f->directory);
        f->fd=open(f->temporary,O_CREAT|O_TRUNC|O_WRONLY,0600);return f->fd>=0;
#else
        snprintf(f->temporary,sizeof(f->temporary),"%s.pttmp-%lu-%u",f->path,(unsigned long)getpid(),i);
        f->fd=open(f->temporary,O_CREAT|O_EXCL|O_WRONLY,0600);
        if(f->fd>=0) {f->owned=1;return 1;}
        if(errno!=EEXIST)return 0;
#endif
    }
    return 0;
}
/* No stdio read-ahead allocation: verification uses only caller-sized blocks.
 * Short reads and interrupted syscalls are normal, including at EOF. */
static long read_retry(int fd,uint8_t *data,size_t n)
{
    long count;
    do {count=(long)read(fd,data,n);} while(count<0 && errno==EINTR);
    return count;
}
static int bytes(struct file *f,const uint8_t *data,size_t n)
{
    size_t pos=0;
    if(f->verifying) {
        if(n>sizeof(f->check))return 0;
        while(pos<n) {
            long count=read_retry(f->reader,f->check+pos,n-pos);
            if(count<=0)return 0;
            pos+=(size_t)count;
        }
        return !memcmp(f->check,data,n);
    }
    while(pos<n) {
        long count=(long)write(f->fd,data+pos,n-pos);
        if(count<0 && errno==EINTR)continue;
        if(count<=0)return 0;
        pos+=(size_t)count;
    }
    return 1;
}
static int sink(void *ctx,const struct pt_pcm *p,uint64_t offset)
{
    struct file *f=ctx;uint32_t i;unsigned width=p->bits/8,j;size_t pos=0;
    if(p->channels!=2 || p->bits!=f->bits || p->rate!=f->rate || p->frames>256 || offset!=f->frames)return 0;
    for(i=0;i<p->frames*2;++i)for(j=0;j<width;++j)f->encoded[pos++]=(uint8_t)((uint32_t)p->data[i]>>(j*8));
    if(!bytes(f,f->encoded,pos))return 0;
    f->frames+=p->frames;return 1;
}
static int progress(void *ctx,enum pt_render_phase phase,uint32_t ticks,uint64_t frames)
{
    struct file *f=ctx;return !f->progress || f->progress(f->progress_ctx,f->verifying?PT_RENDER_VERIFY:phase,ticks,f->verifying?f->frames:frames);
}
static void abort_file(struct file *f)
{
    if(f->reader>=0) {close(f->reader);f->reader=-1;}
    if(f->fd>=0) {close(f->fd);f->fd=-1;}
    if(f->owned) {
        if(unlink(f->temporary) && errno!=ENOENT)fprintf(stderr,"Render staging retained: %s\n",f->temporary);
#ifdef __amigaos__
        if(!DeleteFile((STRPTR)f->directory))fprintf(stderr,"Render staging directory retained: %s\n",f->directory);
#endif
        f->owned=0;
    }
}
static enum pt_render_file_result file_new(const char *path,const struct pt_project *p,
    const struct pt_render_options *o,pt_render_progress notify,void *ctx,struct pt_render_report *out,enum pt_render_result *detail,const struct pt_allocator *allocator,struct file *f,const struct pt_render_file_engine *engine)
{
    struct pt_render_report plan,rendered,checked;
    enum pt_render_result result;enum pt_render_file_result failure=PT_RENDER_FILE_RENDER;int ok;
    if(detail)*detail=PT_RENDER_INVALID;
    if(!path || !*path || strlen(path)>1400 || !out || !detail)return PT_RENDER_FILE_INVALID;
    memset(f,0,sizeof(*f));f->path=path;f->fd=-1;f->reader=-1;f->progress=notify;f->progress_ctx=ctx;
    result=engine?engine->run(engine->context,p,o,NULL,NULL,notify,ctx,&plan,allocator):(allocator?pt_render_measure_allocated(p,o,notify,ctx,&plan,allocator):pt_render_measure(p,o,notify,ctx,&plan));*detail=result;if(result!=PT_RENDER_OK)return failure;
    if(plan.frames>(0x7fffffffUL-44)/(o->bits/4)) {*detail=PT_RENDER_FRAME_LIMIT;return failure;}
    f->bits=o->bits;f->rate=o->rate;header(f->wav,o,(uint32_t)plan.frames);
    failure=PT_RENDER_FILE_BEGIN;if(!begin(f))goto fail;
    failure=PT_RENDER_FILE_WRITE;if(!bytes(f,f->wav,sizeof(f->wav)))goto fail;
    result=engine?engine->run(engine->context,p,o,sink,f,progress,f,&rendered,allocator):(allocator?pt_render_stream_allocated(p,o,sink,f,progress,f,&rendered,allocator):pt_render_stream(p,o,sink,f,progress,f,&rendered));*detail=result;
    if(result!=PT_RENDER_OK) {if(result!=PT_RENDER_SINK)failure=PT_RENDER_FILE_RENDER;goto fail;}
    if(rendered.frames!=plan.frames || rendered.ticks!=plan.ticks || rendered.end!=plan.end) {failure=PT_RENDER_FILE_RENDER;*detail=PT_RENDER_INVALID;goto fail;}
    failure=PT_RENDER_FILE_FINISH;
#ifndef __amigaos__
    if(fsync(f->fd))goto fail;
#endif
    ok=close(f->fd);f->fd=-1;if(ok)goto fail;
    failure=PT_RENDER_FILE_VERIFY;f->verifying=1;f->frames=0;
    if(!progress(f,PT_RENDER_VERIFY,0,0)) {*detail=PT_RENDER_CANCELLED;goto fail;}
    f->reader=open(f->temporary,O_RDONLY);if(f->reader<0 || !bytes(f,f->wav,sizeof(f->wav)))goto fail;
    result=engine?engine->run(engine->context,p,o,sink,f,progress,f,&checked,allocator):(allocator?pt_render_stream_allocated(p,o,sink,f,progress,f,&checked,allocator):pt_render_stream(p,o,sink,f,progress,f,&checked));*detail=result;
    if(result!=PT_RENDER_OK || checked.frames!=rendered.frames || checked.ticks!=rendered.ticks || checked.clipped!=rendered.clipped ||
       checked.end!=rendered.end || read_retry(f->reader,f->wav,1)!=0)goto fail;
    if(!progress(f,PT_RENDER_VERIFY,checked.ticks,f->frames)) {*detail=PT_RENDER_CANCELLED;goto fail;}
    ok=close(f->reader);f->reader=-1;if(ok)goto fail;
    failure=PT_RENDER_FILE_PUBLISH;
#ifdef __amigaos__
    if(!Rename((STRPTR)f->temporary,(STRPTR)path))goto fail;
    if(!DeleteFile((STRPTR)f->directory))fprintf(stderr,"Render staging directory retained: %s\n",f->directory);
#else
    if(link(f->temporary,path))goto fail;
    if(unlink(f->temporary))fprintf(stderr,"Render staging retained: %s\n",f->temporary);
#endif
    f->owned=0;*out=rendered;return PT_RENDER_FILE_OK;
fail:
    abort_file(f);return failure;
}

enum pt_render_file_result pt_render_file_new(const char *path,const struct pt_project *p,
    const struct pt_render_options *o,pt_render_progress notify,void *ctx,struct pt_render_report *out,enum pt_render_result *detail)
{
    struct file f;return file_new(path,p,o,notify,ctx,out,detail,NULL,&f,NULL);
}
enum pt_render_file_result pt_render_file_new_allocated(const char *path,const struct pt_project *p,
    const struct pt_render_options *o,pt_render_progress notify,void *ctx,struct pt_render_report *out,
    enum pt_render_result *detail,const struct pt_allocator *allocator)
{
    struct file *f;enum pt_render_file_result result;
    if(!allocator)return pt_render_file_new(path,p,o,notify,ctx,out,detail);
    if(detail)*detail=PT_RENDER_INVALID;
    if(!path || !*path || strlen(path)>1400 || !out || !detail || !allocator->allocate || !allocator->release)return PT_RENDER_FILE_INVALID;
    f=allocator->allocate(allocator->context,sizeof(*f));
    if(!f) {*detail=PT_RENDER_MEMORY;return PT_RENDER_FILE_RENDER;}
    result=file_new(path,p,o,notify,ctx,out,detail,allocator,f,NULL);
    allocator->release(allocator->context,f);return result;
}

enum pt_render_file_result pt_render_file_engine_new(const char *path,const struct pt_project *p,
    const struct pt_render_options *o,pt_render_progress notify,void *ctx,struct pt_render_report *out,
    enum pt_render_result *detail,const struct pt_allocator *allocator,const struct pt_render_file_engine *engine)
{
    struct file *f;enum pt_render_file_result result;
    if(detail)*detail=PT_RENDER_INVALID;
    if(!path || !*path || strlen(path)>1400 || !out || !detail || !allocator ||
       !allocator->allocate || !allocator->release || !engine || !engine->run)return PT_RENDER_FILE_INVALID;
    f=allocator->allocate(allocator->context,sizeof(*f));
    if(!f){*detail=PT_RENDER_MEMORY;return PT_RENDER_FILE_RENDER;}
    result=file_new(path,p,o,notify,ctx,out,detail,allocator,f,engine);
    allocator->release(allocator->context,f);return result;
}
