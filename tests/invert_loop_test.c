#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "invert_loop.h"
int main(void)
{
    unsigned speed,t;uint32_t index;
    for(speed=0;speed<16;++speed) {
        struct pt_invert_loop s={0};int pcm[8]={-128,-64,-1,0,1,63,64,127};
        assert(pt_invert_loop_bind(&s,8,2,8));assert(pt_invert_loop_speed(&s,speed));
        for(t=0;t<512;++t){index=999;if(pt_invert_loop_update(&s,&index)){assert(index>=2 && index<8);pcm[index]=-1-pcm[index];printf("%u %u %u\n",speed,t,index);}else assert(index==999);}
        for(t=0;t<8;++t)assert(pcm[t]>=-128 && pcm[t]<=127);
    }
    {
        struct pt_invert_loop s={0},before;
        assert(pt_invert_loop_speed(&s,14));index=999;
        assert(!pt_invert_loop_update(&s,&index));assert(s.accumulator==64);
        assert(!pt_invert_loop_update(&s,&index));assert(s.accumulator==0 && index==999);
        assert(pt_invert_loop_bind(&s,8,0,2));assert(!pt_invert_loop_update(&s,&index));
        assert(pt_invert_loop_speed(&s,0));assert(!pt_invert_loop_update(&s,&index) && s.accumulator==64);
        assert(pt_invert_loop_bind(&s,8,4,8));assert(s.accumulator==64 && s.cursor==4);
        assert(pt_invert_loop_speed(&s,14));assert(pt_invert_loop_update(&s,&index) && index==5);
        before=s;assert(!pt_invert_loop_bind(&s,8,3,8));assert(!memcmp(&s,&before,sizeof(s)));
        assert(!pt_invert_loop_speed(&s,16));assert(!memcmp(&s,&before,sizeof(s)));
        assert(!pt_invert_loop_update(&s,0));assert(!memcmp(&s,&before,sizeof(s)));
    }
    puts("INVERT clock PASS");return 0;
}
