#include "invert_loop.h"
int pt_invert_loop_bind(struct pt_invert_loop *s,uint32_t frames,uint32_t start,uint32_t end)
{
    if(!s || !frames || frames>131070 || (frames&1) || start>=end || end>frames || ((start|end)&1))return 0;
    s->start=start;s->end=end;s->cursor=start;s->bound=1;return 1;
}
int pt_invert_loop_speed(struct pt_invert_loop *s,unsigned speed)
{
    if(!s || speed>15)return 0;
    s->speed=(uint8_t)speed;return 1;
}
int pt_invert_loop_update(struct pt_invert_loop *s,uint32_t *index)
{
    static const uint8_t increments[]={0,5,6,7,8,10,11,13,16,19,22,26,32,43,64,128};
    if(!s || !index || !s->speed || s->speed>15)return 0;
    s->accumulator=(uint8_t)(s->accumulator+increments[s->speed]);
    if(!(s->accumulator&128))return 0;
    s->accumulator=0;
    if(!s->bound)return 0;
    if(++s->cursor>=s->end)s->cursor=s->start;
    *index=s->cursor;return 1;
}
