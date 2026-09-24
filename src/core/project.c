#include <string.h>
#include "project.h"

#define TAG(a,b,c,d) ((uint32_t)(a)<<24 | (uint32_t)(b)<<16 | (uint32_t)(c)<<8 | (d))
static const uint32_t tags[6] = {TAG('H','E','A','D'),TAG('C','H','A','N'),TAG('O','R','D','R'),
                                TAG('P','A','T','T'),TAG('S','A','M','P'),TAG('M','I','D','I')};
static const uint8_t magic[8] = {'P','T','2','4','G','\r','\n',26};
static uint32_t u16(const uint8_t *p) { return (uint32_t)p[0]*256 + p[1]; }
static uint32_t u32(const uint8_t *p) { return u16(p)*65536 + u16(p+2); }
static void w16(uint8_t *p, uint32_t n) { p[0]=(uint8_t)(n>>8); p[1]=(uint8_t)n; }
static void w32(uint8_t *p, uint32_t n) { w16(p,n>>16); w16(p+2,n); }
static size_t pad(size_t n) { return (4-(n&3))&3; }
static int add(size_t *n, size_t amount) { if (amount>SIZE_MAX-*n) return 0; *n+=amount; return 1; }
static int overlap(const void *a, size_t an, const void *b, size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if (!an || !bn) return 0;
    if (an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y) return 1;
    return x<y+bn && y<x+an;
}
static int known(uint32_t tag)
{
    int i; for(i=0;i<6;++i) if(tags[i]==tag) return i;
    return -1;
}
static uint32_t checksum(const uint8_t *p, size_t n)
{
    uint32_t crc=0xffffffffUL; size_t i; unsigned j;
    for(i=0;i<n;++i) {
        crc^=i>=20 && i<24 ? 0 : p[i];
        for(j=0;j<8;++j) crc=(crc>>1)^((crc&1)?0xedb88320UL:0);
    }
    return crc^0xffffffffUL;
}
static int terminated(const char *s,size_t n) { return memchr(s,0,n)!=NULL; }
static int sample_meta(const struct pt_sample *s)
{
    if(!terminated(s->name,32) || !s->pcm.rate || s->pcm.rate>192000 ||
       (s->pcm.bits!=8 && s->pcm.bits!=16 && s->pcm.bits!=24) ||
       (s->pcm.channels!=1 && s->pcm.channels!=2) || s->volume>64 ||
       s->finetune < -8 || s->finetune>7 || s->interpolation>1 ||
       s->loop>PT_LOOP_CROSSFADE || s->slice_count>PT_PROJECT_SLICES) return 0;
    if(s->loop==PT_LOOP_NONE) {
        if(s->loop_start || s->loop_end || s->crossfade) return 0;
    } else {
        if(s->loop_start>=s->loop_end || s->loop_end>s->pcm.frames) return 0;
        if(s->loop==PT_LOOP_CROSSFADE) {
            if(!s->crossfade || s->crossfade>(s->loop_end-s->loop_start)/2) return 0;
        } else if(s->crossfade) return 0;
    }
    return 1;
}
static uint32_t sample_caps(const struct pt_sample *s)
{
    uint32_t c=0;
    if(s->pcm.bits==16) c|=PT_CAP_16BIT;
    if(s->pcm.bits==24) c|=PT_CAP_24BIT;
    if(s->pcm.channels==2) c|=PT_CAP_STEREO;
    if(s->slice_count) c|=PT_CAP_SLICES;
    if(s->loop==PT_LOOP_PINGPONG) c|=PT_CAP_PINGPONG;
    if(s->loop==PT_LOOP_CROSSFADE) c|=PT_CAP_CROSSFADE;
    return c;
}
static int event_valid(const struct pt_event *e, unsigned samples, uint16_t slice_limit)
{
    if(e->kind>PT_NOTE_OFF || e->instrument>samples || e->effect>15 ||
       e->flags>1 || e->velocity>127 || (!(e->flags&1) && e->velocity)) return 0;
    if((e->kind==PT_NOTE_NONE || e->kind==PT_NOTE_OFF) && e->pitch) return 0;
    if(e->kind==PT_NOTE_PERIOD && (!e->pitch || e->pitch>4095)) return 0;
    if(e->kind==PT_NOTE_MIDI && e->pitch>127) return 0;
    if(e->slice && (!e->instrument || e->slice>slice_limit)) return 0;
    return 1;
}
int pt_project_event_valid(const struct pt_project *p,const struct pt_event *e)
{
    uint16_t limit=0;
    if(!p || !e || p->sample_count>255 || (p->sample_count && !p->samples))return 0;
    if(e->instrument && e->instrument<=p->sample_count)limit=p->samples[e->instrument-1].slice_count;
    return event_valid(e,p->sample_count,limit);
}
static uint32_t event_caps(const struct pt_event *e)
{
    return (e->kind==PT_NOTE_OFF?PT_CAP_OFF:0) |
           (e->kind==PT_NOTE_MIDI?PT_CAP_MIDI_NOTE:0) | (e->flags?PT_CAP_VELOCITY:0);
}
static void read_event(const uint8_t *p,struct pt_event *e)
{
    memset(e,0,sizeof(*e)); e->kind=p[0];e->instrument=p[1];e->pitch=(uint16_t)u16(p+2);
    e->effect=p[4];e->parameter=p[5];e->velocity=p[6];e->flags=p[7];e->slice=(uint16_t)u16(p+8);
}
static void read_sample(const uint8_t *p,struct pt_sample *s)
{
    memset(s,0,sizeof(*s));memcpy(s->name,p,32);s->pcm.rate=u32(p+32);s->pcm.frames=u32(p+36);
    s->pcm.bits=p[40];s->pcm.channels=p[41];s->volume=p[42];
    s->finetune=(int8_t)(p[43]<128?p[43]:(int)p[43]-256);
    s->loop=p[44];s->interpolation=p[45];s->slice_count=(uint16_t)u16(p+46);
    s->loop_start=u32(p+48);s->loop_end=u32(p+52);s->crossfade=u32(p+56);
}
static int basic(const struct pt_project *p)
{
    return p && terminated(p->title,32) && pt_channels_validate(&p->channels)==PT_CHANNEL_OK &&
        p->order_count && p->order_count<=256 && p->pattern_count && p->pattern_count<=256 &&
        p->sample_count<=255 && p->bpm>=32 && p->bpm<=255 && p->speed && p->speed<=31 &&
        p->mode<=PT_MODE_STUDIO && p->midi_flags<=3 && terminated(p->midi_input,64);
}
static uint32_t project_caps(const struct pt_project *p)
{
    unsigned i;uint32_t c=p->channels.count>4?PT_CAP_CHANNELS:0;
    if(p->mode==PT_MODE_STUDIO)c|=PT_CAP_STUDIO;
    if(p->midi_flags)c|=PT_CAP_MIDI;
    for(i=0;i<p->channels.count;++i) {
        if(p->channels.track[i].route==PT_AMIGUS)c|=PT_CAP_AMIGUS;
        if(p->channels.track[i].route==PT_MIDI)c|=PT_CAP_MIDI;
    }
    return c;
}
enum pt_project_result pt_project_validate(const struct pt_project *p,uint32_t *caps)
{
    unsigned i,j;size_t n;uint32_t c;
    if(!basic(p) || !p->orders || !p->events || (p->sample_count && !p->samples) ||
       p->extension_count>4090 || (p->extension_count && !p->extensions)) return PT_PROJECT_INVALID;
    c=project_caps(p);
    for(i=0;i<p->channels.count;++i) if(!terminated(p->midi_output[i],64))return PT_PROJECT_INVALID;
    for(i=0;i<p->order_count;++i)if(p->orders[i]>=p->pattern_count)return PT_PROJECT_INVALID;
    for(i=0;i<p->sample_count;++i) {
        const struct pt_sample *s=&p->samples[i];
        if(!sample_meta(s) || pt_pcm_validate(&s->pcm)!=PT_PCM_OK ||
           (s->slice_count && !s->slices))return PT_PROJECT_INVALID;
        for(j=0;j<s->slice_count;++j)if(s->slices[j]>=s->pcm.frames ||
            (j && s->slices[j]<=s->slices[j-1]))return PT_PROJECT_INVALID;
        c|=sample_caps(s);
    }
    n=(size_t)p->pattern_count*64*p->channels.count;
    for(i=0;i<n;++i) {
        if(!pt_project_event_valid(p,&p->events[i]))return PT_PROJECT_INVALID;
        c|=event_caps(&p->events[i]);
    }
    for(i=0;i<p->extension_count;++i)if(known(p->extensions[i].id)>=0 ||
        (p->extensions[i].length && !p->extensions[i].data))return PT_PROJECT_INVALID;
    if(caps)*caps=c;
    return PT_PROJECT_OK;
}
static enum pt_project_result sizes(const struct pt_project *p,size_t lengths[6],size_t *total)
{
    unsigned i;size_t n=32;enum pt_project_result r=pt_project_validate(p,NULL);
    if(r!=PT_PROJECT_OK)return r;
    lengths[0]=44;lengths[1]=(size_t)p->channels.count*22;lengths[2]=(size_t)p->order_count*2;
    lengths[3]=(size_t)p->pattern_count*64*p->channels.count*12;lengths[4]=0;
    lengths[5]=64*((size_t)p->channels.count+1)+4;
    for(i=0;i<p->sample_count;++i) {
        const struct pt_sample *s=&p->samples[i];size_t k=64+(size_t)s->slice_count*4;
        size_t count=(size_t)s->pcm.frames*s->pcm.channels;
        if(count>SIZE_MAX/(s->pcm.bits/8) || !add(&k,count*(s->pcm.bits/8)) ||
           !add(&k,pad(k)) || !add(&lengths[4],k))return PT_PROJECT_CAPACITY;
    }
    for(i=0;i<6;++i)if(!add(&n,12) || !add(&n,lengths[i]) || !add(&n,pad(lengths[i])))return PT_PROJECT_CAPACITY;
    for(i=0;i<p->extension_count;++i)if(!add(&n,12) || !add(&n,p->extensions[i].length) ||
        !add(&n,pad(p->extensions[i].length)))return PT_PROJECT_CAPACITY;
    if(n>UINT32_MAX)return PT_PROJECT_CAPACITY;
    *total=n;return PT_PROJECT_OK;
}
enum pt_project_result pt_project_size(const struct pt_project *p,size_t *out)
{
    size_t lengths[6],n;enum pt_project_result r;
    if(!out)return PT_PROJECT_INVALID;
    r=sizes(p,lengths,&n);if(r==PT_PROJECT_OK)*out=n;return r;
}
static int source_alias(const struct pt_project *p,const void *out,size_t n)
{
    unsigned i;size_t events=(size_t)p->pattern_count*64*p->channels.count;
    if(overlap(out,n,p,sizeof(*p)) || overlap(out,n,p->orders,p->order_count*sizeof(*p->orders)) ||
       overlap(out,n,p->events,events*sizeof(*p->events)) ||
       overlap(out,n,p->samples,p->sample_count*sizeof(*p->samples)) ||
       overlap(out,n,p->extensions,p->extension_count*sizeof(*p->extensions)))return 1;
    for(i=0;i<p->sample_count;++i)if(overlap(out,n,p->samples[i].pcm.data,
        (size_t)p->samples[i].pcm.frames*p->samples[i].pcm.channels*4) ||
        overlap(out,n,p->samples[i].slices,(size_t)p->samples[i].slice_count*4))return 1;
    for(i=0;i<p->extension_count;++i)if(overlap(out,n,p->extensions[i].data,p->extensions[i].length))return 1;
    return 0;
}
enum pt_project_result pt_project_encode(const struct pt_project *p,uint8_t *out,size_t capacity,size_t *written)
{
    size_t lengths[6],n,pos=32,i,j;unsigned k;uint32_t caps;uint8_t *q;
    enum pt_project_result r=sizes(p,lengths,&n);
    if(r!=PT_PROJECT_OK)return r;
    if(!out || !written)return PT_PROJECT_INVALID;
    if(capacity<n)return PT_PROJECT_CAPACITY;
    if(source_alias(p,out,n) || overlap(out,n,written,sizeof(*written)))return PT_PROJECT_ALIAS;
    pt_project_validate(p,&caps);memset(out,0,n);memcpy(out,magic,8);w16(out+8,1);
    w32(out+12,(uint32_t)n);w32(out+16,caps);w32(out+24,6+p->extension_count);
    for(k=0;k<6;++k) {
        w32(out+pos,tags[k]);w16(out+pos+4,1);w16(out+pos+6,1);w32(out+pos+8,(uint32_t)lengths[k]);
        q=out+pos+12;
        if(k==0) {
            memcpy(q,p->title,32);q[32]=p->channels.count;q[33]=p->channels.selected;
            q[34]=p->speed;q[35]=p->mode;w16(q+36,p->bpm);w16(q+38,p->order_count);
            w16(q+40,p->pattern_count);w16(q+42,p->sample_count);
        } else if(k==1)for(i=0;i<p->channels.count;++i,q+=22) {
            const struct pt_channel *c=&p->channels.track[i];
            q[0]=c->route;q[1]=c->pan;q[2]=c->muted;q[3]=c->solo;q[4]=c->group;q[5]=c->midi_channel;
            memcpy(q+6,c->name,16);
        } else if(k==2)for(i=0;i<p->order_count;++i)w16(q+i*2,p->orders[i]);
        else if(k==3)for(i=0;i<(size_t)p->pattern_count*64*p->channels.count;++i,q+=12) {
            const struct pt_event *e=&p->events[i];q[0]=e->kind;q[1]=e->instrument;w16(q+2,e->pitch);
            q[4]=e->effect;q[5]=e->parameter;q[6]=e->velocity;q[7]=e->flags;w16(q+8,e->slice);
        } else if(k==4)for(i=0;i<p->sample_count;++i) {
            const struct pt_sample *s=&p->samples[i];size_t count=(size_t)s->pcm.frames*s->pcm.channels;
            size_t bytes=count*(s->pcm.bits/8),record=64+(size_t)s->slice_count*4+bytes;
            memcpy(q,s->name,32);w32(q+32,s->pcm.rate);w32(q+36,s->pcm.frames);q[40]=s->pcm.bits;
            q[41]=s->pcm.channels;q[42]=s->volume;q[43]=(uint8_t)s->finetune;q[44]=s->loop;
            q[45]=s->interpolation;w16(q+46,s->slice_count);w32(q+48,s->loop_start);
            w32(q+52,s->loop_end);w32(q+56,s->crossfade);q+=64;
            for(j=0;j<s->slice_count;++j,q+=4)w32(q,s->slices[j]);
            for(j=0;j<count;++j) {
                uint32_t value=(uint32_t)s->pcm.data[j];unsigned width=s->pcm.bits/8,b;
                for(b=0;b<width;++b)q[b]=(uint8_t)(value>>(8*(width-1-b)));
                q+=width;
            }
            q+=pad(record);
        } else if(k==5) {
            memcpy(q,p->midi_input,64);q+=64;
            for(i=0;i<p->channels.count;++i,q+=64)memcpy(q,p->midi_output[i],64);
            w32(q,p->midi_flags);
        }
        pos+=12+lengths[k]+pad(lengths[k]);
    }
    for(i=0;i<p->extension_count;++i) {
        const struct pt_extension *e=&p->extensions[i];w32(out+pos,e->id);w16(out+pos+4,e->version);
        w32(out+pos+8,e->length);if(e->length)memcpy(out+pos+12,e->data,e->length);
        pos+=12+e->length+pad(e->length);
    }
    w32(out+20,checksum(out,n));*written=n;return PT_PROJECT_OK;
}

