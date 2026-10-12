#include "paula_segment_packet32.h"
#include <stddef.h>
#include <string.h>
#define ALIGNMENT(T) offsetof(struct {char before;T object;},object)
typedef char record_exact_size[(sizeof(union pt_paula_packet32_record)==PT_PAULA_PACKET32_BYTES)?1:-1];
static int span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int object(const void *p,size_t n,size_t alignment)
{return p&&span(p,n)&&!((uintptr_t)p%alignment);}
static int apart(const void *a,size_t n,const void *b,size_t m)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return span(a,n)&&span(b,m)&&(!n||!m||(x<=y?n<=y-x:m<=x-y));}
static int u32(uintmax_t value,uint32_t *out)
{if(value>UINT32_MAX)return 0;*out=(uint32_t)value;return 1;}
static int model_span(uint32_t base,uint32_t bytes)
{return base&&bytes&&bytes<=UINT32_MAX-base;}
static int model_apart(uint32_t a,uint32_t n,uint32_t b,uint32_t m)
{return model_span(a,n)&&model_span(b,m)&&(a<=b?n<=b-a:m<=a-b);}
static int label_equal(const void *a,uint32_t x,const void *b,uint32_t y)
{return (a==b)==(x==y);}
static void word(union pt_paula_packet32_record *r,unsigned index,uint32_t value)
{
    unsigned n=index*4U;
    r->bytes[n]=(uint8_t)(value>>24);r->bytes[n+1]=(uint8_t)(value>>16);
    r->bytes[n+2]=(uint8_t)(value>>8);r->bytes[n+3]=(uint8_t)value;
}
static void wide(union pt_paula_packet32_record *r,unsigned index,uint64_t value)
{word(r,index,(uint32_t)(value>>32));word(r,index+1,(uint32_t)(value&UINT32_MAX));}
/* Metadata shape only; no address here is followed. */
static int source_shape(const struct pt_paula_segment_source *s)
{
    const struct pt_pcm *p=&s->master;size_t padded;unsigned i;uint32_t ignored;
    if(!s->version||!s->sampler_owner||!s->project_identity||!s->cache_owner||!s->cache_version||
       !s->descriptor||!object(s->descriptor,sizeof(*s->descriptor),ALIGNMENT(struct pt_pcm))||
       p->channels!=1||(p->bits!=8&&p->bits!=16&&p->bits!=24)||!p->rate||p->rate>192000||
       !p->frames||p->capacity<p->frames||p->capacity>SIZE_MAX/sizeof(*p->data)||
       !u32(p->capacity,&ignored)||!object(p->data,p->capacity*sizeof(*p->data),ALIGNMENT(int32_t))||
       s->sample>=255||!u32(s->sample,&ignored)||!u32(s->generation,&ignored)||
       s->lease.slot>=PT_CACHE_SLOTS||!u32(s->lease.slot,&ignored)||!s->lease.serial||
       s->guard_count>PT_PAULA_SEGMENT_SOURCE_GUARDS)return 0;
    padded=p->frames;if(padded&1){if(padded==SIZE_MAX)return 0;++padded;}
    if(s->cache_bytes!=padded||!u32(s->cache_bytes,&ignored)||!object(s->cache_data,s->cache_bytes,2))return 0;
    for(i=0;i<s->guard_count;++i)if(!s->guard[i].bytes||!span(s->guard[i].data,s->guard[i].bytes))return 0;
    return 1;
}
static int output_apart(const struct pt_paula_segment_source *s,const void *out,size_t bytes)
{
    unsigned i;
    if(!apart(out,bytes,s->descriptor,sizeof(*s->descriptor))||
       !apart(out,bytes,s->master.data,s->master.capacity*sizeof(*s->master.data))||
       !apart(out,bytes,s->cache_data,s->cache_bytes))return 0;
    for(i=0;i<s->guard_count;++i)if(!apart(out,bytes,s->guard[i].data,s->guard[i].bytes))return 0;
    return 1;
}
static int map_same(const struct pt_paula_segment_source *s,const struct pt_paula_packet32_source_map *m)
{
    uint32_t bytes;
    return s->descriptor==m->descriptor&&s->master.data==m->master_data&&s->version==m->version&&
        s->sampler_owner==m->sampler_owner&&s->project_identity==m->project_identity&&
        s->cache_owner==m->cache_owner&&s->cache_data==m->cache_data&&
        m->descriptor_tag&&m->master_tag&&m->version_tag&&m->sampler_tag&&m->project_tag&&
        m->cache_owner_tag&&!(m->cache_base32&1)&&u32(s->cache_bytes,&bytes)&&model_span(m->cache_base32,bytes);
}
static int maps_consistent(const struct pt_paula_segment_source *a,const struct pt_paula_packet32_source_map *x,
    const struct pt_paula_segment_source *b,const struct pt_paula_packet32_source_map *y)
{
    uint32_t an,bn;
    if(!label_equal(a->descriptor,x->descriptor_tag,b->descriptor,y->descriptor_tag)||
       !label_equal(a->master.data,x->master_tag,b->master.data,y->master_tag)||
       !label_equal(a->version,x->version_tag,b->version,y->version_tag)||
       !label_equal(a->sampler_owner,x->sampler_tag,b->sampler_owner,y->sampler_tag)||
       !label_equal(a->project_identity,x->project_tag,b->project_identity,y->project_tag)||
       !label_equal(a->cache_owner,x->cache_owner_tag,b->cache_owner,y->cache_owner_tag)||
       !label_equal(a->cache_data,x->cache_base32,b->cache_data,y->cache_base32)||
       !u32(a->cache_bytes,&an)||!u32(b->cache_bytes,&bn))return 0;
    if(a->cache_data==b->cache_data)return an==bn;
    return apart(a->cache_data,a->cache_bytes,b->cache_data,b->cache_bytes)&&
        model_apart(x->cache_base32,an,y->cache_base32,bn);
}
static int range(const struct pt_paula_segment_range *r,const struct pt_paula_segment_source *s)
{
    return r->frame_length>=2&&r->frame_length<=131070&&!(r->frame_offset&1)&&!(r->frame_length&1)&&
        r->byte_offset==r->frame_offset&&r->byte_length==r->frame_length&&
        r->words==r->frame_length/2&&r->frame_offset<=s->master.frames&&
        r->frame_length<=s->master.frames-r->frame_offset&&r->byte_offset<=s->cache_bytes&&
        r->byte_length<=s->cache_bytes-r->byte_offset;
}
static int empty_range(const struct pt_paula_segment_range *r)
{return !r->frame_offset&&!r->frame_length&&!r->byte_offset&&!r->byte_length&&!r->words;}
static void put_range(union pt_paula_packet32_record *r,unsigned at,const struct pt_paula_segment_range *g)
{word(r,at,g->frame_offset);word(r,at+1,g->frame_length);word(r,at+2,g->byte_offset);word(r,at+3,g->byte_length);word(r,at+4,g->words);}
static void put_source(union pt_paula_packet32_record *r,unsigned at,
    const struct pt_paula_segment_source *s,const struct pt_paula_packet32_source_map *m)
{
    word(r,at,m->descriptor_tag);word(r,at+1,m->master_tag);word(r,at+2,m->version_tag);
    word(r,at+3,m->sampler_tag);word(r,at+4,m->project_tag);word(r,at+5,m->cache_owner_tag);
    word(r,at+6,m->cache_base32);word(r,at+7,(uint32_t)s->cache_bytes);word(r,at+8,(uint32_t)s->master.capacity);
    word(r,at+9,s->master.frames);word(r,at+10,s->master.rate);word(r,at+11,s->master.bits);
    word(r,at+12,s->master.channels);word(r,at+13,(uint32_t)s->generation);word(r,at+14,(uint32_t)s->sample);
    wide(r,at+15,s->cache_version);word(r,at+17,(uint32_t)s->lease.slot);wide(r,at+18,s->lease.serial);
    word(r,at+20,(uint32_t)s->guard_count);
}
enum pt_paula_packet32_result pt_paula_segment_packet32_build(
    const struct pt_paula_future_packet *packet,const struct pt_paula_packet32_mapping *mapping,
    union pt_paula_packet32_record *out)
{
    struct pt_paula_future_packet p;struct pt_paula_packet32_mapping m;
    union pt_paula_packet32_record r;const struct pt_paula_segment_plan *g;
    const struct pt_paula_segment_source *next;uint64_t start,end;unsigned i;
    uint32_t initial_address=0,repeat_address=0;
    if(!object(packet,sizeof(*packet),ALIGNMENT(struct pt_paula_future_packet))||
       !object(mapping,sizeof(*mapping),ALIGNMENT(struct pt_paula_packet32_mapping))||
       !object(out,sizeof(*out),ALIGNMENT(union pt_paula_packet32_record))||
       !apart(out,sizeof(*out),packet,sizeof(*packet))||!apart(out,sizeof(*out),mapping,sizeof(*mapping)))
        return PT_PAULA_PACKET32_ALIAS;
    memcpy(&p,packet,sizeof(p));memcpy(&m,mapping,sizeof(m));g=&p.geometry;
    if(!p.ticket||p.first>=p.last||g->frame==UINT64_MAX||g->track>=16||g->slot>=4||
       (g->source_count!=1&&g->source_count!=2)||m.source_count!=g->source_count)return PT_PAULA_PACKET32_INVALID;
    for(i=0;i<g->source_count;++i){
        if(!source_shape(g->source+i))return PT_PAULA_PACKET32_INVALID;
        if(!output_apart(g->source+i,out,sizeof(*out)))return PT_PAULA_PACKET32_ALIAS;
        if(!map_same(g->source+i,m.source+i))return PT_PAULA_PACKET32_MAPPING;
    }
    if(g->source_count==2){const struct pt_paula_segment_source *a=g->source,*b=g->source+1;
        if(a->sampler_owner!=b->sampler_owner||a->project_identity!=b->project_identity||
           a->cache_owner!=b->cache_owner||a->generation!=b->generation||a->cache_version!=b->cache_version||
           a->master.bits!=8||b->master.bits!=8||a->master.rate!=b->master.rate)return PT_PAULA_PACKET32_UNSUPPORTED;
        if(!maps_consistent(a,m.source,b,m.source+1))return PT_PAULA_PACKET32_MAPPING;
    }
    next=g->source+(g->source_count==2?1:0);
    if(g->kind==PT_PAULA_SEGMENT_INITIAL||g->kind==PT_PAULA_SEGMENT_REPROGRAM){
        if(g->write_initial!=(unsigned)(g->kind==PT_PAULA_SEGMENT_INITIAL)||g->write_repeat!=1||
           !g->period||g->volume>64||!range(&g->initial,g->source)||!range(&g->repeat,next))return PT_PAULA_PACKET32_INVALID;
        if(g->kind==PT_PAULA_SEGMENT_INITIAL&&g->source_count!=1)return PT_PAULA_PACKET32_UNSUPPORTED;
        start=(uint64_t)g->initial.frame_offset<<32;
        end=(uint64_t)(g->initial.frame_offset+g->initial.frame_length)<<32;
        if(g->phase_q32<start||g->phase_q32>=end||
           (g->kind==PT_PAULA_SEGMENT_INITIAL&&g->phase_q32!=start))return PT_PAULA_PACKET32_INVALID;
        if(g->write_initial)initial_address=m.source[0].cache_base32+g->initial.byte_offset;
        repeat_address=m.source[g->source_count==2?1:0].cache_base32+g->repeat.byte_offset;
    }else if(g->kind==PT_PAULA_SEGMENT_STOP_REQUIREMENTS){
        if(g->write_initial||g->write_repeat||g->period||g->volume||
           !empty_range(&g->initial)||!empty_range(&g->repeat))return PT_PAULA_PACKET32_INVALID;
    }else return PT_PAULA_PACKET32_UNSUPPORTED;
    memset(&r,0,sizeof(r));word(&r,0,UINT32_C(0x50473332));word(&r,1,PT_PAULA_PACKET32_VERSION);
    word(&r,2,PT_PAULA_PACKET32_BYTES);word(&r,3,g->source_count);word(&r,4,(uint32_t)g->kind);
    word(&r,5,(uint32_t)g->track);word(&r,6,(uint32_t)g->slot);word(&r,7,g->write_initial|(g->write_repeat<<1));
    word(&r,8,g->period);word(&r,9,g->volume);wide(&r,10,p.ticket);wide(&r,12,p.first);wide(&r,14,p.last);
    wide(&r,16,g->frame);wide(&r,18,g->phase_q32);word(&r,20,initial_address);word(&r,21,repeat_address);
    put_range(&r,22,&g->initial);put_range(&r,27,&g->repeat);
    for(i=0;i<g->source_count;++i)put_source(&r,PT_PAULA_PACKET32_HEADER_WORDS+i*PT_PAULA_PACKET32_SOURCE_WORDS,g->source+i,m.source+i);
    memcpy(out,&r,sizeof(r));return PT_PAULA_PACKET32_OK;
}
