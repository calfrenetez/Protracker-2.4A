#include "clock_pair.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void clock_pair_fixture(void)
{
    struct pt_clock_pair p={{7,0xfffffff0U,709379},{8,0x12,709379}};
    struct pt_clock_pair_result r={99,88,77},before=r;unsigned i;
    assert(!pt_clock_pair_analyse(NULL,&r) && !memcmp(&r,&before,sizeof(r)));
    assert(!pt_clock_pair_analyse(&p,NULL));
    assert(!pt_clock_pair_analyse(&p,(struct pt_clock_pair_result *)&p));
    assert(!pt_clock_pair_analyse(&p,(struct pt_clock_pair_result *)&p.second));
    assert(pt_clock_pair_analyse(&p,&r) && !r.status && !r.delta_hi && r.delta_lo==34);
    p.second=p.first;assert(pt_clock_pair_analyse(&p,&r) && !r.status && !r.delta_hi && !r.delta_lo);
    p.second.hi=9;assert(pt_clock_pair_analyse(&p,&r) && !r.status && r.delta_hi==2 && !r.delta_lo);
    p.second.hi=6;assert(pt_clock_pair_analyse(&p,&r) && r.status==PT_CLOCK_PAIR_REGRESSION && !r.delta_hi && !r.delta_lo);
    p.second.hi=7;p.second.lo=0xffffffefU;
    assert(pt_clock_pair_analyse(&p,&r) && r.status==PT_CLOCK_PAIR_REGRESSION);
    p.first.hi=0xffffffffU;p.second.hi=0;
    assert(pt_clock_pair_analyse(&p,&r) && r.status==PT_CLOCK_PAIR_REGRESSION);
    for(i=0;i<2;++i){p.second.frequency=i?715909:0;
        assert(pt_clock_pair_analyse(&p,&r) && r.status==PT_CLOCK_PAIR_RATE && !r.delta_hi && !r.delta_lo);}
    p.first.frequency=0;p.second.frequency=0;
    assert(pt_clock_pair_analyse(&p,&r) && r.status==PT_CLOCK_PAIR_RATE);
    for(i=0;i<1024;++i){uint64_t first=((uint64_t)(i*7919U)<<32)|(0xffffffffU-i),second=first+i*65537ULL;
        p.first=(struct pt_clock_sample){(uint32_t)(first>>32),(uint32_t)first,715909};
        p.second=(struct pt_clock_sample){(uint32_t)(second>>32),(uint32_t)second,715909};
        assert(pt_clock_pair_analyse(&p,&r) && !r.status && (((uint64_t)r.delta_hi<<32)|r.delta_lo)==i*65537ULL);
    }
    puts("CLOCK PAIR MODEL PASS: full-width saved counters, unchanged invalid outputs, rate and regression accounting; observation only");
}
#ifndef PT_CLOCK_PAIR_EMBEDDED
int main(void){clock_pair_fixture();return 0;}
#endif
