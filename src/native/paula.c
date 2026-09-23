/* Owned native Paula/CIA bridge. Effects run only in the pinned 2.3F engine. */
#include <devices/audio.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <stdlib.h>
#include <string.h>
#include "mod_project.h"
#include "mod_inspect.h"
#include "paula.h"
#include "paula_cache.h"
#include "paula_preview.h"
extern int pt_replay_start(void *,unsigned long,void *,unsigned long,void *);
extern void pt_replay_stop(void);
extern volatile uint32_t pt_replay_ticks;
extern volatile uint32_t pt_replay_clock;
extern volatile uint8_t pt_replay_scopes[];
extern volatile uint16_t pt_replay_rowbytes,pt_replay_tempo,pt_replay_audible;
extern volatile uint8_t pt_replay_order,pt_replay_speed,pt_replay_voices[],pt_replay_enabled,pt_replay_rawvol[4],pt_replay_outputvol[4];
static unsigned be16(const uint8_t *p) {return ((unsigned)p[0]<<8)|p[1];}
static uintptr_t be32(const uint8_t *p)
{return ((uintptr_t)p[0]<<24)|((uintptr_t)p[1]<<16)|((uintptr_t)p[2]<<8)|p[3];}
/* Playback represents mute/solo at the owned output stage. Names/groups and
   dormant MIDI assignments do not alter Paula audio. Clear them in the private
   replay snapshot only; strict disk export still refuses their loss. Panning,
   routes, sample format and all other audio requirements remain validated. */