struct stream_writer {
    pt_project_sink sink;void *context;size_t pos,used;uint32_t crc;uint8_t block[1024];
};
static int stream_bytes(struct stream_writer *w,const void *data,size_t n)
{
    const uint8_t *p=data;size_t i;unsigned j;
    if(!w->sink) {
        for(i=0;i<n;++i,++w->pos) {
            w->crc^=w->pos>=20 && w->pos<24?0:p[i];
            for(j=0;j<8;++j)w->crc=(w->crc>>1)^((w->crc&1)?0xedb88320UL:0);
        }
        return 1;
    }
    while(n) {
        size_t take=sizeof(w->block)-w->used;if(take>n)take=n;
        memcpy(w->block+w->used,p,take);w->used+=take;w->pos+=take;p+=take;n-=take;
        if(w->used==sizeof(w->block)) {
            if(w->sink(w->context,w->block,w->used)!=1)return 0;
            w->used=0;
        }
    }
    return 1;
}
static int stream_body(const struct pt_project *p,const size_t lengths[6],size_t total,
    uint32_t caps,uint32_t crc,struct stream_writer *w)
{
    uint8_t q[64],zero[3]={0};unsigned k;size_t i,j;
#define EMIT(data,bytes) do {if(!stream_bytes(w,data,bytes))return 0;}while(0)
    memset(q,0,32);memcpy(q,magic,8);w16(q+8,1);w32(q+12,(uint32_t)total);
    w32(q+16,caps);w32(q+20,crc);w32(q+24,6+p->extension_count);EMIT(q,32);
    for(k=0;k<6;++k) {
        memset(q,0,12);w32(q,tags[k]);w16(q+4,1);w16(q+6,1);w32(q+8,(uint32_t)lengths[k]);EMIT(q,12);
        if(k==0) {
            memset(q,0,44);memcpy(q,p->title,32);q[32]=p->channels.count;q[33]=p->channels.selected;
            q[34]=p->speed;q[35]=p->mode;w16(q+36,p->bpm);w16(q+38,p->order_count);
            w16(q+40,p->pattern_count);w16(q+42,p->sample_count);EMIT(q,44);
        } else if(k==1)for(i=0;i<p->channels.count;++i) {
            const struct pt_channel *c=&p->channels.track[i];
            q[0]=c->route;q[1]=c->pan;q[2]=c->muted;q[3]=c->solo;q[4]=c->group;q[5]=c->midi_channel;
            memcpy(q+6,c->name,16);EMIT(q,22);
        } else if(k==2)for(i=0;i<p->order_count;++i) {w16(q,p->orders[i]);EMIT(q,2);}
        else if(k==3)for(i=0;i<(size_t)p->pattern_count*64*p->channels.count;++i) {
            const struct pt_event *e=&p->events[i];memset(q,0,12);q[0]=e->kind;q[1]=e->instrument;w16(q+2,e->pitch);
            q[4]=e->effect;q[5]=e->parameter;q[6]=e->velocity;q[7]=e->flags;w16(q+8,e->slice);EMIT(q,12);
        } else if(k==4)for(i=0;i<p->sample_count;++i) {
            const struct pt_sample *sample=&p->samples[i];size_t count=(size_t)sample->pcm.frames*sample->pcm.channels;
            unsigned width=sample->pcm.bits/8;size_t record=64+(size_t)sample->slice_count*4+count*width;
            memset(q,0,64);memcpy(q,sample->name,32);w32(q+32,sample->pcm.rate);w32(q+36,sample->pcm.frames);
            q[40]=sample->pcm.bits;q[41]=sample->pcm.channels;q[42]=sample->volume;q[43]=(uint8_t)sample->finetune;
            q[44]=sample->loop;q[45]=sample->interpolation;w16(q+46,sample->slice_count);
            w32(q+48,sample->loop_start);w32(q+52,sample->loop_end);w32(q+56,sample->crossfade);EMIT(q,64);
            for(j=0;j<sample->slice_count;++j) {w32(q,sample->slices[j]);EMIT(q,4);}
            for(j=0;j<count;++j) {
                unsigned b;uint32_t value=(uint32_t)sample->pcm.data[j];
                for(b=0;b<width;++b)q[b]=(uint8_t)(value>>(8*(width-1-b)));
                EMIT(q,width);
            }
            EMIT(zero,pad(record));
        } else {
            EMIT(p->midi_input,64);
            for(i=0;i<p->channels.count;++i) {EMIT(p->midi_output[i],64);}
            w32(q,p->midi_flags);EMIT(q,4);
        }
        EMIT(zero,pad(lengths[k]));
    }
    for(i=0;i<p->extension_count;++i) {
        const struct pt_extension *e=&p->extensions[i];memset(q,0,12);
        w32(q,e->id);w16(q+4,e->version);w32(q+8,e->length);EMIT(q,12);
        EMIT(e->data,e->length);EMIT(zero,pad(e->length));
    }
#undef EMIT
    return 1;
}
enum pt_project_result pt_project_stream(const struct pt_project *p,pt_project_sink sink,void *context,size_t *written)
{
    size_t lengths[6],total;uint32_t caps,crc;struct stream_writer w;
    enum pt_project_result r;
    if(!sink || !written)return PT_PROJECT_INVALID;
    r=sizes(p,lengths,&total);if(r!=PT_PROJECT_OK)return r;
    /* written must not overwrite any part of the master after successful emit. */
    if(source_alias(p,written,sizeof(*written)))return PT_PROJECT_ALIAS;
    pt_project_validate(p,&caps);memset(&w,0,sizeof(w));w.crc=0xffffffffUL;
    if(!stream_body(p,lengths,total,caps,0,&w) || w.pos!=total)return PT_PROJECT_INVALID;
    crc=w.crc^0xffffffffUL;memset(&w,0,sizeof(w));w.sink=sink;w.context=context;
    if(!stream_body(p,lengths,total,caps,crc,&w) || w.pos!=total ||
       (w.used && sink(context,w.block,w.used)!=1))return PT_PROJECT_INVALID;
    *written=total;return PT_PROJECT_OK;
}

