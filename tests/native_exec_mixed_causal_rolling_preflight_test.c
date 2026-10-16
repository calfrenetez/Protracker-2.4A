/* Separate original preflight/cancellation component. The rolling68 entry is
 * retained but never invoked. All clocks/buses and allocations remain models.
 * Only an existing exact owned progress sidecar may be overwritten. */
#if defined(PT_ROLLING_PREFLIGHT_AUDIT_BOUNDARY) || defined(PT_ROLLING_PREFLIGHT_PIN_BOUNDARY) || defined(PT_ROLLING_PREFLIGHT_TRACE) || (defined(PT_ROLLING_PREFLIGHT_WORK256) || defined(PT_ROLLING_PREFLIGHT_WORK4096))
struct sl_trial;
static void pt_preflight_observe_step(struct sl_trial *,unsigned);
#define PT_PRIVATE_ROLLING_STEP_OBSERVE pt_preflight_observe_step
#endif
static int pt_preflight_full_native_not_called(void) __attribute__((unused));
#define PT_PRIVATE_ROLLING_NATIVE_ENTRY pt_preflight_full_native_not_called
#include "native_exec_mixed_causal_rolling_test.c"
#undef PT_PRIVATE_ROLLING_NATIVE_ENTRY
#if defined(PT_ROLLING_PREFLIGHT_AUDIT_BOUNDARY) || defined(PT_ROLLING_PREFLIGHT_PIN_BOUNDARY) || defined(PT_ROLLING_PREFLIGHT_TRACE) || (defined(PT_ROLLING_PREFLIGHT_WORK256) || defined(PT_ROLLING_PREFLIGHT_WORK4096))
#undef PT_PRIVATE_ROLLING_STEP_OBSERVE
#endif

#ifndef PT_ROLLING_PREFLIGHT_PREFIX
#define PT_ROLLING_PREFLIGHT_PREFIX 0
#endif
#if PT_ROLLING_PREFLIGHT_PREFIX != 0 && PT_ROLLING_PREFLIGHT_PREFIX != 64
#error "Only the original preflight or the separately bounded64-step prefix is supported"
#endif
#if defined(PT_ROLLING_PREFLIGHT_PIN_BOUNDARY) && PT_ROLLING_PREFLIGHT_PIN_BOUNDARY != 1
#error "Pin-boundary component must be selected with value1"
#endif
#if defined(PT_ROLLING_PREFLIGHT_PIN_BOUNDARY) && (defined(PT_ROLLING_PREFLIGHT_AUDIT_BOUNDARY) || PT_ROLLING_PREFLIGHT_PREFIX || defined(PT_ROLLING_PREFLIGHT_TRACE) || defined(PT_ROLLING_PREFLIGHT_WORK256) || defined(PT_ROLLING_PREFLIGHT_WORK4096))
#error "Pin-boundary and other preflight modes are separate components"
#endif
#if defined(PT_ROLLING_PREFLIGHT_AUDIT_BOUNDARY) && PT_ROLLING_PREFLIGHT_AUDIT_BOUNDARY != 1
#error "Audit-boundary component must be selected with value1"
#endif
#if defined(PT_ROLLING_PREFLIGHT_AUDIT_BOUNDARY) && (defined(PT_ROLLING_PREFLIGHT_PIN_BOUNDARY) || PT_ROLLING_PREFLIGHT_PREFIX || defined(PT_ROLLING_PREFLIGHT_TRACE) || defined(PT_ROLLING_PREFLIGHT_WORK256) || defined(PT_ROLLING_PREFLIGHT_WORK4096))
#error "Audit-boundary and other preflight modes are separate components"
#endif
#if PT_ROLLING_PREFLIGHT_PREFIX && (defined(PT_ROLLING_PREFLIGHT_TRACE) || (defined(PT_ROLLING_PREFLIGHT_WORK256) || defined(PT_ROLLING_PREFLIGHT_WORK4096)))
#error "Prefix and complete-preflight trace are separate components"
#endif
#if defined(PT_ROLLING_PREFLIGHT_TRACE) && (defined(PT_ROLLING_PREFLIGHT_WORK256) || defined(PT_ROLLING_PREFLIGHT_WORK4096))
#error "Original17-unit and supported256-unit preflights are separate components"
#endif
#if defined(PT_ROLLING_PREFLIGHT_WORK256) && defined(PT_ROLLING_PREFLIGHT_WORK4096)
#error "Supported preparation budgets are separate components"
#endif
#ifdef PT_ROLLING_PREFLIGHT_AUDIT_BOUNDARY
#define PT_PREFLIGHT_OBSERVATION_TAG "ROLLING AUDIT BOUNDARY"
#elif defined(PT_ROLLING_PREFLIGHT_PIN_BOUNDARY)
#define PT_PREFLIGHT_OBSERVATION_TAG "ROLLING PIN BOUNDARY"
#elif defined(PT_ROLLING_PREFLIGHT_WORK4096)
#define PT_PREFLIGHT_OBSERVATION_TAG "ROLLING WORK4096"
#elif defined(PT_ROLLING_PREFLIGHT_WORK256)
#define PT_PREFLIGHT_OBSERVATION_TAG "ROLLING WORK256"
#elif defined(PT_ROLLING_PREFLIGHT_TRACE)
#define PT_PREFLIGHT_OBSERVATION_TAG "ROLLING TRACE"
#endif

