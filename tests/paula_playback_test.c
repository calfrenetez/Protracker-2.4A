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
    sample.pcm.frames=3;
    assert(!pt_paula_playback_snapshot(&p,&copy,&report));
    assert(report.issues==(PT_EXPORT_PRECISION|PT_EXPORT_PADDING) && report.bytes==2112);
    {uint8_t mod[2112];size_t n=0;
        assert(pt_mod_playback_encode(&copy,mod,sizeof(mod),&n)==PT_PROJECT_OK && n==sizeof(mod));
        assert(mod[43]==2 && mod[2111]==0 && sample.pcm.frames==3 && pcm[3]==8388607);
        assert(pt_mod_export_round8(&copy,mod,sizeof(mod),&n)==PT_PROJECT_UNSUPPORTED);
    }
    sample.pcm.frames=4;
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
    {struct pt_paula_samples work;struct pt_project selected;
        struct pt_sample slots[255];struct pt_event stored[512]={0};
        uint8_t header[1084]={0},saved_header[1084],mod[3200];
        struct pt_extension ext={PT_CLASSIC_HEADER_TAG,1084,1,header};
        size_t n;unsigned j;
        memset(&p,0,sizeof(p));pt_channels_init(&p.channels);
        p.speed=6;p.bpm=125;p.orders=&order;p.order_count=1;
        p.pattern_count=2;p.events=stored;p.samples=slots;p.sample_count=255;
        for(j=0;j<255;++j)slots[j]=sample;
        /* Unused stereo/high-rate masters, even above31, are never converted. */
        slots[1].pcm.channels=2;slots[1].pcm.frames=2;slots[1].pcm.rate=48000;
        slots[254]=slots[1];stored[0].instrument=1;
        stored[511].instrument=31; /* instrument-only in an unordered pattern */
        before=p;
        assert(!pt_paula_playback_samples(&p,&selected,&work,NULL,0));
        assert(selected.sample_count==31 && selected.samples[0].pcm.data==pcm);
        assert(selected.samples[1].pcm.frames==0 && selected.samples[30].pcm.data==pcm);
        assert(!pt_paula_playback_prepare(&selected,&copy,&report,NULL,0));
        assert(report.bytes==1084+2048+8);
        assert(pt_mod_export_round8(&copy,mod,sizeof(mod),&n)==PT_PROJECT_OK && n==report.bytes);
        assert(!memcmp(&p,&before,sizeof(p)) && slots[1].pcm.channels==2 && slots[254].pcm.rate==48000);
        stored[1].instrument=2;
        assert(!pt_paula_playback_samples(&p,&selected,&work,NULL,0));
        assert(strstr(pt_paula_playback_snapshot(&selected,&copy,&report),"STEREO"));
        stored[1].instrument=32;
        assert(strstr(pt_paula_playback_samples(&p,&selected,&work,NULL,0),"WORKSPACE"));
        stored[1].instrument=0;
        /* Empty selection has no payload, and malformed unused PCM is refused. */
        memset(stored,0,sizeof(stored));
        assert(!pt_paula_playback_samples(&p,&selected,&work,NULL,0) && selected.sample_count==0);
        assert(!pt_paula_playback_snapshot(&selected,&copy,&report) && report.bytes==1084+2048);
        slots[254].pcm.bits=7;assert(pt_paula_playback_samples(&p,&selected,&work,NULL,0));slots[254].pcm.bits=8;
        header[950]=1;memcpy(header+1080,"M.K.",4);
        header[20+30+27]=1;header[20+30+29]=1; /* valid one-word start for4frames */
        slots[1]=sample;p.extensions=&ext;p.extension_count=1;
        memcpy(saved_header,header,sizeof(header));
        assert(!pt_paula_playback_samples(&p,&selected,&work,NULL,0));
        assert(!pt_paula_playback_snapshot(&selected,&copy,&report));
        assert(work.header[20+30+27]==0 && work.header[20+30+29]==1);
        assert(!memcmp(header,saved_header,sizeof(header)));
        header[20+30+27]=2;
        assert(strstr(pt_paula_playback_samples(&p,&selected,&work,NULL,0),"LOOP NOT PAULA"));
        header[20+30+27]=0;ext.id=0x41424344;
        assert(!pt_paula_playback_samples(&p,&selected,&work,NULL,0));
        assert(pt_paula_playback_snapshot(&selected,&copy,&report));
    }
    {struct pt_paula_samples work;struct pt_project selected;
        struct pt_sample slots[255];struct pt_event input[256]={0},mapped[256],saved[256];
        unsigned j,channels;uint8_t identities[31];
        memset(&p,0,sizeof(p));pt_channels_init(&p.channels);p.speed=6;p.bpm=125;
        p.orders=&order;p.order_count=p.pattern_count=1;p.samples=slots;p.sample_count=255;p.events=input;
        for(j=0;j<255;++j)slots[j]=sample;
        for(channels=1;channels<=4;++channels) {
            p.channels.count=(uint8_t)channels;
            memset(input,0,sizeof(input));
            input[0].instrument=1;input[channels].instrument=31;
            input[channels*2].instrument=32;input[channels*3].instrument=255;
            memcpy(saved,input,sizeof(input));
            assert(pt_paula_playback_event_count(&p)==256);
            assert(pt_paula_playback_samples(&p,&selected,&work,mapped,255));
            assert(!pt_paula_playback_samples(&p,&selected,&work,mapped,256));
            assert(selected.channels.count==4 && selected.events==mapped && selected.sample_count==31);
            assert(mapped[0].instrument==1 && mapped[4].instrument==31 && mapped[8].instrument==2 && mapped[12].instrument==3);
            assert(work.source[0]==1 && work.source[1]==32 && work.source[2]==255 && work.source[30]==31);
            assert(work.samples[1].pcm.data==slots[31].pcm.data);
            for(j=0;j<64;++j) {unsigned ch;for(ch=channels;ch<4;++ch)assert(!mapped[j*4+ch].instrument);}
            assert(!pt_paula_playback_prepare(&selected,&copy,&report,mapped,256));
            assert(!memcmp(input,saved,sizeof(input)));
            memcpy(identities,work.source,31);input[channels*3].instrument=254;
            assert(!pt_paula_playback_samples(&p,&selected,&work,mapped,256));
            assert(mapped[12].instrument==3 && memcmp(identities,work.source,31));
            assert(!memcmp(work.samples[2].pcm.data,slots[254].pcm.data,4*sizeof(int32_t)));
        }
        memset(input,0,sizeof(input));p.channels.count=4;
        for(j=0;j<31;++j)input[j].instrument=(uint8_t)(225+j);
        assert(!pt_paula_playback_samples(&p,&selected,&work,mapped,256));
        assert(selected.sample_count==31 && work.source[0]==225 && work.source[30]==255);
        input[31].instrument=1;
        assert(strstr(pt_paula_playback_samples(&p,&selected,&work,mapped,256),"MORE THAN 31"));
    }
    puts("PAULA PLAYBACK PREFLIGHT PASS: private metadata, optional8-bit playback copies, unchanged16/24-bit masters, explicit unavailable routes");
    return 0;
}
