/* The entry-renamed accepted sequence35 donor supplies genuine measured
 * renderer preparation, current master pins, selective Chip leases, scheduled
 * publication and independent reader retirement. Its old main is never called.
 * The unqualified future backend's callbacks are not used by this fixture. */
#include "paula_segment_sequence_packet32_basis.c"
#include "../src/editor/paula_segment_packet32.h"
struct packet32_capture {
    struct sequence_fixture *owner;unsigned conversions,negative_cases;
    union pt_paula_packet32_record record[4];
};
static struct packet32_capture capture;
static unsigned packet32_cases;
static uint32_t read_word(const union pt_paula_packet32_record *r,unsigned at)
{const uint8_t *b=r->bytes+4U*at;return ((uint32_t)b[0]<<24)|((uint32_t)b[1]<<16)|((uint32_t)b[2]<<8)|b[3];}
static uint64_t read_wide(const union pt_paula_packet32_record *r,unsigned at)
{return ((uint64_t)read_word(r,at)<<32)|read_word(r,at+1);}
static void mapping_for(const struct pt_paula_future_packet *p,struct pt_paula_packet32_mapping *m)
{
    unsigned i;memset(m,0,sizeof(*m));m->source_count=p->geometry.source_count;
    for(i=0;i<m->source_count;++i){const struct pt_paula_segment_source *s=p->geometry.source+i;
        struct pt_paula_packet32_source_map *r=m->source+i;assert(s->sample<3);
        r->descriptor=s->descriptor;r->master_data=s->master.data;r->version=s->version;
        r->sampler_owner=s->sampler_owner;r->project_identity=s->project_identity;
        r->cache_owner=s->cache_owner;r->cache_data=s->cache_data;
        r->descriptor_tag=UINT32_C(0x100)+s->sample;r->master_tag=UINT32_C(0x200)+s->sample;
        r->version_tag=UINT32_C(0x300)+s->sample;r->sampler_tag=UINT32_C(0x400);
        r->project_tag=UINT32_C(0x500);r->cache_owner_tag=UINT32_C(0x600);
        r->cache_base32=UINT32_C(0x100000)+UINT32_C(0x1000)*s->sample;
    }
}
static void preserved(const struct pt_paula_future_packet *original,const struct pt_paula_packet32_mapping *mapping,unsigned which)
{
    struct pt_paula_future_packet p;struct pt_paula_packet32_mapping m;
    union pt_paula_packet32_record out,before;unsigned fast=capture.owner->f.fast.calls;
    memcpy(&p,original,sizeof(p));memcpy(&m,mapping,sizeof(m));memset(&out,0x6d,sizeof(out));memcpy(&before,&out,sizeof(before));
    switch(which){
    case 0:p.ticket=0;break;
    case 1:p.first=p.last;break;
    case 2:p.geometry.frame=UINT64_MAX;break;
    case 3:p.geometry.track=16;break;
    case 4:p.geometry.slot=4;break;
    case 5:p.geometry.source_count=3;m.source_count=3;break;
    case 6:m.source_count=0;break;
    case 7:p.geometry.period=0;break;
    case 8:p.geometry.volume=65;break;
    case 9:p.geometry.write_initial=0;break;
    case 10:p.geometry.write_repeat=0;break;
    case 11:++p.geometry.initial.frame_offset;++p.geometry.initial.byte_offset;break;
    case 12:--p.geometry.initial.frame_length;--p.geometry.initial.byte_length;break;
    case 13:p.geometry.initial.words=0;break;
    case 14:--p.geometry.repeat.byte_length;break;
    case 15:p.geometry.repeat.frame_offset=p.geometry.source[0].master.frames;p.geometry.repeat.byte_offset=p.geometry.repeat.frame_offset;break;
    case 16:++p.geometry.phase_q32;break;
    case 17:m.source[0].descriptor=NULL;break;
    case 18:m.source[0].version_tag=0;break;
    case 19:++m.source[0].cache_base32;break;
    case 20:m.source[0].cache_base32=UINT32_MAX-1U;break;
    case 21:p.geometry.source[0].master.capacity=0;break;
    case 22:p.geometry.source[0].master.channels=2;break;
    case 23:p.geometry.source[0].master.bits=32;break;
    case 24:--p.geometry.source[0].cache_bytes;break;
    case 25:p.geometry.source[0].guard_count=PT_PAULA_SEGMENT_SOURCE_GUARDS+1U;break;
    case 26:assert(p.geometry.source[0].guard_count);p.geometry.source[0].guard[0].bytes=0;break;
    case 27:++p.geometry.source[0].cache_data;m.source[0].cache_data=p.geometry.source[0].cache_data;break;
    default:assert(0);
    }
    assert(pt_paula_segment_packet32_build(&p,&m,&out)!=PT_PAULA_PACKET32_OK);
    assert(!memcmp(&out,&before,sizeof(out))&&capture.owner->f.fast.calls==fast);
    assert(pinned(&capture.owner->f)==3&&capture.owner->f.chip.bytes==2304);
    storage(&capture.owner->f);++packet32_cases;++capture.negative_cases;
}
static void alias_refusers(const struct pt_paula_future_packet *p,const struct pt_paula_packet32_mapping *m)
{
    uint8_t before[PT_PAULA_PACKET32_BYTES];union pt_paula_packet32_record out;
    union {uint32_t alignment;uint8_t bytes[PT_PAULA_PACKET32_BYTES+4U];} unaligned;
    struct pt_paula_future_packet copied;struct pt_paula_packet32_mapping mapped;
    const struct pt_paula_segment_source *s=p->geometry.source;
    union pt_paula_packet32_record *master=(union pt_paula_packet32_record *)(void *)(s->master.data+32);
    union pt_paula_packet32_record *cache=(union pt_paula_packet32_record *)(void *)(s->cache_data+4);
    assert(32U*sizeof(int32_t)+sizeof(*master)<=s->master.capacity*sizeof(int32_t));
    assert(4U+sizeof(*cache)<=s->cache_bytes);
    memcpy(before,master,sizeof(before));assert(pt_paula_segment_packet32_build(p,m,master)==PT_PAULA_PACKET32_ALIAS);
    assert(!memcmp(before,master,sizeof(before)));++packet32_cases;
    memcpy(before,cache,sizeof(before));assert(pt_paula_segment_packet32_build(p,m,cache)==PT_PAULA_PACKET32_ALIAS);
    assert(!memcmp(before,cache,sizeof(before)));++packet32_cases;
    memcpy(&copied,p,sizeof(copied));assert(sizeof(copied)>=sizeof(out));
    assert(pt_paula_segment_packet32_build(&copied,m,(union pt_paula_packet32_record *)(void *)&copied)==PT_PAULA_PACKET32_ALIAS);
    assert(!memcmp(&copied,p,sizeof(copied)));++packet32_cases;
    memcpy(&mapped,m,sizeof(mapped));
    assert(pt_paula_segment_packet32_build(p,&mapped,(union pt_paula_packet32_record *)(void *)&mapped)==PT_PAULA_PACKET32_ALIAS);
    assert(!memcmp(&mapped,m,sizeof(mapped)));++packet32_cases;
    memset(&unaligned,0x37,sizeof(unaligned));memcpy(before,unaligned.bytes+1,sizeof(before));
    assert(pt_paula_segment_packet32_build(p,m,(union pt_paula_packet32_record *)(void *)(unaligned.bytes+1))==PT_PAULA_PACKET32_ALIAS);
    assert(!memcmp(before,unaligned.bytes+1,sizeof(before)));++packet32_cases;
    storage(&capture.owner->f);
}
static void full_width(const struct pt_paula_future_packet *original,const struct pt_paula_packet32_mapping *m)
{
    struct pt_paula_future_packet p;union pt_paula_packet32_record out;
    /* A copied metadata observation, not a fabricated core ticket/activation.
     * This independently exercises nonzero high words without changing original
     * descriptor/queue/lease/version objects or claiming currentness. */
    memcpy(&p,original,sizeof(p));p.ticket=UINT64_C(0x1122334455667788);
    p.first=UINT64_C(0x23456789abcdef00);p.last=p.first+UINT64_C(17);
    p.geometry.frame=UINT64_C(0x3456789abcdef001);
    assert(pt_paula_segment_packet32_build(&p,m,&out)==PT_PAULA_PACKET32_OK);
    assert(read_wide(&out,10)==p.ticket&&read_wide(&out,12)==p.first&&read_wide(&out,14)==p.last&&read_wide(&out,16)==p.geometry.frame);
    assert(out.bytes[40]==0x11&&out.bytes[47]==0x88&&read_wide(&out,18)==original->geometry.phase_q32);
    ++packet32_cases;
}
static void dual_refusers(const struct pt_paula_future_packet *original,const struct pt_paula_packet32_mapping *mapping)
{
    unsigned i;assert(original->geometry.source_count==2);
    for(i=0;i<10;++i){struct pt_paula_future_packet p;struct pt_paula_packet32_mapping m;
        union pt_paula_packet32_record out,before;
        memcpy(&p,original,sizeof(p));memcpy(&m,mapping,sizeof(m));memset(&out,0x53,sizeof(out));memcpy(&before,&out,sizeof(before));
        if(i==0)++m.source[1].project_tag;
        else if(i==1)m.source[1].descriptor_tag=m.source[0].descriptor_tag;
        else if(i==2)m.source[1].master_tag=m.source[0].master_tag;
        else if(i==3)m.source[1].version_tag=m.source[0].version_tag;
        else if(i==4)m.source[1].cache_base32=m.source[0].cache_base32;
        else if(i==5)m.source[1].cache_base32=m.source[0].cache_base32+2U;
        else if(i==6)m.source[1].descriptor=m.source[0].descriptor;
        else if(i==7)p.geometry.source[0].master.bits=p.geometry.source[1].master.bits=16;
        else if(i==8)++p.geometry.source[1].master.rate;
        else ++p.geometry.source[1].generation;
        assert(pt_paula_segment_packet32_build(&p,&m,&out)!=PT_PAULA_PACKET32_OK);
        assert(!memcmp(&out,&before,sizeof(out))&&pinned(&capture.owner->f)==3);
        storage(&capture.owner->f);++packet32_cases;++capture.negative_cases;
    }
}
static int packet32_submit(void *v,const struct pt_readers_event *event,const struct pt_paula_segment_plan *plan)
{
    struct pt_paula_future_packet p;struct pt_paula_packet32_mapping m;unsigned i,index=capture.conversions;
    unsigned fast=capture.owner->f.fast.calls;union pt_paula_packet32_record *r;
    assert(v==&capture.owner->provider&&index<4&&pinned(&capture.owner->f)==3);
    p.ticket=event->scheduled.ticket;p.first=event->scheduled.first;p.last=event->scheduled.last;
    memcpy(&p.geometry,plan,sizeof(p.geometry));mapping_for(&p,&m);r=capture.record+index;
    assert(pt_paula_segment_packet32_build(&p,&m,r)==PT_PAULA_PACKET32_OK);
    assert(read_word(r,0)==UINT32_C(0x50473332)&&read_word(r,1)==1&&read_word(r,2)==296);
    assert(read_wide(r,10)==event->scheduled.ticket&&read_wide(r,12)==event->scheduled.first&&read_wide(r,14)==event->scheduled.last);
    assert(read_wide(r,16)==event->scheduled.batch.frame&&read_wide(r,16)==plan->frame&&read_wide(r,18)==plan->phase_q32);
    assert(read_word(r,3)==plan->source_count&&read_word(r,4)==(uint32_t)plan->kind);
    assert(read_word(r,5)==plan->track&&read_word(r,6)==plan->slot&&read_word(r,8)==plan->period&&read_word(r,9)==plan->volume);
    assert(read_word(r,20)==(plan->write_initial?m.source[0].cache_base32+plan->initial.byte_offset:0));
    assert(read_word(r,21)==(plan->write_repeat?m.source[plan->source_count==2?1:0].cache_base32+plan->repeat.byte_offset:0));
    assert(read_word(r,22)==plan->initial.frame_offset&&read_word(r,23)==plan->initial.frame_length&&read_word(r,26)==plan->initial.words);
    assert(read_word(r,27)==plan->repeat.frame_offset&&read_word(r,28)==plan->repeat.frame_length&&read_word(r,31)==plan->repeat.words);
    for(i=0;i<plan->source_count;++i){unsigned at=32U+21U*i;const struct pt_paula_segment_source *s=plan->source+i;
        assert(s->version==capture.owner->f.pin[s->sample]);
        assert(read_word(r,at)==m.source[i].descriptor_tag&&read_word(r,at+1)==m.source[i].master_tag&&read_word(r,at+2)==m.source[i].version_tag);
        assert(read_word(r,at+6)==m.source[i].cache_base32&&read_word(r,at+7)==s->cache_bytes&&read_word(r,at+8)==s->master.capacity);
        assert(read_word(r,at+9)==s->master.frames&&read_word(r,at+10)==s->master.rate&&read_word(r,at+11)==s->master.bits&&read_word(r,at+12)==1);
        assert(read_word(r,at+13)==s->generation&&read_word(r,at+14)==s->sample);
        assert(read_wide(r,at+15)==s->cache_version&&read_word(r,at+17)==s->lease.slot&&read_wide(r,at+18)==s->lease.serial);
    }
    if(plan->source_count==1)for(i=212;i<PT_PAULA_PACKET32_BYTES;++i)assert(!r->bytes[i]);
    if(!capture.conversions&&!capture.negative_cases){
        for(i=0;i<28;++i)preserved(&p,&m,i);
        alias_refusers(&p,&m);full_width(&p,&m);
    }
    if(index==1&&capture.negative_cases==28)dual_refusers(&p,&m);
    assert(capture.owner->f.fast.calls==fast&&capture.owner->f.chip.bytes==2304);
    ++capture.conversions;return sequence_submit(v,event,plan);
}
static void measured(unsigned reader_first)
{
    struct sequence_fixture x;union pt_paula_packet32_record original[4];unsigned i;
    sequence_init(&x,0);memset(&capture,0,sizeof(capture));capture.owner=&x;
    /* Second retirement-order case does not repeat the negative group. */
    if(reader_first)capture.negative_cases=1;
    x.f.backend.submit_plan=packet32_submit;sequence_open(&x);
    for(i=0;i<4;++i){assert(sequence_until(&x,PT_PAULA_SEQUENCE_READY)==PT_PAULA_SEQUENCE_READY);
        sequence_activate(&x,i,i<3);assert(capture.conversions==i+1&&pinned(&x.f)==3);}
    memcpy(original,capture.record,sizeof(original));sequence_terminal(&x,reader_first);
    /* Numeric observation bytes survive disposal; no old pointer is followed or
     * converted again after the original genuine reader/command owners end. */
    assert(!memcmp(original,capture.record,sizeof(original)));capture.owner=NULL;++packet32_cases;
}
int main(void)
{
    measured(0);measured(1);assert(packet32_cases==46);
    puts("paula geometry packet32: 46 cases passed");
    puts("genuine measured frames: 960,1920,2880,3840; initial offset256 length512");
    puts("repeat preserves current Q32 fetch phase; both genuine retirement orders");
    puts("pure big-endian explicit HOST labels; native/output/timing/quiet NOT QUALIFIED");
    return 0;
}