static const char *pt_preflight_progress;
static unsigned pt_preflight_io_failed;
static unsigned pt_preflight_steps;
static void pt_preflight_record(const char *stage,const struct sl_trial *e)
{
    FILE *stream=fopen(pt_preflight_progress,"wb");
    if(!stream){pt_preflight_io_failed=1;return;}
#ifdef PT_PREFLIGHT_OBSERVATION_TAG
    if(!strcmp(stage,"DONE")){
        if(fprintf(stream,PT_PREFLIGHT_OBSERVATION_TAG " DONE phase=0\n")<0)pt_preflight_io_failed=1;
    }else if(fprintf(stream,PT_PREFLIGHT_OBSERVATION_TAG " %s steps=%u phase=%u\n",stage,
                    pt_preflight_steps,e?(unsigned)e->producer.original.phase:0)<0)pt_preflight_io_failed=1;
#elif PT_ROLLING_PREFLIGHT_PREFIX
    if(fprintf(stream,"ROLLING PREFIX %s steps=%u phase=%u\n",stage,
               pt_preflight_steps,e?(unsigned)e->producer.original.phase:0)<0)pt_preflight_io_failed=1;
#else
    if(fprintf(stream,"ROLLING PREFLIGHT %s phase=%u\n",stage,
               e?(unsigned)e->producer.original.phase:0)<0)pt_preflight_io_failed=1;
#endif
    if(fclose(stream))pt_preflight_io_failed=1;
}

#ifdef PT_PREFLIGHT_OBSERVATION_TAG
/* Fixture observation only: leave the original step and every oracle intact.
 * Close a record only when a genuine phase changes; no clock/cost inference. */
static void pt_preflight_observe_step(struct sl_trial *e,unsigned after)
{
    static unsigned last_phase=~0U;
    unsigned phase=e->producer.original.phase;
    if(!after)++pt_preflight_steps;
    if(phase!=last_phase || (!after && !(pt_preflight_steps%64))){
        pt_preflight_record(phase==last_phase?"PROGRESS":after?"PHASE-END":"PHASE-BEGIN",e);
        last_phase=phase;
    }
}
#endif

