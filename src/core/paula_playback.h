#ifndef PT_PAULA_PLAYBACK_H
#define PT_PAULA_PLAYBACK_H
#include <string.h>
#include "mod_project.h"
/* Private shallow replay snapshot: sample/event masters are read-only and never
 * converted. Caller owns distinct snapshot/report storage. Zero means eligible,
 * not allocated/playing. Mute/solo are applied separately by the native output.
 * Eligibility is deliberately unchanged from strict classic replay. */
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
    if(pt_mod_export_analyse(copy,report)!=PT_PROJECT_OK)return "PLAY: INVALID PROJECT";
    if(p->mode==PT_MODE_STUDIO)return "STUDIO PLAYBACK NOT AVAILABLE - USE RENDER WAV";
    if((routes&(PT_AMIGUS|PT_MIDI))==(PT_AMIGUS|PT_MIDI))return "AMIGUS AND MIDI PLAYBACK NOT AVAILABLE";
    if(routes&PT_AMIGUS)return "AMIGUS PLAYBACK NOT AVAILABLE - USE RENDER WAV";
    if(routes&PT_MIDI)return "MIDI PLAYBACK NOT AVAILABLE - MIDI IS NOT RENDERED";
    if(report->issues&PT_EXPORT_PRECISION)return "PLAY: HIGH-RES MASTER - USE SAMPLE PREVIEW OR RENDER";
    if(report->issues&PT_EXPORT_STEREO)return "PLAY: STEREO MASTER - USE SAMPLE PREVIEW OR RENDER";
    if(report->issues&PT_EXPORT_RATE)return "PLAY: SAMPLE RATE - USE SAMPLE PREVIEW OR RENDER";
    if(report->issues&PT_EXPORT_SLICES)return "PLAY: SLICES REQUIRE ENHANCED PLAYBACK - USE RENDER";
    if(report->issues&PT_EXPORT_LOOPS)return "PLAY: LOOP NOT PAULA COMPATIBLE - USE RENDER";
    if(report->issues)return "PLAY: REQUIRES CLASSIC FOUR-CHANNEL PAULA PROJECT";
    return NULL;
}
#endif
