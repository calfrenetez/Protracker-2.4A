#ifndef PT_APERTURE_EMBEDDED
#include <assert.h>
#include <stdio.h>
#endif
#include <string.h>
#include "cia_aperture_model.h"
struct fake_clock {
    uint64_t ticks[80];uint32_t frequency;unsigned count,at,fail,wrong,change,cancel,reenter;
    struct pt_aperture_control *control;struct pt_aperture_state *state;
};
static int aperture_read(void *context,uint64_t *ticks,uint32_t *frequency)
{
    struct fake_clock *f=context;unsigned i=++f->at;
    assert(i<=PT_APERTURE_MAX_READS);
    *ticks=f->ticks[i<=f->count?i-1:f->count-1];
    *frequency=i==f->wrong?f->frequency+1:f->frequency;
    if(i==f->change)++f->control->key.serial;
    if(i==f->cancel)f->control->cancelled=1;
    if(i==f->reenter)assert(pt_aperture_run(f->state,f->control,aperture_read,f,sizeof(*f))==PT_APERTURE_REENTRY);
    return i==f->fail?0:1;
}
static struct pt_aperture_control aperture_control(void)
{
    struct pt_aperture_control c;memset(&c,0,sizeof(c));
    c.key.queue=0x1020;c.key.session=1;c.key.generation=2;c.key.trigger=3;
    c.key.owner=4;c.key.serial=5;c.key.action=0;c.key.slot=1;
    c.publication=7;c.armed=1;return c;
}
static void aperture_setup(struct pt_aperture_state *s,struct pt_aperture_control *c,
    struct fake_clock *f,uint32_t frequency,uint32_t rate,uint64_t epoch)
{
    struct pt_elapsed_clock clock;struct pt_aperture_policy p={128,256,64};
    *c=aperture_control();assert(pt_elapsed_clock_init(&clock,frequency,rate,epoch,0)==PT_ELAPSED_OK);
    assert(pt_aperture_prepare(&clock,1536,&p,c,s)==PT_APERTURE_READY);
    memset(f,0,sizeof(*f));f->frequency=frequency;f->count=2;f->ticks[0]=s->first;f->ticks[1]=s->first;
    f->control=c;f->state=s;
}
static void assert_zero(const struct pt_aperture_state *s)
{assert(!s->busy && !s->shadow && !s->commits && s->entry_valid && s->reads && s->reads<=64);}
static void arithmetic_and_refusal(void)
{
    static const uint32_t frequencies[]={709379,715909},rates[]={44100,48000};
    struct pt_aperture_state s,before;struct pt_elapsed_clock epoch,original;
    struct pt_aperture_control c=aperture_control();struct pt_aperture_policy p={128,256,64},bad;
    unsigned fi,ri,n;uint64_t first,last;
    for(fi=0;fi<2;++fi)for(ri=0;ri<2;++ri) {
        assert(pt_elapsed_clock_init(&epoch,frequencies[fi],rates[ri],9000,0)==PT_ELAPSED_OK);original=epoch;
        for(n=16;n<272;++n) {
            first=9000+((uint64_t)n*frequencies[fi]+rates[ri]-1)/rates[ri];
            last=9000+((uint64_t)(n+1)*frequencies[fi]+rates[ri]-1)/rates[ri];
            assert(pt_aperture_prepare(&epoch,n,&p,&c,&s)==PT_APERTURE_READY);
            assert(s.first==first && s.last==last && s.arm_at==first-128 && s.frame==n);
            assert(!memcmp(&epoch,&original,sizeof(epoch)));
        }
    }
    memset(&s,0xa5,sizeof(s));before=s;
#define REFUSE(cl,fr,po,co) do{assert(pt_aperture_prepare(cl,fr,po,co,&s)==PT_APERTURE_INVALID);assert(!memcmp(&s,&before,sizeof(s)));}while(0)
    REFUSE(NULL,1536,&p,&c);REFUSE(&epoch,1536,NULL,&c);REFUSE(&epoch,1536,&p,NULL);
    REFUSE(&epoch,UINT64_MAX,&p,&c);
    bad=p;bad.early_ticks=0;REFUSE(&epoch,1536,&bad,&c);
    bad=p;bad.maximum_reads=1;REFUSE(&epoch,1536,&bad,&c);
    bad=p;bad.maximum_reads=257;REFUSE(&epoch,1536,&bad,&c);
    bad=p;bad.maximum_residency_ticks=127;REFUSE(&epoch,1536,&bad,&c);
    bad=p;bad.maximum_residency_ticks=4097;REFUSE(&epoch,1536,&bad,&c);
    epoch.failure=PT_ELAPSED_REGRESSION;REFUSE(&epoch,1536,&p,&c);
    assert(pt_elapsed_clock_init(&epoch,709379,48000,0,0)==PT_ELAPSED_OK);
    REFUSE(&epoch,0,&p,&c);REFUSE(&epoch,UINT64_MAX/2,&p,&c);
    c.cancelled=1;REFUSE(&epoch,1536,&p,&c);c=aperture_control();c.key.serial=0;REFUSE(&epoch,1536,&p,&c);
    c=aperture_control();REFUSE((const struct pt_elapsed_clock *)(UINTPTR_MAX-8),1536,&p,&c);
    REFUSE(&epoch,1536,(const struct pt_aperture_policy *)(UINTPTR_MAX-1),&c);
    REFUSE(&epoch,1536,&p,(const struct pt_aperture_control *)(UINTPTR_MAX-8));
#undef REFUSE
    {
        union {struct pt_aperture_state state;struct pt_aperture_control control;} both,copy;
        memset(&both,0x72,sizeof(both));both.control=aperture_control();copy=both;
        assert(pt_aperture_prepare(&epoch,1536,&p,&both.control,&both.state)==PT_APERTURE_INVALID);
        assert(!memcmp(&both,&copy,sizeof(both)));
        assert(pt_aperture_prepare(&epoch,1536,&p,&c,(struct pt_aperture_state *)(UINTPTR_MAX-8))==PT_APERTURE_INVALID);
    }
    {
        union {struct pt_aperture_state state;struct pt_elapsed_clock epoch;} both,copy;
        memset(&both,0x37,sizeof(both));both.epoch=epoch;copy=both;
        assert(pt_aperture_prepare(&both.epoch,1536,&p,&c,&both.state)==PT_APERTURE_INVALID);
        assert(!memcmp(&both,&copy,sizeof(both)));
    }
    {
        union {struct pt_aperture_state state;struct pt_aperture_policy policy;} both,copy;
        memset(&both,0x39,sizeof(both));both.policy=p;copy=both;
        assert(pt_aperture_prepare(&epoch,1536,&both.policy,&c,&both.state)==PT_APERTURE_INVALID);
        assert(!memcmp(&both,&copy,sizeof(both)));
    }
}
static void clock_and_commit_boundaries(void)
{
    struct pt_aperture_state s;struct pt_aperture_control c;struct fake_clock f;unsigned i;
#define SETUP() aperture_setup(&s,&c,&f,709379,48000,UINT64_C(0xfffffff0))
#define RUN(expected) do{assert(pt_aperture_run(&s,&c,aperture_read,&f,sizeof(f))==(expected));assert(f.at==s.reads && s.reads<=64 && !s.busy);}while(0)
    SETUP();f.count=4;f.ticks[0]=s.arm_at;f.ticks[1]=s.first-1;f.ticks[2]=s.first;f.ticks[3]=s.last-1;
    RUN(PT_APERTURE_COMMITTED);assert(s.entry==s.arm_at && s.before==s.first && s.after==s.last-1 && s.shadow==PT_APERTURE_SHADOW && s.commits==1 && s.after_valid);
    SETUP();f.ticks[0]=s.last-1;f.ticks[1]=s.last-1;RUN(PT_APERTURE_COMMITTED);
    SETUP();s.policy.maximum_reads=2;RUN(PT_APERTURE_COMMITTED);assert(s.reads==2);
    SETUP();s.policy.maximum_reads=2;f.ticks[0]=s.first-1;RUN(PT_APERTURE_READ_LIMIT);assert_zero(&s);assert(s.reads==1);
    SETUP();f.ticks[0]=s.last;RUN(PT_APERTURE_EXPIRED);assert_zero(&s);assert(!s.after_valid);
    SETUP();f.ticks[0]=s.arm_at-1;RUN(PT_APERTURE_EARLY);assert_zero(&s);
    SETUP();f.ticks[0]=s.first-1;f.ticks[1]=s.last+1;RUN(PT_APERTURE_EXPIRED);assert_zero(&s);assert(s.before==s.last+1);
    SETUP();f.count=1;f.ticks[0]=s.first-1;RUN(PT_APERTURE_READ_LIMIT);assert_zero(&s);assert(s.reads==63);
    SETUP();f.ticks[0]=s.arm_at;f.ticks[1]=s.first+200;RUN(PT_APERTURE_RESIDENCY);assert_zero(&s);
    SETUP();f.ticks[1]=s.last;RUN(PT_APERTURE_POST_WINDOW);assert(s.shadow==PT_APERTURE_SHADOW && s.commits==1 && s.after_valid && s.after==s.last);
    SETUP();f.wrong=1;RUN(PT_APERTURE_CLOCK);assert_zero(&s);
    SETUP();f.fail=1;RUN(PT_APERTURE_CLOCK);assert(!s.entry_valid && !s.commits && !s.shadow && s.reads==1 && !s.last_read_valid && s.last_read==f.ticks[0]);
    SETUP();f.ticks[0]=s.first-1;f.ticks[1]=s.first-2;RUN(PT_APERTURE_CLOCK);assert_zero(&s);assert(s.last_read==s.first-2 && s.last_read_valid);
    SETUP();f.ticks[0]=s.first-1;f.ticks[1]=s.first;f.fail=2;RUN(PT_APERTURE_CLOCK);assert_zero(&s);assert(!s.last_read_valid && s.last_read==s.first);
    SETUP();f.wrong=2;RUN(PT_APERTURE_CLOCK);assert(s.commits==1 && s.last_read_valid && s.last_read_frequency==f.frequency+1);
    SETUP();f.fail=2;RUN(PT_APERTURE_CLOCK);assert(s.commits==1 && s.shadow==PT_APERTURE_SHADOW && !s.after_valid && !s.last_read_valid);
    SETUP();f.cancel=2;RUN(PT_APERTURE_CHANGED_AFTER);assert(s.commits==1 && s.shadow==PT_APERTURE_SHADOW);
    SETUP();f.reenter=1;RUN(PT_APERTURE_REENTRY);assert_zero(&s);assert(s.reentry);
    for(i=0;i<8;++i) {
        SETUP();
        if(i==0)++c.key.queue;
        if(i==1)++c.key.session;
        if(i==2)++c.key.generation;
        if(i==3)++c.key.trigger;
        if(i==4)++c.key.owner;
        if(i==5)++c.key.serial;
        if(i==6)++c.key.action;
        if(i==7)++c.key.slot;
        RUN(PT_APERTURE_STALE);assert_zero(&s);
    }
    SETUP();++c.publication;RUN(PT_APERTURE_STALE);assert_zero(&s);
    SETUP();c.armed=0;RUN(PT_APERTURE_STALE);assert_zero(&s);
    SETUP();c.cancelled=1;RUN(PT_APERTURE_STALE);assert_zero(&s);
    SETUP();f.ticks[0]=s.first-1;f.ticks[1]=s.first;f.change=2;RUN(PT_APERTURE_STALE);assert_zero(&s);
    SETUP();f.ticks[0]=s.first-1;f.ticks[1]=s.first;f.cancel=2;RUN(PT_APERTURE_STALE);assert_zero(&s);
    {
        struct pt_aperture_state before;struct pt_aperture_control original;
        SETUP();before=s;original=c;
        assert(pt_aperture_run(&s,&c,NULL,&f,sizeof(f))==PT_APERTURE_INVALID);
        assert(pt_aperture_run(&s,&c,aperture_read,NULL,sizeof(f))==PT_APERTURE_INVALID);
        assert(pt_aperture_run(&s,&c,aperture_read,&f,0)==PT_APERTURE_INVALID);
        assert(pt_aperture_run(&s,&c,aperture_read,&s,sizeof(s))==PT_APERTURE_INVALID);
        assert(pt_aperture_run(&s,&c,aperture_read,&c,sizeof(c))==PT_APERTURE_INVALID);
        assert(pt_aperture_run(&s,&c,aperture_read,(void *)(UINTPTR_MAX-1),8)==PT_APERTURE_INVALID);
        assert(!memcmp(&s,&before,sizeof(s)) && !memcmp(&c,&original,sizeof(c)) && f.at==0);
        {
            union {struct pt_aperture_state state;struct pt_aperture_control control;} both,copy;
            memset(&both,0x64,sizeof(both));both.control=c;copy=both;
            assert(pt_aperture_run(&both.state,&both.control,aperture_read,&f,sizeof(f))==PT_APERTURE_INVALID);
            assert(!memcmp(&both,&copy,sizeof(both)) && !f.at);
        }
        RUN(PT_APERTURE_COMMITTED);before=s;
        assert(pt_aperture_run(&s,&c,aperture_read,&f,sizeof(f))==PT_APERTURE_INVALID);
        assert(!memcmp(&s,&before,sizeof(s)));
    }
#undef RUN
#undef SETUP
}
static int cia_aperture_fixture(void)
{
    arithmetic_and_refusal();clock_and_commit_boundaries();
    puts("CIA APERTURE MODEL PASS: original windows, bounded actual reads, exact keys and irreversible postcommit failure; software diagnostic only");
    return 0;
}
#ifndef PT_APERTURE_EMBEDDED
int main(void){return cia_aperture_fixture();}
#endif
