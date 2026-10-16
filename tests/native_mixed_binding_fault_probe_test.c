/* One genuine negative case per selection, with the original factory and
 * independent retirement assertions. This is not the75/495-case matrix. */
#include "sampler_mixed_binding_trace.h"
#define PT_MIXED_BINDING_TRACE(stage,x,y,n) mixed_binding_trace(stage,x,y,n)
#define PT_MIXED_BINDING_MAIN mixed_fault_probe_component_donor_main
#include "sampler_mixed_source_binding_test.c"
#if !defined(PT_MIXED_BINDING_BITS) || PT_MIXED_BINDING_BITS!=8 || PT_MIXED_BINDING_CHANNELS!=1
#error Fault probe requires the explicit8mono component selectors
#endif
#if !defined(PT_MIXED_FAULT_PROBE) || (PT_MIXED_FAULT_PROBE!=1 && PT_MIXED_FAULT_PROBE!=2)
#error Select one explicit genuine fault probe
#endif
int main(int argc,char **argv)
{
 mixed_binding_trace_begin(argc,argv);mixed_binding_trace("entered",0,0,0);
 mixed_binding_trace("case-start",8,1,1);
#if PT_MIXED_FAULT_PROBE==1
 mixed_binding_case(8,1,0,8,0,1,0);
 puts("MIXED BINDING FAULT PROBE PASS:one8mono Paula before-action wrong-sample refusal;genuine factory/cache/pins,unchanged masters and cleanup;other494 and legacy constructor excluded;SOFTWARE_ONLY");
#else
 mixed_binding_case(8,1,1,16,1,1,2);
 puts("MIXED BINDING FAULT PROBE PASS:one8mono AmiGUS16le active wrong-sample refusal;genuine pins retained to independent retirement,unchanged masters and bus;other494 and legacy constructor excluded;SOFTWARE_ONLY");
#endif
 mixed_binding_trace("finished",8,1,1);return 0;
}
