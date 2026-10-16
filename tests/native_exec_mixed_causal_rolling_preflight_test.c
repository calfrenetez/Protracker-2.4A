/* Separate original preflight/cancellation component. The rolling68 entry is
 * retained but never invoked. All clocks/buses and allocations remain models.
 * Only an existing exact owned progress sidecar may be overwritten. */
#ifdef PT_ROLLING_PREFLIGHT_TRACE
struct sl_trial;
static void pt_preflight_observe_step(struct sl_trial *,unsigned);
#define PT_PRIVATE_ROLLING_STEP_OBSERVE pt_preflight_observe_step
#endif
static int pt_preflight_full_native_not_called(void) __attribute__((unused));
#define PT_PRIVATE_ROLLING_NATIVE_ENTRY pt_preflight_full_native_not_called
#include "native_exec_mixed_causal_rolling_test.c"
#undef PT_PRIVATE_ROLLING_NATIVE_ENTRY
#ifdef PT_ROLLING_PREFLIGHT_TRACE
#undef PT_PRIVATE_ROLLING_STEP_OBSERVE
#endif

#ifndef PT_ROLLING_PREFLIGHT_PREFIX
#define PT_ROLLING_PREFLIGHT_PREFIX 0
#endif
#if PT_ROLLING_PREFLIGHT_PREFIX != 0 && PT_ROLLING_PREFLIGHT_PREFIX != 64
#error "Only the original preflight or the separately bounded64-step prefix is supported"
#endif
#if PT_ROLLING_PREFLIGHT_PREFIX && defined(PT_ROLLING_PREFLIGHT_TRACE)
#error "Prefix and complete-preflight trace are separate components"
#endif

static const char *pt_preflight_progress;
static unsigned pt_preflight_io_failed;
static unsigned pt_preflight_steps;
static void pt_preflight_record(const char *stage,const struct sl_trial *e)
{
    FILE *stream=fopen(pt_preflight_progress,"wb");
    if(!stream){pt_preflight_io_failed=1;return;}
#if PT_ROLLING_PREFLIGHT_PREFIX || defined(PT_ROLLING_PREFLIGHT_TRACE)
#ifdef PT_ROLLING_PREFLIGHT_TRACE
    if(!strcmp(stage,"DONE")){
        if(fprintf(stream,"ROLLING TRACE DONE phase=0\n")<0)pt_preflight_io_failed=1;
    }else if(fprintf(stream,"ROLLING TRACE %s steps=%u phase=%u\n",stage,
                    pt_preflight_steps,e?(unsigned)e->producer.original.phase:0)<0)pt_preflight_io_failed=1;
#else
    if(fprintf(stream,"ROLLING PREFIX %s steps=%u phase=%u\n",stage,
               pt_preflight_steps,e?(unsigned)e->producer.original.phase:0)<0)pt_preflight_io_failed=1;
#endif
#else
    if(fprintf(stream,"ROLLING PREFLIGHT %s phase=%u\n",stage,
               e?(unsigned)e->producer.original.phase:0)<0)pt_preflight_io_failed=1;
#endif
    if(fclose(stream))pt_preflight_io_failed=1;
}

#ifdef PT_ROLLING_PREFLIGHT_TRACE
/* Fixture observation only: leave the original step and every oracle intact.
 * Close a record only when a genuine phase changes; no clock/cost inference. */
static void pt_preflight_observe_step(struct sl_trial *e,unsigned after)
{
    static unsigned last_phase=~0U;
    unsigned phase=e->producer.original.phase;
    if(!after)++pt_preflight_steps;
    if(phase!=last_phase){
        pt_preflight_record(after?"PHASE-END":"PHASE-BEGIN",e);
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
    /* Invoke the original bounded helper with every original preflight oracle:
     * genuine source adoption/pins, four normalized events, complete audit,
     * untouched ordinary/Chip/card output and no command publication. */
#if PT_ROLLING_PREFLIGHT_PREFIX
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
    pt_preflight_record("PREFLIGHT-BEGIN",e);sl_preflight(e);
    pt_preflight_record("PREFLIGHT-END",e);
    assert(e->producer.original.phase==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CONTROLLER);
#endif
    pt_preflight_record("CANCEL-BEGIN",e);sl_close(e);
    pt_preflight_record("CANCEL-END",e);sl_same(e);
    /* Original recorded allocation ownership and all zero-byte/no-alias checks. */
    pt_preflight_record("DISPOSE-BEGIN",e);sl_drop(e);
    assert(!sl_current);pt_preflight_record("DONE",NULL);
    if(pt_preflight_io_failed)return 20;
#if PT_ROLLING_PREFLIGHT_PREFIX
    puts("ROLLING PREFIX NATIVE PASS:64 original steps and cancellation;MODELED_ONLY");
#elif defined(PT_ROLLING_PREFLIGHT_TRACE)
    puts("ROLLING TRACE NATIVE PASS:original preflight and cancellation;MODELED_ONLY");
#else
    puts("ROLLING PREFLIGHT NATIVE PASS:original preflight and cancellation;MODELED_ONLY");
#endif
    return 0;
}
