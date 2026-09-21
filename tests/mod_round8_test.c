#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "document.h"
#include "mod_project.h"
static void *allocate(void *c,size_t n){(void)c;return malloc(n);}
static void release(void *c,void *p){(void)c;free(p);}
int main(int argc,char **argv)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d,reopened;
    struct pt_mod_export_report r;struct pt_pcm original;FILE *f;long n;
    uint8_t *input,*out,*before;size_t bytes,w=99;unsigned bits,i;
    int32_t pcm[16],saved[16];
    static const int32_t values[16]={-32768,-32767,-32640,-384,-383,-128,-127,0,127,128,129,383,384,32512,32639,32767};
    static const int8_t expected[16]={-128,-128,-128,-2,-1,-1,0,0,0,1,1,1,2,127,127,127};
    assert(argc==2 || argc==3);f=fopen(argv[1],"rb");assert(f && !fseek(f,0,SEEK_END));n=ftell(f);assert(n>0);rewind(f);
    input=malloc(n);assert(input && fread(input,1,n,f)==(size_t)n && !fclose(f));
    pt_document_init(&d,&a);pt_document_init(&reopened,&a);assert(pt_document_load(&d,input,n,SIZE_MAX)==PT_PROJECT_OK);
    original=d.project.samples[0].pcm;d.project.samples[0].loop=PT_LOOP_NONE;d.project.samples[0].loop_start=d.project.samples[0].loop_end=0;
    /* Remove legacy header extension so old loop metadata cannot affect this fixture. */
    d.project.extension_count=0;
    for(bits=16;bits<=24;bits+=8) {
        for(i=0;i<16;i++)pcm[i]=values[i]*(bits==24?256:1);
        if(bits==24) {pcm[6]=-32767;pcm[8]=32767;pcm[10]=32769;pcm[15]=8388607;}
        memcpy(saved,pcm,sizeof(pcm));d.project.samples[0].pcm=(struct pt_pcm){pcm,16,16,PT_CLASSIC_RATE,1,(uint8_t)bits};
        assert(pt_mod_export_analyse(&d.project,&r)==PT_PROJECT_OK && r.issues==PT_EXPORT_PRECISION && !r.bytes);
        assert(pt_mod_export_analyse_round8(&d.project,&r)==PT_PROJECT_OK && r.issues==PT_EXPORT_PRECISION && r.classification==PT_CONVERSION_CONVERTED && r.bytes);
        bytes=r.bytes;out=malloc(bytes);before=malloc(bytes);assert(out && before);memset(out,0xa5,bytes);memcpy(before,out,bytes);
        assert(pt_mod_export_direct(&d.project,out,bytes,&w)==PT_PROJECT_UNSUPPORTED && w==99 && !memcmp(out,before,bytes));
        assert(pt_mod_export_round8(&d.project,out,bytes-1,&w)==PT_PROJECT_CAPACITY && w==99 && !memcmp(out,before,bytes));
        d.project.samples[0].pcm.rate++;
        assert(pt_mod_export_round8(&d.project,out,bytes,&w)==PT_PROJECT_UNSUPPORTED && w==99 && !memcmp(out,before,bytes));
        d.project.samples[0].pcm.rate--;
        assert(pt_mod_export_round8(&d.project,(uint8_t *)pcm,bytes,&w)==PT_PROJECT_ALIAS && w==99 && !memcmp(pcm,saved,sizeof(pcm)));
        assert(pt_mod_export_round8(&d.project,out,bytes,&w)==PT_PROJECT_OK && w==bytes && !memcmp(pcm,saved,sizeof(pcm)));
        assert(pt_document_load(&reopened,out,bytes,SIZE_MAX)==PT_PROJECT_OK);
        assert(reopened.project.samples[0].pcm.bits==8 && reopened.project.samples[0].pcm.frames==16);
        for(i=0;i<16;i++)assert(reopened.project.samples[0].pcm.data[i]==expected[i]);
        assert(pt_mod_export_analyse_round8(&reopened.project,&r)==PT_PROJECT_OK && !r.issues && r.classification==PT_CONVERSION_LOSSLESS);
        assert(pt_mod_export_round8(&reopened.project,before,bytes,&w)==PT_PROJECT_OK && !memcmp(out,before,bytes));
        if(argc==3 && bits==24) {
            char path[1024];uint8_t *project;size_t size;
            assert(pt_project_size(&d.project,&size)==PT_PROJECT_OK);project=malloc(size);assert(project);
            assert(pt_project_encode(&d.project,project,size,&w)==PT_PROJECT_OK);
            snprintf(path,sizeof(path),"%s/high.ptg",argv[2]);f=fopen(path,"wb");assert(f && fwrite(project,1,size,f)==size && !fclose(f));free(project);
            snprintf(path,sizeof(path),"%s/expected.mod",argv[2]);f=fopen(path,"wb");assert(f && fwrite(out,1,bytes,f)==bytes && !fclose(f));
        }
        free(before);free(out);w=99;
    }
    d.project.samples[0].pcm=original;pt_document_release(&reopened);pt_document_release(&d);free(input);
    puts("MOD ROUND8 PASS: 16/24-bit signed ties and saturation, exact reopen, source preservation, strict/rate/capacity/alias refusal, 8-bit identity");return 0;
}
