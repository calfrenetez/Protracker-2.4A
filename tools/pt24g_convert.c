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
#include "../src/native/master_memory.h"
#endif
#include "document.h"
#include "mod_project.h"
#include "safe_save.h"

#include "../src/platform/file_load.h"
#include "../src/platform/project_file.h"
#include "../src/platform/mod_file.h"
#include "../src/platform/mod_import.h"
#ifndef __amigaos__
static void *allocate(void *ctx,size_t n) {(void)ctx;return malloc(n);}
static void release(void *ctx,void *p) {(void)ctx;free(p);}
#endif
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
#ifdef __amigaos__
    struct pt_master_memory memory;
    struct pt_allocator allocator={&memory,pt_master_allocate,pt_master_release};
#else
    struct pt_allocator allocator={NULL,allocate,release};
#endif
    struct pt_document d;struct pt_mod_export_report report;
    uint8_t *input=NULL;size_t input_size,n=0;int rc=20,mode;
    enum pt_project_result result;
#ifdef __amigaos__
    pt_master_memory_init(&memory);
#endif
    pt_document_init(&d,&allocator);
    if(argc<3 || argc>4 || (strcmp(argv[1],"inspect") && strcmp(argv[1],"project") && strcmp(argv[1],"mod") && strcmp(argv[1],"mod8") && strcmp(argv[1],"mod8tpdf"))) {
        fprintf(stderr,"Usage: PT24GConvert inspect INPUT | project INPUT NEW_OUTPUT | mod INPUT NEW_OUTPUT | mod8 INPUT NEW_OUTPUT | mod8tpdf INPUT NEW_OUTPUT\n");goto done;
    }
    mode=!strcmp(argv[1],"inspect")?0:!strcmp(argv[1],"project")?1:!strcmp(argv[1],"mod8")?3:!strcmp(argv[1],"mod8tpdf")?4:2;
    if((mode==0 && argc!=3) || (mode && argc!=4)) {fprintf(stderr,"Wrong argument count\n");goto done;}
    if(pt_mod_file_candidate(argv[2]))result=pt_mod_file_load(&d,argv[2],64UL*1024*1024,SIZE_MAX);
    else {
        enum pt_load_result loaded=pt_file_load(argv[2],64UL*1024*1024,&allocator,&input,&input_size);
        if(loaded!=PT_LOAD_OK) {fprintf(stderr,"Input read failed phase=%d (64 MiB limit); no output created\n",loaded);goto done;}
        result=pt_document_load(&d,input,(size_t)input_size,SIZE_MAX);
        allocator.release(allocator.context,input);input=NULL;
    }
    if(result!=PT_PROJECT_OK) {fprintf(stderr,"Input rejected result=%d; no output created\n",result);goto done;}
    result=pt_mod_export_analyse(&d.project,&report);if(result!=PT_PROJECT_OK)goto done;print_report(&d.project,&report);
    if(!mode) {rc=0;goto done;}
    if(mode>=3) {
        result=pt_mod_export_analyse_round8(&d.project,&report);if(result!=PT_PROJECT_OK)goto done;
        printf("MOD_POLICY precision=round8 dither=%s result=%s remaining_issues=0x%lx\n",mode==4?"tpdf-fixed":"none",classification(report.classification),(unsigned long)(report.issues&~PT_EXPORT_PRECISION));
    }
    if(mode==1)result=pt_project_size(&d.project,&n);
    else {result=(report.issues&~(mode>=3?PT_EXPORT_PRECISION:0U))?PT_PROJECT_UNSUPPORTED:PT_PROJECT_OK;n=report.bytes;}
    if(result!=PT_PROJECT_OK) {fprintf(stderr,"Direct MOD export refused; transformations require explicit policy and implementation\n");goto done;}
    {
        enum pt_save_result saved=mode==1?pt_project_file_save(argv[3],&d.project,&allocator):
            pt_mod_file_save(argv[3],&d.project,mode==4?2:mode==3?1:0,&allocator);
        if(saved!=PT_SAVE_OK) {fprintf(stderr,"Save failed phase=%d; destination was not replaced\n",saved);goto done;}
    }
    printf("SAVED format=%s bytes=%lu verified=1 new_file=1\n",mode==1?"PT24G-v1":"MOD",(unsigned long)n);rc=0;
 done:
    if(input)allocator.release(allocator.context,input);
    pt_document_release(&d);return rc;
}
