#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "render_invert_file.h"
#include "document.h"
struct memory {unsigned calls,fail,live;};
static void *allocate(void *c,size_t n){struct memory *m=c;void *p;if(++m->calls==m->fail)return NULL;p=malloc(n);if(p)++m->live;return p;}
static void release(void *c,void *p){struct memory *m=c;assert(m->live);--m->live;free(p);}
struct cancellation {unsigned verifies,target;};
static int cancel(void *c,enum pt_render_phase phase,uint32_t t,uint64_t f)
{struct cancellation *s=c;(void)t;(void)f;if(phase==PT_RENDER_VERIFY && ++s->verifies==s->target)return 0;return 1;}
static void check(const char *dir,const char *name,unsigned mask)
{
    char path[1500];FILE *f;unsigned i;unsigned char header[44];
    snprintf(path,sizeof(path),"%s/%s",dir,name);f=fopen(path,"rb");assert(f);
    assert(fread(header,1,44,f)==44 && !memcmp(header,"RIFF",4));
    for(i=0;i<2880;++i){unsigned char raw[6];unsigned index=i%4,tick=i/960;int value=(int)(index+1)*10;if(index && index<=tick+1)value=-1-value;
        assert(fread(raw,1,6,f)==6 && !raw[0] && !raw[1] && raw[2]==((mask&1)?(unsigned char)value:0) && !raw[3] && !raw[4] && raw[5]==((mask&2)?(unsigned char)value:0));}
    assert(fgetc(f)==EOF && !fclose(f));assert(!unlink(path));
}
int main(int argc,char **argv)
{
    struct pt_project p={0};struct pt_sample sample={0};struct pt_event events[256]={0};uint16_t order=0;
    int32_t pcm[4]={10,20,30,40},original[4];struct pt_render_options o={0};struct pt_stem_report r,before;
    struct memory m={0};struct pt_allocator a={&m,allocate,release};struct cancellation c={0,0};enum pt_render_result detail;unsigned i,calls,verifies;
    assert(argc==2);pt_channels_init(&p.channels);p.channels.track[1].pan=255;
    p.events=events;p.orders=&order;p.order_count=p.pattern_count=p.sample_count=1;p.samples=&sample;p.speed=3;p.bpm=125;
    sample.pcm.data=pcm;sample.pcm.capacity=sample.pcm.frames=4;sample.pcm.rate=48000;sample.pcm.bits=8;sample.pcm.channels=1;sample.volume=64;sample.loop=PT_LOOP_FORWARD;sample.loop_end=4;
    events[0].kind=events[1].kind=PT_NOTE_PERIOD;events[0].pitch=events[1].pitch=428;events[0].instrument=events[1].instrument=1;
    events[1].effect=14;events[1].parameter=255;events[4].effect=15;
    o.rate=48000;o.bits=24;o.tracks=3;o.gain_q16=65536;o.tick_limit=100;o.frame_limit=100000;
    memcpy(original,pcm,sizeof(pcm));memset(&before,0x55,sizeof(before));r=before;
    assert(pt_render_invert_stems_new(argv[1],&p,&o,0,cancel,&c,&r,&detail,100000,&a)==PT_RENDER_FILE_OK && !m.live);
    calls=m.calls;verifies=c.verifies;assert(r.plan.count==2 && r.audio[0].frames==2880 && r.audio[1].frames==2880);
    assert(pt_render_invert_stems_new(argv[1],&p,&o,0,NULL,NULL,&r,&detail,100000,&a)==PT_RENDER_FILE_BEGIN && !m.live);
    check(argv[1],"track-01.wav",1);check(argv[1],"track-02.wav",2);assert(!rmdir(argv[1]));
    /* Every allocation failure, including later-stem output/verification,
       leaves no destination, no partial report, no live allocation. */
    for(i=1;i<=calls;++i){memset(&m,0,sizeof(m));m.fail=i;r=before;
        assert(pt_render_invert_stems_new(argv[1],&p,&o,0,NULL,NULL,&r,&detail,100000,&a)!=PT_RENDER_FILE_OK);
        assert(!m.live && access(argv[1],F_OK)!=0 && !memcmp(&before,&r,sizeof(r)) && !memcmp(pcm,original,sizeof(pcm)));
    }
    /* Cancel first verification, second stem verification and final publish. */
    for(i=0;i<3;++i){memset(&m,0,sizeof(m));c.verifies=0;c.target=i==0?1:i==1?(verifies-1)/2+1:verifies;r=before;
        assert(pt_render_invert_stems_new(argv[1],&p,&o,0,cancel,&c,&r,&detail,100000,&a)!=PT_RENDER_FILE_OK);
        assert(detail==PT_RENDER_CANCELLED && !m.live && access(argv[1],F_OK)!=0 && !memcmp(&before,&r,sizeof(r)));
    }
    memset(&m,0,sizeof(m));r=before;
    assert(pt_render_invert_stems_new(argv[1],&p,&o,0,NULL,NULL,&r,&detail,1,&a)==PT_RENDER_FILE_RENDER && detail==PT_RENDER_MEMORY && !m.live && access(argv[1],F_OK)!=0);
    p.channels.track[0].group=p.channels.track[1].group=1;
    assert(pt_render_invert_stems_new(argv[1],&p,&o,1,NULL,NULL,&r,&detail,100000,&a)==PT_RENDER_FILE_OK && !m.live && r.plan.count==1);
    check(argv[1],"group-01.wav",3);assert(!rmdir(argv[1]));
    assert(!memcmp(pcm,original,sizeof(pcm)));
    puts("INVERT STEMS PASS: shared-sample exact PCM, grouped stems, fresh banks, all allocation failures, cancellations, immutable masters and cleanup");return 0;
}