struct view { const uint8_t *part[6];size_t length[6];struct pt_project p;struct pt_project_requirements need; };
static enum pt_project_result scan(const uint8_t *data,size_t length,struct view *v)
{
    size_t pos=32,i,j;unsigned count,mask=0;uint32_t caps;uint16_t slices[255];
    const uint8_t *q;struct pt_event e;
    if(!data)return PT_PROJECT_INVALID;
    if(length<32)return PT_PROJECT_TRUNCATED;
    if(memcmp(data,magic,8))return PT_PROJECT_INVALID;
    if(u16(data+8)!=1 || u16(data+10)!=0 || (u32(data+16)&~PT_CAP_KNOWN))return PT_PROJECT_UNSUPPORTED;
    if(u32(data+12)>length)return PT_PROJECT_TRUNCATED;
    if(u32(data+12)!=length || u32(data+28) || u32(data+24)>4096)return PT_PROJECT_INVALID;
    if(checksum(data,length)!=u32(data+20))return PT_PROJECT_CHECKSUM;
    memset(v,0,sizeof(*v));pt_channels_init(&v->p.channels);
    for(count=0;count<u32(data+24);++count) {
        size_t bytes,body;int index;
        if(length-pos<12)return PT_PROJECT_TRUNCATED;
        bytes=u32(data+pos+8);body=pos+12;
        if(bytes>length-body || pad(bytes)>length-body-bytes)return PT_PROJECT_TRUNCATED;
        for(i=0;i<pad(bytes);++i)if(data[body+bytes+i])return PT_PROJECT_INVALID;
        if(u16(data+pos+6)&~1U)return PT_PROJECT_UNSUPPORTED;
        index=known(u32(data+pos));
        if(index<0) {
            if(u16(data+pos+6)&1)return PT_PROJECT_UNSUPPORTED;
            ++v->need.extensions;
            if(!add(&v->need.extension_bytes,bytes))return PT_PROJECT_CAPACITY;
        } else {
            if(mask&(1U<<index))return PT_PROJECT_INVALID;
            if(u16(data+pos+4)!=1)return PT_PROJECT_UNSUPPORTED;
            if(u16(data+pos+6)!=1)return PT_PROJECT_INVALID;
            mask|=1U<<index;v->part[index]=data+body;v->length[index]=bytes;
        }
        pos=body+bytes+pad(bytes);
    }
    if(pos!=length || mask!=63 || v->length[0]!=44)return PT_PROJECT_INVALID;
    q=v->part[0];memcpy(v->p.title,q,32);v->p.channels.count=q[32];v->p.channels.selected=q[33];
    v->p.speed=q[34];v->p.mode=q[35];v->p.bpm=(uint16_t)u16(q+36);
    v->p.order_count=(uint16_t)u16(q+38);v->p.pattern_count=(uint16_t)u16(q+40);v->p.sample_count=(uint16_t)u16(q+42);
    if(!v->p.channels.count || v->p.channels.count>16 ||
       v->length[1]!=(size_t)v->p.channels.count*22 ||
       v->length[5]!=64*((size_t)v->p.channels.count+1)+4)return PT_PROJECT_INVALID;
    q=v->part[1];
    for(i=0;i<v->p.channels.count;++i,q+=22) {
        struct pt_channel *c=&v->p.channels.track[i];
        c->route=q[0];c->pan=q[1];c->muted=q[2];c->solo=q[3];c->group=q[4];c->midi_channel=q[5];memcpy(c->name,q+6,16);
    }
    q=v->part[5];memcpy(v->p.midi_input,q,64);q+=64;
    for(i=0;i<v->p.channels.count;++i,q+=64) {
        memcpy(v->p.midi_output[i],q,64);
        if(!terminated(v->p.midi_output[i],64))return PT_PROJECT_INVALID;
    }
    v->p.midi_flags=u32(q);
    if(!basic(&v->p))return PT_PROJECT_INVALID;
    caps=project_caps(&v->p);v->need.orders=v->p.order_count;v->need.samples=v->p.sample_count;
    v->need.events=(size_t)v->p.pattern_count*64*v->p.channels.count;
    if(v->length[2]!=(size_t)v->p.order_count*2 || v->length[3]!=v->need.events*12)return PT_PROJECT_INVALID;
    for(i=0;i<v->p.order_count;++i)if(u16(v->part[2]+i*2)>=v->p.pattern_count)return PT_PROJECT_INVALID;
    pos=0;
    for(i=0;i<v->p.sample_count;++i) {
        struct pt_sample s;size_t count_values,bytes,record;uint32_t previous=0;
        if(v->length[4]-pos<64)return PT_PROJECT_TRUNCATED;
        q=v->part[4]+pos;read_sample(q,&s);
        if(!sample_meta(&s) || u32(q+60))return PT_PROJECT_INVALID;
        if(s.pcm.frames>SIZE_MAX/s.pcm.channels/sizeof(int32_t))return PT_PROJECT_CAPACITY;
        count_values=(size_t)s.pcm.frames*s.pcm.channels;bytes=count_values*(s.pcm.bits/8);
        record=64+(size_t)s.slice_count*4;
        if(!add(&record,bytes) || record>v->length[4]-pos || pad(record)>v->length[4]-pos-record)return PT_PROJECT_TRUNCATED;
        q+=64;
        for(j=0;j<s.slice_count;++j,q+=4) {
            uint32_t value=u32(q);
            if(value>=s.pcm.frames || (j && value<=previous))return PT_PROJECT_INVALID;
            previous=value;
        }
        for(j=0;j<pad(record);++j)if(v->part[4][pos+record+j])return PT_PROJECT_INVALID;
        if(!add(&v->need.pcm_values,count_values) || !add(&v->need.slices,s.slice_count))return PT_PROJECT_CAPACITY;
        if(v->need.pcm_values>SIZE_MAX/sizeof(int32_t) || v->need.slices>SIZE_MAX/sizeof(uint32_t))return PT_PROJECT_CAPACITY;
        slices[i]=s.slice_count;caps|=sample_caps(&s);pos+=record+pad(record);
    }
    if(pos!=v->length[4])return PT_PROJECT_INVALID;
    for(i=0;i<v->need.events;++i) {
        q=v->part[3]+i*12;read_event(q,&e);
        if(u16(q+10) || !event_valid(&e,v->p.sample_count,e.instrument && e.instrument<=v->p.sample_count?slices[e.instrument-1]:0))return PT_PROJECT_INVALID;
        caps|=event_caps(&e);
    }
    if(caps!=u32(data+16))return PT_PROJECT_INVALID;
    v->need.capabilities=caps;return PT_PROJECT_OK;
}
enum pt_project_result pt_project_probe(const uint8_t *data,size_t length,struct pt_project_requirements *out)
{
    struct view v;enum pt_project_result r;
    if(!out)return PT_PROJECT_INVALID;
    r=scan(data,length,&v);if(r==PT_PROJECT_OK)*out=v.need;return r;
}
enum pt_project_result pt_project_decode(const uint8_t *data,size_t length,
                                         const struct pt_project_storage *s,struct pt_project *out)
{
    struct view v;enum pt_project_result r=scan(data,length,&v);
    const void *pointers[9];size_t bytes[9],i,j,pos,pc=0,sc=0,ec=0,eb=0;
    if(r!=PT_PROJECT_OK)return r;
    if(!s || !out)return PT_PROJECT_INVALID;
    if(s->order_capacity<v.need.orders || s->event_capacity<v.need.events || s->sample_capacity<v.need.samples ||
       s->pcm_capacity<v.need.pcm_values || s->slice_capacity<v.need.slices ||
       s->extension_capacity<v.need.extensions || s->extension_bytes<v.need.extension_bytes)return PT_PROJECT_CAPACITY;
    pointers[0]=s->orders;bytes[0]=v.need.orders*sizeof(*s->orders);
    pointers[1]=s->events;bytes[1]=v.need.events*sizeof(*s->events);
    pointers[2]=s->samples;bytes[2]=v.need.samples*sizeof(*s->samples);
    pointers[3]=s->pcm;bytes[3]=v.need.pcm_values*sizeof(*s->pcm);
    pointers[4]=s->slices;bytes[4]=v.need.slices*sizeof(*s->slices);
    pointers[5]=s->extensions;bytes[5]=v.need.extensions*sizeof(*s->extensions);
    pointers[6]=s->extension_data;bytes[6]=v.need.extension_bytes;
    pointers[7]=out;bytes[7]=sizeof(*out);pointers[8]=s;bytes[8]=sizeof(*s);
    for(i=0;i<9;++i) {
        if(bytes[i] && !pointers[i])return PT_PROJECT_INVALID;
        if(overlap(pointers[i],bytes[i],data,length))return PT_PROJECT_ALIAS;
        for(j=0;j<i;++j)if(overlap(pointers[i],bytes[i],pointers[j],bytes[j]))return PT_PROJECT_ALIAS;
    }
    for(i=0;i<v.need.orders;++i)s->orders[i]=(uint16_t)u16(v.part[2]+i*2);
    for(i=0;i<v.need.events;++i)read_event(v.part[3]+i*12,&s->events[i]);
    pos=0;
    for(i=0;i<v.need.samples;++i) {
        struct pt_sample *sample=&s->samples[i];const uint8_t *q=v.part[4]+pos;
        size_t values,record;unsigned width;
        read_sample(q,sample);q+=64;
        sample->slices=sample->slice_count?s->slices+sc:NULL;
        for(j=0;j<sample->slice_count;++j,q+=4)s->slices[sc++]=u32(q);
        values=(size_t)sample->pcm.frames*sample->pcm.channels;width=sample->pcm.bits/8;
        sample->pcm.data=values?s->pcm+pc:NULL;sample->pcm.capacity=values;
        for(j=0;j<values;++j,q+=width) {
            uint32_t value=q[0];unsigned b;
            for(b=1;b<width;++b)value=(value<<8)|q[b];
            s->pcm[pc++]=(int32_t)value-((value&(1UL<<(sample->pcm.bits-1)))?(1L<<sample->pcm.bits):0);
        }
        record=64+(size_t)sample->slice_count*4+values*width;pos+=record+pad(record);
    }
    pos=32;
    for(i=0;i<u32(data+24);++i) {
        size_t n=u32(data+pos+8);
        if(known(u32(data+pos))<0) {
            struct pt_extension *e=&s->extensions[ec++];e->id=u32(data+pos);e->version=(uint16_t)u16(data+pos+4);
            e->length=(uint32_t)n;e->data=n?s->extension_data+eb:NULL;
            if(n)memcpy(s->extension_data+eb,data+pos+12,n);
            eb+=n;
        }
        pos+=12+n+pad(n);
    }
    v.p.orders=s->orders;v.p.events=s->events;v.p.samples=s->samples;
    v.p.extension_count=v.need.extensions;v.p.extensions=s->extensions;*out=v.p;
    return PT_PROJECT_OK;
}

