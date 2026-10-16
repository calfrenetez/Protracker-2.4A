/* Fifteen unchanged genuine cases, with closed observations only between
 * cases. No copied layout, forged owner, or counter/key repair for cleanup. */
#include "sampler_mixed_binding_trace.h"
#define PT_MIXED_BINDING_MAIN mixed_family_component_donor_main
#include "sampler_mixed_source_binding_test.c"
#if !defined(PT_MIXED_BINDING_BITS) || PT_MIXED_BINDING_BITS!=8 || PT_MIXED_BINDING_CHANNELS!=1
#error Family requires the explicit8mono component selectors
#endif
#if !defined(PT_MIXED_FAMILY) || PT_MIXED_FAMILY<0 || PT_MIXED_FAMILY>4
#error Select one explicit genuine backend family
#endif
#if PT_MIXED_FAMILY==0
#define FAMILY_COMPLETE "MIXED BINDING FAMILY PASS:8mono paula;15 cases;3 controls12 refusals;before-action/enqueue/active readiness;genuine pins and independent retirement;other480 and legacy constructor excluded;SOFTWARE_ONLY"
#define FAMILY_CARD 0
#define FAMILY_BITS 8
#define FAMILY_LITTLE 0
#elif PT_MIXED_FAMILY==1
#define FAMILY_COMPLETE "MIXED BINDING FAMILY PASS:8mono amigus8be;15 cases;3 controls12 refusals;before-action/enqueue/active readiness;genuine pins and independent retirement;other480 and legacy constructor excluded;SOFTWARE_ONLY"
#define FAMILY_CARD 1
#define FAMILY_BITS 8
#define FAMILY_LITTLE 0
#elif PT_MIXED_FAMILY==2
#define FAMILY_COMPLETE "MIXED BINDING FAMILY PASS:8mono amigus8le;15 cases;3 controls12 refusals;before-action/enqueue/active readiness;genuine pins and independent retirement;other480 and legacy constructor excluded;SOFTWARE_ONLY"
#define FAMILY_CARD 1
#define FAMILY_BITS 8
#define FAMILY_LITTLE 1
#elif PT_MIXED_FAMILY==3
#define FAMILY_COMPLETE "MIXED BINDING FAMILY PASS:8mono amigus16be;15 cases;3 controls12 refusals;before-action/enqueue/active readiness;genuine pins and independent retirement;other480 and legacy constructor excluded;SOFTWARE_ONLY"
#define FAMILY_CARD 1
#define FAMILY_BITS 16
#define FAMILY_LITTLE 0
#else
#define FAMILY_COMPLETE "MIXED BINDING FAMILY PASS:8mono amigus16le;15 cases;3 controls12 refusals;before-action/enqueue/active readiness;genuine pins and independent retirement;other480 and legacy constructor excluded;SOFTWARE_ONLY"
#define FAMILY_CARD 1
#define FAMILY_BITS 16
#define FAMILY_LITTLE 1
#endif
int main(int argc,char **argv)
{
 unsigned fault,phase,count=0;
 mixed_binding_trace_begin(argc,argv);mixed_binding_trace("family-entered",8,1,0);
 for(fault=0;fault<5;++fault)for(phase=0;phase<3;++phase){
   mixed_binding_trace("family-case-start",8,phase,fault);
   mixed_binding_case(8,1,FAMILY_CARD,FAMILY_BITS,FAMILY_LITTLE,fault,phase);
   mixed_binding_trace("family-case-closed",8,phase,fault);++count;
 }
 assert(count==15);
 puts(FAMILY_COMPLETE);
 mixed_binding_trace("family-finished",8,1,count);return 0;
}
