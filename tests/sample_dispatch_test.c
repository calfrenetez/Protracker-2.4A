#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/platform/sample_import.h"
#include "wav.h"
static unsigned calls,live;
static void *allocate(void *context,size_t n)
{void *p;(void)context;++calls;p=malloc(n);if(p)++live;return p;}
static void release(void *context,void *p)
{(void)context;if(p){assert(live);--live;free(p);}}
static void write_file(const char *path,const void *data,size_t n)
{FILE *f=fopen(path,"wb");assert(f && fwrite(data,1,n,f)==n && !fclose(f));}
int main(int argc,char **argv)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d;
    struct pt_editor *e;FILE *f;unsigned before;int preview;uint8_t wav[80];size_t n;
    int32_t values[]={-8388607,8388606,1234567,-654321};
    struct pt_pcm pcm={values,4,2,44100,2,24};
    assert(argc==2);pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    e=malloc(sizeof(*e));assert(e && pt_editor_init(e,&d.project));e->sampler.allocator=a;e->sample=1;e->panel=5;
    /* A large unsupported input must not allocate, edit or create undo state. */
    f=fopen(argv[1],"wb");assert(f && !fseek(f,1024L*1024-1,SEEK_SET) && fputc(0,f)==0 && !fclose(f));
    before=calls;preview=9;
    assert(pt_editor_sample_file_import(e,argv[1],0,0,&preview)==PT_EDIT_UNSUPPORTED);
    assert(calls==before && !preview && !e->history.revision && !d.project.samples[0].pcm.frames);
    assert(pt_editor_sample_file_import(e,argv[1],0,1,&preview)==PT_EDIT_UNSUPPORTED && calls==before);
    assert(pt_wav_encode(&pcm,wav,sizeof(wav),&n)==PT_WAV_OK);write_file(argv[1],wav,n);
    /* Donor-only selection cannot accidentally import a WAV. */
    assert(pt_editor_sample_file_import(e,argv[1],0,1,&preview)==PT_EDIT_UNSUPPORTED && calls==before);
    assert(pt_editor_sample_file_import(e,argv[1],0,0,&preview)==PT_EDIT_OK && !preview);
    assert(e->history.revision==1 && d.project.samples[0].pcm.bits==24 && d.project.samples[0].pcm.channels==2);
    assert(!memcmp(d.project.samples[0].pcm.data,values,sizeof(values)));
    pt_editor_key(e,0x31,8);assert(!e->history.revision && !d.project.samples[0].pcm.frames);
    write_file(argv[1],"invalid sample",14);before=calls;
    assert(pt_editor_sample_file_import(e,argv[1],0,0,&preview)==PT_EDIT_UNSUPPORTED && calls==before);
    pt_editor_key(e,0x31,9);assert(e->history.revision==1 && !memcmp(d.project.samples[0].pcm.data,values,sizeof(values)));
    pt_editor_dispose(e);free(e);pt_document_release(&d);assert(!live && !remove(argv[1]));
    puts("SAMPLE DISPATCH PASS: unsupported input without allocation, donor-only refusal, exact24-bit master and retained redo");return 0;
}