/* Bounded positional preflight. Keep the contiguous scanner as an independent
 * compatibility oracle while the document reader integration is developed. */
struct reader_view {size_t part[6],length[6];struct pt_project p;struct pt_project_requirements need;};
static int reader_bytes(pt_project_read read,void *context,size_t length,size_t offset,uint8_t *out,size_t count)
{return offset<=length && count<=length-offset && (!count || read(context,offset,out,count)==1);}
static int reader_checksum(pt_project_read read,void *context,size_t length,uint32_t *out)
{
    uint8_t block[1024];size_t pos=0,i;unsigned j;uint32_t crc=0xffffffffUL;
    while(pos<length) {
        size_t count=length-pos;if(count>sizeof(block))count=sizeof(block);
        if(!reader_bytes(read,context,length,pos,block,count))return 0;
        for(i=0;i<count;++i) {
            crc^=pos+i>=20 && pos+i<24?0:block[i];
            for(j=0;j<8;++j)crc=(crc>>1)^((crc&1)?0xedb88320UL:0);
        }
        pos+=count;
    }
    *out=crc^0xffffffffUL;return 1;
}
static enum pt_project_result scan_reader(pt_project_read read,void *context,size_t length,struct reader_view *v)
{
    size_t pos=32,i,j;unsigned count,mask=0;uint32_t caps;uint16_t slices[255];
    uint8_t data[32],buffer[1092],chunk[12];const uint8_t *q;struct pt_event e;
    uint32_t crc;
    if(!read)return PT_PROJECT_INVALID;
    if(length<32)return PT_PROJECT_TRUNCATED;
    if(!reader_bytes(read,context,length,0,data,32))return PT_PROJECT_TRUNCATED;
    if(memcmp(data,magic,8))return PT_PROJECT_INVALID;
    if(u16(data+8)!=1 || u16(data+10)!=0 || (u32(data+16)&~PT_CAP_KNOWN))return PT_PROJECT_UNSUPPORTED;
    if(u32(data+12)>length)return PT_PROJECT_TRUNCATED;
    if(u32(data+12)!=length || u32(data+28) || u32(data+24)>4096)return PT_PROJECT_INVALID;
    if(!reader_checksum(read,context,length,&crc))return PT_PROJECT_TRUNCATED;
    if(crc!=u32(data+20))return PT_PROJECT_CHECKSUM;
    memset(v,0,sizeof(*v));pt_channels_init(&v->p.channels);
    for(count=0;count<u32(data+24);++count) {
        size_t bytes,body;int index;
        if(length-pos<12)return PT_PROJECT_TRUNCATED;
        if(!reader_bytes(read,context,length,pos,chunk,12))return PT_PROJECT_TRUNCATED;
        bytes=u32(chunk+8);body=pos+12;
        if(bytes>length-body || pad(bytes)>length-body-bytes)return PT_PROJECT_TRUNCATED;
        if(!reader_bytes(read,context,length,body+bytes,buffer,pad(bytes)))return PT_PROJECT_TRUNCATED;
        for(i=0;i<pad(bytes);++i)if(buffer[i])return PT_PROJECT_INVALID;
        if(u16(chunk+6)&~1U)return PT_PROJECT_UNSUPPORTED;
        index=known(u32(chunk));
        if(index<0) {
            if(u16(chunk+6)&1)return PT_PROJECT_UNSUPPORTED;
            ++v->need.extensions;
            if(!add(&v->need.extension_bytes,bytes))return PT_PROJECT_CAPACITY;
        } else {
            if(mask&(1U<<index))return PT_PROJECT_INVALID;
            if(u16(chunk+4)!=1)return PT_PROJECT_UNSUPPORTED;
            if(u16(chunk+6)!=1)return PT_PROJECT_INVALID;
            mask|=1U<<index;v->part[index]=body;v->length[index]=bytes;
        }
        pos=body+bytes+pad(bytes);
    }
    if(pos!=length || mask!=63 || v->length[0]!=44)return PT_PROJECT_INVALID;
    if(!reader_bytes(read,context,length,v->part[0],buffer,44))return PT_PROJECT_TRUNCATED;
    q=buffer;memcpy(v->p.title,q,32);v->p.channels.count=q[32];v->p.channels.selected=q[33];
    v->p.speed=q[34];v->p.mode=q[35];v->p.bpm=(uint16_t)u16(q+36);
    v->p.order_count=(uint16_t)u16(q+38);v->p.pattern_count=(uint16_t)u16(q+40);v->p.sample_count=(uint16_t)u16(q+42);
    if(!v->p.channels.count || v->p.channels.count>16 ||
       v->length[1]!=(size_t)v->p.channels.count*22 ||
       v->length[5]!=64*((size_t)v->p.channels.count+1)+4)return PT_PROJECT_INVALID;
    if(!reader_bytes(read,context,length,v->part[1],buffer,v->length[1]))return PT_PROJECT_TRUNCATED;
    q=buffer;
    for(i=0;i<v->p.channels.count;++i,q+=22) {
        struct pt_channel *c=&v->p.channels.track[i];
        c->route=q[0];c->pan=q[1];c->muted=q[2];c->solo=q[3];c->group=q[4];c->midi_channel=q[5];memcpy(c->name,q+6,16);
    }
    if(!reader_bytes(read,context,length,v->part[5],buffer,v->length[5]))return PT_PROJECT_TRUNCATED;
    q=buffer;memcpy(v->p.midi_input,q,64);q+=64;
    for(i=0;i<v->p.channels.count;++i,q+=64) {
        memcpy(v->p.midi_output[i],q,64);
        if(!terminated(v->p.midi_output[i],64))return PT_PROJECT_INVALID;
    }
    v->p.midi_flags=u32(q);
    if(!basic(&v->p))return PT_PROJECT_INVALID;
    caps=project_caps(&v->p);v->need.orders=v->p.order_count;v->need.samples=v->p.sample_count;
    v->need.events=(size_t)v->p.pattern_count*64*v->p.channels.count;
    if(v->length[2]!=(size_t)v->p.order_count*2 || v->length[3]!=v->need.events*12)return PT_PROJECT_INVALID;
    for(i=0;i<v->p.order_count;++i) {
        if(!reader_bytes(read,context,length,v->part[2]+i*2,buffer,2))return PT_PROJECT_TRUNCATED;
        if(u16(buffer)>=v->p.pattern_count)return PT_PROJECT_INVALID;
    }
    pos=0;
    for(i=0;i<v->p.sample_count;++i) {
        struct pt_sample s;size_t count_values,bytes,record;uint32_t previous=0;
        if(v->length[4]-pos<64)return PT_PROJECT_TRUNCATED;
        if(!reader_bytes(read,context,length,v->part[4]+pos,buffer,64))return PT_PROJECT_TRUNCATED;
        q=buffer;read_sample(q,&s);
        if(!sample_meta(&s) || u32(q+60))return PT_PROJECT_INVALID;
        if(s.pcm.frames>SIZE_MAX/s.pcm.channels/sizeof(int32_t))return PT_PROJECT_CAPACITY;
        count_values=(size_t)s.pcm.frames*s.pcm.channels;bytes=count_values*(s.pcm.bits/8);
        record=64+(size_t)s.slice_count*4;
        if(!add(&record,bytes) || record>v->length[4]-pos || pad(record)>v->length[4]-pos-record)return PT_PROJECT_TRUNCATED;
        for(j=0;j<s.slice_count;++j) {
            uint32_t value;
            if(!reader_bytes(read,context,length,v->part[4]+pos+64+j*4,buffer,4))return PT_PROJECT_TRUNCATED;
            value=u32(buffer);
            if(value>=s.pcm.frames || (j && value<=previous))return PT_PROJECT_INVALID;
            previous=value;
        }
        if(!reader_bytes(read,context,length,v->part[4]+pos+record,buffer,pad(record)))return PT_PROJECT_TRUNCATED;
        for(j=0;j<pad(record);++j)if(buffer[j])return PT_PROJECT_INVALID;
        if(!add(&v->need.pcm_values,count_values) || !add(&v->need.slices,s.slice_count))return PT_PROJECT_CAPACITY;
        if(v->need.pcm_values>SIZE_MAX/sizeof(int32_t) || v->need.slices>SIZE_MAX/sizeof(uint32_t))return PT_PROJECT_CAPACITY;
        slices[i]=s.slice_count;caps|=sample_caps(&s);pos+=record+pad(record);
    }
    if(pos!=v->length[4])return PT_PROJECT_INVALID;
    for(i=0;i<v->need.events;++i) {
        if(!reader_bytes(read,context,length,v->part[3]+i*12,buffer,12))return PT_PROJECT_TRUNCATED;
        q=buffer;read_event(q,&e);
        if(u16(q+10) || !event_valid(&e,v->p.sample_count,e.instrument && e.instrument<=v->p.sample_count?slices[e.instrument-1]:0))return PT_PROJECT_INVALID;
        caps|=event_caps(&e);
    }
    if(caps!=u32(data+16))return PT_PROJECT_INVALID;
    v->need.capabilities=caps;return PT_PROJECT_OK;
}
enum pt_project_result pt_project_probe_reader(pt_project_read read,void *context,size_t length,struct pt_project_requirements *out)
{
    struct reader_view v;enum pt_project_result r;
    if(!out)return PT_PROJECT_INVALID;
    r=scan_reader(read,context,length,&v);if(r==PT_PROJECT_OK)*out=v.need;return r;
}

