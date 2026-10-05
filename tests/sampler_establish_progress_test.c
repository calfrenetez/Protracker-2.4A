/* Same complete assertion workload, with observable per-case boundaries.
 * Flushed progress is diagnostic only: no target clock or timing acceptance. */
#include <stdio.h>
#include <assert.h>
static unsigned progress_sequence;
static void case_progress(const char *group,const char *phase,unsigned bits,unsigned mode)
{
    assert(printf("PTPROGRESS %u %s %s %u %u\n",++progress_sequence,group,phase,bits,mode)>0);
    assert(fflush(stdout)==0);
}
#define PT_TEST_CASE_PROGRESS(group,phase,bits,mode) case_progress(group,phase,bits,mode)
#define PT_TEST_ESTABLISH_INCLUDED
#include "sampler_establish_test.c"
int main(void)
{
    int result;
    puts("PTFIXTURE BEGIN MASTER_ESTABLISH_PROGRESS_V1");assert(fflush(stdout)==0);
    result=sampler_establish_fixture();
    assert(progress_sequence==684);
    printf("PTFIXTURE END MASTER_ESTABLISH_PROGRESS_V1 %d\n",result);
    assert(fflush(stdout)==0);return result;
}
