/* Bounded reference WAV renderer. No overwrite, hardware or external MIDI. */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "document.h"
#include "../src/platform/render_file.h"
#include "../src/platform/stem_file.h"
static void *allocate(void *ctx,size_t n) {(void)ctx;return malloc(n);}
static void release(void *ctx,void *p) {(void)ctx;free(p);}
static int number(const char *s,unsigned base,uint32_t *out)
{
    char *end;unsigned long n;if(!s || !*s || *s=='-')return 0;errno=0;n=strtoul(s,&end,base);if(errno || *end)return 0;
#if ULONG_MAX > UINT32_MAX
    if(n>UINT32_MAX)return 0;
#endif
    *out=(uint32_t)n;return 1;
}
int main(int argc,char **argv)
{
    struct pt_allocator allocator={NULL,allocate,release};struct pt_document doc;struct pt_render_options o;
    struct pt_render_report report;enum pt_render_result detail=PT_RENDER_INVALID;enum pt_render_file_result saved;
    FILE *f=NULL;uint8_t *input=NULL;long size;uint32_t value;int i,rc=20,tracks_given=0,stems=0,grouped=0;
    pt_document_init(&doc,&allocator);memset(&o,0,sizeof(o));o.rate=48000;o.bits=24;o.gain_q16=32768;o.tick_limit=1000000;
    if(argc<3)goto usage;
    for(i=3;i<argc;++i) {
        if(!strcmp(argv[i],"--stems")) {stems=1;continue;}
        if(!strcmp(argv[i],"--groups")) {stems=grouped=1;continue;}
        if(!strcmp(argv[i],"--lead-in")) {o.include_lead_in=1;continue;}
        if(i+1>=argc || !number(argv[i+1],!strcmp(argv[i],"--tracks")?16:10,&value))goto usage;
        if(!strcmp(argv[i],"--pattern") && value<256) {o.pattern_only=1;o.pattern=(uint16_t)value;}
        else if(!strcmp(argv[i],"--rate") && (value==44100 || value==48000))o.rate=value;
        else if(!strcmp(argv[i],"--bits") && (value==16 || value==24))o.bits=(uint8_t)value;
        else if(!strcmp(argv[i],"--gain") && value<=65536)o.gain_q16=value;
        else if(!strcmp(argv[i],"--tracks") && value && value<=65535) {o.tracks=(uint16_t)value;tracks_given=1;}
        else goto usage;
        ++i;
    }
    o.frame_limit=(uint64_t)o.rate*60*30; /* Includes internal startup lead-in. */
    f=fopen(argv[1],"rb");if(!f) {fprintf(stderr,"Cannot open input\n");goto done;}
    if(fseek(f,0,SEEK_END) || (size=ftell(f))<0 || size>64L*1024*1024) {fprintf(stderr,"Input exceeds 64 MiB limit or cannot be read\n");goto done;}
    rewind(f);input=malloc(size?(size_t)size:1);if(!input || fread(input,1,(size_t)size,f)!=(size_t)size)goto done;
    if(fclose(f)) {f=NULL;goto done;}f=NULL;
    if(pt_document_load(&doc,input,(size_t)size,64UL*1024*1024)!=PT_PROJECT_OK) {fprintf(stderr,"Input invalid, unsupported or above document memory budget\n");goto done;}
    free(input);input=NULL;
    if(!tracks_given)o.tracks=(uint16_t)((1UL<<doc.project.channels.count)-1);
    if(stems) {
        struct pt_stem_report batch;unsigned n;
        saved=pt_stem_file_new(argv[2],&doc.project,&o,(unsigned)grouped,NULL,NULL,&batch,&detail);
        if(saved!=PT_RENDER_FILE_OK) {fprintf(stderr,"Stem export refused save_phase=%d render_result=%d; no destination replaced\n",saved,detail);goto done;}
        for(n=0;n<batch.plan.count;++n)printf("STEM %s-%02u.wav tracks=%04x frames=%lu clipped_values=%lu\n",
            batch.plan.item[n].group?"group":"track",batch.plan.item[n].group?batch.plan.item[n].group:batch.plan.item[n].channel+1,
            batch.plan.item[n].tracks,(unsigned long)batch.audio[n].frames,(unsigned long)batch.audio[n].clipped);
        printf("STEMS count=%u verified=1 new_directory=1\n",batch.plan.count);rc=0;goto done;
    }
    saved=pt_render_file_new(argv[2],&doc.project,&o,NULL,NULL,&report,&detail);
    if(saved!=PT_RENDER_FILE_OK) {fprintf(stderr,"Render refused save_phase=%d render_result=%d; no destination replaced\n",saved,detail);goto done;}
    printf("RENDERED profile=IDEAL_BPM_Q32 pitch=PCM_RATE_PERIOD428 rate=%lu bits=%u tracks=%04x gain_q16=%lu lead_in=%u\n",
           (unsigned long)o.rate,o.bits,o.tracks,(unsigned long)o.gain_q16,o.include_lead_in);
    printf("WAV frames=%lu ticks=%lu clipped_values=%lu end=%s verified=1 new_file=1\n",(unsigned long)report.frames,
           (unsigned long)report.ticks,(unsigned long)report.clipped,report.end==PT_RENDER_F00?"F00":"first-position-return");rc=0;goto done;
usage:
    fprintf(stderr,"Usage: PT24GRender INPUT NEW.wav [--pattern N] [--rate 44100|48000] [--bits 16|24] [--tracks HEX] [--gain 0..65536] [--lead-in] [--stems|--groups]\n");
    fprintf(stderr,"Reference renderer; bounded classic-effect subset including finetune/E5. Instrument-only events and MIDI audio unsupported. See docs/REFERENCE_RENDERER.md. Default gain32768, 30-minute bound.\n");
done:
    if(f)fclose(f);
    free(input);pt_document_release(&doc);return rc;
}