enum pt_project_result pt_project_decode_reader(pt_project_read read,void *context,size_t length,
                                                const struct pt_project_storage *s,struct pt_project *out)
{
    struct reader_view v;enum pt_project_result r=scan_reader(read,context,length,&v);
    const void *pointers[9];size_t bytes[9],i,j,pos,pc=0,sc=0,ec=0,eb=0;
    uint8_t block[1024],chunk[12];
    if(r!=PT_PROJECT_OK)return r;
    if(!s || !out)return PT_PROJECT_INVALID;
    if(s->order_capacity<v.need.orders || s->event_capacity<v.need.events || s->sample_capacity<v.need.samples ||
       s->pcm_capacity<v.need.pcm_values || s->slice_capacity<v.need.slices ||
       s->extension_capacity<v.need.extensions || s->extension_bytes<v.need.extension_bytes)return PT_PROJECT_CAPACITY;
    pointers[0]=s->orders;bytes[0]=v.need.orders*sizeof(*s->orders);
    pointers[1]=s->events;bytes[1]=v.need.events*sizeof(*s->events);
    pointers[2]=s->samples;bytes[2]=v.need.samples*sizeof(*s->samples);
    pointers[3]=s->pcm;bytes[3]=v.need.pcm_values*sizeof(*s->pcm);
    pointers[4]=s->slices;bytes[4]=v.need.slices*sizeof(*s->slices);
    pointers[5]=s->extensions;bytes[5]=v.need.extensions*sizeof(*s->extensions);
    pointers[6]=s->extension_data;bytes[6]=v.need.extension_bytes;
    pointers[7]=out;bytes[7]=sizeof(*out);pointers[8]=s;bytes[8]=sizeof(*s);
    for(i=0;i<9;++i) {
        if(bytes[i] && !pointers[i])return PT_PROJECT_INVALID;
        for(j=0;j<i;++j)if(overlap(pointers[i],bytes[i],pointers[j],bytes[j]))return PT_PROJECT_ALIAS;
    }
    for(i=0;i<v.need.orders;++i) {
        if(!reader_bytes(read,context,length,v.part[2]+i*2,block,2))return PT_PROJECT_TRUNCATED;
        s->orders[i]=(uint16_t)u16(block);
    }
    for(i=0;i<v.need.events;++i) {
        if(!reader_bytes(read,context,length,v.part[3]+i*12,block,12))return PT_PROJECT_TRUNCATED;
        if(u16(block+10))return PT_PROJECT_INVALID;
        read_event(block,&s->events[i]);
    }
    pos=0;
    for(i=0;i<v.need.samples;++i) {
        struct pt_sample *sample=&s->samples[i];size_t values,record,offset;unsigned width;
        if(pos>v.length[4] || v.length[4]-pos<64 ||
           !reader_bytes(read,context,length,v.part[4]+pos,block,64))return PT_PROJECT_TRUNCATED;
        read_sample(block,sample);
        if(!sample_meta(sample) || u32(block+60))return PT_PROJECT_INVALID;
        if(sample->pcm.frames>SIZE_MAX/sample->pcm.channels)return PT_PROJECT_CAPACITY;
        values=(size_t)sample->pcm.frames*sample->pcm.channels;width=sample->pcm.bits/8;
        if(values>s->pcm_capacity-pc || sample->slice_count>s->slice_capacity-sc || values>SIZE_MAX/width)return PT_PROJECT_CAPACITY;
        record=64+(size_t)sample->slice_count*4;
        if(!add(&record,values*width) || record>v.length[4]-pos || pad(record)>v.length[4]-pos-record)return PT_PROJECT_TRUNCATED;
        offset=v.part[4]+pos+64;
        sample->slices=sample->slice_count?s->slices+sc:NULL;
        for(j=0;j<sample->slice_count;++j,offset+=4) {
            if(!reader_bytes(read,context,length,offset,block,4))return PT_PROJECT_TRUNCATED;
            s->slices[sc++]=u32(block);
        }
        sample->pcm.data=values?s->pcm+pc:NULL;sample->pcm.capacity=values;
        for(j=0;j<values;) {
            size_t count=values-j,k;if(count>sizeof(block)/width)count=sizeof(block)/width;
            if(!reader_bytes(read,context,length,offset,block,count*width))return PT_PROJECT_TRUNCATED;
            for(k=0;k<count;++k) {
                const uint8_t *q=block+k*width;uint32_t value=q[0];unsigned bit;
                for(bit=1;bit<width;++bit)value=(value<<8)|q[bit];
                s->pcm[pc++]=(int32_t)value-((value&(1UL<<(sample->pcm.bits-1)))?(1L<<sample->pcm.bits):0);
            }
            j+=count;offset+=count*width;
        }
        if(!reader_bytes(read,context,length,v.part[4]+pos+record,block,pad(record)))return PT_PROJECT_TRUNCATED;
        for(j=0;j<pad(record);++j)if(block[j])return PT_PROJECT_INVALID;
        pos+=record+pad(record);
    }
    if(pos!=v.length[4] || pc!=v.need.pcm_values || sc!=v.need.slices)return PT_PROJECT_INVALID;
    pos=32;
    while(pos<length) {
        size_t n,body;
        if(!reader_bytes(read,context,length,pos,chunk,12))return PT_PROJECT_TRUNCATED;
        n=u32(chunk+8);body=pos+12;
        if(n>length-body || pad(n)>length-body-n)return PT_PROJECT_TRUNCATED;
        if(known(u32(chunk))<0) {
            struct pt_extension *e;
            if(ec>=s->extension_capacity || n>s->extension_bytes-eb)return PT_PROJECT_CAPACITY;
            if(u16(chunk+6))return PT_PROJECT_UNSUPPORTED;
            e=&s->extensions[ec++];e->id=u32(chunk);e->version=(uint16_t)u16(chunk+4);
            e->length=(uint32_t)n;e->data=n?s->extension_data+eb:NULL;
            for(j=0;j<n;) {
                size_t count=n-j;if(count>sizeof(block))count=sizeof(block);
                if(!reader_bytes(read,context,length,body+j,s->extension_data+eb+j,count))return PT_PROJECT_TRUNCATED;
                j+=count;
            }
            eb+=n;
        }
        pos=body+n+pad(n);
    }
    if(ec!=v.need.extensions || eb!=v.need.extension_bytes)return PT_PROJECT_INVALID;
    v.p.orders=s->orders;v.p.events=s->events;v.p.samples=s->samples;
    v.p.extension_count=ec;v.p.extensions=s->extensions;
    if(pt_project_validate(&v.p,NULL)!=PT_PROJECT_OK)return PT_PROJECT_INVALID;
    *out=v.p;return PT_PROJECT_OK;
}
