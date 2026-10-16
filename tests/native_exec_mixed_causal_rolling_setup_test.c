/* Separately scoped constructor/early-adoption component. This never runs or
 * qualifies the original rollingcase1/full68. All clocks/buses remain models.
 * The owned harness passes its separately prepared progress.log pathname.
 * Each diagnostic record is closed; partial observations never establish PASS. */
#ifndef PT_ROLLING_SETUP_MODE
#define PT_ROLLING_SETUP_MODE 0
#endif
#if PT_ROLLING_SETUP_MODE < 0 || PT_ROLLING_SETUP_MODE > 2
#error "setup mode must be 0 (construct), 1 (adopt/cancel), or 2 (one step/cancel)"
#endif
static int pt_setup_full_native_not_called(void) __attribute__((unused));
#define PT_PRIVATE_ROLLING_NATIVE_ENTRY pt_setup_full_native_not_called
#include "native_exec_mixed_causal_rolling_test.c"
#undef PT_PRIVATE_ROLLING_NATIVE_ENTRY

static const char *pt_setup_progress;
static unsigned pt_setup_io_failed,pt_setup_steps;
static void pt_setup_record(const char *stage,const struct sl_trial *e)
{
    FILE *stream=fopen(pt_setup_progress,"wb");
    if(!stream){pt_setup_io_failed=1;return;}
    if(fprintf(stream,"ROLLING SETUP %s mode=%u steps=%u phase=%u\n",stage,
               (unsigned)PT_ROLLING_SETUP_MODE,pt_setup_steps,
               e?(unsigned)e->producer.original.phase:0)<0)pt_setup_io_failed=1;
    if(fclose(stream))pt_setup_io_failed=1;
}


int main(int argc,char **argv)
{
    struct sl_trial *e;
    FILE *stream;
    char initial[32];size_t bytes;int failed;
    if(argc!=2 || !argv[1] || !argv[1][0] || strlen(argv[1])>255)return 20;
    pt_setup_progress=argv[1];stream=fopen(pt_setup_progress,"rb");
    if(!stream)return 20;
    bytes=fread(initial,1,sizeof(initial),stream);failed=ferror(stream);
    if(fclose(stream))failed=1;
    if(failed || bytes!=12 || memcmp(initial,"NOT STARTED\n",12))return 20;
    pt_setup_record("ENTRY",NULL);
    if(pt_setup_io_failed)return 20;
    pt_rolling_header_byte_controls();pt_setup_record("MAKE-BEGIN",NULL);
    e=sl_make(16,16,0,CS_MIXED,0);pt_setup_record("MAKE-END",e);
    assert(sl_cases==1);sl_same(e);
    if(PT_ROLLING_SETUP_MODE){
        pt_setup_record("ADOPT-BEGIN",e);sl_begin(e);pt_setup_record("ADOPT-END",e);
        if(PT_ROLLING_SETUP_MODE==2){
            pt_setup_record("STEP-BEGIN",e);
            assert(sl_step(e,17)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PENDING);
            ++pt_setup_steps;pt_setup_record("STEP-END",e);
        }
        pt_setup_record("CANCEL-BEGIN",e);sl_close(e);pt_setup_record("CANCEL-END",e);
        sl_same(e);
    }
    /* PCM fields are borrowed views; their pointer equality is not an
     * allocation-ownership proof. Reuse the original recorded fixture release
     * path, including its zero-accounting and no-alias-release assertions. */
    pt_setup_record("DISPOSE-BEGIN",e);sl_drop(e);
    assert(!sl_current);pt_setup_record("DONE",NULL);
    if(pt_setup_io_failed)return 20;
    printf("ROLLING SETUP NATIVE PASS:mode=%u;MODELED_ONLY\n",(unsigned)PT_ROLLING_SETUP_MODE);
    return 0;
}
