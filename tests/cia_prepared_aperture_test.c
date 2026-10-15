#ifndef PT_PREPARED_APERTURE_EMBEDDED
#include <assert.h>
#include <stdio.h>
#endif
#include <string.h>
#include "cia_prepared_aperture.h"
static struct pt_aperture_control prepared_control(void)
{
    struct pt_aperture_control c;memset(&c,0,sizeof(c));c.key.queue=0x1020;
    c.key.session=1;c.key.generation=2;c.key.trigger=3;c.key.owner=4;c.key.serial=5;
    c.key.action=2;c.key.slot=1;c.publication=7;c.armed=1;return c;
}
static void prepared_setup(struct pt_aperture_state *s,struct pt_aperture_control *c,struct pt_prepared_clock *d,uint64_t start)
{
    struct pt_elapsed_clock e;struct pt_aperture_policy p={128,256,64};
    *c=prepared_control();assert(pt_elapsed_clock_init(&e,709379,48000,start,0)==PT_ELAPSED_OK);
    assert(pt_aperture_prepare(&e,1536,&p,c,s)==PT_APERTURE_READY);
    assert(pt_prepared_clock_bind(s,c,d)==PT_APERTURE_READY);
    assert((((uint64_t)d->arm_hi<<32)|d->arm_lo)==s->arm_at);
    assert(d->first_offset==128 && s->first==s->arm_at+d->first_offset && s->last==s->arm_at+d->last_offset);
}
static struct pt_prepared_read sample(uint64_t ticks,uint32_t frequency)
{struct pt_prepared_read v={(uint32_t)(ticks>>32),(uint32_t)ticks,frequency};return v;}
static void preparation(void)
{
    struct pt_aperture_state s,original;struct pt_aperture_control c;struct pt_prepared_clock d,before;
    unsigned i;
    prepared_setup(&s,&c,&d,UINT64_C(0x100000000));original=s;
    for(i=0;i<11;++i) {
        c=prepared_control();
        if(i==0)++c.key.queue;
        if(i==1)++c.key.session;
        if(i==2)++c.key.generation;
        if(i==3)++c.key.trigger;
        if(i==4)++c.key.owner;
        if(i==5)++c.key.serial;
        if(i==6)++c.key.action;
        if(i==7)++c.key.slot;
        if(i==8)++c.publication;
        if(i==9)c.armed=0;
        if(i==10)c.cancelled=1;
        assert(pt_prepared_clock_bind(&s,&c,&d)==PT_APERTURE_STALE && d.gate==PT_APERTURE_STALE);
        assert(!memcmp(&s,&original,sizeof(s)));
    }
    c=prepared_control();memset(&d,0xa5,sizeof(d));before=d;
#define REFUSE(st,co,ou) do{assert(pt_prepared_clock_bind(st,co,ou)==PT_APERTURE_INVALID);assert(!memcmp(&d,&before,sizeof(d)));}while(0)
    REFUSE(NULL,&c,&d);REFUSE(&s,NULL,&d);REFUSE(&s,&c,NULL);
    REFUSE((const struct pt_aperture_state *)(UINTPTR_MAX-8),&c,&d);
    REFUSE(&s,(const struct pt_aperture_control *)(UINTPTR_MAX-1),&d);
    REFUSE(&s,&c,(struct pt_prepared_clock *)(UINTPTR_MAX-1));
    s=original;s.policy.maximum_residency_ticks=128;REFUSE(&s,&c,&d);
    s=original;s.policy.maximum_reads=257;REFUSE(&s,&c,&d);
    s=original;s.policy.maximum_reads=1;REFUSE(&s,&c,&d);
    s=original;s.busy=1;REFUSE(&s,&c,&d);
    s=original;s.reads=1;REFUSE(&s,&c,&d);
    s=original;s.commits=1;REFUSE(&s,&c,&d);
    s=original;s.shadow=1;REFUSE(&s,&c,&d);
    s=original;s.reentry=1;REFUSE(&s,&c,&d);
    s=original;s.expected.serial=0;REFUSE(&s,&c,&d);
    s=original;s.publication=0;REFUSE(&s,&c,&d);
    s=original;s.frequency=0;REFUSE(&s,&c,&d);
    s=original;s.arm_at++;REFUSE(&s,&c,&d);
    s=original;
    {
        union {struct pt_aperture_state s;struct pt_prepared_clock d;} u,copy;
        memset(&u,0,sizeof(u));u.s=s;copy=u;
        assert(pt_prepared_clock_bind(&u.s,&c,&u.d)==PT_APERTURE_INVALID && !memcmp(&u,&copy,sizeof(u)));
    }
    {
        union {struct pt_aperture_control c;struct pt_prepared_clock d;} u,copy;
        memset(&u,0,sizeof(u));u.c=c;copy=u;
        assert(pt_prepared_clock_bind(&s,&u.c,&u.d)==PT_APERTURE_INVALID && !memcmp(&u,&copy,sizeof(u)));
    }
#undef REFUSE
}
static void replay_boundaries(void)
{
    struct pt_aperture_state s;struct pt_aperture_control c;struct pt_prepared_clock d;
    struct pt_prepared_read reads[64],saved[64];struct pt_prepared_result r,before;
    unsigned i;uint64_t start;
#define SETUP() prepared_setup(&s,&c,&d,start)
#define TWO(a,b) do{reads[0]=sample(a,d.frequency);reads[1]=sample(b,d.frequency);}while(0)
#define RUN(n,expected,committed) do{memcpy(saved,reads,sizeof(reads));assert(pt_prepared_replay(&d,reads,n,0,&r));assert(r.result==(expected) && r.reads==(n) && r.commits==(committed) && r.shadow==((committed)?PT_APERTURE_SHADOW:0) && !memcmp(reads,saved,sizeof(reads)));}while(0)
    /* The origin puts first/last on each side of a low-word carry. */
    for(i=0;i<3;++i) {
        start=i==0?100:i==1?UINT64_C(0xffffa74d):UINT64_C(0xfffffffe00000001);
        SETUP();TWO(s.first,s.last-1);RUN(2,PT_APERTURE_COMMITTED,1);assert(r.before_index==1 && r.after_index==2);
        TWO(s.last-1,s.last-1);RUN(2,PT_APERTURE_COMMITTED,1);
        TWO(s.first,s.last);RUN(2,PT_APERTURE_POST_WINDOW,1);
        TWO(s.first,s.first-1);RUN(2,PT_APERTURE_CLOCK,1);
        reads[0]=sample(s.last,d.frequency);RUN(1,PT_APERTURE_EXPIRED,0);
        reads[0]=sample(s.arm_at-1,d.frequency);RUN(1,PT_APERTURE_EARLY,0);
        TWO(s.first-1,s.last+UINT64_C(0x100000000));RUN(2,PT_APERTURE_EXPIRED,0);
        TWO(s.first-1,s.first-2);RUN(2,PT_APERTURE_CLOCK,0);
        TWO(s.first-1,s.first);reads[1].frequency++;RUN(2,PT_APERTURE_CLOCK,0);
        TWO(s.first,s.first);reads[1].frequency=0;RUN(2,PT_APERTURE_CLOCK,1);
        reads[0]=sample(s.first,0);RUN(1,PT_APERTURE_CLOCK,0);
        reads[0]=sample(s.first,d.frequency+1);RUN(1,PT_APERTURE_CLOCK,0);
        reads[0]=sample(s.arm_at,d.frequency);reads[1]=sample(s.first-1,d.frequency);
        reads[2]=sample(s.first,d.frequency);reads[3]=sample(s.last-1,d.frequency);RUN(4,PT_APERTURE_COMMITTED,1);
        d.read_cap=2;reads[0]=sample(s.first-1,d.frequency);RUN(1,PT_APERTURE_READ_LIMIT,0);
        d.read_cap=64;for(unsigned j=0;j<63;++j)reads[j]=sample(s.first-1,d.frequency);RUN(63,PT_APERTURE_READ_LIMIT,0);
        reads[62]=sample(s.first,d.frequency);reads[63]=sample(s.first,d.frequency);RUN(64,PT_APERTURE_COMMITTED,1);
    }
    start=UINT64_C(0xffffa74d);SETUP();
    assert((s.arm_at>>32)!=(s.last>>32)); /* Explicit full-width carry case. */
    reads[0]=sample(s.first,d.frequency);assert(pt_prepared_replay(&d,reads,1,1,&r) && r.result==PT_APERTURE_REENTRY && !r.commits && r.reentry==1);
    memset(&r,0x5a,sizeof(r));before=r;
#define REJECT(de,rs,n,re,ou) do{assert(!pt_prepared_replay(de,rs,n,re,ou));assert(!memcmp(&r,&before,sizeof(r)));}while(0)
    REJECT(NULL,reads,1,0,&r);REJECT(&d,NULL,1,0,&r);REJECT(&d,reads,0,0,&r);
    REJECT(&d,reads,1,0,NULL);REJECT(&d,reads,1,2,&r);REJECT(&d,reads,257,0,&r);
    REJECT(&d,reads,1,0,&r); /* Missing actual post-store read. */
    TWO(s.first,s.first);reads[2]=reads[1];REJECT(&d,reads,3,0,&r); /* Appended read. */
    REJECT(&d,(const struct pt_prepared_read *)(UINTPTR_MAX-1),2,0,&r);
    REJECT(&d,(const struct pt_prepared_read *)&r,2,0,&r);
    REJECT(&d,reads,2,0,(struct pt_prepared_result *)&d);
    d.read_cap=1;REJECT(&d,reads,2,0,&r);d.read_cap=64;
    d.first_offset=d.last_offset;REJECT(&d,reads,2,0,&r);SETUP();
    d.arm_hi=UINT32_MAX;d.arm_lo=UINT32_MAX-2;REJECT(&d,reads,2,0,&r);SETUP();
    d.gate=PT_APERTURE_STALE;REJECT(&d,reads,1,0,&r);
    assert(pt_prepared_replay(&d,NULL,0,0,&r) && r.result==PT_APERTURE_STALE && !r.reads && !r.commits && !r.shadow);
#undef REJECT
#undef RUN
#undef TWO
#undef SETUP
}
struct paired_clock {struct pt_prepared_read values[64];unsigned at;};
static int paired_read(void *context,uint64_t *ticks,uint32_t *frequency)
{
    struct paired_clock *f=context;struct pt_prepared_read v;
    assert(f->at<64);v=f->values[f->at++];*ticks=((uint64_t)v.hi<<32)|v.lo;
    *frequency=v.frequency;return v.frequency?1:0;
}
/* Differential software check against the old arbitrary-callback model when
 * its source identity really is immutable. It says nothing about assembly
 * equivalence, asynchronous ownership or actual execution time. */
