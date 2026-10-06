/* Preserve and run the exact committed legacy fixture once. New mode tests
 * below have independent prefixed names and never modify that source. */
#define main qmode_legacy_fixture_main
#include "mixed_readers_plan_normalize_test.c"
#undef main
#include "../src/core/amigus_trigger_levels.h"

struct qmode_fixture {
    struct fixture *f;
    struct pt_mixed_plan_quantized_batch *out;
};
static struct qmode_fixture qmode_make(unsigned bits,unsigned cache,unsigned endian,unsigned paula)
{
    struct qmode_fixture q;size_t used=pt_mixed_plan_normalizer_workspace_size();
    q.f=make(bits,cache,endian,paula);q.out=malloc(sizeof(*q.out));assert(q.out);
    free(q.f->owner);q.f->owner_capacity=sizeof(*q.out);
    q.f->owner=calloc(1,q.f->owner_capacity);assert(q.f->owner);
    free(q.f->workspace);q.f->capacity=used+sizeof(*q.out)+64;
    q.f->workspace=calloc(1,q.f->capacity);assert(q.f->workspace);
    memset((uint8_t *)q.f->workspace+used,0xa7,q.f->capacity-used);
    assert((uintptr_t)q.f->workspace%pt_mixed_plan_normalizer_workspace_alignment()==0);
    assert((uintptr_t)q.f->owner%_Alignof(struct pt_mixed_plan_quantized_batch)==0);
    return q;
}
static void qmode_drop(struct qmode_fixture *q)
{free(q->out);drop(q->f);q->out=NULL;q->f=NULL;}
static void qmode_begin(struct qmode_fixture *q)
{
    assert(!*q->f->owner);
    assert(pt_mixed_plan_normalizer_begin_quantized_in_workspace(q->f->workspace,
        q->f->capacity,&q->f->inputs,q->f->owner)==PT_MIXED_PLAN_PENDING);
    memset(q->out,0xa5,sizeof(*q->out));
}
static void qmode_unused(const struct pt_mixed_plan_quantized_batch *q)
{
    unsigned i;
    for(i=0;i<PT_MIXED_PLAN_RECORDS;++i){
        const struct pt_mixed_plan_trigger_levels *l=q->levels+i;
        const struct pt_mixed_plan_record *r=q->normalized.record+i;
        if(i>=q->normalized.count){
            assert(!r->kind&&!r->track&&!r->route&&!r->slot&&!r->sample&&!r->channel&&
                !r->first_action&&!r->control_action&&!r->geometry.amigus.bits&&
                !r->geometry.amigus.little_endian&&!r->geometry.amigus.trigger.rate_numerator&&
                !r->geometry.amigus.trigger.rate_denominator&&!r->geometry.amigus.trigger.offset&&
                !r->geometry.amigus.trigger.volume&&!r->geometry.amigus.trigger.pan&&
                !r->geometry.amigus.rate&&!r->geometry.amigus.left&&!r->geometry.amigus.right&&
                !r->image.start&&!r->image.loop&&!r->image.end_exclusive&&!r->image.rate&&
                !r->image.control&&!r->image.left&&!r->image.right);
        }
        if(i>=q->normalized.count||r->kind!=PT_MIXED_PLAN_TRIGGER||r->route!=PT_AMIGUS)
            assert(l->mode==PT_MIXED_PLAN_LEVEL_LEGACY&&!l->left&&!l->right);
        else assert(l->mode==PT_MIXED_PLAN_LEVEL_QUANTIZED&&l->left==r->image.left&&l->right==r->image.right&&
            !r->geometry.amigus.trigger.volume&&!r->geometry.amigus.trigger.pan);
    }
}
static void qmode_expect(struct qmode_fixture *q,enum pt_mixed_plan_result result,
    enum pt_mixed_plan_reason reason)
{
    struct pt_mixed_plan_quantized_batch *old=malloc(sizeof(*old));
    struct fixture *source=malloc(sizeof(*source));struct pt_mixed_plan_report report;
    assert(old&&source);memcpy(source,q->f,sizeof(*source));qmode_begin(q);
    memcpy(old,q->out,sizeof(*old));
    assert(pt_mixed_plan_normalizer_get_quantized(*q->f->owner,31,7,q->out)==PT_MIXED_PLAN_PENDING&&
        !memcmp(old,q->out,sizeof(*old)));
    assert(finish(q->f,17)==result);
    assert(pt_mixed_plan_normalizer_get(*q->f->owner,31,7,NULL)==PT_MIXED_PLAN_INVALID);
    assert(pt_mixed_plan_normalizer_get_quantized(*q->f->owner,31,7,q->out)==result);
    assert(pt_mixed_plan_normalizer_report(*q->f->owner,&report)==result&&report.reason==reason&&!report.candidates);
    if(result==PT_MIXED_PLAN_READY)qmode_unused(q->out);
    else assert(!memcmp(old,q->out,sizeof(*old)));
    assert(pt_mixed_plan_normalizer_close(q->f->owner)&&!*q->f->owner);
    assert(!memcmp(source,q->f,sizeof(*source)));free(source);free(old);
}
static void qmode_centre_and_shapes(void)
{
    struct qmode_fixture q=qmode_make(24,16,1,4);struct fixture *f=q.f;unsigned i;
    struct pt_mixed_plan_report before,after;struct pt_mixed_plan_batch *old=malloc(sizeof(*old));
    struct pt_mixed_plan_quantized_batch *saved=malloc(sizeof(*saved));assert(old&&saved);
    singleton(f,4,0);f->plan.action[0].gain[0]=32639;f->plan.action[0].gain[1]=32896;
    qmode_begin(&q);memset(f->output,0xa3,sizeof(*f->output));memcpy(old,f->output,sizeof(*old));
    memcpy(saved,q.out,sizeof(*saved));assert(pt_mixed_plan_normalizer_report(*f->owner,&before)==PT_MIXED_PLAN_PENDING);
    assert(pt_mixed_plan_normalizer_get(*f->owner,32,8,f->output)==PT_MIXED_PLAN_INVALID&&
        !memcmp(old,f->output,sizeof(*old)));
    assert(pt_mixed_plan_normalizer_get(*f->owner,32,8,NULL)==PT_MIXED_PLAN_INVALID);
    assert(pt_mixed_plan_normalizer_report(*f->owner,&after)==PT_MIXED_PLAN_PENDING&&!memcmp(&before,&after,sizeof(before)));
    assert(pt_mixed_plan_normalizer_get_quantized(*f->owner,31,7,NULL)==PT_MIXED_PLAN_PENDING);
    assert(pt_mixed_plan_normalizer_get_quantized(*f->owner,31,7,q.out)==PT_MIXED_PLAN_PENDING&&
        !memcmp(saved,q.out,sizeof(*saved)));
    assert(finish(f,1)==PT_MIXED_PLAN_READY);
    assert(pt_mixed_plan_normalizer_get_quantized(*f->owner,31,7,q.out)==PT_MIXED_PLAN_READY);
    assert(q.out->normalized.count==1&&q.out->levels[0].left==32639&&q.out->levels[0].right==32895);
    assert(q.out->normalized.record[0].image.left==32639&&q.out->normalized.record[0].image.right==32895);
    qmode_unused(q.out);assert(pt_mixed_plan_normalizer_close(f->owner));
    for(i=0;i<16;++i){struct pt_render_action *a=f->plan.action+i;
        *a=(struct pt_render_action){PT_RENDER_TRIGGER,i,initial(f,i%3),{1024,0}};
        if(i==1||i==2){a->gain[0]=0;a->gain[1]=1024;}
        if(i>=4){a->gain[0]=32639;a->gain[1]=32896;}
        f->plan.action[16+i]=*a;f->plan.action[16+i].kind=PT_RENDER_CONTROL;
        ++f->plan.action[16+i].voice.step;
    }
    f->plan.count=32;qmode_expect(&q,PT_MIXED_PLAN_READY,PT_MIXED_PLAN_REASON_NONE);
    assert(q.out->normalized.count==16);
    for(i=0;i<16;++i){const struct pt_mixed_plan_record *r=q.out->normalized.record+i;
        assert(r->track==i&&r->slot==(i<4?i:i-4)&&r->first_action==i&&r->control_action==16+i);
        assert(q.out->normalized.next[i].present&&q.out->normalized.next[i].frame==960);
    }
    for(i=32;i<64;++i)f->plan.action[i]=f->plan.action[i%16];
    f->plan.action[63].kind=PT_RENDER_SEGMENT;f->plan.count=64;
    /* With16 tracks, valid shapes can reach at most32 raw actions. The first
     * duplicate32 refuses; slot63 is still part of the immutable guarded input. */
    qmode_expect(&q,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_DUPLICATE);
    f->plan.count=65;qmode_expect(&q,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_COUNT);
    singleton(f,4,0);f->plan.action[1]=f->plan.action[0];f->plan.action[1].kind=PT_RENDER_CONTROL;
    f->plan.count=2;++f->plan.action[1].voice.phase;
    qmode_expect(&q,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_FOLD);
    memset(&f->plan,0,sizeof(f->plan));qmode_expect(&q,PT_MIXED_PLAN_READY,PT_MIXED_PLAN_REASON_NONE);
    assert(!q.out->normalized.count);free(saved);free(old);qmode_drop(&q);
    q=qmode_make(8,8,0,0);f=q.f;
    for(i=0;i<16;++i)f->plan.action[i]=(struct pt_render_action){PT_RENDER_TRIGGER,i,initial(f,0),{65536,65536}};
    f->plan.count=16;qmode_expect(&q,PT_MIXED_PLAN_READY,PT_MIXED_PLAN_REASON_NONE);
    for(i=0;i<16;++i)assert(q.out->normalized.record[i].slot==i&&q.out->levels[i].left==65535&&q.out->levels[i].right==65535);
    qmode_drop(&q);puts("QMODE-CENTRE PASS");
}
static void qmode_numeric(void)
{
    const unsigned sources[]={8,16,24},rates[]={44100,48000};
    const uint32_t gains[][2]={{0,0},{65536,65536},{32639,32896},{1,65535},{65536,0},{0,65536}};
    const uint32_t addresses[]={0,64,24576};unsigned b,cache,endian,rate,loop,step,g,addr,rows=0,oracles=0;
    for(b=0;b<3;++b)for(cache=8;cache<=16;cache+=8)for(endian=0;endian<2;++endian)
    for(rate=0;rate<2;++rate){struct qmode_fixture q=qmode_make(sources[b],cache,endian,4);struct fixture *f=q.f;
        uint64_t minimum=(768000U+rates[rate]-1)/rates[rate];
        uint64_t upper=((uint64_t)192000<<32)/rates[rate];
        const uint64_t steps[]={minimum,((uint64_t)8000<<32)/rates[rate],upper};
        f->inputs.rate=rates[rate];
        for(loop=0;loop<2;++loop)for(step=0;step<3;++step)for(g=0;g<6;++g){
            struct pt_amigus_voice_plan canonical,expected={0};
            f->samples[0].loop=loop?PT_LOOP_FORWARD:PT_LOOP_NONE;
            f->samples[0].loop_start=loop?2:0;f->samples[0].loop_end=loop?4096:0;
            f->samples[0].interpolation=loop;
            singleton(f,4,0);f->plan.action[0].voice.step=steps[step];
            f->plan.action[0].voice.loop=loop?PT_VOICE_FORWARD:PT_VOICE_ONCE;
            f->plan.action[0].voice.loop_start=loop?2:0;f->plan.action[0].voice.loop_end=loop?4096:0;
            f->plan.action[0].voice.cycle=loop?(uint64_t)4094<<32:0;f->plan.action[0].voice.linear=loop;
            if(loop){f->plan.action[0].voice.start=2;f->plan.action[0].voice.phase=(uint64_t)2<<32;}
            f->plan.action[0].gain[0]=gains[g][0];f->plan.action[0].gain[1]=gains[g][1];
            assert(pt_amigus_render_voice(&f->plan.action[0].voice,rates[rate],gains[g],&f->format,0,
                4096*(cache/8),&canonical));
            expected.start=loop?2*(cache/8):0;expected.loop=loop?2*(cache/8):0;expected.end_exclusive=4096*(cache/8);
            expected.rate=(uint32_t)(steps[step]*rates[rate]/768000);
            expected.control=(uint16_t)(0x8000|(cache==16?1:0)|(loop?6:0)|(cache==16&&endian?8:0));
            expected.left=(uint16_t)(((uint64_t)gains[g][0]*65535+32768)/65536);
            expected.right=(uint16_t)(((uint64_t)gains[g][1]*65535+32768)/65536);
            assert(image_same(&canonical,&expected));qmode_expect(&q,PT_MIXED_PLAN_READY,PT_MIXED_PLAN_REASON_NONE);
            assert(image_same(&q.out->normalized.record[0].image,&expected));
            for(addr=0;addr<3;++addr){struct pt_amigus_trigger_levels_request request={0};
                struct pt_amigus_voice_plan actual,relocated=expected;
                request.geometry=q.out->normalized.record[0].geometry.amigus.trigger;
                request.left=q.out->levels[0].left;request.right=q.out->levels[0].right;
                assert(pt_amigus_trigger_levels_prepare(&f->samples[0],&f->format,&request,addresses[addr],
                    4096*(cache/8),&actual));
                relocated.start+=addresses[addr];relocated.loop+=addresses[addr];relocated.end_exclusive+=addresses[addr];
                assert(image_same(&actual,&relocated));++oracles;
            }
            ++rows;
        }
        f->samples[0].loop=PT_LOOP_NONE;f->samples[0].loop_start=f->samples[0].loop_end=0;f->samples[0].interpolation=0;
        singleton(f,4,0);f->plan.action[0].voice.step=minimum-1;
        qmode_expect(&q,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_AMIGUS);
        singleton(f,4,0);f->plan.action[0].voice.step=upper+1;
        qmode_expect(&q,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_AMIGUS);
        singleton(f,4,0);f->plan.action[0].gain[0]=65537;
        qmode_expect(&q,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_AMIGUS);
        singleton(f,4,0);f->samples[0].pcm.channels=2;
        qmode_expect(&q,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_AMIGUS);f->samples[0].pcm.channels=1;
        singleton(f,4,0);f->plan.action[0].voice.pcm=(const struct pt_pcm *)(uintptr_t)1;
        qmode_expect(&q,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_SOURCE);
        singleton(f,4,0);f->plan.action[0].voice.end=4094;
        qmode_expect(&q,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_AMIGUS);
        f->format.channel=1;
        assert(pt_mixed_plan_normalizer_begin_quantized_in_workspace(f->workspace,f->capacity,
            &f->inputs,f->owner)==PT_MIXED_PLAN_INVALID&&!*f->owner);f->format.channel=0;
        f->inputs.rate=44101;
        assert(pt_mixed_plan_normalizer_begin_quantized_in_workspace(f->workspace,f->capacity,
            &f->inputs,f->owner)==PT_MIXED_PLAN_INVALID&&!*f->owner);f->inputs.rate=rates[rate];
        assert(!f->allocations.calls&&!f->allocations.releases);qmode_drop(&q);
    }
    assert(rows==864&&oracles==2592);puts("QMODE-NUMERIC PASS");
}
static void qmode_spans(void)
{
    struct qmode_fixture q=qmode_make(24,16,1,4);struct fixture *f=q.f;
    void *aliases[10];unsigned i;uint8_t *workspace=malloc(f->capacity),*source=malloc(sizeof(*f));
    struct pt_mixed_plan_report before,after;assert(workspace&&source);
    singleton(f,4,0);f->inputs.context_count=0;
    f->samples[2].slices=f->slices[2];f->samples[2].slice_count=2;f->slices[2][1]=2048;
    aliases[0]=&f->plan.action[63];aliases[1]=&f->origins[15];aliases[2]=f->pcm[2]+5000;
    aliases[3]=f->samples+2;aliases[4]=f->payload+1024;aliases[5]=f->slices[2];
    aliases[6]=&f->inputs;aliases[7]=f->contexts;aliases[8]=f->owner;
    aliases[9]=(uint8_t *)f->workspace+pt_mixed_plan_normalizer_workspace_size()+8;
    qmode_begin(&q);memcpy(workspace,f->workspace,f->capacity);memcpy(source,f,sizeof(*f));
    assert(pt_mixed_plan_normalizer_report(*f->owner,&before)==PT_MIXED_PLAN_PENDING);
    for(i=0;i<10;++i){
        assert(pt_mixed_plan_normalizer_get_quantized(*f->owner,32,8,aliases[i])==PT_MIXED_PLAN_ALIAS);
        assert(!memcmp(workspace,f->workspace,f->capacity)&&!memcmp(source,f,sizeof(*f)));
        assert(pt_mixed_plan_normalizer_report(*f->owner,&after)==PT_MIXED_PLAN_PENDING&&!memcmp(&before,&after,sizeof(before)));
    }
    assert(pt_mixed_plan_normalizer_close(f->owner));
    /* Only the new tag tail overlaps this genuine accessible context. A
     * legacy-size write would miss it; the full quantized getter must not. */
    f->inputs.context_count=1;f->contexts[0]=(struct pt_mixed_plan_span){q.out->levels,sizeof(q.out->levels)};
    qmode_begin(&q);memcpy(workspace,f->workspace,f->capacity);
    assert(pt_mixed_plan_normalizer_get_quantized(*f->owner,32,8,q.out)==PT_MIXED_PLAN_ALIAS);
    assert(!memcmp(workspace,f->workspace,f->capacity));
    assert(pt_mixed_plan_normalizer_get(*f->owner,32,8,(void *)q.out)==PT_MIXED_PLAN_INVALID);
    assert(!memcmp(workspace,f->workspace,f->capacity));assert(pt_mixed_plan_normalizer_close(f->owner));
    free(source);free(workspace);qmode_drop(&q);puts("QMODE-SPANS PASS");
}
static void qmode_stale(void)
{
    struct qmode_fixture q=qmode_make(24,16,1,4);struct fixture *f=q.f;
    struct pt_mixed_plan_report before,after;struct pt_project header;
    struct pt_mixed_plan_quantized_batch *saved=malloc(sizeof(*saved));assert(saved);
    singleton(f,4,0);qmode_begin(&q);memcpy(saved,q.out,sizeof(*saved));
    f->project.channels.selected=15;
    assert(pt_mixed_plan_normalizer_step(*f->owner,31,7,1)==PT_MIXED_PLAN_PENDING);
    assert(pt_mixed_plan_normalizer_report(*f->owner,&before)==PT_MIXED_PLAN_PENDING);
    header=f->project;f->project.samples=(void *)(uintptr_t)1;f->project.events=(void *)(uintptr_t)1;
    assert(pt_mixed_plan_normalizer_get(*f->owner,32,8,NULL)==PT_MIXED_PLAN_INVALID);
    assert(pt_mixed_plan_normalizer_report(*f->owner,&after)==PT_MIXED_PLAN_PENDING&&!memcmp(&before,&after,sizeof(before)));
    assert(pt_mixed_plan_normalizer_get_quantized(*f->owner,31,7,q.out)==PT_MIXED_PLAN_STALE&&!memcmp(saved,q.out,sizeof(*saved)));
    assert(pt_mixed_plan_normalizer_close(f->owner));f->project=header;
    qmode_begin(&q);--f->samples[0].pcm.capacity;
    assert(pt_mixed_plan_normalizer_step(*f->owner,31,7,1)==PT_MIXED_PLAN_STALE);
    assert(pt_mixed_plan_normalizer_close(f->owner));++f->samples[0].pcm.capacity;
    qmode_begin(&q);f->plan.action[63].channel=1;
    assert(pt_mixed_plan_normalizer_step(*f->owner,31,7,1)==PT_MIXED_PLAN_STALE);
    assert(pt_mixed_plan_normalizer_close(f->owner));f->plan.action[63].channel=0;
    qmode_begin(&q);assert(pt_mixed_plan_normalizer_get_quantized(*f->owner,31,8,q.out)==PT_MIXED_PLAN_STALE);
    assert(pt_mixed_plan_normalizer_close(f->owner));free(saved);qmode_drop(&q);puts("QMODE-STALE PASS");
}
static void qmode_cancel(void)
{
    const enum pt_mixed_plan_phase phases[]={PT_MIXED_PLAN_SHAPES,PT_MIXED_PLAN_ORIGINS,
        PT_MIXED_PLAN_RECORD,PT_MIXED_PLAN_SOURCE,PT_MIXED_PLAN_GEOMETRY,PT_MIXED_PLAN_COMPLETE};
    struct qmode_fixture q=qmode_make(24,16,1,4);struct fixture *f=q.f;unsigned i;
    singleton(f,4,0);f->plan.action[0].gain[0]=32639;f->plan.action[0].gain[1]=32896;
    for(i=0;i<sizeof(phases)/sizeof(phases[0]);++i){
        struct pt_mixed_plan_report report;struct pt_project header;unsigned steps=0;
        struct pt_mixed_plan_normalizer *copy;
        qmode_begin(&q);
        for(;;){enum pt_mixed_plan_result result=pt_mixed_plan_normalizer_report(*f->owner,&report);
            if(report.phase==phases[i])break;
            assert(result==PT_MIXED_PLAN_PENDING&&report.phase!=PT_MIXED_PLAN_GAINS&&!report.candidates);
            assert(pt_mixed_plan_normalizer_step(*f->owner,31,7,1)!=PT_MIXED_PLAN_REFUSED);
            assert(++steps<1000);
        }
        copy=*f->owner;assert(!pt_mixed_plan_normalizer_close(&copy)&&copy==*f->owner);
        header=f->project;f->project.samples=(void *)(uintptr_t)1;f->project.extensions=(void *)(uintptr_t)1;
        /* Both begin wrappers must recognize adopted mode before old/new walks. */
        assert(pt_mixed_plan_normalizer_begin_in_workspace(f->workspace,f->capacity,&f->inputs,f->owner)==PT_MIXED_PLAN_INVALID);
        assert(pt_mixed_plan_normalizer_begin_quantized_in_workspace(f->workspace,f->capacity,&f->inputs,f->owner)==PT_MIXED_PLAN_INVALID);
        assert(pt_mixed_plan_normalizer_close(f->owner)&&!*f->owner);
        assert(pt_mixed_plan_normalizer_close(f->owner));f->project=header;
    }
    qmode_drop(&q);puts("QMODE-CANCEL PASS");
}
static void qmode_legacy(void)
{
    struct qmode_fixture q=qmode_make(24,16,1,4);struct fixture *f=q.f;
    struct pt_mixed_plan_report before,after;struct pt_mixed_plan_quantized_batch *saved=malloc(sizeof(*saved));assert(saved);
    singleton(f,4,0);f->plan.action[0].gain[0]=32639;f->plan.action[0].gain[1]=32896;
    begin(f);memset(q.out,0xa5,sizeof(*q.out));memcpy(saved,q.out,sizeof(*saved));
    assert(pt_mixed_plan_normalizer_report(*f->owner,&before)==PT_MIXED_PLAN_PENDING);
    assert(pt_mixed_plan_normalizer_get_quantized(*f->owner,32,8,q.out)==PT_MIXED_PLAN_INVALID&&!memcmp(saved,q.out,sizeof(*saved)));
    assert(pt_mixed_plan_normalizer_get_quantized(*f->owner,32,8,NULL)==PT_MIXED_PLAN_INVALID);
    assert(pt_mixed_plan_normalizer_report(*f->owner,&after)==PT_MIXED_PLAN_PENDING&&!memcmp(&before,&after,sizeof(before)));
    assert(finish(f,256)==PT_MIXED_PLAN_REFUSED);
    assert(pt_mixed_plan_normalizer_report(*f->owner,&after)==PT_MIXED_PLAN_REFUSED&&
        after.reason==PT_MIXED_PLAN_REASON_GAINS&&after.candidates==16449);
    assert(pt_mixed_plan_normalizer_get_quantized(*f->owner,31,7,NULL)==PT_MIXED_PLAN_INVALID);
    assert(pt_mixed_plan_normalizer_close(f->owner));
    singleton(f,4,0);expect(f,PT_MIXED_PLAN_READY,PT_MIXED_PLAN_REASON_NONE);
    singleton(f,4,0);f->origins[4]=(struct pt_mixed_plan_origin){1,0,0,0};f->plan.action[0].kind=PT_RENDER_CONTROL;
    qmode_expect(&q,PT_MIXED_PLAN_READY,PT_MIXED_PLAN_REASON_NONE);
    assert(q.out->normalized.record[0].kind==PT_MIXED_PLAN_CONTROL&&!q.out->levels[0].mode);
    f->plan.action[0].kind=PT_RENDER_STOP;f->plan.action[0].voice=(struct pt_voice){0};
    qmode_expect(&q,PT_MIXED_PLAN_READY,PT_MIXED_PLAN_REASON_NONE);
    assert(!q.out->normalized.next[4].present&&!q.out->levels[0].mode);
    free(saved);qmode_drop(&q);puts("QMODE-LEGACY PASS");
}
int main(void)
{
    assert(qmode_legacy_fixture_main()==0);
    qmode_centre_and_shapes();qmode_numeric();qmode_spans();qmode_stale();qmode_cancel();qmode_legacy();
    return 0;
}
