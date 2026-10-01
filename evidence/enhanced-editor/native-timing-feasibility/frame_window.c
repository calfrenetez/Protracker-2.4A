/* Host-only numerical feasibility probe. No device, IRQ, task or output code.
 * Independent integer oracle checks the current elapsed-clock admission window,
 * not the physical time at which Paula activates. Compile with elapsed_clock.c. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../../../src/core/elapsed_clock.h"

static uint64_t ceiling(uint64_t n, uint32_t d)
{return n/d + (n%d != 0);}

static uint64_t observed(const struct pt_elapsed_clock *epoch, uint64_t ticks)
{
    struct pt_elapsed_clock copy=*epoch;
    uint64_t frame=UINT64_MAX;
    assert(pt_elapsed_clock_advance(&copy,epoch->frequency,ticks,&frame)==PT_ELAPSED_OK);
    return frame;
}

int main(void)
{
    const uint32_t frequencies[]={709379,715909},rates[]={44100,48000};
    const uint64_t epoch_ticks=UINT64_C(0x123456789);
    unsigned f,r;
    puts("{\"scope\":\"host integer frame admission only; no native timing or output proof\",\"cases\":[");
    for(f=0;f<2;++f)for(r=0;r<2;++r) {
        struct pt_elapsed_clock epoch;
        uint64_t n,low,high,width,min=UINT64_MAX,max=0;
        unsigned counts[32]={0};
        assert(pt_elapsed_clock_init(&epoch,frequencies[f],rates[r],epoch_ticks,0)==PT_ELAPSED_OK);
        for(n=1;n<=rates[r];++n) {
            assert(pt_elapsed_clock_deadline(&epoch,n,&low)==PT_ELAPSED_OK);
            assert(pt_elapsed_clock_deadline(&epoch,n+1,&high)==PT_ELAPSED_OK);
            assert(low==epoch_ticks+ceiling(n*frequencies[f],rates[r]));
            assert(high==epoch_ticks+ceiling((n+1)*frequencies[f],rates[r]));
            assert(observed(&epoch,low-1)==n-1);
            assert(observed(&epoch,low)==n);
            assert(observed(&epoch,high-1)==n);
            assert(observed(&epoch,high)==n+1);
            width=high-low;assert(width<32);
            ++counts[width];if(width<min)min=width;if(width>max)max=width;
        }
        assert(min==frequencies[f]/rates[r] && max==min+1);
        printf("%s{\"frequency\":%lu,\"rate\":%lu,\"windows_checked\":%lu,"
               "\"minimum_ticks\":%lu,\"maximum_ticks\":%lu,"
               "\"minimum_count\":%u,\"maximum_count\":%u}",
               f||r?",\n":"",(unsigned long)frequencies[f],(unsigned long)rates[r],
               (unsigned long)rates[r],(unsigned long)min,(unsigned long)max,
               counts[min],counts[max]);
    }
    puts("\n]}");return 0;
}
