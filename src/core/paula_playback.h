#ifndef PT_PAULA_PLAYBACK_H
#define PT_PAULA_PLAYBACK_H
#include <string.h>
#include "mod_project.h"
/* Fixed selected-sample view. No PCM ownership is transferred. Output and
 * workspace are disjoint from the stable source. All stored patterns count,
 * including instrument-only events. Preserve selected low-slot identities and
 * map high source slots into otherwise unused replay slots, at most31 total.
 * The caller retains source[] as the identity key for live comparison. */
struct pt_paula_samples {
    struct pt_sample samples[31];
    struct pt_extension extension;
    uint8_t header[1084],map[256],source[31];
};
/* Call only after project validation. Padding and high-slot remapping share
 * one bounded four-track event workspace. Ordinary four-track MODs need none. */
static inline size_t pt_paula_playback_event_count(const struct pt_project *p)
{
    size_t n,count=(size_t)p->pattern_count*PT_PROJECT_ROWS*p->channels.count;
    if(p->channels.count<4)return (size_t)p->pattern_count*PT_PROJECT_ROWS*4;
    for(n=0;n<count;++n)if(p->events[n].instrument>31)
        return (size_t)p->pattern_count*PT_PROJECT_ROWS*4;
    return 0;
}
static inline const char *pt_paula_playback_samples(const struct pt_project *p,
    struct pt_project *copy,struct pt_paula_samples *work,
    struct pt_event *events,size_t capacity)
{
    size_t count,n,rows;unsigned i,highest=0,slot,remapped=0;
    if(!p || !copy || copy==p || !work || pt_project_validate(p,NULL)!=PT_PROJECT_OK)
        return "PLAY: INVALID PROJECT";
    count=(size_t)p->pattern_count*PT_PROJECT_ROWS*p->channels.count;
    memset(work,0,sizeof(*work));
    for(n=0;n<count;++n)work->map[p->events[n].instrument]=1;
    work->map[0]=0;
    for(i=1;i<=31;++i)if(work->map[i]) {
        work->map[i]=(uint8_t)i;work->source[i-1]=(uint8_t)i;highest=i;
    }
    for(i=32;i<=255;++i)if(work->map[i]) {
        for(slot=0;slot<31 && work->source[slot];++slot) {}
        if(slot==31)return "PLAY: MORE THAN 31 REFERENCED SAMPLES";
        work->source[slot]=(uint8_t)i;work->map[i]=(uint8_t)(slot+1);
        if(slot+1>highest)highest=slot+1;
        remapped=1;
    }
    *copy=*p;copy->samples=work->samples;copy->sample_count=(uint16_t)highest;
    for(i=0;i<highest;++i) {
        if(work->source[i])work->samples[i]=p->samples[work->source[i]-1];
        else {
            work->samples[i].pcm.bits=8;work->samples[i].pcm.channels=1;
            work->samples[i].pcm.rate=PT_CLASSIC_RATE;
        }
    }
    if(p->extension_count==1 && p->extensions[0].id==PT_CLASSIC_HEADER_TAG &&
       p->extensions[0].version==1 && p->extensions[0].length==1084) {
        memcpy(work->header,p->extensions[0].data,1084);
        for(i=0;i<31;++i) {
            uint8_t *h=work->header+20+i*30;
            unsigned start=((unsigned)h[26]<<8)|h[27],repeat=((unsigned)h[28]<<8)|h[29];
            /* Retain refusal of unsafe original DMA metadata, even for unused
             * masters. High-slot masters have no preserved classic header. */
            if(i<p->sample_count && p->samples[i].loop==PT_LOOP_NONE &&
               repeat<=1 && start && start+1>p->samples[i].pcm.frames/2)
                return "PLAY: LOOP NOT PAULA COMPATIBLE - USE RENDER";
            if(work->source[i]!=i+1) {memset(h,0,30);h[29]=1;}
        }
        work->extension=p->extensions[0];work->extension.data=work->header;
        copy->extensions=&work->extension;
    }
    if(remapped) {
        if(p->channels.count>4)return "PLAY: REQUIRES AT MOST FOUR PAULA CHANNELS";
        rows=(size_t)p->pattern_count*PT_PROJECT_ROWS;
        if(!events || capacity<rows*4)return "PLAY: OUT OF REPLAY WORKSPACE MEMORY";
        memset(events,0,rows*4*sizeof(*events));
        for(n=0;n<rows;++n)for(i=0;i<p->channels.count;++i) {
            struct pt_event *e=events+n*4+i;*e=p->events[n*p->channels.count+i];
            e->instrument=work->map[e->instrument];
        }
        copy->events=events;pt_channels_init(&copy->channels);
        copy->channels.selected=p->channels.selected;
        for(i=0;i<p->channels.count;++i)copy->channels.track[i]=p->channels.track[i];
        for(i=p->channels.count;i<4;++i)memset(copy->midi_output[i],0,PT_MIDI_ENDPOINT);
    }
    return NULL;
}
/* Private shallow replay snapshot: sample/event masters are read-only and never
 * converted. Caller owns distinct snapshot/report storage. Zero means eligible,
 * not allocated/playing. Mute/solo are applied separately by the native output.
 * Paula derives a private rounded8-bit copy for mono16/24-bit masters. All
 * odd sample lengths are silently padded in that copy; loop and all other
 * classic constraints remain. This policy never changes saved masters. */
