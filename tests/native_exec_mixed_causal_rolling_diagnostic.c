/* Separately scoped native diagnostic, never a replacement full68 PASS.
 * Select one original case at compile time (1..68), or 0 for the HOST audit.
 * Preserve all called case assertions, source/reader ownership and cleanup.
 * Flush BEGIN/END witnesses so an interrupted run is not mistaken for success.
 * The clock, buses and retirement callbacks remain software models. */
#ifndef PT_ROLLING_DIAGNOSTIC_CASE
#define PT_ROLLING_DIAGNOSTIC_CASE 1
#endif
#if PT_ROLLING_DIAGNOSTIC_CASE < 0 || PT_ROLLING_DIAGNOSTIC_CASE > 68
#error "rolling diagnostic case must be 0 (all), or 1..68"
#endif
static int pt_rolling_full_native_not_called(void) __attribute__((unused));
#define PT_PRIVATE_ROLLING_NATIVE_ENTRY pt_rolling_full_native_not_called
#include "native_exec_mixed_causal_rolling_test.c"
#undef PT_PRIVATE_ROLLING_NATIVE_ENTRY

static void pt_rolling_diagnostic_case(unsigned index)
{
    unsigned b,r,offset;
    if(index<=4){
        static const char *names[]={"mixed16","mixed24-sourcehold","card16","card24-sourcehold"};
        sl_case=names[index-1];sl_four(index%2?16:24,index>2,index%2?0:1);
    }else if(index<=8){
        offset=index-5;b=2+offset/2;r=offset%2;
        sl_case=r?"root-history-refusal":"latest-history-refusal";sl_refuse_history(b,r);
    }else if(index<=14){
        offset=index-9;b=2+offset/3;r=offset%3;
        sl_case=r==0?"clean-arm0":r==1?"unknown-arm":"reentrant-arm0";sl_refuse_arm(b,r);
    }else if(index==15){
        sl_case="administrative-cancel";sl_admin_cancel();
    }else if(index<=27){
        offset=index-16;b=1+offset/4;r=(offset/2)%2;
        sl_case=offset%2?"arm-custody-mutation":"clock-custody-mutation";
        sl_callback_mutation(b,offset%2,r);
    }else if(index<=29){
        sl_case="quiet-custody-mutation";sl_quiet_mutation(index-28);
    }else if(index<=38){
        offset=index-30;r=offset/3;
        if(offset%3==0){sl_case="clock-mode-mutation";sl_callback_mutation(2,0,r+3);}
        else if(offset%3==1){sl_case="arm-mode-mutation";sl_callback_mutation(2,1,r+2);}
        else{sl_case="quiet-mode-mutation";sl_quiet_mutation(r+2);}
    }else if(index<=41){
        sl_case="arm-entry-state-mutation";sl_arm_state_mutation(index-39);
    }else if(index==42){
        sl_case="quiet-entry-state-mutation";sl_quiet_mutation(5);
    }else if(index<=48){
        sl_case="initial-clock-refusal";sl_initial_clock_refusal(index-43);
    }else if(index<=51){
        sl_case="initial-arm-refusal";sl_refuse_arm(0,index-49);
    }else if(index==52){
        sl_case="persistent-profile-quiet-state-mutation";sl_persistent_profile_quiet_mutation();
    }else if(index<=64){
        offset=index-53;b=offset/6;r=(offset/2)%3;
        sl_case=offset%2?"quiet-owned-row-erasure":"arm-owned-row-erasure";
        sl_owned_row_mutation(offset%2?0:1,b,r);
    }else{
        static const char *names[]={"cancelled-target-not-quiet","retired-target","draining-target","refusing-current-owner"};
        assert(index<=68);sl_case=names[index-65];sl_target_refusal(index-65);
    }
}

int main(void)
{
    unsigned first=PT_ROLLING_DIAGNOSTIC_CASE?PT_ROLLING_DIAGNOSTIC_CASE:1;
    unsigned last=PT_ROLLING_DIAGNOSTIC_CASE?PT_ROLLING_DIAGNOSTIC_CASE:68;
    unsigned i;
    assert(sizeof(uint64_t)==8 && sizeof(uint32_t)==4);
    assert(printf("ROLLING DIAGNOSTIC ENTRY:case=%u;SOFTWARE_MODELS_ONLY\n",
                  (unsigned)PT_ROLLING_DIAGNOSTIC_CASE)>0 && fflush(stdout)==0);
    pt_rolling_header_byte_controls();
    assert(printf("ROLLING HEADER CONTROLS PASS\n")>0 && fflush(stdout)==0);
    for(i=first;i<=last;++i){
        assert(printf("ROLLING CASE %u BEGIN\n",i)>0 && fflush(stdout)==0);
        pt_rolling_diagnostic_case(i);
        assert(sl_cases==i-first+1 && !sl_current);
        assert(printf("ROLLING CASE %u END\n",i)>0 && fflush(stdout)==0);
    }
    assert(sl_cases==last-first+1);
    assert(printf("ROLLING DIAGNOSTIC PASS:case=%u cases=%u of68;SOFTWARE_MODELS_ONLY\n",
                  (unsigned)PT_ROLLING_DIAGNOSTIC_CASE,sl_cases)>0 && fflush(stdout)==0);
    return 0;
}
