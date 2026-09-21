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

#include "../src/platform/file_save.h"
static void *allocate(void *ctx,size_t n) {(void)ctx;return malloc(n);}
static void release(void *ctx,void *p) {(void)ctx;free(p);}
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
    enum pt_project_result result;
    pt_document_init(&d,&allocator);
    if(argc<3 || argc>4 || (strcmp(argv[1],"inspect") && strcmp(argv[1],"project") && strcmp(argv[1],"mod") && strcmp(argv[1],"mod8") && strcmp(argv[1],"mod8tpdf"))) {
        fprintf(stderr,"Usage: PT24GConvert inspect INPUT | project INPUT NEW_OUTPUT | mod INPUT NEW_OUTPUT | mod8 INPUT NEW_OUTPUT | mod8tpdf INPUT NEW_OUTPUT\n");goto done;
    }
    mode=!strcmp(argv[1],"inspect")?0:!strcmp(argv[1],"project")?1:!strcmp(argv[1],"mod8")?3:!strcmp(argv[1],"mod8tpdf")?4:2;
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
    if(mode>=3) {
        result=pt_mod_export_analyse_round8(&d.project,&report);if(result!=PT_PROJECT_OK)goto done;
        printf("MOD_POLICY precision=round8 dither=%s result=%s remaining_issues=0x%lx\n",mode==4?"tpdf-fixed":"none",classification(report.classification),(unsigned long)(report.issues&~PT_EXPORT_PRECISION));
    }
    if(mode==1)result=pt_project_size(&d.project,&n);
    else {result=(report.issues&~(mode>=3?PT_EXPORT_PRECISION:0U))?PT_PROJECT_UNSUPPORTED:PT_PROJECT_OK;n=report.bytes;}
    if(result!=PT_PROJECT_OK) {fprintf(stderr,"Direct MOD export refused; transformations require explicit policy and implementation\n");goto done;}
    output=malloc(n);if(!output)goto done;
    result=mode==1?pt_project_encode(&d.project,output,n,&w):mode==4?pt_mod_export_tpdf8(&d.project,output,n,&w):mode==3?pt_mod_export_round8(&d.project,output,n,&w):pt_mod_export_direct(&d.project,output,n,&w);
    if(result!=PT_PROJECT_OK || w!=n)goto done;
    {
        enum pt_save_result saved=pt_file_save_new(argv[3],output,n);
        if(saved!=PT_SAVE_OK) {fprintf(stderr,"Save failed phase=%d; destination was not replaced\n",saved);goto done;}
    }
    printf("SAVED format=%s bytes=%lu verified=1 new_file=1\n",mode==1?"PT24G-v1":"MOD",(unsigned long)n);rc=0;
 done:
    if(file)fclose(file);
    free(input);free(output);pt_document_release(&d);return rc;
}