int main(int argc,char **argv)
{
    struct sl_trial *e;
    FILE *stream;
    char initial[32];size_t bytes;int failed;
    if(argc!=2 || !argv[1] || !argv[1][0] || strlen(argv[1])>255)return 20;
    pt_preflight_progress=argv[1];stream=fopen(pt_preflight_progress,"rb");
    if(!stream)return 20;
    bytes=fread(initial,1,sizeof(initial),stream);failed=ferror(stream);
    if(fclose(stream))failed=1;
    if(failed || bytes!=12 || memcmp(initial,"NOT STARTED\n",12))return 20;
    pt_preflight_record("ENTRY",NULL);if(pt_preflight_io_failed)return 20;
    pt_rolling_header_byte_controls();pt_preflight_record("MAKE-BEGIN",NULL);
    e=sl_make(16,16,0,CS_MIXED,0);pt_preflight_record("MAKE-END",e);
    assert(sl_cases==1);sl_same(e);
    /* Complete-preflight modes retain every original helper oracle: source
     * adoption/pins, four normalized events and complete audit. Prefix/boundary
     * modes qualify only their distinct staged component and cancellation. */
#if defined(PT_ROLLING_PREFLIGHT_AUDIT_BOUNDARY)
    /* Reaching AUDIT_BEGIN proves genuine promotion but stops before the audit
     * case allocates anything. Keep original17-unit steps and semantic phase
     * selection; no HOST or native call count authorizes this transition. */
    {
        struct cp_trial *f=&e->base;unsigned n=0,i;
        pt_preflight_record("ADOPT-BEGIN",e);sl_begin(e);
        pt_preflight_record("ADOPT-END",e);
        while(e->producer.original.phase!=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_AUDIT_BEGIN){
            assert(sl_step(e,17)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PENDING&&++n<100000);
            assert(!f->ordinary.calls&&!e->port.binds&&!e->port.external.clocks&&!e->port.ram.publishes&&
                   !f->chip.calls&&!f->card->writes&&!sl_s(e)->causal&&!sl_s(e)->pool);
        }
        assert(e->producer.original.phase==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_AUDIT_BEGIN&&
               n&&n==pt_preflight_steps&&e->producer.original.activated&&sl_s(e)->source_activated);
        assert(e->producer.original.pin_slot==f->editor->project->sample_count&&
               e->producer.original.pin_count==f->editor->project->sample_count&&
               !e->producer.original.establish.active&&!e->producer.original.pin_job.owner&&
               !e->outputs->original.temporary_pin&&!e->outputs->original.persistent_pin);
        for(i=0;i<f->editor->project->sample_count;++i)
            assert(e->producer.original.pin[i]&&e->producer.original.pin[i]==f->editor->sampler.current[i]);
        assert(f->editor->sampler.allocator.context==&f->masters&&
               f->editor->sampler.allocator.allocate==sl_master_new&&
               f->editor->sampler.allocator.release==cp_master_free&&e->master_observed);
        assert(e->producer.original.original==&e->producer.original&&e->producer.original.adopted&&
               e->producer.original.controller==sl_s(e)&&!e->producer.original.borrow_closed&&
               !e->producer.original.hook_closed&&e->borrow->address==sl_s(e)&&e->borrow->serial&&
               e->borrow->serial==sl_s(e)->source_serial&&sl_s(e)->source_publisher==e->borrow&&
               sl_s(e)->source_held&&!sl_s(e)->source_released&&
               f->binding->preparation_context==sl_s(e)&&f->binding->preparation_close);
        assert(sl_s(e)->inputs==&e->producer.preparation.original.original&&
               !sl_s(e)->queue&&!e->outputs->original.audit&&!e->outputs->original.sequence&&
               !e->outputs->original.normalizer&&!e->producer.original.candidate_count&&
               !e->producer.original.frames&&!e->producer.original.intervals);
        sl_same(e);pt_preflight_record("AUDIT-BOUNDARY",e);
    }
#elif defined(PT_ROLLING_PREFLIGHT_PIN_BOUNDARY)
    /* Stop at the genuine establishment transition, rather than a HOST step
     * count: original17-unit work may cross this boundary at another call on
     * the native ABI. This does not invoke the complete preflight helper. */
    {
        unsigned n=0;
        pt_preflight_record("ADOPT-BEGIN",e);sl_begin(e);
        pt_preflight_record("ADOPT-END",e);
        while(e->producer.original.phase!=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PIN_BEGIN){
            assert(sl_step(e,17)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PENDING&&++n<100000);
            assert(!e->base.ordinary.calls&&!e->port.binds&&!e->port.external.clocks&&!e->port.ram.publishes&&
                   !e->base.chip.calls&&!e->base.card->writes&&!sl_s(e)->causal&&!sl_s(e)->pool);
        }
        assert(e->producer.original.phase==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PIN_BEGIN&&
               n&&n==pt_preflight_steps);
        pt_preflight_record("PIN-BOUNDARY",e);
    }
#elif PT_ROLLING_PREFLIGHT_PREFIX
    /* A distinct cancellation component, never a complete preflight oracle.
     * Keep the genuine begin,17-unit steps and every no-output prefix guard. */
    pt_preflight_record("ADOPT-BEGIN",e);sl_begin(e);
    pt_preflight_record("ADOPT-END",e);
    while(pt_preflight_steps<PT_ROLLING_PREFLIGHT_PREFIX){
        pt_preflight_record("STEP-BEGIN",e);
        assert(sl_step(e,17)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PENDING);
        ++pt_preflight_steps;
        assert(!e->base.ordinary.calls&&!e->port.binds&&!e->port.external.clocks&&!e->port.ram.publishes&&
               !e->base.chip.calls&&!e->base.card->writes&&!sl_s(e)->causal&&!sl_s(e)->pool);
        pt_preflight_record("STEP-END",e);
    }
    assert(pt_preflight_steps==64&&e->producer.original.phase==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_ESTABLISH_STEP);
#else
    (void)pt_preflight_steps;
    pt_preflight_record("PREFLIGHT-BEGIN",e);
#ifdef PT_ROLLING_PREFLIGHT_WORK4096
    /* Existing API maximum, separate from both17- and256-unit evidence. */
    sl_preflight_work(e,4096);
#elif defined(PT_ROLLING_PREFLIGHT_WORK256)
    /* Existing production API budget, separate from original17-unit evidence. */
    sl_preflight_work(e,256);
#else
    sl_preflight(e);
#endif
    pt_preflight_record("PREFLIGHT-END",e);
    assert(e->producer.original.phase==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CONTROLLER);
#endif
    pt_preflight_record("CANCEL-BEGIN",e);sl_close(e);
    pt_preflight_record("CANCEL-END",e);sl_same(e);
    /* Original recorded allocation ownership and all zero-byte/no-alias checks. */
    pt_preflight_record("DISPOSE-BEGIN",e);sl_drop(e);
    assert(!sl_current);pt_preflight_record("DONE",NULL);
    if(pt_preflight_io_failed)return 20;
#if defined(PT_ROLLING_PREFLIGHT_AUDIT_BOUNDARY)
    puts("ROLLING AUDIT BOUNDARY NATIVE PASS:original17 promotion and cancellation;MODELED_ONLY");
#elif defined(PT_ROLLING_PREFLIGHT_PIN_BOUNDARY)
    puts("ROLLING PIN BOUNDARY NATIVE PASS:original17 establishment and cancellation;MODELED_ONLY");
#elif PT_ROLLING_PREFLIGHT_PREFIX
    puts("ROLLING PREFIX NATIVE PASS:64 original steps and cancellation;MODELED_ONLY");
#elif defined(PT_ROLLING_PREFLIGHT_WORK4096)
    puts("ROLLING WORK4096 NATIVE PASS:complete preflight and cancellation;MODELED_ONLY");
#elif defined(PT_ROLLING_PREFLIGHT_WORK256)
    puts("ROLLING WORK256 NATIVE PASS:complete preflight and cancellation;MODELED_ONLY");
#elif defined(PT_ROLLING_PREFLIGHT_TRACE)
    puts("ROLLING TRACE NATIVE PASS:original preflight and cancellation;MODELED_ONLY");
#else
    puts("ROLLING PREFLIGHT NATIVE PASS:original preflight and cancellation;MODELED_ONLY");
#endif
    return 0;
}
