/* One genuine control, not completion of the75/495-case matrix. The selected
 * component declarations exclude the separately qualified legacy constructor. */
#include "sampler_mixed_binding_trace.h"
#define PT_MIXED_BINDING_TRACE(stage,x,y,n) mixed_binding_trace(stage,x,y,n)
#define PT_MIXED_BINDING_MAIN mixed_probe_component_donor_main
#include "sampler_mixed_source_binding_test.c"
#if !defined(PT_MIXED_BINDING_BITS) || PT_MIXED_BINDING_BITS!=8 || PT_MIXED_BINDING_CHANNELS!=1
#error Probe requires the explicit8mono component selectors
#endif
int main(int argc,char **argv)
{
 mixed_binding_trace_begin(argc,argv);mixed_binding_trace("entered",0,0,0);
 mixed_binding_trace("case-start",8,1,0);
 mixed_binding_case(8,1,0,8,0,0,0);
 puts("MIXED BINDING PROBE PASS:one8mono Paula before-action control;genuine factory/cache/pins and cleanup;other494 cases and legacy constructor excluded;SOFTWARE_ONLY");
 mixed_binding_trace("finished",8,1,1);return 0;
}
