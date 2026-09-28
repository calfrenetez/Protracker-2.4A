#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/core/capture_session.h"
#include "../src/editor/sampler_capture.h"
static size_t live,allocations;
static unsigned fail_allocate,device_owned;
static void *allocate(void *ctx,size_t bytes)
{void *p;(void)ctx;++allocations;if(fail_allocate)return NULL;p=malloc(bytes);if(p)++live;return p;}
static void release(void *ctx,void *p)
{(void)ctx;if(p){assert(!device_owned && live);--live;free(p);}}
static struct pt_allocator allocator={NULL,allocate,release};
struct fake {
    int start_rc,read_rc,stop_rc;
    unsigned starts,reads,stops,bits,channels,frames,position,max_request,bad_count,bad_value;
    uint32_t rate;
};
static int start(void *ctx,unsigned bits,unsigned channels,uint32_t rate)
{struct fake *f=ctx;++f->starts;device_owned=1;f->bits=bits;f->channels=channels;f->rate=rate;return f->start_rc;}
static int32_t value(unsigned i,unsigned bits)
{return i%4==0?-(1L<<(bits-1)):i%4==1?(1L<<(bits-1))-1:i%4==2?1:-1;}
static int read_input(void *ctx,int32_t *out,unsigned max,unsigned *frames)
{
    struct fake *f=ctx;unsigned i,n=f->frames;if(n>max)n=max;
    assert(device_owned && max && max<=256);++f->reads;f->max_request=max;
    *frames=f->read_rc==1?n:0;
    if(f->bad_count)*frames=f->bad_count==1?max+1:1;
    if(f->read_rc!=1)return f->read_rc;
    for(i=0;i<n*f->channels;++i)out[i]=value(f->position+i,f->bits);
    if(f->bad_value)out[0]=1L<<(f->bits-1);
    f->position+=n*f->channels;return 1;
}
static int stop(void *ctx)
{struct fake *f=ctx;++f->stops;if(f->stop_rc==1)device_owned=0;return f->stop_rc;}
static struct pt_capture_input input(struct fake *f)
{struct pt_capture_input in={f,start,read_input,stop};return in;}
static void completed_formats(void)
{
    unsigned bits,channels,i;
    for(bits=8;bits<=24;bits+=8)for(channels=1;channels<=2;++channels) {
        struct fake f={0};struct pt_capture_session s={0};struct pt_capture c={0};struct pt_capture_input in=input(&f);
        const struct pt_pcm *pcm;size_t calls;
        f.frames=256;f.read_rc=1;
        assert(pt_capture_session_open(&s,&allocator,&in,bits,channels,48000,257,257*channels*4)==PT_CAPTURE_OK);
        calls=allocations;assert(!f.starts && !device_owned && live==1);
        assert(!pt_capture_session_close(&s) && !pt_capture_session_take(&s,&c));
        assert(pt_capture_session_step(&s)==PT_CS_PENDING && s.phase==PT_CS_START && device_owned);
        assert(pt_capture_session_step(&s)==PT_CS_PENDING && !f.reads);
        f.start_rc=1;assert(pt_capture_session_step(&s)==PT_CS_PENDING && s.phase==PT_CS_RECORD);
        assert(f.bits==bits && f.channels==channels && f.rate==48000 && f.starts==3);
        assert(pt_capture_session_step(&s)==PT_CS_PENDING && s.capture.pcm.frames==256);
        assert(pt_capture_session_step(&s)==PT_CS_PENDING && s.phase==PT_CS_STOP && f.max_request==1);
        assert(allocations==calls && !pt_capture_pcm(&s.capture));
        assert(pt_capture_session_step(&s)==PT_CS_PENDING && live==1 && device_owned);
        assert(!pt_capture_session_close(&s) && !pt_capture_session_take(&s,&c));
        f.stop_rc=1;assert(pt_capture_session_step(&s)==PT_CS_COMPLETE && !device_owned);
        assert(pt_capture_session_step(&s)==PT_CS_COMPLETE && f.stops==2 && f.reads==2);
        assert(!pt_capture_session_take(&s,&s.capture));
        assert(pt_capture_session_take(&s,&c) && !pt_capture_session_take(&s,&c));
        assert(pt_capture_session_close(&s) && pt_capture_session_close(&s));
        pcm=pt_capture_pcm(&c);assert(pcm && pcm->frames==257 && pcm->bits==bits && pcm->channels==channels);
        for(i=0;i<257*channels;++i)assert(pcm->data[i]==value(i,bits));
        pt_capture_close(&c);assert(!live);
    }
}
static void faults(void)
{
    unsigned mode;
    for(mode=0;mode<10;++mode) {
        struct fake f={0};struct pt_capture_session s={0};struct pt_capture c={0};struct pt_capture_input in=input(&f);
        enum pt_capture_fault expected=PT_CS_INPUT_FAULT;unsigned reads;
        f.start_rc=1;f.read_rc=1;f.frames=1;
        assert(pt_capture_session_open(&s,&allocator,&in,24,2,48000,4,32)==PT_CAPTURE_OK);
        if(mode<2) {f.start_rc=mode?2:-1;assert(pt_capture_session_step(&s)==PT_CS_ERROR);}
        else {
            assert(pt_capture_session_step(&s)==PT_CS_PENDING);
            assert(pt_capture_session_step(&s)==PT_CS_PENDING && s.capture.pcm.frames==1);
            if(mode==2)f.read_rc=-1;
            if(mode==3)f.read_rc=2;
            if(mode==4){f.read_rc=-2;expected=PT_CS_OVERRUN;}
            if(mode==5){f.bad_count=1;expected=PT_CS_DATA_FAULT;}
            if(mode==6){f.frames=0;expected=PT_CS_DATA_FAULT;}
            if(mode==7){f.bad_value=1;expected=PT_CS_DATA_FAULT;}
            if(mode==8){f.read_rc=0;f.bad_count=2;}
            if(mode==9) {pt_capture_session_finish(&s);f.stop_rc=2;expected=PT_CS_STOP_FAULT;}
            assert(pt_capture_session_step(&s)==PT_CS_ERROR);
            assert(s.capture.pcm.frames==1 && s.capture.pcm.data[0]==-8388608);
        }
        assert(s.fault==expected && s.phase==PT_CS_STOP && !pt_capture_session_take(&s,&c));
        reads=f.reads;assert(device_owned && live==1);
        f.stop_rc=-1;assert(pt_capture_session_step(&s)==PT_CS_ERROR && s.fault==expected);
        assert(!pt_capture_session_close(&s) && live==1);
        f.stop_rc=0;assert(pt_capture_session_step(&s)==PT_CS_PENDING && device_owned);
        pt_capture_session_finish(&s);assert(!pt_capture_session_take(&s,&c));
        f.stop_rc=1;assert(pt_capture_session_step(&s)==PT_CS_ERROR && s.phase==PT_CS_DONE);
        assert(!device_owned && !live && !s.capture.pcm.data && f.reads==reads);
        assert(!pt_capture_session_take(&s,&c) && pt_capture_session_close(&s));
    }
}
static void cancellation(void)
{
    unsigned mode;
    for(mode=0;mode<5;++mode) {
        struct fake f={0};struct pt_capture_session s={0};struct pt_capture c={0};struct pt_capture_input in=input(&f);
        f.read_rc=1;f.frames=1;
        assert(pt_capture_session_open(&s,&allocator,&in,16,1,22050,4,16)==PT_CAPTURE_OK);
        if(mode)assert(pt_capture_session_step(&s)==PT_CS_PENDING);
        if(mode>1){f.start_rc=1;assert(pt_capture_session_step(&s)==PT_CS_PENDING);assert(pt_capture_session_step(&s)==PT_CS_PENDING);}
        if(mode==3 || mode==4)pt_capture_session_finish(&s);else pt_capture_session_abort(&s);
        assert(pt_capture_session_step(&s)==PT_CS_PENDING && live==1);
        assert(!pt_capture_session_take(&s,&c) && !pt_capture_session_close(&s));
        if(mode==4)pt_capture_session_abort(&s); /* cancel overrides pending finish */
        f.stop_rc=1;assert(pt_capture_session_step(&s)==PT_CS_COMPLETE && !device_owned);
        if(mode==3) {assert(pt_capture_session_take(&s,&c));assert(c.pcm.frames==1);pt_capture_close(&c);}
        else assert(!live && !pt_capture_session_take(&s,&c));
        assert(pt_capture_session_close(&s) && !live);
    }
    {
        struct fake f={0};struct pt_capture_session s={0};struct pt_capture c={0};struct pt_capture_input in=input(&f);
        assert(pt_capture_session_open(&s,&allocator,&in,8,1,8000,1,4)==PT_CAPTURE_OK);
        pt_capture_session_finish(&s);f.stop_rc=1;assert(pt_capture_session_step(&s)==PT_CS_COMPLETE);
        assert(!f.starts && !f.reads && !pt_capture_session_take(&s,&c));
        pt_capture_session_abort(&s);assert(!live && pt_capture_session_close(&s));
    }
}
static void publication(void)
{
    struct fake f={0};struct pt_capture_session s={0};struct pt_capture c={0};struct pt_capture_input in=input(&f);
    struct pt_document d;struct pt_sampler sampler;struct pt_pattern_history h;
    struct pt_pattern_command commands[4];struct pt_event_change changes[4];
    pt_document_init(&d,&allocator);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    pt_sampler_init(&sampler,&allocator,1024*1024);
    assert(pt_pattern_history_init(&h,&d.project,commands,4,changes,4)==PT_EDIT_OK);
    f.start_rc=f.read_rc=f.stop_rc=1;f.frames=3;
    assert(pt_capture_session_open(&s,&allocator,&in,24,2,48000,3,24)==PT_CAPTURE_OK);
    assert(pt_capture_session_step(&s)==PT_CS_PENDING && pt_capture_session_step(&s)==PT_CS_PENDING);
    assert(!pt_capture_session_take(&s,&c));
    assert(pt_capture_session_step(&s)==PT_CS_COMPLETE && pt_capture_session_take(&s,&c));
    assert(pt_capture_session_close(&s));fail_allocate=1;
    assert(pt_sampler_capture_append(&sampler,&d.project,&h,&c,"captured")==PT_EDIT_CAPACITY);
    fail_allocate=0;assert(pt_capture_pcm(&c) && d.project.sample_count==31);
    assert(pt_sampler_capture_append(&sampler,&d.project,&h,&c,"captured")==PT_EDIT_OK);
    assert(d.project.sample_count==32 && d.project.samples[31].pcm.data[2]==1 && !c.pcm.data);
    assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK && d.project.sample_count==31);
    assert(pt_pattern_undo(&d.project,&h,1)==PT_EDIT_OK && d.project.samples[31].pcm.data[2]==1);
    pt_pattern_history_release(&h);pt_sampler_release(&sampler);pt_document_release(&d);assert(!live);
}
static void invalid_open(void)
{
    struct fake f={0};struct pt_capture_session s={0};struct pt_capture_input in=input(&f);
    in.stop=NULL;assert(pt_capture_session_open(&s,&allocator,&in,24,2,48000,1,8)==PT_CAPTURE_INVALID);
    in=input(&f);fail_allocate=1;
    assert(pt_capture_session_open(&s,&allocator,&in,24,2,48000,1,8)==PT_CAPTURE_CAPACITY);fail_allocate=0;
    assert(!f.starts && !live && s.phase==PT_CS_IDLE && pt_capture_session_close(&s));
    assert(pt_capture_session_open(&s,&allocator,&in,24,2,48000,1,8)==PT_CAPTURE_OK);
    assert(pt_capture_session_open(&s,&allocator,&in,24,2,48000,1,8)==PT_CAPTURE_INVALID);
    pt_capture_session_abort(&s);f.stop_rc=1;assert(pt_capture_session_step(&s)==PT_CS_COMPLETE);
    assert(pt_capture_session_close(&s) && !live);
}
int main(void)
{
    invalid_open();completed_formats();faults();cancellation();publication();assert(!live && !device_owned);
    puts("CAPTURE SESSION PASS: exact formats, bounded polls, delayed start/stop, stop faults retain ownership, explicit overruns, cancellation and undoable publication; injected input only");return 0;
}
