#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "render_lookahead.h"
#include "midi.h"
#include "document.h"

struct wire {
    uint8_t bytes[16][3];
    unsigned count;
};

static int send_midi(void *context,unsigned endpoint,const uint8_t *bytes,size_t length)
{
    struct wire *wire=context;
    assert(endpoint==0 && length==3 && wire->count<16);
    memcpy(wire->bytes[wire->count++],bytes,length);
    return 1;
}

static void *allocate(void *context,size_t size)
{
    (void)context;
    return malloc(size);
}

static void release(void *context,void *address)
{
    (void)context;
    free(address);
}

static void consume(struct pt_render_sequence *sequence,uint32_t frames)
{
    while(frames) {
        uint32_t chunk=frames>73?73:frames;
        assert(pt_render_sequence_consume(sequence,chunk)==PT_RENDER_OK);
        frames-=chunk;
    }
}

static void exercise(struct pt_render_sequence *sequence,int forecast)
{
    static const uint64_t midi_frames[]={0,6080,10080,12480};
    static const uint8_t expected[][3]={
        {0x92,60,90},{0x82,60,0},{0x92,62,127},
        {0x82,62,0},{0x92,0,127},{0x82,0,0}
    };
    struct wire wire={0};
    struct pt_midi midi={0};
    struct pt_midi_sink sink={&wire,send_midi};
    struct pt_render_lookahead *ahead=calloc(1,sizeof(*ahead));
    struct pt_routed_render_plan *plan=calloc(1,sizeof(*plan));
    uint64_t elapsed=0;
    unsigned midi_events=0,triggers=0,steps=0;
    assert(ahead && plan && pt_midi_init(&midi,&sink));
    assert(pt_midi_connection(&midi,0,1)==PT_MIDI_OK);
    assert(pt_midi_bind(&midi,2,0,3)==PT_MIDI_OK);
    for(;;) {
        struct pt_render_interval interval;
        unsigned i,ready=0;
        assert(++steps<30 && pt_render_sequence_next(sequence,&interval)==PT_RENDER_OK);
        assert(pt_render_sequence_complete(sequence,&plan->audio)==PT_RENDER_ROUTE);
        if(interval.frames) {
            plan->midi_count=17;
            assert(pt_render_sequence_complete_routed(sequence,plan)==PT_RENDER_INVALID);
            assert(plan->midi_count==17);
        }
        if(forecast) {
            assert(pt_render_lookahead_begin(ahead,sequence)==PT_RENDER_OK);
            assert(pt_render_lookahead_step(ahead,73,&plan->audio,&ready)==PT_RENDER_INVALID);
            do {
                assert(pt_render_lookahead_step_routed(ahead,73,plan,&ready)==PT_RENDER_OK);
            } while(!ready);
            if(interval.frames)assert(pt_render_lookahead_commit(ahead)==PT_RENDER_INVALID);
            consume(sequence,interval.frames);
            assert(pt_render_lookahead_commit(ahead)==PT_RENDER_OK);
            assert(pt_render_lookahead_commit(ahead)==PT_RENDER_INVALID);
        } else {
            consume(sequence,interval.frames);
            assert(pt_render_sequence_complete_routed(sequence,plan)==PT_RENDER_OK);
        }
        elapsed+=interval.frames;
        assert(plan->frame==elapsed && plan->end==interval.end);
        if(plan->end) {
            assert(plan->frame==14880 && plan->end==PT_RENDER_F00+1);
            assert(!plan->audio.count && !plan->midi_count);
            assert(pt_midi_stop(&midi)==PT_MIDI_OK);
            break;
        }
        for(i=0;i<plan->audio.count;++i) {
            const struct pt_render_action *action=&plan->audio.action[i];
            assert(action->channel<2);
            if(action->kind==PT_RENDER_TRIGGER || action->kind==PT_RENDER_SEGMENT) {
                assert(plan->frame==0);
                triggers|=1U<<action->channel;
            }
        }
        for(i=0;i<plan->midi_count;++i) {
            const struct pt_routed_midi_action *action=&plan->midi[i];
            assert(midi_events<4 && midi_frames[midi_events++]==plan->frame);
            assert(action->track==2);
            if(action->kind==PT_NOTE_OFF)assert(pt_midi_note_off(&midi,action->track)==PT_MIDI_OK);
            else {
                assert(action->kind==PT_NOTE_MIDI);
                assert(pt_midi_note_on(&midi,action->track,action->note,action->velocity)==PT_MIDI_OK);
            }
        }
    }
    assert(triggers==3U && midi_events==4 && wire.count==sizeof(expected)/3);
    assert(!memcmp(wire.bytes,expected,sizeof(expected)));
    free(ahead);
    free(plan);
}

