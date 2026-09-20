#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../src/platform/render_file.h"
#include "wav.h"
struct failure {char path[1024];unsigned mode,done,verify_calls;uint64_t verified;};
static int progress(void *ctx,enum pt_render_phase phase,uint32_t ticks,uint64_t frames)
{
    struct failure *f=ctx;FILE *file;char path[1200];(void)ticks;
    if(phase==PT_RENDER_VERIFY) {assert(frames>=f->verified);f->verified=frames;++f->verify_calls;}
    if(f->mode==1 && phase==PT_RENDER_MIX && frames>=256)return 0;
    if(phase!=PT_RENDER_VERIFY || f->done)return 1;
    f->done=1;
    if(f->mode==2) {
#ifdef __amigaos__
        snprintf(path,sizeof(path),"%s.pttmp-%lu-0/data",f->path,(unsigned long)getpid());
#else
        snprintf(path,sizeof(path),"%s.pttmp-%lu-0",f->path,(unsigned long)getpid());
#endif
        file=fopen(path,"r+b");assert(file);assert(fputc('X',file)!=EOF);assert(!fclose(file));
    } else if(f->mode==3) {
        file=fopen(f->path,"wb");assert(file);assert(fwrite("PRESERVE",1,8,file)==8);assert(!fclose(file));
    } else if(f->mode==4)return 0;
    return 1;
}
static size_t read_file(const char *path,uint8_t *data,size_t capacity)
{FILE *f=fopen(path,"rb");size_t n;assert(f);n=fread(data,1,capacity,f);assert(fgetc(f)==EOF && !ferror(f));assert(!fclose(f));return n;}
static void path_for(char *out,size_t bytes,const char *dir,const char *name)
{snprintf(out,bytes,"%s%s%s",dir,*dir?"/":"",name);}
int main(int argc,char **argv)
{
    struct pt_project p;struct pt_sample sample;struct pt_event events[64*4];uint16_t orders[1]={0};int32_t pcm[2]={0x123456,-0x345678};
    struct pt_render_options o;struct pt_render_report report,before;enum pt_render_result detail;
    struct pt_wav_info wav;struct failure failure;uint8_t data[5805],original[5805];char path[1024];const char *dir;size_t n;unsigned i,j;
    assert(argc==2 && strlen(argv[1])<900);dir=!strcmp(argv[1],".")?"":argv[1];
    memset(&p,0,sizeof(p));memset(&sample,0,sizeof(sample));memset(events,0,sizeof(events));memset(&o,0,sizeof(o));
    pt_channels_init(&p.channels);p.channels.track[0].pan=128;p.events=events;p.orders=orders;p.order_count=p.pattern_count=1;p.speed=1;p.bpm=125;
    p.samples=&sample;p.sample_count=1;sample.pcm.data=pcm;sample.pcm.capacity=2;sample.pcm.frames=1;sample.pcm.rate=48000;sample.pcm.channels=2;sample.pcm.bits=24;
    sample.volume=64;sample.loop=PT_LOOP_FORWARD;sample.loop_end=1;
    events[0].kind=PT_NOTE_PERIOD;events[0].pitch=428;events[0].instrument=1;events[4].effect=15;
    o.rate=48000;o.bits=24;o.tracks=1;o.gain_q16=65536;o.tick_limit=100;o.frame_limit=100000;
    path_for(path,sizeof(path),dir,"saved.wav");
    memset(&failure,0,sizeof(failure));
    assert(pt_render_file_new(path,&p,&o,progress,&failure,&report,&detail)==PT_RENDER_FILE_OK && detail==PT_RENDER_OK && report.frames==960 && !report.clipped);
    assert(failure.verify_calls>1 && failure.verified==960);
    n=read_file(path,data,sizeof(data));assert(n==5804 && pt_wav_inspect(data,n,&wav)==PT_WAV_OK && wav.bits==24 && wav.channels==2 && wav.rate==48000 && wav.frames==960);
    for(i=0;i<960*2;++i)for(j=0;j<3;++j)assert(data[44+i*3+j]==(uint8_t)((uint32_t)pcm[i%2]>>(8*j)));
    memcpy(original,data,n);before=report;
    assert(pt_render_file_new(path,&p,&o,NULL,NULL,&report,&detail)==PT_RENDER_FILE_BEGIN && !memcmp(&report,&before,sizeof(report)));
    assert(read_file(path,data,sizeof(data))==n && !memcmp(data,original,n));
    for(i=1;i<=4;++i) {
        static const char *names[]={"","cancel.wav","corrupt.wav","race.wav","verifycancel.wav"};
        enum pt_render_file_result result;
        memset(&failure,0,sizeof(failure));failure.mode=i;path_for(failure.path,sizeof(failure.path),dir,names[i]);
        result=pt_render_file_new(failure.path,&p,&o,progress,&failure,&report,&detail);
        assert(result==(i==1?PT_RENDER_FILE_RENDER:i==3?PT_RENDER_FILE_PUBLISH:PT_RENDER_FILE_VERIFY));
        assert(!memcmp(&report,&before,sizeof(report)));
        if(i==3) {assert(read_file(failure.path,data,sizeof(data))==8 && !memcmp(data,"PRESERVE",8));}
        else assert(access(failure.path,F_OK)!=0);
        if(i==1 || i==4)assert(detail==PT_RENDER_CANCELLED);
    }
    path_for(path,sizeof(path),dir,"unsupported.wav");sample.finetune=1;
    assert(pt_render_file_new(path,&p,&o,NULL,NULL,&report,&detail)==PT_RENDER_FILE_RENDER && detail==PT_RENDER_SAMPLE && access(path,F_OK)!=0);
    assert(pcm[0]==0x123456 && pcm[1]==-0x345678);
    puts("RENDER FILE PASS: exact true24 WAV, byte verification, existing/late destination preservation, cancellation, corrupt staging refusal and cleanup");return 0;
}
