#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "../src/core/amigus_pcm_pack.h"
int main(void)
{
    int32_t data[514];uint32_t words[384];unsigned block,i,padding;size_t n;
    for(i=0;i<514;++i)data[i]=(int32_t)((i*7919U)&0xffffffU)-8388608;
    data[0]=-8388608;data[1]=8388607;data[2]=-1;data[3]=1;
    for(block=1;block<=256;block=block==1?17:block==17?256:257) {
        struct pt_amigus_pcm_pack s={0};unsigned at=0;size_t byte=0;
        while(at<257) {
            unsigned frames=257-at<block?257-at:block;
            struct pt_pcm p={data+at*2,frames*2,frames,48000,2,24};
            assert(pt_amigus_pcm_pack_block(&s,&p,words,384,&n));
            for(i=0;i<n*4;++i,++byte) {
                unsigned shift=16-(byte%3)*8;
                assert(((words[i/4]>>(24-(i%4)*8))&255)==(((uint32_t)data[byte/3]>>shift)&255));
            }
            at+=frames;
        }
        {struct pt_amigus_pcm_pack before=s;n=999;padding=99;
            assert(!pt_amigus_pcm_pack_finish(&s,words,2,&n,&padding));
            assert(!memcmp(&s,&before,sizeof(s)) && n==999 && padding==99);}
        assert(pt_amigus_pcm_pack_finish(&s,words,384,&n,&padding) && n==3 && padding==1);
        for(i=0;i<12;++i,++byte) {
            unsigned want=byte<514*3?((uint32_t)data[byte/3]>>(16-(byte%3)*8))&255:0;
            assert(((words[i/4]>>(24-(i%4)*8))&255)==want);
        }
        assert(byte==258*6);assert(pt_amigus_pcm_pack_finish(&s,words,384,&n,&padding) && !n && !padding);
    }
    {struct pt_amigus_pcm_pack s={0},before=s;struct pt_pcm p={data,4,2,48000,2,24};n=99;
        assert(!pt_amigus_pcm_pack_block(&s,&p,words,2,&n) && n==99 && !memcmp(&s,&before,sizeof(s)));
        assert(pt_amigus_pcm_pack_block(&s,&p,words,3,&n) && n==3);
        assert(words[0]==0x8000007fU && words[1]==0xffffFFFFU && words[2]==0xff000001U);
        assert(pt_amigus_pcm_pack_finish(&s,words,3,&n,&padding) && !n && !padding);
        assert(!pt_amigus_pcm_pack_block(&s,&p,words,3,&n));
        memset(&s,0,sizeof(s));p.bits=16;assert(!pt_amigus_pcm_pack_block(&s,&p,words,3,&n));
    }
    assert(data[0]==-8388608 && data[3]==1);
    puts("AMIGUS PACK PASS: true24 MSB words, stereo order, partition tails, explicit final padding, atomic refusal");return 0;
}
