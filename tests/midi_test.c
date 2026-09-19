#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "midi.h"
struct packet {unsigned port;size_t n;uint8_t bytes[3];};
struct recorder {struct packet packets[8192];unsigned count,calls,fail;};
static struct recorder log;
static int transmit(void *ctx,unsigned port,const uint8_t *bytes,size_t n)
{
    struct recorder *r=ctx;struct packet *p;
    ++r->calls;if(r->calls==r->fail)return 0;
    assert(r->count<8192 && n && n<=3);p=&r->packets[r->count++];p->port=port;p->n=n;memcpy(p->bytes,bytes,n);return 1;
}
static void packet(unsigned index,unsigned port,size_t n,unsigned status,unsigned a,unsigned b)
{
    const struct packet *p=&log.packets[index];assert(p->port==port && p->n==n && p->bytes[0]==status);
    if(n>1)assert(p->bytes[1]==a);
    if(n>2)assert(p->bytes[2]==b);
}
int main(void)
{
    struct pt_midi m;struct pt_midi_sink sink={&log,transmit};unsigned before,i,pass;
    assert(pt_midi_init(&m,&sink));assert(pt_midi_bind(&m,0,0,1)==PT_MIDI_OK);
    assert(pt_midi_note_on(&m,0,60,100)==PT_MIDI_UNAVAILABLE && !log.count);
    assert(pt_midi_connection(&m,0,1)==PT_MIDI_OK);
    assert(pt_midi_note_on(&m,0,60,100)==PT_MIDI_OK);packet(0,0,3,0x90,60,100);
    /* Tracks sharing the same endpoint/channel/note co-own one wire note. */
    assert(pt_midi_bind(&m,1,0,1)==PT_MIDI_OK && pt_midi_note_on(&m,1,60,70)==PT_MIDI_OK && log.count==1);
    assert(pt_midi_note_off(&m,0)==PT_MIDI_OK && log.count==1 && m.voices[1].active);
    assert(pt_midi_note_off(&m,1)==PT_MIDI_OK && log.count==2);packet(1,0,3,0x80,60,0);
    assert(pt_midi_note_on(&m,0,64,127)==PT_MIDI_OK);
    before=log.count;assert(pt_midi_note_on(&m,0,67,90)==PT_MIDI_OK);
    packet(before,0,3,0x80,64,0);packet(before+1,0,3,0x90,67,90);
    /* Release failure keeps both ownership and old binding. */
    log.fail=log.calls+1;assert(pt_midi_bind(&m,0,1,2)==PT_MIDI_SEND_FAILED);
    assert(m.voices[0].active && m.voices[0].endpoint==0 && m.voices[0].note==67);
    log.fail=0;assert(pt_midi_bind(&m,0,1,2)==PT_MIDI_OK && !m.voices[0].active);
    assert(pt_midi_connection(&m,1,1)==PT_MIDI_OK && pt_midi_note_on(&m,0,0,1)==PT_MIDI_OK);
    packet(log.count-1,1,3,0x91,0,1);
    /* A successful old off followed by a failed new on leaves no phantom voice. */
    log.fail=log.calls+2;assert(pt_midi_note_on(&m,0,127,127)==PT_MIDI_SEND_FAILED && !m.voices[0].active);
    log.fail=0;assert(pt_midi_note_on(&m,0,127,127)==PT_MIDI_OK);
    assert(pt_midi_controller(&m,0,64,127)==PT_MIDI_OK && m.ports[1].sustain==2);
    before=log.count;assert(pt_midi_connection(&m,1,1)==PT_MIDI_OK && log.count==before && m.ports[1].sustain==2);
    assert(pt_midi_connection(&m,1,0)==PT_MIDI_OK && !m.voices[0].active && m.ports[1].needs_flush);
    assert(pt_midi_note_on(&m,0,60,100)==PT_MIDI_UNAVAILABLE);
    before=log.count;log.fail=log.calls+2;
    assert(pt_midi_connection(&m,1,1)==PT_MIDI_SEND_FAILED && m.ports[1].needs_flush);
    packet(before,1,3,0xb1,64,0);assert(m.ports[1].sustain==0);
    log.fail=0;assert(pt_midi_note_on(&m,0,60,100)==PT_MIDI_OK && !m.ports[1].needs_flush);
    packet(before+1,1,3,0x81,127,0);packet(before+2,1,3,0x91,60,100);
    assert(pt_midi_program(&m,0,127)==PT_MIDI_OK);packet(log.count-1,1,2,0xc1,127,0);
    assert(pt_midi_bend(&m,0,8192)==PT_MIDI_OK);packet(log.count-1,1,3,0xe1,0,64);
    assert(pt_midi_bend(&m,0,16383)==PT_MIDI_OK);packet(log.count-1,1,3,0xe1,127,127);
    assert(pt_midi_realtime(&m,1,0xf8)==PT_MIDI_OK);packet(log.count-1,1,1,0xf8,0,0);
    before=log.count;
    assert(pt_midi_program(&m,0,128)==PT_MIDI_INVALID && pt_midi_bend(&m,0,16384)==PT_MIDI_INVALID);
    assert(pt_midi_controller(&m,0,123,0)==PT_MIDI_INVALID && pt_midi_realtime(&m,1,0xff)==PT_MIDI_INVALID);
    assert(pt_midi_note_on(&m,16,0,1)==PT_MIDI_INVALID && pt_midi_bind(&m,0,16,1)==PT_MIDI_INVALID);
    assert(pt_midi_bind(&m,0,0,0)==PT_MIDI_INVALID && pt_midi_note_on(&m,0,128,1)==PT_MIDI_INVALID && log.count==before);
    assert(pt_midi_note_on(&m,0,60,0)==PT_MIDI_OK && !m.voices[0].active);
    /* Pedal-only reconnect failure must remain pending even with no held notes. */
    assert(pt_midi_controller(&m,0,64,127)==PT_MIDI_OK);
    assert(pt_midi_connection(&m,1,0)==PT_MIDI_OK);log.fail=log.calls+1;
    assert(pt_midi_connection(&m,1,1)==PT_MIDI_SEND_FAILED && m.ports[1].needs_flush);
    log.fail=0;before=log.count;assert(pt_midi_connection(&m,1,1)==PT_MIDI_OK);
    packet(before,1,3,0xb1,64,0);assert(!m.ports[1].sustain && !m.ports[1].needs_flush);
    /* Repeated endpoint cycling and Stop leave no active notes or releases. */
    for(pass=0;pass<100;++pass) {
        for(i=0;i<16;++i) {
            assert(pt_midi_bind(&m,i,i%2,i+1)==PT_MIDI_OK);
            assert(pt_midi_note_on(&m,i,(pass+i)%128,80)==PT_MIDI_OK);
        }
        assert(pt_midi_connection(&m,0,0)==PT_MIDI_OK && pt_midi_connection(&m,0,1)==PT_MIDI_OK);
        assert(pt_midi_stop(&m)==PT_MIDI_OK);
        for(i=0;i<16;++i)assert(!m.voices[i].active && !m.ports[i].needs_flush && !m.ports[i].sustain);
    }
    assert(pt_midi_unbind(&m,0)==PT_MIDI_OK && !m.voices[0].bound);
    assert(pt_midi_note_on(&m,0,60,100)==PT_MIDI_INVALID);
    puts("MIDI PASS: exclusive track ownership, shared-note releases, replacement ordering, endpoint loss/reconnect, sustain cleanup, failed sends, controller/program/bend/clock bytes and 100 lifecycle cycles");return 0;
}