int main(void)
{
    struct pt_project project={0};
    int32_t pcm[]={17,90,-42,24,7,-5,27,10};
    struct pt_sample sample={0};
    struct pt_event event[PT_PROJECT_ROWS*4]={0};
    uint16_t order=0;
    struct pt_allocator allocator={NULL,allocate,release};
    struct pt_render_options options={0};
    struct pt_render_sequence *sequence=NULL;
    unsigned ready=0;

    pt_channels_init(&project.channels);
    assert(pt_channels_route(&project.channels,1,PT_AMIGUS)==PT_CHANNEL_OK);
    assert(pt_channels_route(&project.channels,2,PT_MIDI)==PT_CHANNEL_OK);
    assert(pt_channels_route(&project.channels,3,PT_MIDI)==PT_CHANNEL_OK);
    project.channels.track[3].muted=1;
    strcpy(project.midi_output[2],"synth.out");
    strcpy(project.midi_output[3],"mute.out");
    project.orders=&order;
    project.events=event;
    project.samples=&sample;
    project.order_count=project.pattern_count=project.sample_count=1;
    project.speed=3;project.bpm=125;
    sample.pcm=(struct pt_pcm){pcm,8,8,48000,1,8};
    sample.volume=64;
    sample.loop=PT_LOOP_FORWARD;
    sample.loop_end=8;
    event[0]=(struct pt_event){.kind=PT_NOTE_PERIOD,.pitch=428,.instrument=1};
    event[1]=event[0];
    event[2]=(struct pt_event){.kind=PT_NOTE_MIDI,.pitch=60,.instrument=1,.velocity=90,.flags=1};
    event[3]=event[2];
    event[4]=(struct pt_event){.effect=15,.parameter=150};
    event[8]=(struct pt_event){.effect=14,.parameter=0xe1};
    event[10]=(struct pt_event){.kind=PT_NOTE_MIDI,.pitch=62,.effect=14,.parameter=0xd1};
    event[14]=(struct pt_event){.kind=PT_NOTE_OFF,.instrument=1};
    event[18]=(struct pt_event){.kind=PT_NOTE_MIDI,.pitch=0};
    event[20]=(struct pt_event){.effect=15}; /* F00 */
    event[22]=(struct pt_event){.kind=PT_NOTE_MIDI,.pitch=64}; /* must not sound */
    options.rate=48000;options.bits=16;options.tracks=15;options.gain_q16=65536;
    options.tick_limit=128;options.frame_limit=100000;

    assert(pt_render_sequence_begin(&project,&options,&allocator,&sequence)==PT_RENDER_ROUTE && !sequence);
    event[6].effect=9;
    assert(pt_render_sequence_begin_routed(&project,&options,&allocator,&sequence)==PT_RENDER_EFFECT && !sequence);
    event[6].effect=0;
    project.midi_flags=1;
    assert(pt_render_sequence_begin_routed(&project,&options,&allocator,&sequence)==PT_RENDER_EFFECT && !sequence);
    project.midi_flags=0;
    assert(pt_render_sequence_begin_routed(&project,&options,&allocator,&sequence)==PT_RENDER_OK);
    do {assert(pt_render_sequence_prepare(sequence,2,&ready)==PT_RENDER_OK);}while(!ready);
    exercise(sequence,0);
    assert(pt_render_sequence_rewind(sequence)==PT_RENDER_OK);
    exercise(sequence,1);
    pt_render_sequence_close(sequence);
    puts("Routed sequence: same Paula/AmiGUS attack frame, MIDI delay after Fxx, OFF, mute, F00 cleanup and replay via lookahead");
    return 0;
}
