#include <string.h>
#include "midi.h"
static int valid(const struct pt_midi *m) {return m && m->sink.send;}
static enum pt_midi_result send(struct pt_midi *m,unsigned port,unsigned status,unsigned a,unsigned b,size_t n)
{
    uint8_t bytes[3];bytes[0]=(uint8_t)status;bytes[1]=(uint8_t)a;bytes[2]=(uint8_t)b;
    if(!m->ports[port].connected)return PT_MIDI_UNAVAILABLE;
    return m->sink.send(m->sink.context,port,bytes,n)?PT_MIDI_OK:PT_MIDI_SEND_FAILED;
}
int pt_midi_init(struct pt_midi *m,const struct pt_midi_sink *sink)
{
    struct pt_midi_sink copy;
    if(!m || !sink || !sink->send)return 0;
    copy=*sink;memset(m,0,sizeof(*m));m->sink=copy;return 1;
}
static int another_owner(const struct pt_midi *m,unsigned except,const struct pt_midi_voice *v)
{
    unsigned i;
    for(i=0;i<PT_MIDI_TRACKS;++i) {
        const struct pt_midi_voice *other=&m->voices[i];
        if(i!=except && other->active && other->endpoint==v->endpoint &&
            other->channel==v->channel && other->note==v->note)return 1;
    }
    return 0;
}
static enum pt_midi_result flush(struct pt_midi *m,unsigned port)
{
    struct pt_midi_port *p=&m->ports[port];unsigned channel,note;enum pt_midi_result r;
    if(!p->connected)return PT_MIDI_UNAVAILABLE;
    for(channel=0;channel<16;++channel) {
        if(p->sustain&(1U<<channel)) {
            r=send(m,port,0xb0|channel,64,0,3);if(r!=PT_MIDI_OK)return r;
            p->sustain&=(uint16_t)~(1U<<channel);
        }
        for(note=0;note<128;++note)if(p->pending_off[channel][note/8]&(1U<<(note%8))) {
            r=send(m,port,0x80|channel,note,0,3);if(r!=PT_MIDI_OK)return r;
            p->pending_off[channel][note/8]&=(uint8_t)~(1U<<(note%8));
        }
    }
    p->needs_flush=0;return PT_MIDI_OK;
}
enum pt_midi_result pt_midi_connection(struct pt_midi *m,unsigned port,int connected)
{
    unsigned i;struct pt_midi_port *p;
    if(!valid(m) || port>=PT_MIDI_PORTS || (connected!=0 && connected!=1))return PT_MIDI_INVALID;
    p=&m->ports[port];
    if(!connected) {
        p->connected=0;p->needs_flush=1;
        for(i=0;i<PT_MIDI_TRACKS;++i) {
            struct pt_midi_voice *v=&m->voices[i];
            if(v->active && v->endpoint==port) {
                p->pending_off[v->channel][v->note/8]|=(uint8_t)(1U<<(v->note%8));v->active=0;
            }
        }
        return PT_MIDI_OK;
    }
    /* An already connected notification must not cancel live sustain. */
    if(p->connected)return p->needs_flush?flush(m,port):PT_MIDI_OK;
    p->connected=1;return flush(m,port);
}
enum pt_midi_result pt_midi_note_off(struct pt_midi *m,unsigned track)
{
    struct pt_midi_voice *v;enum pt_midi_result r;
    if(!valid(m) || track>=PT_MIDI_TRACKS)return PT_MIDI_INVALID;
    v=&m->voices[track];if(!v->active)return PT_MIDI_OK;
    if(!another_owner(m,track,v)) {
        r=send(m,v->endpoint,0x80|v->channel,v->note,0,3);if(r!=PT_MIDI_OK)return r;
    }
    v->active=0;return PT_MIDI_OK;
}
enum pt_midi_result pt_midi_bind(struct pt_midi *m,unsigned track,unsigned port,unsigned channel)
{
    struct pt_midi_voice *v;enum pt_midi_result r;
    if(!valid(m) || track>=PT_MIDI_TRACKS || port>=PT_MIDI_PORTS || channel<1 || channel>16)return PT_MIDI_INVALID;
    v=&m->voices[track];
    if(v->bound && v->endpoint==port && v->channel==channel-1)return PT_MIDI_OK;
    r=pt_midi_note_off(m,track);if(r!=PT_MIDI_OK)return r;
    v->bound=1;v->endpoint=(uint8_t)port;v->channel=(uint8_t)(channel-1);return PT_MIDI_OK;
}
enum pt_midi_result pt_midi_unbind(struct pt_midi *m,unsigned track)
{
    enum pt_midi_result r=pt_midi_note_off(m,track);
    if(r==PT_MIDI_OK)m->voices[track].bound=0;
    return r;
}
static enum pt_midi_result ready(struct pt_midi *m,unsigned port)
{
    struct pt_midi_port *p=&m->ports[port];
    if(!p->connected)return PT_MIDI_UNAVAILABLE;
    return p->needs_flush?flush(m,port):PT_MIDI_OK;
}
enum pt_midi_result pt_midi_note_on(struct pt_midi *m,unsigned track,unsigned note,unsigned velocity)
{
    struct pt_midi_voice *v;enum pt_midi_result r;
    if(!valid(m) || track>=PT_MIDI_TRACKS || note>127 || velocity>127 || !m->voices[track].bound)return PT_MIDI_INVALID;
    if(!velocity)return pt_midi_note_off(m,track);
    v=&m->voices[track];r=ready(m,v->endpoint);if(r!=PT_MIDI_OK)return r;
    r=pt_midi_note_off(m,track);if(r!=PT_MIDI_OK)return r;
    v->note=(uint8_t)note;
    if(!another_owner(m,track,v)) {
        r=send(m,v->endpoint,0x90|v->channel,note,velocity,3);if(r!=PT_MIDI_OK)return r;
    }
    v->active=1;return PT_MIDI_OK;
}
static enum pt_midi_result track_ready(struct pt_midi *m,unsigned track)
{
    if(!valid(m) || track>=PT_MIDI_TRACKS || !m->voices[track].bound)return PT_MIDI_INVALID;
    return ready(m,m->voices[track].endpoint);
}
enum pt_midi_result pt_midi_controller(struct pt_midi *m,unsigned track,unsigned controller,unsigned value)
{
    enum pt_midi_result r;struct pt_midi_voice *v;
    if(controller>=120 || value>127)return PT_MIDI_INVALID;
    r=track_ready(m,track);if(r!=PT_MIDI_OK)return r;v=&m->voices[track];
    r=send(m,v->endpoint,0xb0|v->channel,controller,value,3);
    if(r==PT_MIDI_OK && controller==64) {
        if(value>=64)m->ports[v->endpoint].sustain|=(uint16_t)(1U<<v->channel);
        else m->ports[v->endpoint].sustain&=(uint16_t)~(1U<<v->channel);
    }
    return r;
}
enum pt_midi_result pt_midi_program(struct pt_midi *m,unsigned track,unsigned program)
{
    enum pt_midi_result r;struct pt_midi_voice *v;
    if(program>127)return PT_MIDI_INVALID;
    r=track_ready(m,track);if(r!=PT_MIDI_OK)return r;v=&m->voices[track];
    return send(m,v->endpoint,0xc0|v->channel,program,0,2);
}
enum pt_midi_result pt_midi_bend(struct pt_midi *m,unsigned track,unsigned bend)
{
    enum pt_midi_result r;struct pt_midi_voice *v;
    if(bend>16383)return PT_MIDI_INVALID;
    r=track_ready(m,track);if(r!=PT_MIDI_OK)return r;v=&m->voices[track];
    return send(m,v->endpoint,0xe0|v->channel,bend&127,bend>>7,3);
}
enum pt_midi_result pt_midi_realtime(struct pt_midi *m,unsigned port,unsigned status)
{
    if(!valid(m) || port>=PT_MIDI_PORTS || (status!=0xf8 && status!=0xfa && status!=0xfb && status!=0xfc))return PT_MIDI_INVALID;
    return send(m,port,status,0,0,1);
}
enum pt_midi_result pt_midi_stop(struct pt_midi *m)
{
    enum pt_midi_result result=PT_MIDI_OK,r;unsigned i,c;
    if(!valid(m))return PT_MIDI_INVALID;
    for(i=0;i<PT_MIDI_TRACKS;++i) {r=pt_midi_note_off(m,i);if(r!=PT_MIDI_OK)result=r;}
    for(i=0;i<PT_MIDI_PORTS;++i) {
        int pending=m->ports[i].sustain!=0;
        for(c=0;c<16 && !pending;++c) {unsigned b;for(b=0;b<16;++b)pending|=m->ports[i].pending_off[c][b];}
        if(pending) {r=flush(m,i);if(r!=PT_MIDI_OK)result=r;}
    }
    return result;
}
