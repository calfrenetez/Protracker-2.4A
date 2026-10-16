/* Explicit remaining formats reuse each unchanged genuine binding case. The
 * already qualified8mono fixture and its exact candidates remain separate. */
#include "sampler_mixed_binding_trace.h"
#define PT_MIXED_BINDING_MAIN mixed_format_family_donor_main
#include "sampler_mixed_source_binding_test.c"
#if !defined(PT_MIXED_BINDING_BITS) || (PT_MIXED_BINDING_BITS==8 && PT_MIXED_BINDING_CHANNELS==1)
#error Select an explicit remaining source format
#endif
#if !defined(PT_MIXED_FAMILY) || PT_MIXED_FAMILY<0 || PT_MIXED_FAMILY>4
#error Select one explicit genuine backend family
#endif
#if PT_MIXED_BINDING_BITS==8
#define FORMAT_LABEL "8stereo"
#elif PT_MIXED_BINDING_BITS==16 && PT_MIXED_BINDING_CHANNELS==1
#define FORMAT_LABEL "16mono"
#elif PT_MIXED_BINDING_BITS==16
#define FORMAT_LABEL "16stereo"
#elif PT_MIXED_BINDING_CHANNELS==1
#define FORMAT_LABEL "24mono"
#else
#define FORMAT_LABEL "24stereo"
#endif
#if PT_MIXED_BINDING_CHANNELS==1
#define FORMAT_FAULTS 5
#define FORMAT_COUNT 15
#define FORMAT_COUNTS ";15 cases;3 controls12 refusals;"
#define FORMAT_EXCLUDED ";other480 and legacy constructor excluded;SOFTWARE_ONLY"
#else
#define FORMAT_FAULTS 6
#define FORMAT_COUNT 18
#define FORMAT_COUNTS ";18 cases;3 controls15 refusals;"
#define FORMAT_EXCLUDED ";other477 and legacy constructor excluded;SOFTWARE_ONLY"
#endif
#if PT_MIXED_FAMILY==0
#define FAMILY_LABEL "paula"
#define FAMILY_CARD 0
#define FAMILY_BITS 8
#define FAMILY_LITTLE 0
#elif PT_MIXED_FAMILY==1
#define FAMILY_LABEL "amigus8be"
#define FAMILY_CARD 1
#define FAMILY_BITS 8
#define FAMILY_LITTLE 0
#elif PT_MIXED_FAMILY==2
#define FAMILY_LABEL "amigus8le"
#define FAMILY_CARD 1
#define FAMILY_BITS 8
#define FAMILY_LITTLE 1
#elif PT_MIXED_FAMILY==3
#define FAMILY_LABEL "amigus16be"
#define FAMILY_CARD 1
#define FAMILY_BITS 16
#define FAMILY_LITTLE 0
#else
#define FAMILY_LABEL "amigus16le"
#define FAMILY_CARD 1
#define FAMILY_BITS 16
#define FAMILY_LITTLE 1
#endif
int main(int argc,char **argv)
{
 unsigned fault,phase,count=0;
 mixed_binding_trace_begin(argc,argv);
 mixed_binding_trace("format-family-entered",PT_MIXED_BINDING_BITS,PT_MIXED_BINDING_CHANNELS,0);
 for(fault=0;fault<FORMAT_FAULTS;++fault)for(phase=0;phase<3;++phase){
   mixed_binding_trace("format-family-case-start",PT_MIXED_BINDING_BITS,phase,fault);
   mixed_binding_case(PT_MIXED_BINDING_BITS,PT_MIXED_BINDING_CHANNELS,
                      FAMILY_CARD,FAMILY_BITS,FAMILY_LITTLE,fault,phase);
   mixed_binding_trace("format-family-case-closed",PT_MIXED_BINDING_BITS,phase,fault);++count;
 }
 assert(count==FORMAT_COUNT);
 puts("MIXED FORMAT FAMILY PASS:" FORMAT_LABEL " " FAMILY_LABEL FORMAT_COUNTS
      "before-action/enqueue/active readiness;genuine pins and independent retirement" FORMAT_EXCLUDED);
 mixed_binding_trace("format-family-finished",PT_MIXED_BINDING_BITS,PT_MIXED_BINDING_CHANNELS,count);
 return 0;
}