static int playback_project(const struct pt_project *p,struct pt_project *copy)
{
    unsigned i;
    if(pt_project_validate(p,NULL)!=PT_PROJECT_OK)return 0;
    *copy=*p;copy->title[20]=0; /* Display text never changes classic replay audio. */
    for(i=0;i<copy->channels.count;++i) {
        copy->channels.track[i].muted=0;copy->channels.track[i].solo=0;
        copy->channels.track[i].group=0;copy->channels.track[i].midi_channel=(uint8_t)(i+1);
        memset(copy->channels.track[i].name,0,sizeof(copy->channels.track[i].name));
    }
    return 1;
}
static unsigned audible_mask(const struct pt_project *p)
{
    unsigned i,mask=0;
    for(i=0;i<4;++i)if(pt_channel_audible(&p->channels,i,PT_PAULA))mask|=1U<<i;
    return mask;
}
static void set_audible(struct pt_paula *a,unsigned mask)
{
    unsigned i;
    Disable();pt_replay_audible=(uint16_t)mask;a->audible=mask;
    for(i=0;i<4;++i) {
        uint8_t volume=(mask&(1U<<i))?pt_replay_rawvol[i]:0;
        pt_replay_outputvol[i]=volume;*(volatile UWORD *)(0xdff0a8+i*16)=volume;
    }
    Enable();
}
static void *chip_allocate(void *context,size_t bytes)
{(void)context;return AllocMem(bytes,MEMF_CHIP|MEMF_PUBLIC);}
static void chip_release(void *context,void *data,size_t bytes)
{(void)context;FreeMem(data,bytes);}
static void halt(struct pt_paula *a,int retain)
{
    unsigned i;struct pt_sample_cache cache;uint64_t version;
    if(a->started) {pt_replay_stop();a->started=0;}
    if(a->opened) {
        a->audio->ioa_Request.io_Command=ADCMD_FREE;
        a->audio->ioa_Request.io_Flags=0;
        DoIO((struct IORequest *)a->audio);
        if(a->locked) {WaitIO((struct IORequest *)a->lock);a->locked=0;}
        CloseDevice((struct IORequest *)a->audio);a->opened=0;
    }
    if(a->lock)DeleteIORequest((struct IORequest *)a->lock);
    if(a->audio)DeleteIORequest((struct IORequest *)a->audio);
    if(a->port)DeleteMsgPort(a->port);
    for(i=0;i<31;++i)if(a->sample_bytes[i])pt_cache_unpin(&a->cache,a->lease[i]);
    if(a->silence)FreeMem(a->silence,2);
    pt_master_release(&a->memory,a->data);
    pt_master_release(&a->memory,a->staging);pt_master_release(&a->memory,a->check);
    if(!retain)pt_cache_clear(&a->cache);
    cache=a->cache;version=a->cache_version;memset(a,0,sizeof(*a));
    if(retain) {a->cache=cache;a->cache_version=version;}
}
void pt_paula_stop(struct pt_paula *a) {halt(a,0);}
const char *pt_paula_play(struct pt_paula *a,const struct pt_project *p,unsigned mode,unsigned position,unsigned pattern)
{
    struct pt_project playback;struct pt_mod_export_report report;struct pt_paula_cache_plan plan;size_t written,offset;unsigned i;UBYTE channels=15;
    const char *error="PLAY: OUT OF CHIP MEMORY";
    if(mode>1 || position>=p->order_count || pattern>=p->pattern_count)return "PLAY: INVALID POSITION";
    if(!playback_project(p,&playback) || pt_mod_export_analyse(&playback,&report)!=PT_PROJECT_OK || report.issues)
        return "PLAY: REQUIRES CLASSIC FOUR-CHANNEL PAULA PROJECT";
    halt(a,1);
    if(!a->cache.allocate)pt_cache_init(&a->cache,NULL,chip_allocate,chip_release,AvailMem(MEMF_CHIP));
    if(a->cache_version==UINT64_MAX) {pt_paula_stop(a);return "PLAY: CACHE GENERATION EXHAUSTED";}
    ++a->cache_version;
    a->order_count=p->order_count;memcpy(a->orders,p->orders,p->order_count*sizeof(*p->orders));a->source_bytes=report.bytes;a->pattern_bytes=(size_t)p->pattern_count*1024;
    a->mode=mode;a->pattern=pattern;
    pt_master_memory_init(&a->memory);
    error="PLAY: OUT OF REPLAY WORKSPACE MEMORY";
    a->staging=pt_master_allocate(&a->memory,a->source_bytes);
    a->check=pt_master_allocate(&a->memory,a->source_bytes);
    if(!a->staging || !a->check)goto failed;
    if(pt_mod_export_direct(&playback,a->staging,a->source_bytes,&written)!=PT_PROJECT_OK || written!=a->source_bytes) {
        error="PLAY: SNAPSHOT ENCODE FAILED";goto failed;
    }
    error="PLAY: UNSAFE CLASSIC SAMPLE METADATA";
    if(!pt_paula_cache_plan(a->staging,a->source_bytes,&plan))goto failed;
    a->bytes=plan.mod.sample_offset;a->cached_instruments=plan.instruments;
    error="PLAY: OUT OF REPLAY WORKSPACE MEMORY";
    a->data=pt_master_allocate(&a->memory,a->bytes);
    if(!a->data)goto failed;
    memcpy(a->data,a->staging,a->bytes);
    error="PLAY: OUT OF CHIP MEMORY";
    a->silence=AllocMem(2,MEMF_CHIP|MEMF_PUBLIC|MEMF_CLEAR);
    if(!a->silence)goto failed;
    a->chip_bytes=2;offset=plan.mod.sample_offset;
    for(i=0;i<31;++i) {
        uint8_t *h=a->data+20+i*30;
        size_t length=be16(h+22)*2;
        unsigned start=be16(h+26),repeat=be16(h+28);
        a->sample_data[i]=a->silence;
        error="PLAY: UNSAFE CLASSIC SAMPLE METADATA";
        if(start && start+(repeat?repeat:1)>length/2)goto failed;
        if(length && (plan.instruments&(UINT32_C(1)<<i))) {
            error="PLAY: OUT OF CHIP MEMORY";
            if(pt_cache_take(&a->cache,i,a->cache_version,length,a->lease+i)!=PT_CACHE_LOAD)goto failed;
            a->sample_data[i]=pt_cache_data(&a->cache,a->lease[i]);
            a->sample_bytes[i]=length;a->chip_bytes+=length;
            memcpy(a->sample_data[i],a->staging+offset,length);
            pt_cache_publish(&a->cache,a->lease[i]);
        } else {h[22]=h[23]=h[26]=h[27]=h[28]=0;h[29]=1;}
        offset+=length;
    }
    if(mode) {
        a->data[950]=1;a->data[952]=(uint8_t)pattern;
        /* Keep sample-data offset correct even when order zero was the only
           reference to the highest stored pattern. Unused orders count too. */
        a->data[1079]=(uint8_t)(p->pattern_count-1);
    }
    error="PLAY: AUDIO CHANNELS BUSY OR UNAVAILABLE";
    a->port=CreateMsgPort();if(!a->port)goto failed;
    a->audio=(struct IOAudio *)CreateIORequest(a->port,sizeof(*a->audio));
    a->lock=(struct IOAudio *)CreateIORequest(a->port,sizeof(*a->lock));
    if(!a->audio || !a->lock)goto failed;
    a->audio->ioa_Request.io_Message.mn_Node.ln_Pri=-128;
    a->audio->ioa_Data=&channels;a->audio->ioa_Length=1;
    if(OpenDevice("audio.device",0,(struct IORequest *)a->audio,0))goto failed;
    a->opened=1;
    /* Allocate only free channels, then prevent subsequent stealing. A failed
       precedence change means we lost ownership before touching hardware. */
    a->audio->ioa_Request.io_Message.mn_Node.ln_Pri=127;
    a->audio->ioa_Request.io_Command=ADCMD_SETPREC;
    a->audio->ioa_Request.io_Flags=0;
    if(DoIO((struct IORequest *)a->audio) || (ULONG)a->audio->ioa_Request.io_Unit!=15)goto failed;
    a->lock->ioa_Request.io_Device=a->audio->ioa_Request.io_Device;
    a->lock->ioa_Request.io_Unit=a->audio->ioa_Request.io_Unit;
    a->lock->ioa_AllocKey=a->audio->ioa_AllocKey;
    a->lock->ioa_Request.io_Command=ADCMD_LOCK;
    SendIO((struct IORequest *)a->lock);a->locked=1;
    if(CheckIO((struct IORequest *)a->lock)) {WaitIO((struct IORequest *)a->lock);a->locked=0;goto failed;}
    error="PLAY: CIA TIMER UNAVAILABLE";
    a->audible=audible_mask(p);
    if(!pt_replay_start(a->data,mode?0:position,a->silence,a->audible,a->sample_data))goto failed;
    a->started=1;return NULL;
failed:
    pt_paula_stop(a);return error;
}
const char *pt_paula_sync(struct pt_paula *a,const struct pt_project *p)
{
    struct pt_project playback;struct pt_mod_export_report report;size_t n,offset;
    if(!a->started)return NULL;
    if(!p || !p->orders || p->order_count!=a->order_count || memcmp(p->orders,a->orders,p->order_count*sizeof(*p->orders))) {
        pt_paula_stop(a);return "STOPPED: SONG POSITIONS CHANGED - PRESS PLAY TO RESTART";
    }
    if(!playback_project(p,&playback) || pt_mod_export_analyse(&playback,&report)!=PT_PROJECT_OK || report.issues || report.bytes!=a->source_bytes ||
       (size_t)p->pattern_count*1024!=a->pattern_bytes ||
       pt_mod_export_direct(&playback,a->check,a->source_bytes,&n)!=PT_PROJECT_OK || n!=a->source_bytes) {
        pt_paula_stop(a);return "STOPPED: EDIT REQUIRES ENHANCED REPLAY BACKEND";
    }
    /* Compare with immutable export, not EFx-mutated Chip playback bytes. */
    if(!pt_paula_cache_compatible(a->staging,a->check,a->source_bytes)) {
        pt_paula_stop(a);return "STOPPED: SAMPLE CACHE CHANGED - PRESS PLAY TO REBUILD";
    }
    /* Publish only changed rows, with short interrupt exclusion. Do not copy
       samples: EFx intentionally modifies the private replay snapshot. */
    for(offset=1084;offset<1084+a->pattern_bytes;offset+=16)
        if(memcmp(a->data+offset,a->check+offset,16)) {
            Disable();CopyMem(a->check+offset,a->data+offset,16);Enable();
        }
    set_audible(a,audible_mask(p));
    return NULL;
}
void pt_paula_poll(struct pt_paula *a,struct pt_playback *s)
{
    uint8_t voices[176],volumes[4],scopes[80];uint32_t clock;unsigned i;memset(s,0,sizeof(*s));
    if(!a->started)return;
    if(!pt_replay_enabled || CheckIO((struct IORequest *)a->lock)) {pt_paula_stop(a);return;}
    Disable();
    s->ticks=pt_replay_ticks;s->order=pt_replay_order;s->row=pt_replay_rowbytes/16;
    s->speed=pt_replay_speed;s->bpm=pt_replay_tempo;
    CopyMem((const void *)pt_replay_voices,voices,sizeof(voices));
    CopyMem((const void *)pt_replay_scopes,scopes,sizeof(scopes));clock=pt_replay_clock*2;
    for(i=0;i<4;++i)volumes[i]=pt_replay_outputvol[i];
    Enable();s->active=1;s->mode=a->mode;s->pattern=a->data[952+s->order];
    for(i=0;i<4;++i) {
        const uint8_t *v=voices+i*44,*scope=scopes+i*20;
        uintptr_t address=be32(scope+8),loop=be32(scope+12),base=(uintptr_t)a->silence;
        size_t bytes=2;unsigned sample;
        s->volume[i]=volumes[i];s->period[i]=(uint16_t)be16(v+24);
        for(sample=0;sample<31;++sample)if(a->sample_bytes[sample]) {
            uintptr_t candidate=(uintptr_t)a->sample_data[sample];
            size_t length=a->sample_bytes[sample];
            if(address>=candidate && loop>=candidate && address-candidate<=length && loop-candidate<=length) {
                base=candidate;bytes=length;break;
            }
        }
        if(address<base || loop<base || address-base>bytes || loop-base>bytes)continue;
        pt_scope_wave(&a->scope[i],s->wave[i],(const int8_t *)base,bytes,
            (uint32_t)(address-base),be16(scope+16)*2,(uint32_t)(loop-base),be16(scope+18)*2,
            (uint32_t)be32(scope),(uint32_t)be32(scope+4),s->ticks,s->period[i],s->bpm,clock);
    }
}

