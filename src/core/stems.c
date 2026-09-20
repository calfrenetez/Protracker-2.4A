#include "stems.h"
#include <string.h>
enum pt_stem_result pt_stems_plan(const struct pt_channels *channels,uint16_t selected,
                                unsigned grouped,struct pt_stem_plan *out)
{
    struct pt_stem_plan plan;unsigned ch,i;
    if(!out || !channels || pt_channels_validate(channels)!=PT_CHANNEL_OK ||
       !selected || (selected>>channels->count) || grouped>1)return PT_STEM_INVALID;
    memset(&plan,0,sizeof(plan));
    for(ch=0;ch<channels->count;++ch)if(selected&(1U<<ch)) {
        unsigned group=grouped?channels->track[ch].group:0;
        if(channels->track[ch].route==PT_MIDI)return PT_STEM_MIDI;
        for(i=0;i<plan.count;++i)if(group && plan.item[i].group==group)break;
        if(i==plan.count) {
            plan.item[i].channel=(uint8_t)ch;plan.item[i].group=(uint8_t)group;++plan.count;
        }
        plan.item[i].tracks|=(uint16_t)(1U<<ch);
    }
    *out=plan;return PT_STEM_OK;
}
