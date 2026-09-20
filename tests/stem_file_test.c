#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../src/platform/stem_file.h"
#ifdef __amigaos__
#include <proto/dos.h>
#endif
static int make_dir(const char *path)
{
#ifdef __amigaos__
    BPTR lock=CreateDir((STRPTR)path);if(!lock)return -1;UnLock(lock);return 0;
#else
    return mkdir(path,0700);
#endif
}
struct control {int mode,created;const char *destination;};
static int progress(void *ctx,enum pt_render_phase phase,uint32_t ticks,uint64_t frames)
{
    struct control *c=ctx;(void)ticks;(void)frames;
    if(c->mode==1 && phase==PT_RENDER_MIX)return 0;
    if(c->mode==2 && !c->created && phase==PT_RENDER_VERIFY) {assert(!make_dir(c->destination));c->created=1;}
    return 1;
}
static void equal(const char *a,const char *b)
{
    FILE *x=fopen(a,"rb"),*y=fopen(b,"rb");int c,d;assert(x && y);
    do {c=fgetc(x);d=fgetc(y);assert(c==d);}while(c!=EOF);
    assert(!ferror(x) && !ferror(y));fclose(x);fclose(y);
}
int main(int argc,char **argv)
{
    struct pt_project p;struct pt_sample sample;struct pt_event events[256];uint16_t order=0;
    int32_t data[4]={1000000,-1000000,2000000,-2000000};struct pt_render_options o;
    struct pt_stem_report report,before;struct pt_render_report single;enum pt_render_result detail;
    char batch[1300],file[1400],other[1400];struct control c={0,0,NULL};unsigned ch;
    assert(argc==2);memset(&p,0,sizeof(p));memset(&sample,0,sizeof(sample));memset(events,0,sizeof(events));memset(&o,0,sizeof(o));
    pt_channels_init(&p.channels);p.events=events;p.orders=&order;p.order_count=p.pattern_count=1;p.samples=&sample;p.sample_count=1;p.speed=2;p.bpm=125;
    sample.pcm.data=data;sample.pcm.frames=sample.pcm.capacity=4;sample.pcm.bits=24;sample.pcm.channels=1;sample.pcm.rate=48000;sample.volume=64;sample.loop=PT_LOOP_FORWARD;sample.loop_end=4;
    for(ch=0;ch<4;++ch) {events[ch].kind=PT_NOTE_PERIOD;events[ch].instrument=1;events[ch].pitch=(uint16_t)(428-ch*20);p.channels.track[ch].group=ch<2?1:0;}
    events[7].effect=15;events[7].parameter=150; /* Tempo comes from an unselected track. */
    events[8].effect=15;events[8].parameter=0;
    o.rate=48000;o.bits=24;o.gain_q16=32768;o.tracks=15;o.tick_limit=100;o.frame_limit=100000;
    snprintf(batch,sizeof(batch),"%s/batch",argv[1]);
    {enum pt_render_file_result result=pt_stem_file_new(batch,&p,&o,1,NULL,NULL,&report,&detail);
    printf("STEMS first export phase=%u detail=%u\n",result,detail);fflush(stdout);assert(result==PT_RENDER_FILE_OK && report.plan.count==3);}
    for(ch=0;ch<3;++ch) {
        o.tracks=report.plan.item[ch].tracks;
        snprintf(file,sizeof(file),"%s/%s-%02u.wav",batch,ch?"track":"group",ch?ch+2:1);
        snprintf(other,sizeof(other),"%s/single-%u.wav",argv[1],ch);
        assert(pt_render_file_new(other,&p,&o,NULL,NULL,&single,&detail)==PT_RENDER_FILE_OK);
        equal(file,other);assert(single.frames==report.audio[ch].frames && single.frames==report.audio[0].frames);
    }
    before=report;o.tracks=15;
    assert(pt_stem_file_new(batch,&p,&o,1,NULL,NULL,&report,&detail)==PT_RENDER_FILE_BEGIN && !memcmp(&report,&before,sizeof(report)));
    snprintf(batch,sizeof(batch),"%s/cancel",argv[1]);c.mode=1;
    assert(pt_stem_file_new(batch,&p,&o,0,progress,&c,&report,&detail)==PT_RENDER_FILE_RENDER && detail==PT_RENDER_CANCELLED && access(batch,F_OK));
    assert(!memcmp(&report,&before,sizeof(report)));
    snprintf(batch,sizeof(batch),"%s/race",argv[1]);c.mode=2;c.destination=batch;
    assert(pt_stem_file_new(batch,&p,&o,0,progress,&c,&report,&detail)==PT_RENDER_FILE_PUBLISH && c.created && !access(batch,F_OK));
    assert(!memcmp(&report,&before,sizeof(report)));
    snprintf(batch,sizeof(batch),"%s/refused",argv[1]);events[3].effect=14;events[3].parameter=0xf1;
    assert(pt_stem_file_new(batch,&p,&o,0,NULL,NULL,&report,&detail)==PT_RENDER_FILE_RENDER && detail==PT_RENDER_EFFECT && access(batch,F_OK));
    assert(!memcmp(&report,&before,sizeof(report)));
    puts("STEMS files PASS: grouped PCM equals individual export, global tempo alignment, cancellation, preflight refusal and no-replace race");return 0;
}