const char *pt_paula_audition(struct pt_paula *a,const struct pt_project *p,unsigned sample,unsigned period)
{
    struct pt_project q;struct pt_event *events;uint16_t order=0;
    struct pt_sample selected[2];const char *error;struct pt_master_memory workspace;
    uint32_t frames,i;unsigned channels,ch;int32_t *converted=NULL;struct pt_pcm preview;
    if(!sample || sample>p->sample_count || period<113 || period>856)return "SAMPLE: SELECT A VALID SAMPLE";
    if(p->channels.track[p->channels.selected].route!=PT_PAULA)return "SAMPLE: SELECT A PAULA CHANNEL FOR THIS BACKEND";
    selected[0]=p->samples[sample-1];
    if(selected[0].loop!=PT_LOOP_NONE && selected[0].loop!=PT_LOOP_FORWARD)
        return "SAMPLE: FORMAT NEEDS ENHANCED PREVIEW OR CONVERSION";
    if(pt_paula_preview_frames(&selected[0].pcm,PT_CLASSIC_RATE,&frames)!=PT_PCM_OK)
        return "SAMPLE: INVALID OR TOO LARGE FOR PAULA PREVIEW";
    if(selected[0].loop==PT_LOOP_FORWARD &&
       pt_paula_preview_loop(&selected[0].pcm,PT_CLASSIC_RATE,selected[0].loop_start,selected[0].loop_end,
                             &selected[0].loop_start,&selected[0].loop_end)!=PT_PCM_OK)
        return "SAMPLE: LOOP TOO SHORT FOR PAULA PREVIEW";
    channels=selected[0].pcm.channels;
    pt_master_memory_init(&workspace);
    preview=selected[0].pcm;preview.rate=PT_CLASSIC_RATE;preview.frames=frames;
    preview.bits=8;preview.capacity=(size_t)frames*channels;preview.data=NULL;
    if(frames) {
        converted=pt_master_allocate(&workspace,(size_t)frames*(channels==2?4:1)*sizeof(*converted));
        if(!converted)return "SAMPLE: OUT OF CONVERSION MEMORY";
        preview.data=converted;
    }
    if(pt_paula_preview_prepare(&selected[0].pcm,&preview)!=PT_PCM_OK) {
        pt_master_release(&workspace,converted);return "SAMPLE: PLAYBACK CONVERSION FAILED";
    }
    selected[0].pcm=preview;
    if(channels==2) {
        selected[1]=selected[0];
        for(ch=0;ch<2;++ch) {
            selected[ch].pcm.channels=1;selected[ch].pcm.capacity=frames;
            selected[ch].pcm.data=frames?converted+(size_t)frames*(2+ch):NULL;
            for(i=0;i<frames;++i)selected[ch].pcm.data[i]=converted[(size_t)i*2+ch];
        }
    }
    memset(&q,0,sizeof(q));pt_channels_init(&q.channels);q.order_count=1;q.pattern_count=1;
    q.sample_count=(uint16_t)channels;q.orders=&order;q.speed=6;q.bpm=125;q.samples=selected;
    events=pt_master_allocate(&workspace,256*sizeof(*events));
    if(!events) {pt_master_release(&workspace,converted);return "SAMPLE: OUT OF MEMORY";}
    memset(events,0,256*sizeof(*events));
    q.events=events;
    /* Classic voices 0/1 are hard left/right, triggered on the same row. */
    for(ch=0;ch<channels;++ch) {
        events[ch].kind=PT_NOTE_PERIOD;events[ch].pitch=(uint16_t)period;events[ch].instrument=(uint8_t)(ch+1);
    }
    error=pt_paula_play(a,&q,0,0,0);
    pt_master_release(&workspace,events);pt_master_release(&workspace,converted);
    if(!error)a->mode=2;
    return error;
}
