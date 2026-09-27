#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "document.h"
#include "wav.h"
#include "../src/platform/render_invert_file.h"
#include "../src/platform/project_file.h"
#include "../src/editor/bounce_invert.h"
static unsigned live;
static void *allocate(void *ctx,size_t n){void *p;(void)ctx;p=malloc(n);if(p)++live;return p;}
static void release(void *ctx,void *p){(void)ctx;if(p){assert(live);--live;free(p);}}
static unsigned char *read_file(const char *path,size_t *size)
{
    FILE *f=fopen(path,"rb");long n;unsigned char *data;
    assert(f && !fseek(f,0,SEEK_END));n=ftell(f);assert(n>0);rewind(f);
    *size=(size_t)n;data=malloc(*size);assert(data && fread(data,1,*size,f)==*size && !fclose(f));return data;
}
static void unchanged(const struct pt_project *p,const unsigned char *saved,size_t n)
{
    unsigned char *data;size_t size,written;
    assert(pt_project_size(p,&size)==PT_PROJECT_OK && size==n);data=malloc(size);assert(data);
    assert(pt_project_encode(p,data,size,&written)==PT_PROJECT_OK && written==n && !memcmp(data,saved,n));free(data);
}
static int cancel_verify(void *ctx,enum pt_render_phase phase,uint32_t tick,uint64_t frame)
{(void)ctx;(void)tick;(void)frame;return phase!=PT_RENDER_VERIFY;}
static int cancel_after_handoff(void *ctx,enum pt_render_phase phase,uint32_t tick,uint64_t frame)
{(void)ctx;(void)tick;return !(phase==PT_RENDER_MIX && frame>=6000);}
static void check_bounce(const struct pt_sample *s,const struct pt_pcm *expected)
{
    assert(s->pcm.bits==24 && s->pcm.channels==2 && s->pcm.rate==48000 && s->pcm.frames==expected->frames && !s->loop);
    assert(!memcmp(s->pcm.data,expected->data,expected->frames*2*sizeof(int32_t)));
}
static struct pt_pattern_command commands[16];
static struct pt_event_change changes[32];
int main(int argc,char **argv)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d,copy;struct pt_sampler sampler;
    struct pt_pattern_history history;struct pt_render_options options={0};struct pt_render_report report,before;
    struct pt_stem_report stems;enum pt_render_result detail;struct pt_project *p;struct pt_pcm pcm={0};struct pt_wav_info info;
    char wave[1024],stemdir[1024],groupdir[1024],cancelled[1024],project[1024],bounced[1024],undone[1024];
    unsigned char *data,*saved,*wave_data;size_t n,written,wave_size;unsigned i,base,expected_frames;
    assert(argc==3 || argc==4);expected_frames=argc==4?(unsigned)strtoul(argv[3],NULL,10):17280;assert(expected_frames==17280 || expected_frames==23040 || expected_frames==28800 || expected_frames==40320);pt_document_init(&d,&a);pt_document_init(&copy,&a);
    data=read_file(argv[1],&n);assert(pt_document_load(&d,data,n,SIZE_MAX)==PT_PROJECT_OK);free(data);p=&d.project;
    for(i=0;i<p->sample_count;++i)p->samples[i].pcm.rate=48000;
    p->channels.track[0].pan=0;p->channels.track[1].pan=255;p->channels.track[0].group=p->channels.track[1].group=1;
    options.rate=48000;options.bits=24;options.gain_q16=65536;options.tracks=1;options.tick_limit=100;options.frame_limit=100000;
    assert(pt_project_size(p,&n)==PT_PROJECT_OK);saved=malloc(n);assert(saved);
    assert(pt_project_encode(p,saved,n,&written)==PT_PROJECT_OK && written==n);
    snprintf(wave,sizeof(wave),"%s/song.wav",argv[2]);snprintf(stemdir,sizeof(stemdir),"%s/stems",argv[2]);
    snprintf(groupdir,sizeof(groupdir),"%s/groups",argv[2]);snprintf(cancelled,sizeof(cancelled),"%s/cancelled",argv[2]);
    snprintf(project,sizeof(project),"%s/master.ptg",argv[2]);snprintf(bounced,sizeof(bounced),"%s/bounced.ptg",argv[2]);
    snprintf(undone,sizeof(undone),"%s/undone.ptg",argv[2]);base=live;
    assert(pt_project_file_save(project,p,&a)==PT_SAVE_OK && live==base);
    memset(&before,0x55,sizeof(before));report=before;
    assert(pt_render_invert_file_new(cancelled,p,&options,cancel_verify,NULL,&report,&detail,SIZE_MAX,&a)!=PT_RENDER_FILE_OK);
    assert(detail==PT_RENDER_CANCELLED && live==base && access(cancelled,F_OK)!=0 && !memcmp(&report,&before,sizeof(report)));unchanged(p,saved,n);
    assert(pt_render_invert_file_new(wave,p,&options,NULL,NULL,&report,&detail,SIZE_MAX,&a)==PT_RENDER_FILE_OK && live==base);
    assert(report.frames==expected_frames);wave_data=read_file(wave,&wave_size);
    assert(pt_wav_inspect(wave_data,wave_size,&info)==PT_WAV_OK && info.frames==expected_frames && info.bits==24 && info.channels==2);
    pcm.data=malloc(info.frames*2*sizeof(int32_t));assert(pcm.data);pcm.capacity=info.frames*2;pcm.frames=info.frames;pcm.rate=info.rate;pcm.bits=info.bits;pcm.channels=info.channels;
    assert(pt_wav_decode(wave_data,wave_size,&pcm)==PT_WAV_OK);free(wave_data);
    assert(pt_render_invert_file_new(wave,p,&options,NULL,NULL,&report,&detail,SIZE_MAX,&a)==PT_RENDER_FILE_BEGIN && live==base);
    options.tracks=3;
    assert(pt_render_invert_stems_new(cancelled,p,&options,0,cancel_verify,NULL,&stems,&detail,SIZE_MAX,&a)!=PT_RENDER_FILE_OK);
    assert(detail==PT_RENDER_CANCELLED && live==base && access(cancelled,F_OK)!=0);unchanged(p,saved,n);
    assert(pt_render_invert_stems_new(stemdir,p,&options,0,NULL,NULL,&stems,&detail,SIZE_MAX,&a)==PT_RENDER_FILE_OK && live==base && stems.plan.count==2);
    assert(pt_render_invert_stems_new(groupdir,p,&options,1,NULL,NULL,&stems,&detail,SIZE_MAX,&a)==PT_RENDER_FILE_OK && live==base && stems.plan.count==1);
    options.tracks=1;unchanged(p,saved,n);
    pt_sampler_init(&sampler,&a,4*1024*1024);
    assert(pt_pattern_history_init(&history,p,commands,16,changes,32)==PT_EDIT_OK);report=before;
    assert(pt_sampler_bounce_invert(&sampler,p,&history,&options,"HANDOFF",cancel_after_handoff,NULL,&report,&detail,SIZE_MAX)==PT_EDIT_CANCELLED);
    assert(live==base && !history.count && !sampler.bytes && !memcmp(&report,&before,sizeof(report)));unchanged(p,saved,n);
    assert(pt_sampler_bounce_invert(&sampler,p,&history,&options,"HANDOFF",NULL,NULL,&report,&detail,SIZE_MAX)==PT_EDIT_OK);
    assert(history.count==1 && history.cursor==1 && p->sample_count==32);check_bounce(p->samples+31,&pcm);
    assert(pt_project_file_save(bounced,p,&a)==PT_SAVE_OK);
    data=read_file(bounced,&written);assert(pt_document_load(&copy,data,written,SIZE_MAX)==PT_PROJECT_OK);free(data);
    check_bounce(copy.project.samples+31,&pcm);
    /* Temporarily omit only the appended output when comparing saved source data. */
    copy.project.sample_count=31;unchanged(&copy.project,saved,n);copy.project.sample_count=32;
    assert(pt_pattern_undo(p,&history,-1)==PT_EDIT_OK && p->sample_count==31);unchanged(p,saved,n);
    assert(pt_project_file_save(undone,p,&a)==PT_SAVE_OK);base=live;report=before;
    assert(pt_sampler_bounce_invert(&sampler,p,&history,&options,"REFUSE",NULL,NULL,&report,&detail,1)==PT_EDIT_CAPACITY && detail==PT_RENDER_MEMORY);
    assert(live==base && history.count==1 && !history.cursor && !memcmp(&report,&before,sizeof(report)));unchanged(p,saved,n);
    assert(pt_pattern_undo(p,&history,1)==PT_EDIT_OK);check_bounce(p->samples+31,&pcm);
    assert(pt_pattern_undo(p,&history,-1)==PT_EDIT_OK);unchanged(p,saved,n);
    pt_pattern_history_release(&history);pt_sampler_release(&sampler);pt_document_release(&d);pt_document_release(&copy);
    free(pcm.data);free(saved);assert(!live);
    puts("EFx HANDOFF WORKFLOWS PASS: WAV/stems/group, cancellation, bounce undo/redo, master save/reload, no leaks");return 0;
}
