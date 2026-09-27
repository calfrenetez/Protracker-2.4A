/* Independent pinned-reference PCM oracle shared by render and ownership fixtures. */
static unsigned word(const unsigned char *p){return p[0]*256U+p[1];}
static uint32_t lng(const unsigned char *p){return (uint32_t)word(p)*65536+word(p+2);}
struct oracle {
    unsigned char *data,*initial;size_t size;
    unsigned char record[100][INVERT_RECORD_BYTES];uint64_t tick_end[100];unsigned ticks;
    unsigned tick,applied,end,last_trigger,repeat_loop,changes,delayed_changes;uint64_t phase,frames;
};
static void reset(struct oracle *o)
{
    memcpy(o->data,o->initial,o->size);o->tick=0;o->applied=0;o->end=0;
    o->last_trigger=0;o->repeat_loop=0;o->changes=0;o->delayed_changes=0;o->phase=0;o->frames=0;
}
static int receive(void *ctx,const struct pt_pcm *pcm,uint64_t offset)
{
    struct oracle *o=ctx;unsigned i;
    assert(offset==o->frames && pcm->channels==2 && pcm->bits==24);
    for(i=0;i<pcm->frames;++i) {
        const unsigned char *r;unsigned loop,length;int value;
        while(o->tick<o->ticks && offset+i>=o->tick_end[o->tick])++o->tick;
        assert(o->tick<o->ticks);r=o->record[o->tick];loop=lng(r+58);length=word(r+62)*2;
        assert(length>=2 && loop+length<=o->size);
        if(o->applied!=o->tick+1) {
            /* Independent reference snapshots: do not call production EFx code. */
            unsigned ch;
            if(INVERT_RECORD_BYTES==208) {
                assert(word(r+140)<=8 && !word(r+142));
                for(ch=0;ch<word(r+140);++ch) {
                    unsigned address=lng(r+144+8*ch);unsigned char value=r[148+8*ch];
                    assert(address>=2108 && address<o->size);
                    assert((unsigned char)(o->data[address]^255)==value);
                    o->data[address]=value;
                }
            } else for(ch=0;ch<(INVERT_RECORD_BYTES==188?2U:1U);++ch) {
                unsigned begin=lng(r+58+22*ch),bytes=word(r+62+22*ch)*2;
                assert((bytes==2 || bytes==16) && begin+bytes<=o->size);
                if(ch && begin==loop)assert(!memcmp(r+148,r+172,bytes));
                memcpy(o->data+begin,r+148+24*ch,bytes);
            }
            o->applied=o->tick+1;
        }
        if(word(r+72)!=o->last_trigger) {
            o->phase=(uint64_t)lng(r+66)<<32;o->end=lng(r+66)+word(r+70)*2;
            o->last_trigger=word(r+72);o->repeat_loop=loop;
        }
        assert((o->phase>>32)<o->size);value=o->data[o->phase>>32];if(value>127)value-=256;
        if(pcm->data[i*2]!=value*1024*(int)r[32] || pcm->data[i*2+1]!=0) {
            fprintf(stderr,"EFx handoff mismatch frame=%llu tick=%u phase=%llu got=%ld expected=%d\n",
                (unsigned long long)(offset+i),o->tick,(unsigned long long)o->phase,(long)pcm->data[i*2],value*1024*(int)r[32]);abort();
        }
        assert(word(r+44));o->phase+=(428ULL<<32)/word(r+44);
        if((o->phase>>32)>=o->end) {
            if(loop!=o->repeat_loop) {
                /* Demonstrate the transfer waits for a repeat boundary inside
                   the tick, retaining the fractional phase remainder. */
                assert(o->tick && offset+i>=o->tick_end[o->tick-1]);++o->changes;
                if(offset+i>o->tick_end[o->tick-1])++o->delayed_changes;
            }
            o->phase-=(uint64_t)o->end<<32;o->phase%=((uint64_t)length<<32);
            o->phase+=(uint64_t)loop<<32;o->end=loop+length;o->repeat_loop=loop;
        }
    }
    o->frames+=pcm->frames;return 1;
}
