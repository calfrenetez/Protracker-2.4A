#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "stems.h"
int main(void)
{
    struct pt_channels c;struct pt_stem_plan p,before;unsigned ch,mask;
    pt_channels_init(&c);assert(pt_channels_resize(&c,16)==PT_CHANNEL_OK);
    for(ch=0;ch<16;++ch)c.track[ch].group=(uint8_t)(ch%4);
    assert(pt_stems_plan(&c,65535,0,&p)==PT_STEM_OK && p.count==16);
    for(ch=0;ch<16;++ch)assert(p.item[ch].tracks==(1U<<ch) && !p.item[ch].group && p.item[ch].channel==ch);
    assert(pt_stems_plan(&c,65535,1,&p)==PT_STEM_OK && p.count==7);
    assert(p.item[0].tracks==1 && p.item[1].tracks==0x2222 && p.item[2].tracks==0x4444 && p.item[3].tracks==0x8888);
    assert(p.item[4].tracks==0x10 && p.item[5].tracks==0x100 && p.item[6].tracks==0x1000);
    /* Every nonempty selection partitions exactly, without overlap or widening. */
    for(mask=1;mask<65536;++mask) {
        unsigned union_mask=0;
        assert(pt_stems_plan(&c,(uint16_t)mask,1,&p)==PT_STEM_OK);
        for(ch=0;ch<p.count;++ch) {assert(!(union_mask&p.item[ch].tracks));union_mask|=p.item[ch].tracks;}
        assert(union_mask==mask);
    }
    before=p;c.track[15].route=PT_MIDI;c.track[15].muted=1;
    assert(pt_stems_plan(&c,65535,1,&p)==PT_STEM_MIDI && !memcmp(&p,&before,sizeof(p)));
    assert(pt_stems_plan(&c,32767,1,&p)==PT_STEM_OK);before=p;
    assert(pt_stems_plan(&c,0,1,&p)==PT_STEM_INVALID && !memcmp(&p,&before,sizeof(p)));
    assert(pt_stems_plan(&c,1,2,&p)==PT_STEM_INVALID && !memcmp(&p,&before,sizeof(p)));
    assert(pt_channels_resize(&c,4)==PT_CHANNEL_OK);
    assert(pt_stems_plan(&c,16,0,&p)==PT_STEM_INVALID && !memcmp(&p,&before,sizeof(p)));
    puts("STEMS plan PASS: all65535 selections,16 tracks, groups, exact partition and MIDI/invalid preservation");return 0;
}
