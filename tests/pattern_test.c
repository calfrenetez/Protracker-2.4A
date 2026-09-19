#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "pattern.h"
static struct pt_event events[2048],snapshot[2048],clip_events[1024];
static struct pt_event_update updates[1024];
static struct pt_event_change changes[32];
static struct pt_pattern_command commands[3];
int main(void)
{
    struct pt_project p;struct pt_pattern_history h,before;struct pt_block block={clip_events,1024,0,0};unsigned i;
    memset(&p,0,sizeof(p));pt_channels_init(&p.channels);assert(pt_channels_resize(&p.channels,16)==PT_CHANNEL_OK);
    p.events=events;p.pattern_count=2;p.order_count=1;p.speed=6;p.bpm=125;
    assert(pt_pattern_history_init(&h,&p,commands,3,changes,8)==PT_EDIT_OK);
    memset(updates,0,sizeof(updates));updates[0].index=0;updates[0].event.kind=PT_NOTE_PERIOD;updates[0].event.pitch=428;
    updates[0].event.effect=0;updates[0].event.parameter=7;
    updates[1].index=15;updates[1].event.kind=PT_NOTE_MIDI;updates[1].event.pitch=60;
    updates[2].index=31;updates[2].event.kind=PT_NOTE_OFF;
    assert(pt_pattern_apply(&p,&h,updates,3)==PT_EDIT_OK && h.used==3 && pt_pattern_dirty(&h));
    pt_pattern_mark_saved(&h);assert(!pt_pattern_dirty(&h));
    assert(pt_pattern_transpose(&p,&h,0,0,2,0,16,12,updates,1024)==PT_EDIT_OK);
    assert(events[0].pitch==214 && events[15].pitch==72 && events[31].kind==PT_NOTE_OFF && events[0].parameter==7);
    assert(pt_pattern_undo(&p,&h,-1)==PT_EDIT_OK && events[0].pitch==428 && !pt_pattern_dirty(&h));
    before=h;events[0].pitch=300;
    assert(pt_pattern_undo(&p,&h,1)==PT_EDIT_CONFLICT && !memcmp(&h,&before,sizeof(h)));events[0].pitch=428;
    assert(pt_pattern_undo(&p,&h,1)==PT_EDIT_OK && events[0].pitch==214);
    memcpy(snapshot,events,sizeof(events));before=h;
    assert(pt_pattern_transpose(&p,&h,0,0,2,0,16,127,updates,1024)==PT_EDIT_UNSUPPORTED);
    assert(!memcmp(snapshot,events,sizeof(events)) && !memcmp(&h,&before,sizeof(h)));
    updates[0].index=0;updates[0].event=events[0];updates[1]=updates[0];
    assert(pt_pattern_apply(&p,&h,updates,2)==PT_EDIT_INVALID && !memcmp(&h,&before,sizeof(h)));
    for(i=0;i<9;++i) {memset(&updates[i],0,sizeof(updates[i]));updates[i].index=40+i;updates[i].event.kind=PT_NOTE_MIDI;updates[i].event.pitch=(uint16_t)(60+i);}
    assert(pt_pattern_apply(&p,&h,updates,9)==PT_EDIT_CAPACITY && !memcmp(snapshot,events,sizeof(events)) && !memcmp(&h,&before,sizeof(h)));
    assert(pt_pattern_copy(&p,0,0,2,0,16,&block)==PT_EDIT_OK && block.rows==2 && block.channels==16);
    assert(pt_pattern_paste(&p,&h,1,0,0,&block,updates,1024)==PT_EDIT_OK && events[1024].pitch==214);
    assert(pt_pattern_undo(&p,&h,-1)==PT_EDIT_OK && events[1024].kind==PT_NOTE_NONE);
    /* A no-op does not discard redo or change the clean/dirty revision. */
    before=h;updates[0].index=0;updates[0].event=events[0];
    assert(pt_pattern_apply(&p,&h,updates,1)==PT_EDIT_OK && !memcmp(&h,&before,sizeof(h)));
    updates[0].event.pitch=202;assert(pt_pattern_apply(&p,&h,updates,1)==PT_EDIT_OK);
    assert(pt_pattern_undo(&p,&h,1)==PT_EDIT_END);
    assert(pt_pattern_paste(&p,&h,1,63,0,&block,updates,1024)==PT_EDIT_INVALID);
    block.events[0].instrument=1;before=h;memcpy(snapshot,events,sizeof(events));
    assert(pt_pattern_paste(&p,&h,1,0,0,&block,updates,1024)==PT_EDIT_INVALID);
    assert(!memcmp(snapshot,events,sizeof(events)) && !memcmp(&h,&before,sizeof(h)));
    assert(pt_pattern_clone(&p,&h,0,1,updates,1024)==PT_EDIT_OK && !memcmp(events,events+1024,1024*sizeof(*events)));
    assert(pt_pattern_undo(&p,&h,-1)==PT_EDIT_OK && events[1024].kind==PT_NOTE_NONE);
    for(i=0;i<10;++i) {
        memset(updates,0,sizeof(*updates));updates[0].index=63;updates[0].event.kind=PT_NOTE_MIDI;updates[0].event.pitch=(uint16_t)(30+i);
        assert(pt_pattern_apply(&p,&h,updates,1)==PT_EDIT_OK);
    }
    assert(h.count==3 && h.used==3);
    for(i=0;i<3;++i)assert(pt_pattern_undo(&p,&h,-1)==PT_EDIT_OK);
    assert(pt_pattern_undo(&p,&h,-1)==PT_EDIT_END && events[63].pitch==36);
    assert(pt_pattern_copy(&p,0,0,1,0,1,&block)==PT_EDIT_OK);
    block.events=events;assert(pt_pattern_copy(&p,0,0,1,0,1,&block)==PT_EDIT_ALIAS);
    /* Metadata shares chronological history with notes, without spending event slots. */
    pt_channels_init(&p.channels);assert(pt_channels_resize(&p.channels,16)==PT_CHANNEL_OK);
    assert(pt_pattern_history_init(&h,&p,commands,3,changes,8)==PT_EDIT_OK);
    {
        struct pt_channel v=p.channels.track[0];struct pt_channels channel_snapshot;
        v.route=PT_MIDI;
        assert(pt_pattern_channel_apply(&p,&h,0,&v)==PT_EDIT_OK && h.used==0);
        updates[0].index=63;updates[0].event=events[63];updates[0].event.pitch=70;
        assert(pt_pattern_apply(&p,&h,updates,1)==PT_EDIT_OK);
        v.muted=1;v.solo=1;
        assert(pt_pattern_channel_apply(&p,&h,0,&v)==PT_EDIT_OK && h.used==1);
        pt_pattern_mark_saved(&h);p.channels.selected=15;
        assert(pt_pattern_undo(&p,&h,-1)==PT_EDIT_OK && !p.channels.track[0].muted && pt_pattern_dirty(&h));
        assert(pt_pattern_undo(&p,&h,-1)==PT_EDIT_OK && events[63].pitch==36);
        assert(pt_pattern_undo(&p,&h,-1)==PT_EDIT_OK && p.channels.track[0].route==PT_PAULA);
        assert(p.channels.selected==15);
        before=h;channel_snapshot=p.channels;v=p.channels.track[4];v.route=PT_PAULA;
        assert(pt_pattern_channel_apply(&p,&h,4,&v)==PT_EDIT_PAULA_LIMIT);
        assert(!memcmp(&h,&before,sizeof(h)) && !memcmp(&p.channels,&channel_snapshot,sizeof(channel_snapshot)));
        assert(pt_pattern_channel_apply(&p,&h,0,&p.channels.track[0])==PT_EDIT_OK && !memcmp(&h,&before,sizeof(h)));
        for(i=0;i<3;++i)assert(pt_pattern_undo(&p,&h,1)==PT_EDIT_OK);
        assert(!pt_pattern_dirty(&h) && p.channels.track[0].muted && events[63].pitch==70);
        /* Conflicts do not advance history or alter unrelated channels. */
        before=h;p.channels.track[0].muted=0;
        assert(pt_pattern_undo(&p,&h,-1)==PT_EDIT_CONFLICT && !memcmp(&h,&before,sizeof(h)));
        p.channels.track[0].muted=1;
        for(i=0;i<10;++i) {
            v=p.channels.track[0];v.muted^=1;
            assert(pt_pattern_channel_apply(&p,&h,0,&v)==PT_EDIT_OK);
        }
        assert(h.count==3 && !h.used);
        for(i=0;i<3;++i)assert(pt_pattern_undo(&p,&h,-1)==PT_EDIT_OK);
        assert(pt_pattern_undo(&p,&h,-1)==PT_EDIT_END);
        /* Undo cannot make a fifth Paula route after an outside edit. */
        assert(pt_pattern_history_init(&h,&p,commands,3,changes,8)==PT_EDIT_OK);
        v=p.channels.track[1];v.route=PT_AMIGUS;
        assert(pt_pattern_channel_apply(&p,&h,1,&v)==PT_EDIT_OK);
        p.channels.track[4].route=PT_PAULA;p.channels.track[5].route=PT_PAULA;
        before=h;assert(pt_pattern_undo(&p,&h,-1)==PT_EDIT_CONFLICT && !memcmp(&h,&before,sizeof(h)));
        p.channels.count=15;assert(pt_pattern_undo(&p,&h,-1)==PT_EDIT_INVALID);p.channels.count=16;
    }
    p.events=snapshot;assert(pt_pattern_undo(&p,&h,1)==PT_EDIT_INVALID);
    puts("PATTERN PASS: 16-channel block edits, exact period/MIDI transpose, OFF retention, clone, bounded undo/redo, dirty tracking, eviction and conflict/no-op safety");return 0;
}