static void paired_reference(void)
{
    const uint32_t frequencies[2]={709379,715909},rates[2]={44100,48000};
    unsigned fi,ri,n,j;uint32_t seed=42;
    for(fi=0;fi<2;++fi)for(ri=0;ri<2;++ri)for(n=0;n<128;++n) {
        struct pt_aperture_state s;struct pt_aperture_control c=prepared_control();
        struct pt_prepared_clock d;struct pt_prepared_result r;struct paired_clock f;
        struct pt_elapsed_clock epoch;struct pt_aperture_policy p={128,256,2+n%63};
        uint64_t now;enum pt_aperture_result expected;
        assert(pt_elapsed_clock_init(&epoch,frequencies[fi],rates[ri],UINT64_C(0xffffa74d),0)==PT_ELAPSED_OK);
        assert(pt_aperture_prepare(&epoch,1536,&p,&c,&s)==PT_APERTURE_READY);
        assert(pt_prepared_clock_bind(&s,&c,&d)==PT_APERTURE_READY);
        memset(&f,0,sizeof(f));now=n%3?s.arm_at:s.first;
        for(j=0;j<64;++j) {
            seed=seed*UINT32_C(1664525)+UINT32_C(1013904223);
            now+=seed%21;f.values[j]=sample(now,frequencies[fi]);
        }
        if(n%7==0)f.values[n%5].frequency=0;
        if(n%11==0)f.values[n%5].frequency++;
        expected=pt_aperture_run(&s,&c,paired_read,&f,sizeof(f));
        assert(f.at==s.reads && f.at<=64);
        assert(pt_prepared_replay(&d,f.values,f.at,0,&r));
        assert(r.result==(unsigned)expected && r.reads==s.reads && r.commits==s.commits && r.shadow==s.shadow);
    }
}
static int cia_prepared_aperture_fixture(void)
{
    preparation();replay_boundaries();paired_reference();
    puts("CIA PREPARED MODEL PASS: serialized exact-key admission, original windows, full-width recorded-clock replay and irreversible commit accounting; diagnostic only");return 0;
}
#ifndef PT_PREPARED_APERTURE_EMBEDDED
int main(void){return cia_prepared_aperture_fixture();}
#endif
