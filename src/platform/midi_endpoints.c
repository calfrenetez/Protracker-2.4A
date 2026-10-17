#include "midi_endpoints.h"
#include <string.h>

static int mapped_send(void *,unsigned,const uint8_t *,size_t);
static int span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int apart(const void *p,size_t n,const void *q,size_t m)
{uintptr_t a=(uintptr_t)p,b=(uintptr_t)q;return span(p,n)&&span(q,m)&&(!n||!m||a>=b+m||b>=a+n);}
static int zero(const void *p,size_t n)
{const unsigned char *b=p;size_t i;for(i=0;i<n;++i)if(b[i])return 0;return 1;}
static int valid(const struct pt_midi_endpoints *r)
{return r&&r->self==r&&r->sink.send&&r->midi.sink.context==r&&r->midi.sink.send==mapped_send;}
static int assigned(const struct pt_midi_endpoints *r,unsigned p)
{return p<PT_MIDI_PORTS&&r->names[p].identity[0];}
static int ready(const struct pt_midi_endpoints *r,unsigned p)
{return assigned(r,p)&&(r->available&(1U<<p))&&(r->link_active&(1U<<p));}
static enum pt_midi_endpoint_result admission(const struct pt_midi_endpoints *r)
{return !valid(r)?PT_ENDPOINT_INVALID:r->busy?PT_ENDPOINT_BUSY:PT_ENDPOINT_OK;}
static enum pt_midi_endpoint_result result(enum pt_midi_result v)
{
    switch(v){case PT_MIDI_OK:return PT_ENDPOINT_OK;case PT_MIDI_UNAVAILABLE:return PT_ENDPOINT_UNAVAILABLE;
    case PT_MIDI_SEND_FAILED:return PT_ENDPOINT_SEND_FAILED;default:return PT_ENDPOINT_INVALID;}
}
static enum pt_midi_endpoint_result leave(struct pt_midi_endpoints *r,enum pt_midi_result v)
{r->busy=0;return result(v);}
static int pending(const struct pt_midi_port *p)
{
    unsigned channel,byte;
    if(p->sustain||p->needs_flush)return 1;
    for(channel=0;channel<16;++channel)for(byte=0;byte<16;++byte)if(p->pending_off[channel][byte])return 1;
    return 0;
}
static int used(const struct pt_midi_endpoints *r,unsigned p)
{
    unsigned t;
    if(pending(&r->midi.ports[p]))return 1;
    for(t=0;t<PT_MIDI_TRACKS;++t)if((r->midi.voices[t].bound||r->midi.voices[t].active)&&
        r->midi.voices[t].endpoint==p)return 1;
    return 0;
}
static int track_ready(const struct pt_midi_endpoints *r,unsigned t)
{return t<PT_MIDI_TRACKS&&r->midi.voices[t].bound&&ready(r,r->midi.voices[t].endpoint);}
static int mapped_send(void *context,unsigned port,const uint8_t *bytes,size_t n)
{
    struct pt_midi_endpoints *r=context;
    if(!valid(r)||!r->busy||!ready(r,port)||!span(bytes,n)||!n||n>3)return 0;
    return r->sink.send(r->sink.context,r->names[port].identity,bytes,n)!=0;
}
int pt_midi_endpoints_init(struct pt_midi_endpoints *r,const struct pt_midi_endpoint_sink *sink)
{
    struct pt_midi_sink core;
    if(!r||!sink||!span(r,sizeof(*r))||!apart(r,sizeof(*r),sink,sizeof(*sink))||!sink->send||
       (sink->context?(!sink->context_bytes||!span(sink->context,sink->context_bytes)):sink->context_bytes!=0)||
       !apart(r,sizeof(*r),sink->context,sink->context_bytes)||!zero(r,sizeof(*r)))return 0;
    core.context=r;core.send=mapped_send;
    if(!pt_midi_init(&r->midi,&core))return 0;
    r->sink=*sink;r->self=r;return 1;
}
enum pt_midi_endpoint_result pt_midi_endpoints_observe(struct pt_midi_endpoints *r,
    const struct pt_midi_endpoint_name *names,size_t count)
{
    unsigned slot[PT_MIDI_PORTS],i,j,p,free_count=0,need=0;uint16_t seen=0;
    enum pt_midi_endpoint_result a=admission(r);
    if(a!=PT_ENDPOINT_OK)return a;
    if(count>PT_MIDI_PORTS||!span(names,count*sizeof(*names))||
       !apart(r,sizeof(*r),names,count*sizeof(*names)))return PT_ENDPOINT_INVALID;
    for(i=0;i<count;++i){
        if(!names[i].identity[0]||!memchr(names[i].identity,0,PT_MIDI_ENDPOINT))return PT_ENDPOINT_INVALID;
        for(j=0;j<i;++j)if(!strcmp(names[i].identity,names[j].identity))return PT_ENDPOINT_INVALID;
        slot[i]=PT_MIDI_PORTS;
        for(p=0;p<PT_MIDI_PORTS;++p)if(!strcmp(names[i].identity,r->names[p].identity)){slot[i]=p;break;}
        if(slot[i]==PT_MIDI_PORTS)++need;else seen|=(uint16_t)(1U<<slot[i]);
    }
    for(p=0;p<PT_MIDI_PORTS;++p)if(!assigned(r,p))++free_count;
    if(need>free_count)return PT_ENDPOINT_CAPACITY;
    r->busy=1;
    /* Fully validated admission above; these genuine disconnects cannot send. */
    for(p=0;p<PT_MIDI_PORTS;++p)if(assigned(r,p)&&!(seen&(1U<<p))){
        if(r->midi.ports[p].connected)(void)pt_midi_connection(&r->midi,p,0);
        r->link_active&=(uint16_t)~(1U<<p);
    }
    for(i=0;i<count;++i)if(slot[i]==PT_MIDI_PORTS){
        for(p=0;p<PT_MIDI_PORTS&&assigned(r,p);++p){}
        memset(r->names[p].identity,0,PT_MIDI_ENDPOINT);
        memcpy(r->names[p].identity,names[i].identity,strlen(names[i].identity)+1);
        slot[i]=p;seen|=(uint16_t)(1U<<p);
    }
    r->available=seen;r->busy=0;return PT_ENDPOINT_OK;
}
int pt_midi_endpoints_find(const struct pt_midi_endpoints *r,const char *name,size_t n)
{
    unsigned p;
    if(admission(r)!=PT_ENDPOINT_OK||n<2||n>PT_MIDI_ENDPOINT||!span(name,n)||
       name[n-1]||memchr(name,0,n-1))return -1;
    for(p=0;p<PT_MIDI_PORTS;++p)if(!strcmp(r->names[p].identity,name))return (int)p;
    return -1;
}
int pt_midi_endpoints_available(const struct pt_midi_endpoints *r,unsigned p)
{return admission(r)!=PT_ENDPOINT_OK||!assigned(r,p)?-1:(r->available&(1U<<p))!=0;}
const struct pt_midi_endpoint_name *pt_midi_endpoints_name(const struct pt_midi_endpoints *r,unsigned p)
{return admission(r)!=PT_ENDPOINT_OK||!assigned(r,p)?NULL:&r->names[p];}
enum pt_midi_endpoint_result pt_midi_endpoints_connection(struct pt_midi_endpoints *r,unsigned p,int connected)
{
    enum pt_midi_endpoint_result a=admission(r);enum pt_midi_result v;uint16_t old;
    if(a!=PT_ENDPOINT_OK)return a;
    if(!assigned(r,p)||(connected!=0&&connected!=1))return PT_ENDPOINT_INVALID;
    if(connected&&!(r->available&(1U<<p)))return PT_ENDPOINT_UNAVAILABLE;
    old=r->link_active;r->busy=1;
    if(connected)r->link_active|=(uint16_t)(1U<<p);
    v=pt_midi_connection(&r->midi,p,connected);
    if(!connected)r->link_active&=(uint16_t)~(1U<<p);
    else if(v!=PT_MIDI_OK)r->link_active=old;
    return leave(r,v);
}
enum pt_midi_endpoint_result pt_midi_endpoints_forget(struct pt_midi_endpoints *r,unsigned p)
{
    enum pt_midi_endpoint_result a=admission(r);
    if(a!=PT_ENDPOINT_OK)return a;
    if(!assigned(r,p))return PT_ENDPOINT_INVALID;
    if(used(r,p))return PT_ENDPOINT_IN_USE;
    r->busy=1;r->link_active&=(uint16_t)~(1U<<p);r->available&=(uint16_t)~(1U<<p);
    memset(&r->names[p],0,sizeof(r->names[p]));r->busy=0;return PT_ENDPOINT_OK;
}
enum pt_midi_endpoint_result pt_midi_endpoints_bind(struct pt_midi_endpoints *r,unsigned t,unsigned p,unsigned channel)
{
    enum pt_midi_endpoint_result a=admission(r);
    if(a!=PT_ENDPOINT_OK)return a;
    if(t>=PT_MIDI_TRACKS||!assigned(r,p)||channel<1||channel>16)return PT_ENDPOINT_INVALID;
    if(r->midi.voices[t].active)return PT_ENDPOINT_IN_USE;
    r->busy=1;return leave(r,pt_midi_bind(&r->midi,t,p,channel));
}
enum pt_midi_endpoint_result pt_midi_endpoints_unbind(struct pt_midi_endpoints *r,unsigned t)
{
    enum pt_midi_endpoint_result a=admission(r);
    if(a!=PT_ENDPOINT_OK)return a;
    if(t>=PT_MIDI_TRACKS)return PT_ENDPOINT_INVALID;
    if(r->midi.voices[t].active&&!track_ready(r,t))return PT_ENDPOINT_UNAVAILABLE;
    r->busy=1;return leave(r,pt_midi_unbind(&r->midi,t));
}
enum pt_midi_endpoint_result pt_midi_endpoints_note_on(struct pt_midi_endpoints *r,unsigned t,unsigned n,unsigned v)
{
    enum pt_midi_endpoint_result a=admission(r);
    if(a!=PT_ENDPOINT_OK)return a;
    if(t>=PT_MIDI_TRACKS||n>127||v>127||!r->midi.voices[t].bound)return PT_ENDPOINT_INVALID;
    if(!track_ready(r,t))return PT_ENDPOINT_UNAVAILABLE;
    r->busy=1;return leave(r,pt_midi_note_on(&r->midi,t,n,v));
}
enum pt_midi_endpoint_result pt_midi_endpoints_note_off(struct pt_midi_endpoints *r,unsigned t)
{
    enum pt_midi_endpoint_result a=admission(r);
    if(a!=PT_ENDPOINT_OK)return a;
    if(t>=PT_MIDI_TRACKS)return PT_ENDPOINT_INVALID;
    if(r->midi.voices[t].active&&!track_ready(r,t))return PT_ENDPOINT_UNAVAILABLE;
    r->busy=1;return leave(r,pt_midi_note_off(&r->midi,t));
}
enum pt_midi_endpoint_result pt_midi_endpoints_controller(struct pt_midi_endpoints *r,unsigned t,unsigned c,unsigned v)
{
    enum pt_midi_endpoint_result a=admission(r);
    if(a!=PT_ENDPOINT_OK)return a;
    if(t>=PT_MIDI_TRACKS||c>=120||v>127||!r->midi.voices[t].bound)return PT_ENDPOINT_INVALID;
    if(!track_ready(r,t))return PT_ENDPOINT_UNAVAILABLE;
    r->busy=1;return leave(r,pt_midi_controller(&r->midi,t,c,v));
}
enum pt_midi_endpoint_result pt_midi_endpoints_program(struct pt_midi_endpoints *r,unsigned t,unsigned program)
{
    enum pt_midi_endpoint_result a=admission(r);
    if(a!=PT_ENDPOINT_OK)return a;
    if(t>=PT_MIDI_TRACKS||program>127||!r->midi.voices[t].bound)return PT_ENDPOINT_INVALID;
    if(!track_ready(r,t))return PT_ENDPOINT_UNAVAILABLE;
    r->busy=1;return leave(r,pt_midi_program(&r->midi,t,program));
}
enum pt_midi_endpoint_result pt_midi_endpoints_bend(struct pt_midi_endpoints *r,unsigned t,unsigned bend)
{
    enum pt_midi_endpoint_result a=admission(r);
    if(a!=PT_ENDPOINT_OK)return a;
    if(t>=PT_MIDI_TRACKS||bend>16383||!r->midi.voices[t].bound)return PT_ENDPOINT_INVALID;
    if(!track_ready(r,t))return PT_ENDPOINT_UNAVAILABLE;
    r->busy=1;return leave(r,pt_midi_bend(&r->midi,t,bend));
}
enum pt_midi_endpoint_result pt_midi_endpoints_realtime(struct pt_midi_endpoints *r,unsigned p,unsigned status)
{
    enum pt_midi_endpoint_result a=admission(r);
    if(a!=PT_ENDPOINT_OK)return a;
    if(!assigned(r,p)||(status!=0xf8&&status!=0xfa&&status!=0xfb&&status!=0xfc))return PT_ENDPOINT_INVALID;
    if(!ready(r,p))return PT_ENDPOINT_UNAVAILABLE;
    r->busy=1;return leave(r,pt_midi_realtime(&r->midi,p,status));
}
enum pt_midi_endpoint_result pt_midi_endpoints_stop(struct pt_midi_endpoints *r)
{
    enum pt_midi_endpoint_result a=admission(r);
    if(a!=PT_ENDPOINT_OK)return a;
    /* The genuine core releases every reachable destination and retains failed
     * obligations. One missing endpoint must not keep healthy notes sounding. */
    r->busy=1;return leave(r,pt_midi_stop(&r->midi));
}
