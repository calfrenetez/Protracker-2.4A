#ifndef PT_PAULA_PLAYBACK_H
#define PT_PAULA_PLAYBACK_H
#include <string.h>
#include "mod_project.h"
/* Private shallow replay snapshot: sample/event masters are read-only and never
 * converted. Caller owns distinct snapshot/report storage. Zero means eligible,
 * not allocated/playing. Mute/solo are applied separately by the native output.
 * Paula derives a private rounded8-bit copy for mono16/24-bit masters. All
 * other classic constraints remain. This policy never changes saved masters. */
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
    if(pt_mod_export_analyse_round8(copy,report)!=PT_PROJECT_OK)return "PLAY: INVALID PROJECT";
    if(p->mode==PT_MODE_STUDIO)return "STUDIO PLAYBACK NOT AVAILABLE - USE RENDER WAV";
    if((routes&(PT_AMIGUS|PT_MIDI))==(PT_AMIGUS|PT_MIDI))return "AMIGUS AND MIDI PLAYBACK NOT AVAILABLE";
    if(routes&PT_AMIGUS)return "AMIGUS PLAYBACK NOT AVAILABLE - USE RENDER WAV";
    if(routes&PT_MIDI)return "MIDI PLAYBACK NOT AVAILABLE - MIDI IS NOT RENDERED";
    if(report->issues&PT_EXPORT_STEREO)return "PLAY: STEREO MASTER - USE SAMPLE PREVIEW OR RENDER";
    if(report->issues&PT_EXPORT_RATE)return "PLAY: SAMPLE RATE - USE SAMPLE PREVIEW OR RENDER";
    if(report->issues&PT_EXPORT_SLICES)return "PLAY: SLICES REQUIRE ENHANCED PLAYBACK - USE RENDER";
    if(report->issues&PT_EXPORT_LOOPS)return "PLAY: LOOP NOT PAULA COMPATIBLE - USE RENDER";
    if(report->issues&~PT_EXPORT_PRECISION)return "PLAY: REQUIRES CLASSIC FOUR-CHANNEL PAULA PROJECT";
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
