/* Separate original preflight/cancellation component. The rolling68 entry is
 * retained but never invoked. All clocks/buses and allocations remain models.
 * Only an existing exact owned progress sidecar may be overwritten. */
static int pt_preflight_full_native_not_called(void) __attribute__((unused));
#define PT_PRIVATE_ROLLING_NATIVE_ENTRY pt_preflight_full_native_not_called
#include "native_exec_mixed_causal_rolling_test.c"
#undef PT_PRIVATE_ROLLING_NATIVE_ENTRY

static const char *pt_preflight_progress;
static unsigned pt_preflight_io_failed;
static void pt_preflight_record(const char *stage,const struct sl_trial *e)
{
    FILE *stream=fopen(pt_preflight_progress,"wb");
    if(!stream){pt_preflight_io_failed=1;return;}
    if(fprintf(stream,"ROLLING PREFLIGHT %s phase=%u\n",stage,
               e?(unsigned)e->producer.original.phase:0)<0)pt_preflight_io_failed=1;
    if(fclose(stream))pt_preflight_io_failed=1;
}

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
    pt_preflight_record("PREFLIGHT-BEGIN",e);sl_preflight(e);
    pt_preflight_record("PREFLIGHT-END",e);
    assert(e->producer.original.phase==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CONTROLLER);
    pt_preflight_record("CANCEL-BEGIN",e);sl_close(e);
    pt_preflight_record("CANCEL-END",e);sl_same(e);
    /* Original recorded allocation ownership and all zero-byte/no-alias checks. */
    pt_preflight_record("DISPOSE-BEGIN",e);sl_drop(e);
    assert(!sl_current);pt_preflight_record("DONE",NULL);
    if(pt_preflight_io_failed)return 20;
    puts("ROLLING PREFLIGHT NATIVE PASS:original preflight and cancellation;MODELED_ONLY");
    return 0;
}
