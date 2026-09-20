#include <string.h>
#include "mod_project.h"
#include "mod_inspect.h"
static uint32_t u16(const uint8_t *p) {return (uint32_t)p[0]*256+p[1];}
static void w16(uint8_t *p,uint32_t n) {p[0]=(uint8_t)(n>>8);p[1]=(uint8_t)n;}
static int overlap(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if(!an || !bn)return 0;
    if(an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y)return 1;
    return x<y+bn && y<x+an;
}
static const uint8_t *original(const struct pt_project *p)
{
    unsigned i;
    for(i=0;i<p->extension_count;++i)if(p->extensions[i].id==PT_CLASSIC_HEADER_TAG &&
       p->extensions[i].version==1 && p->extensions[i].length==1084)return p->extensions[i].data;
    return NULL;
}
enum pt_project_result pt_mod_project_probe(const uint8_t *data,size_t length,struct pt_project_requirements *out)
{
    struct pt_mod_info info;struct pt_project_requirements r;
    enum pt_mod_status status;
    if(!out)return PT_PROJECT_INVALID;
    status=pt_mod_inspect(data,length,&info);
    if(status==PT_MOD_UNSUPPORTED_FORMAT)return PT_PROJECT_UNSUPPORTED;
    if(status!=PT_MOD_OK)return PT_PROJECT_INVALID;
    if(info.warnings)return PT_PROJECT_UNSUPPORTED;
    memset(&r,0,sizeof(r));r.orders=info.song_length;r.samples=31;r.events=(size_t)info.patterns*64*4;
    r.pcm_values=info.sample_bytes;r.extensions=1;r.extension_bytes=1084;
    *out=r;return PT_PROJECT_OK;
}
enum pt_project_result pt_mod_project_decode(const uint8_t *data,size_t length,
                                             const struct pt_project_storage *s,struct pt_project *out)
{
    struct pt_project_requirements need;struct pt_mod_info info;struct pt_project p;
    const void *pointers[8];size_t bytes[8],i,j,pos,pc=0;
    enum pt_project_result r=pt_mod_project_probe(data,length,&need);
    if(r!=PT_PROJECT_OK)return r;
    if(!s || !out)return PT_PROJECT_INVALID;
    if(s->order_capacity<need.orders || s->event_capacity<need.events || s->sample_capacity<31 ||
       s->pcm_capacity<need.pcm_values || s->extension_capacity<1 || s->extension_bytes<1084)return PT_PROJECT_CAPACITY;
    pointers[0]=s->orders;bytes[0]=need.orders*sizeof(*s->orders);
    pointers[1]=s->events;bytes[1]=need.events*sizeof(*s->events);
    pointers[2]=s->samples;bytes[2]=31*sizeof(*s->samples);
    pointers[3]=s->pcm;bytes[3]=need.pcm_values*sizeof(*s->pcm);
    pointers[4]=s->extensions;bytes[4]=sizeof(*s->extensions);
    pointers[5]=s->extension_data;bytes[5]=1084;pointers[6]=out;bytes[6]=sizeof(*out);pointers[7]=s;bytes[7]=sizeof(*s);
    for(i=0;i<8;++i) {
        if(bytes[i] && !pointers[i])return PT_PROJECT_INVALID;
        if(overlap(pointers[i],bytes[i],data,length))return PT_PROJECT_ALIAS;
        for(j=0;j<i;++j)if(overlap(pointers[i],bytes[i],pointers[j],bytes[j]))return PT_PROJECT_ALIAS;
    }
    pt_mod_inspect(data,length,&info);memset(&p,0,sizeof(p));memcpy(p.title,data,20);pt_channels_init(&p.channels);
    p.bpm=125;p.speed=6;p.order_count=need.orders;p.pattern_count=info.patterns;p.sample_count=31;
    p.orders=s->orders;p.events=s->events;p.samples=s->samples;p.extension_count=1;p.extensions=s->extensions;
    for(i=0;i<need.orders;++i)s->orders[i]=data[952+i];
    for(i=0;i<need.events;++i) {
        const uint8_t *q=data+1084+i*4;struct pt_event *e=&s->events[i];memset(e,0,sizeof(*e));
        e->pitch=(uint16_t)(((q[0]&15)*256)+q[1]);e->kind=e->pitch?PT_NOTE_PERIOD:PT_NOTE_NONE;
        e->instrument=(q[0]&0xf0)|(q[2]>>4);e->effect=q[2]&15;e->parameter=q[3];
    }
    pos=info.sample_offset;
    for(i=0;i<31;++i) {
        struct pt_sample *sample=&s->samples[i];const uint8_t *q=data+20+i*30;uint32_t n=u16(q+22)*2,loop=u16(q+28)*2;
        memset(sample,0,sizeof(*sample));memcpy(sample->name,q,22);sample->pcm.frames=n;sample->pcm.capacity=n;
        sample->pcm.data=n?s->pcm+pc:NULL;sample->pcm.channels=1;sample->pcm.bits=8;sample->pcm.rate=PT_CLASSIC_RATE;
        sample->volume=q[25];sample->finetune=(int8_t)(q[24]>7?(int)q[24]-16:q[24]);
        if(loop>2) {sample->loop=PT_LOOP_FORWARD;sample->loop_start=u16(q+26)*2;sample->loop_end=sample->loop_start+loop;}
        for(j=0;j<n;++j) {unsigned value=data[pos++];s->pcm[pc++]=(int32_t)value-(value>=128?256:0);}
    }
    s->extensions[0].id=PT_CLASSIC_HEADER_TAG;s->extensions[0].version=1;s->extensions[0].length=1084;
    s->extensions[0].data=s->extension_data;memcpy(s->extension_data,data,1084);*out=p;return PT_PROJECT_OK;
}
enum pt_project_result pt_mod_export_analyse(const struct pt_project *p,struct pt_mod_export_report *out)
{
    struct pt_mod_export_report r;unsigned i,classic_headers=0;size_t count;uint32_t maxorder=0;const uint8_t *old;
    enum pt_project_result result=pt_project_validate(p,NULL);
    if(result!=PT_PROJECT_OK)return result;
    if(!out)return PT_PROJECT_INVALID;
    memset(&r,0,sizeof(r));old=original(p);
    if(p->channels.count>4)r.issues|=PT_EXPORT_CHANNELS;
    if(p->channels.count<4)r.issues|=PT_EXPORT_METADATA;
    if(p->sample_count>31 || p->order_count>128 || p->pattern_count>100)r.issues|=PT_EXPORT_LIMITS;
    if(p->bpm!=125 || p->speed!=6)r.issues|=PT_EXPORT_TEMPO;
    if(!memchr(p->title,0,21) && p->title[20])r.issues|=PT_EXPORT_METADATA;
    if(p->mode!=PT_MODE_WAVETABLE || p->midi_flags || p->midi_input[0])r.issues|=PT_EXPORT_METADATA;
    for(i=0;i<p->channels.count;++i) {
        const struct pt_channel *c=&p->channels.track[i];
        if(c->route!=PT_PAULA)r.issues|=PT_EXPORT_ROUTING;
        if(c->route==PT_MIDI)r.issues|=PT_EXPORT_MIDI_AUDIO;
        if(c->pan!=((i%4==0 || i%4==3)?0:255))r.issues|=PT_EXPORT_PANNING;
        if(c->muted || c->solo || c->group || c->name[0] || c->midi_channel!=i+1 || p->midi_output[i][0])r.issues|=PT_EXPORT_METADATA;
    }
    for(i=0;i<p->sample_count;++i) {
        const struct pt_sample *s=&p->samples[i];
        if(s->pcm.bits!=8)r.issues|=PT_EXPORT_PRECISION;
        if(s->pcm.channels!=1)r.issues|=PT_EXPORT_STEREO;
        if(s->pcm.rate!=PT_CLASSIC_RATE)r.issues|=PT_EXPORT_RATE;
        if(s->pcm.frames>131070 || (s->pcm.frames&1))r.issues|=PT_EXPORT_LIMITS;
        if(s->slice_count)r.issues|=PT_EXPORT_SLICES;
        /* A preserved header can contain a one-word loop start which is not
           represented by PT_LOOP_NONE. Never emit an out-of-sample DMA range
           from untrusted CMOD metadata or after shortening that sample. */
        if(old && i<31 && s->loop==PT_LOOP_NONE) {
            uint32_t start=u16(old+20+i*30+26),repeat=u16(old+20+i*30+28);
            if(repeat<=1 && start && start+1>s->pcm.frames/2)r.issues|=PT_EXPORT_LOOPS;
        }
        if(s->loop>PT_LOOP_FORWARD || (s->loop_start&1) || (s->loop_end&1) ||
           (s->loop==PT_LOOP_FORWARD && s->loop_end-s->loop_start<=2))r.issues|=PT_EXPORT_LOOPS;
        if((!memchr(s->name,0,23) && s->name[22]) || s->interpolation)r.issues|=PT_EXPORT_METADATA;
    }
    count=(size_t)p->pattern_count*64*p->channels.count;
    for(i=0;i<count;++i) {
        const struct pt_event *e=&p->events[i];
        if(e->kind==PT_NOTE_OFF)r.issues|=PT_EXPORT_OFF;
        if(e->kind==PT_NOTE_MIDI)r.issues|=PT_EXPORT_NOTES;
        if(e->flags)r.issues|=PT_EXPORT_VELOCITY;
    }
    for(i=0;i<p->extension_count;++i) {
        const struct pt_extension *e=&p->extensions[i];
        if(e->id!=PT_CLASSIC_HEADER_TAG || e->version!=1 || e->length!=1084)r.issues|=PT_EXPORT_METADATA;
        else {
            ++classic_headers;
            if(classic_headers>1 || !e->data[950] || e->data[950]>128 ||
               (memcmp(e->data+1080,"M.K.",4) && memcmp(e->data+1080,"M!K!",4)))r.issues|=PT_EXPORT_METADATA;
        }
    }
    for(i=0;i<p->order_count;++i)if(p->orders[i]>maxorder)maxorder=p->orders[i];
    if(old)for(i=p->order_count;i<128;++i)if(old[952+i]<p->pattern_count && old[952+i]>maxorder)maxorder=old[952+i];
    if(p->order_count==128 && maxorder+1<p->pattern_count)r.issues|=PT_EXPORT_LIMITS;
    if(r.issues&PT_EXPORT_MIDI_AUDIO)r.classification=PT_CONVERSION_INCOMPLETE;
    else if(r.issues&PT_EXPORT_CHANNELS)r.classification=PT_CONVERSION_BOUNCED;
    else if(r.issues)r.classification=PT_CONVERSION_CONVERTED;
    else {
        r.classification=PT_CONVERSION_LOSSLESS;r.bytes=1084+(size_t)p->pattern_count*1024;
        for(i=0;i<p->sample_count;++i)r.bytes+=p->samples[i].pcm.frames;
    }
    *out=r;return PT_PROJECT_OK;
}
enum pt_project_result pt_mod_export_direct(const struct pt_project *p,uint8_t *out,size_t capacity,size_t *written)
{
    struct pt_mod_export_report report;enum pt_project_result r=pt_mod_export_analyse(p,&report);
    const uint8_t *old;unsigned i,j,maxorder=0;size_t pos,count;
    if(r!=PT_PROJECT_OK)return r;
    if(report.issues)return PT_PROJECT_UNSUPPORTED;
    if(!out || !written)return PT_PROJECT_INVALID;
    if(capacity<report.bytes)return PT_PROJECT_CAPACITY;
    count=(size_t)p->pattern_count*64*p->channels.count;
    if(overlap(out,report.bytes,p,sizeof(*p)) || overlap(out,report.bytes,written,sizeof(*written)) ||
       overlap(out,report.bytes,p->orders,p->order_count*sizeof(*p->orders)) ||
       overlap(out,report.bytes,p->events,count*sizeof(*p->events)) ||
       overlap(out,report.bytes,p->samples,p->sample_count*sizeof(*p->samples)) ||
       overlap(out,report.bytes,p->extensions,p->extension_count*sizeof(*p->extensions)))return PT_PROJECT_ALIAS;
    for(i=0;i<p->sample_count;++i)if(overlap(out,report.bytes,p->samples[i].pcm.data,
        (size_t)p->samples[i].pcm.frames*sizeof(int32_t)))return PT_PROJECT_ALIAS;
    for(i=0;i<p->extension_count;++i)if(overlap(out,report.bytes,p->extensions[i].data,p->extensions[i].length))return PT_PROJECT_ALIAS;
    memset(out,0,report.bytes);old=original(p);if(old)memcpy(out,old,1084);
    memcpy(out,p->title,20);out[950]=(uint8_t)p->order_count;if(!old)out[951]=127;
    for(i=0;i<128;++i) {
        unsigned value=i<p->order_count?p->orders[i]:(old && old[952+i]<p->pattern_count?old[952+i]:0);
        out[952+i]=(uint8_t)value;if(value>maxorder)maxorder=value;
    }
    if(maxorder+1<p->pattern_count)out[1079]=(uint8_t)(p->pattern_count-1);
    memcpy(out+1080,p->pattern_count>64 || (old && old[1081]=='!')?"M!K!":"M.K.",4);
    pos=1084;
    for(i=0;i<(unsigned)p->pattern_count*64;++i)for(j=0;j<4;++j,pos+=4)if(j<p->channels.count) {
        const struct pt_event *e=&p->events[(size_t)i*p->channels.count+j];
        out[pos]=(e->instrument&0xf0)|(uint8_t)(e->pitch>>8);out[pos+1]=(uint8_t)e->pitch;
        out[pos+2]=(uint8_t)((e->instrument<<4)|e->effect);out[pos+3]=e->parameter;
    }
    for(i=0;i<31;++i) {
        uint8_t *q=out+20+i*30;
        if(i<p->sample_count) {
            const struct pt_sample *s=&p->samples[i];memcpy(q,s->name,22);w16(q+22,s->pcm.frames/2);
            q[24]=(uint8_t)s->finetune&15;q[25]=s->volume;
            if(s->loop==PT_LOOP_FORWARD) {w16(q+26,s->loop_start/2);w16(q+28,(s->loop_end-s->loop_start)/2);}
            else if(!old || u16(q+28)>1) {w16(q+26,0);w16(q+28,1);}
            for(j=0;j<s->pcm.frames;++j)out[pos++]=(uint8_t)s->pcm.data[j];
        } else {memset(q,0,30);w16(q+28,1);}
    }
    *written=report.bytes;return PT_PROJECT_OK;
}
