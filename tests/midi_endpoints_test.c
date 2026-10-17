#include "src/platform/midi_endpoints.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define MESSAGES 96U
struct message {char identity[PT_MIDI_ENDPOINT];uint8_t bytes[3];size_t n;int accepted;};
struct capture {
    struct pt_midi_endpoints *owner;
    struct message messages[MESSAGES];
    unsigned calls,fail_call,reenter,reentry_checks;
};
struct rig {struct pt_midi_endpoints owner;struct capture output;};
static struct pt_midi_endpoint_name named(const char *s)
{
    struct pt_midi_endpoint_name n;assert(strlen(s)<sizeof(n.identity));
    memset(&n,0,sizeof(n));memcpy(n.identity,s,strlen(s)+1);return n;
}
static int send_message(void *context,const char *identity,const uint8_t *bytes,size_t n)
{
    struct capture *c=context;struct message *m;
    assert(c&&identity&&n&&n<=3&&c->calls<MESSAGES);
    m=&c->messages[c->calls++];assert(strlen(identity)<sizeof(m->identity));
    memcpy(m->identity,identity,strlen(identity)+1);memcpy(m->bytes,bytes,n);m->n=n;
    if(c->reenter){
        struct pt_midi_endpoints *r=c->owner;
        unsigned char before[sizeof(*r)];struct pt_midi_endpoint_sink sink=r->sink;
        memcpy(before,r,sizeof(before));c->reenter=0;
        assert(pt_midi_endpoints_observe(r,(const struct pt_midi_endpoint_name *)(uintptr_t)1,16)==PT_ENDPOINT_BUSY);
        assert(pt_midi_endpoints_connection(r,0,0)==PT_ENDPOINT_BUSY);
        assert(pt_midi_endpoints_forget(r,0)==PT_ENDPOINT_BUSY);
        assert(pt_midi_endpoints_bind(r,0,0,1)==PT_ENDPOINT_BUSY);
        assert(pt_midi_endpoints_unbind(r,0)==PT_ENDPOINT_BUSY);
        assert(pt_midi_endpoints_note_on(r,0,60,100)==PT_ENDPOINT_BUSY);
        assert(pt_midi_endpoints_note_off(r,0)==PT_ENDPOINT_BUSY);
        assert(pt_midi_endpoints_controller(r,0,64,0)==PT_ENDPOINT_BUSY);
        assert(pt_midi_endpoints_program(r,0,1)==PT_ENDPOINT_BUSY);
        assert(pt_midi_endpoints_bend(r,0,8192)==PT_ENDPOINT_BUSY);
        assert(pt_midi_endpoints_realtime(r,0,0xf8)==PT_ENDPOINT_BUSY);
        assert(pt_midi_endpoints_stop(r)==PT_ENDPOINT_BUSY);
        assert(pt_midi_endpoints_find(r,"Port A",7)==-1);
        assert(pt_midi_endpoints_available(r,0)==-1);
        assert(pt_midi_endpoints_name(r,0)==NULL);
        assert(!pt_midi_endpoints_init(r,&sink));
        assert(!memcmp(before,r,sizeof(before)));++c->reentry_checks;
    }
    m->accepted=c->calls!=c->fail_call;return m->accepted;
}
static void setup(struct rig *f)
{
    struct pt_midi_endpoint_sink s;
    memset(f,0,sizeof(*f));f->output.owner=&f->owner;
    s.context=&f->output;s.context_bytes=sizeof(f->output);s.send=send_message;
    assert(pt_midi_endpoints_init(&f->owner,&s));
}
static unsigned port(struct rig *f,const char *s)
{int p=pt_midi_endpoints_find(&f->owner,s,strlen(s)+1);assert(p>=0);return (unsigned)p;}
static void discover(struct rig *f,const char *a,const char *b)
{
    struct pt_midi_endpoint_name n[2];size_t count=0;
    if(a)n[count++]=named(a);
    if(b)n[count++]=named(b);
    assert(pt_midi_endpoints_observe(&f->owner,count?n:NULL,count)==PT_ENDPOINT_OK);
}
static void connect_port(struct rig *f,unsigned p)
{assert(pt_midi_endpoints_connection(&f->owner,p,1)==PT_ENDPOINT_OK);}
static void bind_track(struct rig *f,unsigned track,unsigned p)
{assert(pt_midi_endpoints_bind(&f->owner,track,p,1)==PT_ENDPOINT_OK);}
static void expect(const struct rig *f,unsigned index,const char *id,unsigned status,unsigned a,unsigned b,size_t n,int accepted)
{
    const struct message *m;uint8_t gold[3];assert(index<f->output.calls);m=&f->output.messages[index];
    gold[0]=(uint8_t)status;gold[1]=(uint8_t)a;gold[2]=(uint8_t)b;
    assert(!strcmp(m->identity,id)&&m->n==n&&m->accepted==accepted&&!memcmp(m->bytes,gold,n));
}
static void unchanged(const struct pt_midi_endpoints *r,const unsigned char *before)
{assert(!memcmp(r,before,sizeof(*r)));}
static void cleanup(struct rig *f)
{
    unsigned t,p;assert(pt_midi_endpoints_stop(&f->owner)==PT_ENDPOINT_OK);
    for(t=0;t<PT_MIDI_TRACKS;++t)assert(pt_midi_endpoints_unbind(&f->owner,t)==PT_ENDPOINT_OK);
    for(p=0;p<PT_MIDI_PORTS;++p)if(pt_midi_endpoints_name(&f->owner,p))
        assert(pt_midi_endpoints_forget(&f->owner,p)==PT_ENDPOINT_OK);
    for(t=0;t<PT_MIDI_TRACKS;++t)assert(!f->owner.midi.voices[t].bound&&!f->owner.midi.voices[t].active);
    for(p=0;p<PT_MIDI_PORTS;++p){
        assert(!f->owner.names[p].identity[0]&&!f->owner.midi.ports[p].sustain&&!f->owner.midi.ports[p].needs_flush);
        for(t=0;t<16;++t){unsigned j;for(j=0;j<16;++j)assert(!f->owner.midi.ports[p].pending_off[t][j]);}
    }
    assert(!f->owner.available&&!f->owner.link_active&&!f->owner.busy);
}
static void discovery(void)
{
    struct rig f;struct pt_midi_endpoint_name names[3];unsigned a,b;char longest[64];
    setup(&f);memset(longest,'L',63);longest[63]=0;
    names[0]=named("Port A");names[1]=named("port a");names[2]=named(longest);
    assert(pt_midi_endpoints_observe(&f.owner,names,3)==PT_ENDPOINT_OK);
    a=port(&f,"Port A");b=port(&f,"port a");assert(a!=b);
    assert(pt_midi_endpoints_available(&f.owner,a)==1);
    assert(!strcmp(pt_midi_endpoints_name(&f.owner,port(&f,longest))->identity,longest));
    names[0]=named(longest);names[1]=named("port a");names[2]=named("Port A");
    assert(pt_midi_endpoints_observe(&f.owner,names,3)==PT_ENDPOINT_OK);
    assert(port(&f,"Port A")==a&&port(&f,"port a")==b);
    bind_track(&f,0,a);assert(pt_midi_endpoints_forget(&f.owner,a)==PT_ENDPOINT_IN_USE);
    assert(pt_midi_endpoints_unbind(&f.owner,0)==PT_ENDPOINT_OK);
    assert(!f.output.calls);cleanup(&f);assert(!f.output.calls);
}
static void atomic_admission(void)
{
    struct rig f;struct pt_midi_endpoint_name names[16],bad[16];unsigned i,p;
    unsigned char before[sizeof(f.owner)];setup(&f);
    for(i=0;i<16;++i){char s[20];snprintf(s,sizeof(s),"Port %u",i);names[i]=named(s);}
    assert(pt_midi_endpoints_observe(&f.owner,names,16)==PT_ENDPOINT_OK);
    p=port(&f,"Port 0");connect_port(&f,p);bind_track(&f,0,p);
    assert(pt_midi_endpoints_note_on(&f.owner,0,60,100)==PT_ENDPOINT_OK);
    memcpy(before,&f.owner,sizeof(before));
    for(i=0;i<15;++i)bad[i]=names[i+1];
    bad[15]=named("New destination");
    assert(pt_midi_endpoints_observe(&f.owner,bad,16)==PT_ENDPOINT_CAPACITY);unchanged(&f.owner,before);
    bad[0]=names[1];bad[1]=names[1];assert(pt_midi_endpoints_observe(&f.owner,bad,2)==PT_ENDPOINT_INVALID);unchanged(&f.owner,before);
    memset(bad[0].identity,'x',64);assert(pt_midi_endpoints_observe(&f.owner,bad,1)==PT_ENDPOINT_INVALID);unchanged(&f.owner,before);
    assert(pt_midi_endpoints_observe(&f.owner,NULL,1)==PT_ENDPOINT_INVALID);unchanged(&f.owner,before);
    assert(pt_midi_endpoints_observe(&f.owner,names,17)==PT_ENDPOINT_INVALID);unchanged(&f.owner,before);
    assert(pt_midi_endpoints_observe(&f.owner,(const struct pt_midi_endpoint_name *)(uintptr_t)(UINTPTR_MAX-31),1)==PT_ENDPOINT_INVALID);unchanged(&f.owner,before);
    assert(pt_midi_endpoints_observe(&f.owner,f.owner.names,1)==PT_ENDPOINT_INVALID);unchanged(&f.owner,before);
    assert(f.output.calls==1&&f.owner.midi.voices[0].active&&f.owner.midi.ports[p].connected);
    cleanup(&f);assert(f.output.calls==2);
}
static void loss_reconnect_and_reuse(void)
{
    struct rig f;unsigned a,b;unsigned char before[sizeof(f.owner)];setup(&f);discover(&f,"Port A","Port B");
    a=port(&f,"Port A");b=port(&f,"Port B");connect_port(&f,a);bind_track(&f,0,a);
    assert(pt_midi_endpoints_note_on(&f.owner,0,60,100)==PT_ENDPOINT_OK);
    assert(pt_midi_endpoints_controller(&f.owner,0,64,127)==PT_ENDPOINT_OK);
    discover(&f,"Port B",NULL);assert(f.output.calls==2&&!f.owner.midi.ports[a].connected);
    assert(!f.owner.midi.voices[0].active&&(f.owner.midi.ports[a].pending_off[0][7]&(1U<<4)));
    assert(f.owner.midi.ports[a].sustain==1&&f.owner.midi.ports[a].needs_flush);
    discover(&f,"Port B","Port A");assert(port(&f,"Port A")==a&&port(&f,"Port B")==b&&!f.owner.link_active&&f.output.calls==2);
    assert(pt_midi_endpoints_forget(&f.owner,a)==PT_ENDPOINT_IN_USE);
    connect_port(&f,a);assert(f.output.calls==4);expect(&f,2,"Port A",0xb0,64,0,3,1);expect(&f,3,"Port A",0x80,60,0,3,1);
    assert(pt_midi_endpoints_unbind(&f.owner,0)==PT_ENDPOINT_OK);
    assert(pt_midi_endpoints_forget(&f.owner,a)==PT_ENDPOINT_OK&&f.owner.midi.ports[a].connected);
    discover(&f,"Port C","Port B");assert(port(&f,"Port C")==a);
    bind_track(&f,0,a);memcpy(before,&f.owner,sizeof(before));
    assert(pt_midi_endpoints_note_on(&f.owner,0,64,90)==PT_ENDPOINT_UNAVAILABLE);unchanged(&f.owner,before);assert(f.output.calls==4);
    connect_port(&f,a);assert(f.output.calls==4);assert(pt_midi_endpoints_note_on(&f.owner,0,64,90)==PT_ENDPOINT_OK);
    expect(&f,4,"Port C",0x90,64,90,3,1);cleanup(&f);expect(&f,5,"Port C",0x80,64,0,3,1);
}
static void release_failure(void)
{
    struct rig f;unsigned a;unsigned char before[sizeof(f.owner)];setup(&f);discover(&f,"Port A",NULL);a=port(&f,"Port A");connect_port(&f,a);bind_track(&f,0,a);
    assert(pt_midi_endpoints_note_on(&f.owner,0,65,70)==PT_ENDPOINT_OK);memcpy(before,&f.owner,sizeof(before));
    f.output.fail_call=2;assert(pt_midi_endpoints_unbind(&f.owner,0)==PT_ENDPOINT_SEND_FAILED);unchanged(&f.owner,before);
    expect(&f,1,"Port A",0x80,65,0,3,0);assert(pt_midi_endpoints_forget(&f.owner,a)==PT_ENDPOINT_IN_USE);unchanged(&f.owner,before);
    f.output.fail_call=0;assert(pt_midi_endpoints_unbind(&f.owner,0)==PT_ENDPOINT_OK);expect(&f,2,"Port A",0x80,65,0,3,1);
    assert(pt_midi_endpoints_forget(&f.owner,a)==PT_ENDPOINT_OK);cleanup(&f);
}
static void sustain_and_empty_flush(void)
{
    struct rig f;unsigned a;unsigned char before[sizeof(f.owner)];setup(&f);discover(&f,"Port A",NULL);a=port(&f,"Port A");connect_port(&f,a);bind_track(&f,0,a);
    assert(pt_midi_endpoints_controller(&f.owner,0,64,127)==PT_ENDPOINT_OK);
    assert(pt_midi_endpoints_unbind(&f.owner,0)==PT_ENDPOINT_OK);
    memcpy(before,&f.owner,sizeof(before));assert(pt_midi_endpoints_forget(&f.owner,a)==PT_ENDPOINT_IN_USE);unchanged(&f.owner,before);
    discover(&f,NULL,NULL);assert(f.output.calls==1&&f.owner.midi.ports[a].needs_flush);
    discover(&f,"Port A",NULL);f.output.fail_call=2;
    assert(pt_midi_endpoints_connection(&f.owner,a,1)==PT_ENDPOINT_SEND_FAILED);
    expect(&f,1,"Port A",0xb0,64,0,3,0);assert(f.owner.midi.ports[a].sustain==1&&f.owner.midi.ports[a].needs_flush&&!f.owner.link_active);
    memcpy(before,&f.owner,sizeof(before));assert(pt_midi_endpoints_forget(&f.owner,a)==PT_ENDPOINT_IN_USE);unchanged(&f.owner,before);
    f.output.fail_call=0;connect_port(&f,a);expect(&f,2,"Port A",0xb0,64,0,3,1);
    assert(pt_midi_endpoints_connection(&f.owner,a,0)==PT_ENDPOINT_OK);
    assert(f.owner.midi.ports[a].needs_flush&&!f.owner.midi.ports[a].sustain);
    memcpy(before,&f.owner,sizeof(before));assert(pt_midi_endpoints_forget(&f.owner,a)==PT_ENDPOINT_IN_USE);unchanged(&f.owner,before);
    connect_port(&f,a);assert(f.output.calls==3);assert(pt_midi_endpoints_forget(&f.owner,a)==PT_ENDPOINT_OK);cleanup(&f);
}
static void partial_flush(void)
{
    struct rig f;unsigned a;setup(&f);discover(&f,"Port A",NULL);a=port(&f,"Port A");connect_port(&f,a);bind_track(&f,0,a);bind_track(&f,1,a);
    assert(pt_midi_endpoints_note_on(&f.owner,0,60,100)==PT_ENDPOINT_OK);
    assert(pt_midi_endpoints_note_on(&f.owner,1,61,90)==PT_ENDPOINT_OK);
    assert(pt_midi_endpoints_controller(&f.owner,0,64,127)==PT_ENDPOINT_OK);
    discover(&f,NULL,NULL);assert(f.output.calls==3);
    assert(pt_midi_endpoints_unbind(&f.owner,0)==PT_ENDPOINT_OK&&pt_midi_endpoints_unbind(&f.owner,1)==PT_ENDPOINT_OK);
    discover(&f,"Port A",NULL);f.output.fail_call=5;
    assert(pt_midi_endpoints_connection(&f.owner,a,1)==PT_ENDPOINT_SEND_FAILED);
    expect(&f,3,"Port A",0xb0,64,0,3,1);expect(&f,4,"Port A",0x80,60,0,3,0);
    assert(!f.owner.midi.ports[a].sustain&&f.owner.midi.ports[a].needs_flush&&f.owner.midi.ports[a].pending_off[0][7]==0x30&&!f.owner.link_active);
    assert(pt_midi_endpoints_forget(&f.owner,a)==PT_ENDPOINT_IN_USE);
    f.output.fail_call=0;connect_port(&f,a);assert(f.output.calls==7);
    expect(&f,5,"Port A",0x80,60,0,3,1);expect(&f,6,"Port A",0x80,61,0,3,1);
    assert(!f.owner.midi.ports[a].needs_flush&&!f.owner.midi.ports[a].pending_off[0][7]);cleanup(&f);
}
static void shared_notes(void)
{
    struct rig f;unsigned a;setup(&f);discover(&f,"Port A",NULL);a=port(&f,"Port A");connect_port(&f,a);bind_track(&f,0,a);bind_track(&f,1,a);
    assert(pt_midi_endpoints_note_on(&f.owner,0,60,80)==PT_ENDPOINT_OK);
    assert(pt_midi_endpoints_note_on(&f.owner,1,60,100)==PT_ENDPOINT_OK&&f.output.calls==1);
    assert(pt_midi_endpoints_note_off(&f.owner,0)==PT_ENDPOINT_OK&&f.output.calls==1);
    assert(f.owner.midi.voices[1].active&&pt_midi_endpoints_forget(&f.owner,a)==PT_ENDPOINT_IN_USE);
    assert(pt_midi_endpoints_note_off(&f.owner,1)==PT_ENDPOINT_OK&&f.output.calls==2);
    expect(&f,1,"Port A",0x80,60,0,3,1);cleanup(&f);assert(f.output.calls==2);
}
static void wrapper_gates_and_bytes(void)
{
    char overlong[8]="Port A";
    struct rig f;unsigned a;unsigned char before[sizeof(f.owner)];setup(&f);discover(&f,"Port A",NULL);a=port(&f,"Port A");bind_track(&f,0,a);memcpy(before,&f.owner,sizeof(before));
    assert(pt_midi_endpoints_note_on(&f.owner,0,60,100)==PT_ENDPOINT_UNAVAILABLE);unchanged(&f.owner,before);
    assert(pt_midi_endpoints_controller(&f.owner,0,64,0)==PT_ENDPOINT_UNAVAILABLE);unchanged(&f.owner,before);
    assert(pt_midi_endpoints_program(&f.owner,0,12)==PT_ENDPOINT_UNAVAILABLE);unchanged(&f.owner,before);
    assert(pt_midi_endpoints_bend(&f.owner,0,8193)==PT_ENDPOINT_UNAVAILABLE);unchanged(&f.owner,before);
    assert(pt_midi_endpoints_realtime(&f.owner,a,0xfa)==PT_ENDPOINT_UNAVAILABLE);unchanged(&f.owner,before);
    assert(!f.output.calls);connect_port(&f,a);
    assert(pt_midi_endpoints_program(&f.owner,0,12)==PT_ENDPOINT_OK);expect(&f,0,"Port A",0xc0,12,0,2,1);
    assert(pt_midi_endpoints_bend(&f.owner,0,8193)==PT_ENDPOINT_OK);expect(&f,1,"Port A",0xe0,1,64,3,1);
    assert(pt_midi_endpoints_realtime(&f.owner,a,0xfa)==PT_ENDPOINT_OK);expect(&f,2,"Port A",0xfa,0,0,1,1);
    assert(pt_midi_endpoints_note_on(&f.owner,0,60,100)==PT_ENDPOINT_OK);memcpy(before,&f.owner,sizeof(before));
    assert(pt_midi_endpoints_bind(&f.owner,0,a,2)==PT_ENDPOINT_IN_USE);unchanged(&f.owner,before);
    assert(pt_midi_endpoints_note_on(&f.owner,16,60,100)==PT_ENDPOINT_INVALID);unchanged(&f.owner,before);
    assert(pt_midi_endpoints_note_on(&f.owner,0,128,100)==PT_ENDPOINT_INVALID);unchanged(&f.owner,before);
    assert(pt_midi_endpoints_note_on(&f.owner,0,60,128)==PT_ENDPOINT_INVALID);unchanged(&f.owner,before);
    assert(pt_midi_endpoints_controller(&f.owner,0,120,0)==PT_ENDPOINT_INVALID);unchanged(&f.owner,before);
    assert(pt_midi_endpoints_program(&f.owner,0,128)==PT_ENDPOINT_INVALID);unchanged(&f.owner,before);
    assert(pt_midi_endpoints_bend(&f.owner,0,16384)==PT_ENDPOINT_INVALID);unchanged(&f.owner,before);
    assert(pt_midi_endpoints_realtime(&f.owner,a,0xff)==PT_ENDPOINT_INVALID);unchanged(&f.owner,before);
    assert(pt_midi_endpoints_connection(&f.owner,16,1)==PT_ENDPOINT_INVALID);unchanged(&f.owner,before);
    assert(pt_midi_endpoints_connection(&f.owner,a,2)==PT_ENDPOINT_INVALID);unchanged(&f.owner,before);
    assert(pt_midi_endpoints_bind(&f.owner,0,a,0)==PT_ENDPOINT_INVALID);unchanged(&f.owner,before);
    assert(pt_midi_endpoints_bind(&f.owner,0,a,17)==PT_ENDPOINT_INVALID);unchanged(&f.owner,before);
    assert(pt_midi_endpoints_find(&f.owner,"Port A",6)==-1&&pt_midi_endpoints_find(&f.owner,overlong,sizeof(overlong))==-1);
    unchanged(&f.owner,before);assert(f.output.calls==4);cleanup(&f);
}
static void init_guards(void)
{
    struct rig f;struct pt_midi_endpoint_sink sink;unsigned char before[sizeof(f.owner)];
    memset(&f,0,sizeof(f));sink.context=&f.output;sink.context_bytes=sizeof(f.output);sink.send=send_message;memcpy(before,&f.owner,sizeof(before));
    assert(!pt_midi_endpoints_init(NULL,&sink)&&!pt_midi_endpoints_init(&f.owner,NULL));unchanged(&f.owner,before);
    sink.send=NULL;assert(!pt_midi_endpoints_init(&f.owner,&sink));unchanged(&f.owner,before);sink.send=send_message;
    sink.context=&f.owner;sink.context_bytes=sizeof(f.owner);assert(!pt_midi_endpoints_init(&f.owner,&sink));unchanged(&f.owner,before);
    sink.context=(void *)(uintptr_t)(UINTPTR_MAX-1);sink.context_bytes=4;assert(!pt_midi_endpoints_init(&f.owner,&sink));unchanged(&f.owner,before);
    sink.context=&f.output;sink.context_bytes=0;assert(!pt_midi_endpoints_init(&f.owner,&sink));unchanged(&f.owner,before);
    sink.context=NULL;sink.context_bytes=1;assert(!pt_midi_endpoints_init(&f.owner,&sink));unchanged(&f.owner,before);
    assert(!pt_midi_endpoints_init(&f.owner,(const struct pt_midi_endpoint_sink *)&f.owner));unchanged(&f.owner,before);
    setup(&f);memcpy(before,&f.owner,sizeof(before));assert(!pt_midi_endpoints_init(&f.owner,&f.owner.sink));unchanged(&f.owner,before);
    assert(pt_midi_endpoints_observe(NULL,NULL,0)==PT_ENDPOINT_INVALID);cleanup(&f);
}
static void serialized_reentry(void)
{
    struct rig f;unsigned a;setup(&f);discover(&f,"Port A",NULL);a=port(&f,"Port A");connect_port(&f,a);bind_track(&f,0,a);
    f.output.reenter=1;assert(pt_midi_endpoints_note_on(&f.owner,0,60,100)==PT_ENDPOINT_OK);
    assert(f.output.reentry_checks==1&&f.owner.midi.voices[0].active&&!f.owner.busy);expect(&f,0,"Port A",0x90,60,100,3,1);
    discover(&f,NULL,NULL);discover(&f,"Port A",NULL);f.output.reenter=1;
    connect_port(&f,a);assert(f.output.reentry_checks==2&&!f.owner.midi.ports[a].needs_flush);expect(&f,1,"Port A",0x80,60,0,3,1);
    cleanup(&f);assert(f.output.calls==2);
}
static void partial_stop_two_destinations(void)
{
    struct rig f;unsigned a,b;setup(&f);discover(&f,"Port A","Port B");a=port(&f,"Port A");b=port(&f,"Port B");connect_port(&f,a);connect_port(&f,b);bind_track(&f,0,a);bind_track(&f,1,b);
    assert(pt_midi_endpoints_note_on(&f.owner,0,60,100)==PT_ENDPOINT_OK);
    assert(pt_midi_endpoints_controller(&f.owner,0,64,127)==PT_ENDPOINT_OK);
    assert(pt_midi_endpoints_note_on(&f.owner,1,67,90)==PT_ENDPOINT_OK);
    assert(pt_midi_endpoints_controller(&f.owner,1,64,127)==PT_ENDPOINT_OK);
    discover(&f,"Port B",NULL);assert(f.output.calls==4);
    assert(pt_midi_endpoints_stop(&f.owner)==PT_ENDPOINT_UNAVAILABLE);
    assert(f.output.calls==6);expect(&f,4,"Port B",0x80,67,0,3,1);expect(&f,5,"Port B",0xb0,64,0,3,1);
    assert(!f.owner.midi.voices[1].active&&!f.owner.midi.ports[b].sustain);
    assert(f.owner.midi.ports[a].needs_flush&&f.owner.midi.ports[a].sustain&&f.owner.midi.ports[a].pending_off[0][7]==0x10);
    assert(pt_midi_endpoints_forget(&f.owner,a)==PT_ENDPOINT_IN_USE);
    discover(&f,"Port B","Port A");assert(f.output.calls==6);connect_port(&f,a);
    expect(&f,6,"Port A",0xb0,64,0,3,1);expect(&f,7,"Port A",0x80,60,0,3,1);cleanup(&f);assert(f.output.calls==8);
}
int main(void)
{
    assert(PT_MIDI_PORTS==16&&PT_MIDI_ENDPOINT==64);
    discovery();atomic_admission();loss_reconnect_and_reuse();release_failure();sustain_and_empty_flush();partial_flush();shared_notes();wrapper_gates_and_bytes();init_guards();serialized_reentry();partial_stop_two_destinations();
    puts("MIDI ENDPOINT HOST PASS:11 cases;stable16 identities;genuine note/sustain ownership;sendless discovery;explicit reconnect;partial STOP;serialized reentry;NO CAMD/PLAY/TIMING AUTHORITY");
    return 0;
}
