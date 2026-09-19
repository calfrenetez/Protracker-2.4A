/* Shared-core native/host utility. All output is new-file publication: existing
 * files are never replaced. The editor's future overwrite/recovery adapters
 * must meet the same verified-staging contract with platform-specific replace.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef __amigaos__
#include <proto/dos.h>
#endif
#include "document.h"
#include "mod_project.h"
#include "safe_save.h"

struct output_file {const char *destination;char temporary[1536],directory[1500];int fd,owned;};
static void *allocate(void *ctx,size_t n) {(void)ctx;return malloc(n);}
static void release(void *ctx,void *p) {(void)ctx;free(p);}
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
static const char *classification(enum pt_conversion_class c)
{
    switch(c) {case PT_CONVERSION_LOSSLESS:return "LOSSLESS";case PT_CONVERSION_CONVERTED:return "CONVERTED";
        case PT_CONVERSION_BOUNCED:return "BOUNCED";default:return "INCOMPLETE";}
}
static void print_report(const struct pt_project *p,const struct pt_mod_export_report *r)
{
    static const struct {uint32_t bit;const char *name;} reasons[]={
        {PT_EXPORT_CHANNELS,"more-than-four-channels"},{PT_EXPORT_ROUTING,"non-Paula-routes"},
        {PT_EXPORT_PRECISION,"sample-precision"},{PT_EXPORT_STEREO,"stereo-samples"},{PT_EXPORT_RATE,"sample-rate"},
        {PT_EXPORT_SLICES,"sample-slices"},{PT_EXPORT_LOOPS,"enhanced-loops"},{PT_EXPORT_MIDI_AUDIO,"external-MIDI-audio-missing"},
        {PT_EXPORT_OFF,"explicit-OFF-events"},{PT_EXPORT_LIMITS,"classic-format-limits"},{PT_EXPORT_PANNING,"enhanced-panning"},
        {PT_EXPORT_METADATA,"enhanced-metadata"},{PT_EXPORT_VELOCITY,"explicit-velocity"},{PT_EXPORT_NOTES,"MIDI-note-values"},
        {PT_EXPORT_TEMPO,"initial-speed-or-tempo"}};
    unsigned i;
    printf("PROJECT channels=%u patterns=%u orders=%u samples=%u\n",p->channels.count,p->pattern_count,p->order_count,p->sample_count);
    printf("MOD_ANALYSIS required_strategy=%s issues=0x%lx direct_bytes=%lu\n",classification(r->classification),(unsigned long)r->issues,(unsigned long)r->bytes);
    for(i=0;i<sizeof(reasons)/sizeof(reasons[0]);++i)if(r->issues&reasons[i].bit)printf("REQUIRES %s\n",reasons[i].name);
}
int main(int argc,char **argv)
{
    struct pt_allocator allocator={NULL,allocate,release};struct pt_document d;struct pt_mod_export_report report;
    uint8_t *input=NULL,*output=NULL;FILE *file=NULL;long input_size;size_t n=0,w=0;int rc=20,mode;
    enum pt_project_result result;struct output_file destination;struct pt_save_ops ops;
    pt_document_init(&d,&allocator);
    if(argc<3 || argc>4 || (strcmp(argv[1],"inspect") && strcmp(argv[1],"project") && strcmp(argv[1],"mod"))) {
        fprintf(stderr,"Usage: PT24GConvert inspect INPUT | project INPUT NEW_OUTPUT | mod INPUT NEW_OUTPUT\n");goto done;
    }
    mode=!strcmp(argv[1],"inspect")?0:!strcmp(argv[1],"project")?1:2;
    if((mode==0 && argc!=3) || (mode && argc!=4)) {fprintf(stderr,"Wrong argument count\n");goto done;}
    file=fopen(argv[2],"rb");if(!file) {fprintf(stderr,"Cannot open input\n");goto done;}
    if(fseek(file,0,SEEK_END) || (input_size=ftell(file))<0 || input_size>64L*1024*1024) {
        fprintf(stderr,"Input size invalid or above this utility's 64 MiB file limit\n");goto done;
    }
    rewind(file);input=malloc(input_size?(size_t)input_size:1);if(!input)goto done;
    if(fread(input,1,(size_t)input_size,file)!=(size_t)input_size)goto done;
    if(fclose(file)) {file=NULL;goto done;}file=NULL;
    result=pt_document_load(&d,input,(size_t)input_size,SIZE_MAX);
    if(result!=PT_PROJECT_OK) {fprintf(stderr,"Input rejected result=%d; no output created\n",result);goto done;}
    result=pt_mod_export_analyse(&d.project,&report);if(result!=PT_PROJECT_OK)goto done;print_report(&d.project,&report);
    if(!mode) {rc=0;goto done;}
    if(mode==1)result=pt_project_size(&d.project,&n);
    else {result=report.issues?PT_PROJECT_UNSUPPORTED:PT_PROJECT_OK;n=report.bytes;}
    if(result!=PT_PROJECT_OK) {fprintf(stderr,"Direct MOD export refused; transformations require explicit policy and implementation\n");goto done;}
    output=malloc(n);if(!output)goto done;
    result=mode==1?pt_project_encode(&d.project,output,n,&w):pt_mod_export_direct(&d.project,output,n,&w);
    if(result!=PT_PROJECT_OK || w!=n)goto done;
    memset(&destination,0,sizeof(destination));destination.destination=argv[3];destination.fd=-1;
    ops.context=&destination;ops.begin=begin;ops.write=write_bytes;ops.finish=finish;ops.verify=verify;ops.publish=publish;ops.abort=abort_output;
    {
        enum pt_save_result saved=pt_safe_save(&ops,output,n);
        if(saved!=PT_SAVE_OK) {fprintf(stderr,"Save failed phase=%d; destination was not replaced\n",saved);goto done;}
    }
    printf("SAVED format=%s bytes=%lu verified=1 new_file=1\n",mode==1?"PT24G-v1":"MOD",(unsigned long)n);rc=0;
 done:
    if(file)fclose(file);
    free(input);free(output);pt_document_release(&d);return rc;
}