static inline const char *pt_paula_playback_snapshot(const struct pt_project *p,
    struct pt_project *copy,struct pt_mod_export_report *report)
{
    unsigned i, routes=0;
    if(!p || !copy || copy==p || !report || pt_project_validate(p,NULL)!=PT_PROJECT_OK)
        return "PLAY: INVALID PROJECT";
    *copy=*p;copy->title[20]=0;
    for(i=0;i<copy->channels.count;++i) {
        routes|=copy->channels.track[i].route;
        copy->channels.track[i].muted=0;copy->channels.track[i].solo=0;
        copy->channels.track[i].group=0;copy->channels.track[i].midi_channel=(uint8_t)(i+1);
        memset(copy->channels.track[i].name,0,sizeof(copy->channels.track[i].name));
    }
    if(pt_mod_playback_analyse(copy,report)!=PT_PROJECT_OK)return "PLAY: INVALID PROJECT";
    if(p->mode==PT_MODE_STUDIO)return "STUDIO PLAYBACK NOT AVAILABLE - USE RENDER WAV";
    if((routes&(PT_AMIGUS|PT_MIDI))==(PT_AMIGUS|PT_MIDI))return "AMIGUS AND MIDI PLAYBACK NOT AVAILABLE";
    if(routes&PT_AMIGUS)return "AMIGUS PLAYBACK NOT AVAILABLE - USE RENDER WAV";
    if(routes&PT_MIDI)return "MIDI PLAYBACK NOT AVAILABLE - MIDI IS NOT RENDERED";
    if(report->issues&PT_EXPORT_STEREO)return "PLAY: STEREO MASTER - USE SAMPLE PREVIEW OR RENDER";
    if(report->issues&PT_EXPORT_RATE)return "PLAY: SAMPLE RATE - USE SAMPLE PREVIEW OR RENDER";
    if(report->issues&PT_EXPORT_SLICES)return "PLAY: SLICES REQUIRE ENHANCED PLAYBACK - USE RENDER";
    if(report->issues&PT_EXPORT_LOOPS)return "PLAY: LOOP NOT PAULA COMPATIBLE - USE RENDER";
    if(report->issues&~(PT_EXPORT_PRECISION|PT_EXPORT_PADDING))return "PLAY: REQUIRES CLASSIC FOUR-CHANNEL PAULA PROJECT";
    return NULL;
}
/* Expand one-to-three Paula tracks into a private four-track replay view.
 * Caller workspace is disjoint from every source object and copy/report; its
 * capacity counts events. Failure may change workspace, never the source.
 * Four-track input needs no workspace. No non-Paula track is discarded. */
static inline const char *pt_paula_playback_prepare(const struct pt_project *p,
    struct pt_project *copy,struct pt_mod_export_report *report,
    struct pt_event *events,size_t capacity)
{
    struct pt_project padded;size_t rows,row;unsigned ch;
    if(p && p->channels.count>=4)return pt_paula_playback_snapshot(p,copy,report);
    if(!p || !copy || copy==p || !report || pt_project_validate(p,NULL)!=PT_PROJECT_OK)
        return "PLAY: INVALID PROJECT";
    rows=(size_t)p->pattern_count*PT_PROJECT_ROWS;
    if(!events || capacity<rows*4)return "PLAY: OUT OF REPLAY WORKSPACE MEMORY";
    padded=*p;pt_channels_init(&padded.channels);
    padded.channels.selected=p->channels.selected;
    for(ch=0;ch<p->channels.count;++ch)padded.channels.track[ch]=p->channels.track[ch];
    for(ch=p->channels.count;ch<4;++ch)memset(padded.midi_output[ch],0,PT_MIDI_ENDPOINT);
    memset(events,0,rows*4*sizeof(*events));
    for(row=0;row<rows;++row)
        memcpy(events+row*4,p->events+row*p->channels.count,p->channels.count*sizeof(*events));
    padded.events=events;
    return pt_paula_playback_snapshot(&padded,copy,report);
}
#endif
