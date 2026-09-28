#include <assert.h>
#include <stdio.h>
#include "paula_playback.h"
int main(void)
{
    struct pt_project p={0},copy,before;struct pt_mod_export_report report;
    struct pt_sample sample={0};struct pt_event events[256]={0};uint16_t order=0;
    int32_t pcm[4]={-128,0,1,127};
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
    assert(!pt_paula_playback_snapshot(&p,&copy,&report));
    assert(report.issues==PT_EXPORT_PRECISION && report.bytes && report.classification==PT_CONVERSION_CONVERTED);
    {uint8_t mod[2112];size_t n=0;
        assert(pt_mod_export_round8(&copy,mod,sizeof(mod),&n)==PT_PROJECT_OK && n==sizeof(mod));
        assert(mod[2108]==128 && mod[2109]==0 && mod[2110]==0 && mod[2111]==127);
        assert(pt_mod_export_direct(&copy,mod,sizeof(mod),&n)==PT_PROJECT_UNSUPPORTED);
    }
    assert(sample.pcm.bits==24 && pcm[0]==-8388607 && pcm[3]==8388607);
    assert(!memcmp(&p,&before,sizeof(p)));
    sample.pcm.bits=16;pcm[0]=-32768;pcm[3]=32767;
    assert(!pt_paula_playback_snapshot(&p,&copy,&report) && report.issues==PT_EXPORT_PRECISION);
    /* Precision permission never hides another unsupported attribute. */
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
    sample.pcm.bits=8;pcm[0]=-128;pcm[3]=127;
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
    {struct pt_event packed[256]={0},padded[256],saved[256];unsigned channels,row,ch;
        p.mode=PT_MODE_WAVETABLE;pt_channels_init(&p.channels);p.events=packed;
        for(channels=1;channels<4;++channels) {
            pt_channels_init(&p.channels);memset(p.midi_output,0,sizeof(p.midi_output));
            p.channels.count=(uint8_t)channels;
            p.channels.track[channels].route=PT_MIDI;strcpy(p.midi_output[channels],"INACTIVE ENDPOINT");
            for(row=0;row<64;++row)for(ch=0;ch<channels;++ch) {
                packed[row*channels+ch].kind=PT_NOTE_PERIOD;
                packed[row*channels+ch].pitch=(uint16_t)(428+ch);
                packed[row*channels+ch].instrument=1;
            }
            before=p;memcpy(saved,packed,sizeof(packed));
            assert(pt_paula_playback_prepare(&p,&copy,&report,padded,255));
            assert(!pt_paula_playback_prepare(&p,&copy,&report,padded,256));
            assert(copy.channels.count==4 && copy.events==padded && report.bytes);
            for(row=0;row<64;++row)for(ch=0;ch<4;++ch) {
                struct pt_event silence={0};
                assert(!memcmp(padded+row*4+ch,ch<channels?packed+row*channels+ch:&silence,sizeof(silence)));
            }
            assert(!memcmp(&before,&p,sizeof(p)) && !memcmp(saved,packed,sizeof(packed)));
            p.channels.track[0].route=PT_MIDI;
            assert(strstr(pt_paula_playback_prepare(&p,&copy,&report,padded,256),"MIDI IS NOT RENDERED"));
            p.channels.track[0].route=PT_PAULA;
        }
    }
    puts("PAULA PLAYBACK PREFLIGHT PASS: private metadata, optional8-bit playback copies, unchanged16/24-bit masters, explicit unavailable routes");
    return 0;
}
