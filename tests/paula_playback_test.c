#include <assert.h>
#include <stdio.h>
#include "paula_playback.h"
int main(void)
{
    struct pt_project p={0},copy,before;struct pt_mod_export_report report;
    struct pt_sample sample={0};struct pt_event events[256]={0};uint16_t order=0;
    int32_t pcm[4]={-128,0,1,127};const char *error;
    pt_channels_init(&p.channels);p.bpm=125;p.speed=6;p.orders=&order;p.order_count=1;
    p.pattern_count=1;p.events=events;p.samples=&sample;p.sample_count=1;
    sample.pcm.data=pcm;sample.pcm.capacity=sample.pcm.frames=4;
    sample.pcm.channels=1;sample.pcm.bits=8;sample.pcm.rate=PT_CLASSIC_RATE;sample.volume=64;
    strcpy(p.title,"Title longer than twenty");
    p.channels.track[0].muted=1;p.channels.track[1].solo=1;
    strcpy(p.channels.track[0].name,"BASS");p.channels.track[0].group=3;
    before=p;
    assert(!pt_paula_playback_snapshot(&p,&copy,&report));
    assert(!report.issues && report.bytes && !memcmp(&p,&before,sizeof(p)));
    assert(copy.samples==p.samples && copy.events==p.events);
    assert(!copy.channels.track[0].muted && !copy.channels.track[1].solo);
    assert(pt_mod_export_analyse(&p,&report)==PT_PROJECT_OK && (report.issues&PT_EXPORT_METADATA));
    sample.pcm.bits=24;pcm[0]=-8388607;pcm[3]=8388607;
    error=pt_paula_playback_snapshot(&p,&copy,&report);
    assert(error && strstr(error,"HIGH-RES MASTER"));
    assert(sample.pcm.bits==24 && pcm[0]==-8388607 && pcm[3]==8388607);
    assert(!memcmp(&p,&before,sizeof(p)));
    sample.pcm.bits=8;pcm[0]=-128;pcm[3]=127;
    sample.pcm.channels=2;sample.pcm.frames=2;
    assert(strstr(pt_paula_playback_snapshot(&p,&copy,&report),"STEREO MASTER"));
    sample.pcm.channels=1;sample.pcm.frames=4;sample.pcm.rate=48000;
    assert(strstr(pt_paula_playback_snapshot(&p,&copy,&report),"SAMPLE RATE"));
    sample.pcm.rate=PT_CLASSIC_RATE;
    {uint32_t slice=0;sample.slices=&slice;sample.slice_count=1;
        assert(strstr(pt_paula_playback_snapshot(&p,&copy,&report),"SLICES REQUIRE"));
        sample.slice_count=0;sample.slices=NULL;}
    sample.loop=PT_LOOP_PINGPONG;sample.loop_end=4;
    assert(strstr(pt_paula_playback_snapshot(&p,&copy,&report),"LOOP NOT PAULA"));
    sample.loop=PT_LOOP_NONE;sample.loop_end=0;
    assert(!pt_paula_playback_snapshot(&p,&copy,&report));
    p.channels.track[0].route=PT_AMIGUS;
    assert(strstr(pt_paula_playback_snapshot(&p,&copy,&report),"AMIGUS PLAYBACK NOT AVAILABLE"));
    p.channels.track[1].route=PT_MIDI;
    assert(strstr(pt_paula_playback_snapshot(&p,&copy,&report),"AMIGUS AND MIDI"));
    p.channels.track[0].route=PT_PAULA;
    assert(strstr(pt_paula_playback_snapshot(&p,&copy,&report),"MIDI IS NOT RENDERED"));
    p.mode=PT_MODE_STUDIO;
    assert(strstr(pt_paula_playback_snapshot(&p,&copy,&report),"STUDIO PLAYBACK NOT AVAILABLE"));
    before=p;assert(pt_paula_playback_snapshot(&p,&p,&report));
    assert(!memcmp(&p,&before,sizeof(p)));
    assert(pt_paula_playback_snapshot(NULL,&copy,&report));
    puts("PAULA PLAYBACK PREFLIGHT PASS: private metadata, unchanged 24-bit masters, explicit unavailable routes");
    return 0;
}
