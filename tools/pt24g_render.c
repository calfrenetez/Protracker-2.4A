/* Bounded reference WAV renderer. No overwrite, hardware or external MIDI. */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "document.h"
#include "../src/platform/mod_import.h"
#include "../src/platform/project_import.h"
#include "../src/platform/pp20_import.h"
#ifdef __amigaos__
#include "../src/native/master_memory.h"
#endif
#include "../src/platform/render_file.h"
#include "../src/platform/stem_file.h"
#ifndef __amigaos__
static void *allocate(void *ctx,size_t n) {(void)ctx;return malloc(n);}
static void release(void *ctx,void *p) {(void)ctx;free(p);}
#endif
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
#ifdef __amigaos__
    struct pt_master_memory memory;
    struct pt_allocator allocator={&memory,pt_master_allocate,pt_master_release};
#else
    struct pt_allocator allocator={NULL,allocate,release};
#endif
    struct pt_document doc;struct pt_render_options o;enum pt_project_result loaded;
    struct pt_render_report report;enum pt_render_result detail=PT_RENDER_INVALID;enum pt_render_file_result saved;
    uint32_t value;int i,rc=20,tracks_given=0,stems=0,grouped=0;
#ifdef __amigaos__
    pt_master_memory_init(&memory);
#endif
    pt_document_init(&doc,&allocator);memset(&o,0,sizeof(o));o.rate=48000;o.bits=24;o.gain_q16=32768;o.tick_limit=1000000;
    if(argc<3)goto usage;
    for(i=3;i<argc;++i) {
        if(!strcmp(argv[i],"--stems")) {stems=1;continue;}
        if(!strcmp(argv[i],"--groups")) {stems=grouped=1;continue;}
        if(!strcmp(argv[i],"--lead-in")) {o.include_lead_in=1;continue;}
        if(i+1>=argc || !number(argv[i+1],!strcmp(argv[i],"--tracks")?16:10,&value))goto usage;
        if(!strcmp(argv[i],"--pattern") && value<256) {o.pattern_only=1;o.pattern=(uint16_t)value;}
        else if(!strcmp(argv[i],"--from-row") && value<64) {o.row_range=1;o.row_first=(uint8_t)value;if(!o.row_end)o.row_end=64;}
        else if(!strcmp(argv[i],"--to-row") && value>0 && value<=64) {o.row_range=1;o.row_end=(uint8_t)value;}
        else if(!strcmp(argv[i],"--rate") && (value==44100 || value==48000))o.rate=value;
        else if(!strcmp(argv[i],"--bits") && (value==16 || value==24))o.bits=(uint8_t)value;
        else if(!strcmp(argv[i],"--gain") && value<=65536)o.gain_q16=value;
        else if(!strcmp(argv[i],"--tracks") && value && value<=65535) {o.tracks=(uint16_t)value;tracks_given=1;}
        else goto usage;
        ++i;
    }
    o.frame_limit=(uint64_t)o.rate*60*30; /* Includes internal startup lead-in. */
    if(pt_project_file_candidate(argv[1]))loaded=pt_project_file_load(&doc,argv[1],64UL*1024*1024,64UL*1024*1024);
    else if(pt_mod_file_candidate(argv[1]))loaded=pt_mod_file_load(&doc,argv[1],64UL*1024*1024,64UL*1024*1024);
    else if(pt_pp20_file_candidate(argv[1]))loaded=pt_pp20_file_load(&doc,argv[1],64UL*1024*1024,64UL*1024*1024);
    else loaded=PT_PROJECT_UNSUPPORTED;
    if(loaded!=PT_PROJECT_OK) {fprintf(stderr,"Input invalid, unsupported or above document memory budget result=%d\n",loaded);goto done;}
    if(!tracks_given)o.tracks=(uint16_t)((1UL<<doc.project.channels.count)-1);
    if(stems) {
        struct pt_stem_report batch;unsigned n;
        saved=pt_stem_file_new_allocated(argv[2],&doc.project,&o,(unsigned)grouped,NULL,NULL,&batch,&detail,&allocator);
        if(saved!=PT_RENDER_FILE_OK) {fprintf(stderr,"Stem export refused save_phase=%d render_result=%d; no destination replaced\n",saved,detail);goto done;}
        for(n=0;n<batch.plan.count;++n)printf("STEM %s-%02u.wav tracks=%04x frames=%lu clipped_values=%lu\n",
            batch.plan.item[n].group?"group":"track",batch.plan.item[n].group?batch.plan.item[n].group:batch.plan.item[n].channel+1,
            batch.plan.item[n].tracks,(unsigned long)batch.audio[n].frames,(unsigned long)batch.audio[n].clipped);
        printf("STEMS count=%u verified=1 new_directory=1\n",batch.plan.count);rc=0;goto done;
    }
    saved=pt_render_file_new_allocated(argv[2],&doc.project,&o,NULL,NULL,&report,&detail,&allocator);
    if(saved!=PT_RENDER_FILE_OK) {fprintf(stderr,"Render refused save_phase=%d render_result=%d; no destination replaced\n",saved,detail);goto done;}
    printf("RENDERED profile=IDEAL_BPM_Q32 pitch=PCM_RATE_PERIOD428 rate=%lu bits=%u tracks=%04x gain_q16=%lu lead_in=%u\n",
           (unsigned long)o.rate,o.bits,o.tracks,(unsigned long)o.gain_q16,o.include_lead_in);
    printf("WAV frames=%lu ticks=%lu clipped_values=%lu end=%s verified=1 new_file=1\n",(unsigned long)report.frames,
           (unsigned long)report.ticks,(unsigned long)report.clipped,report.end==PT_RENDER_F00?"F00":report.end==PT_RENDER_ROW_EXIT?"row-range-exit":"first-position-return");rc=0;goto done;
usage:
    fprintf(stderr,"Usage: PT24GRender INPUT NEW.wav [--pattern N] [--rate 44100|48000] [--bits 16|24] [--tracks HEX] [--gain 0..65536] [--lead-in] [--stems|--groups] [--from-row N --to-row N]\n");
    fprintf(stderr,"Reference renderer; bounded classic-effect subset including finetune/E5. Instrument-only events and MIDI audio unsupported. See docs/REFERENCE_RENDERER.md. Default gain32768, 30-minute bound.\n");
done:
    pt_document_release(&doc);return rc;
}
