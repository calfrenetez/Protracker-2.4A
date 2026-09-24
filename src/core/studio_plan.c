#include "studio_plan.h"
#include <string.h>
static const struct pt_studio_binding *resolve(const struct pt_studio_binding *b,unsigned n,const struct pt_pcm *pcm)
{
    const struct pt_studio_binding *found=NULL;unsigned i;
    for(i=0;i<n;++i)if(b[i].pcm==pcm) {if(found || !pcm)return NULL;found=b+i;}
    return found;
}
enum pt_pcm_result pt_studio_dispatch(struct pt_studio_mix *mix,unsigned voices,
    const struct pt_render_plan *plan,const struct pt_studio_binding *bindings,unsigned count)
{
    unsigned i;enum pt_pcm_result result=PT_PCM_INVALID;
    if(!mix)return result;
    if(!voices || voices>16 || !plan || plan->count>PT_RENDER_ACTIONS || (count && !bindings))goto fail;
    /* Reject unknown/ambiguous sources and invalid channels before callbacks. */
    for(i=0;i<plan->count;++i) {
        const struct pt_render_action *a=plan->action+i;
        if(a->channel>=voices || a->kind<PT_RENDER_TRIGGER || a->kind>PT_RENDER_CONTROL)goto fail;
        if(a->kind==PT_RENDER_TRIGGER || a->kind==PT_RENDER_SEGMENT || a->kind==PT_RENDER_REPEAT)
            if(!resolve(bindings,count,a->kind==PT_RENDER_REPEAT?a->voice.repeat_pcm:a->voice.pcm))goto fail;
    }
    for(i=0;i<plan->count;++i) {
        const struct pt_render_action *a=plan->action+i;const struct pt_voice *v=&a->voice;
        const struct pt_studio_binding *b;struct pt_studio_note note;
        result=PT_PCM_OK;
        switch(a->kind) {
        case PT_RENDER_TRIGGER:case PT_RENDER_SEGMENT:
            b=resolve(bindings,count,v->pcm);memset(&note,0,sizeof(note));
            note.key=b->key;note.version=b->version;note.step=v->step;
            note.start=v->start;note.end=v->end;note.loop=(enum pt_voice_loop)v->loop;
            note.loop_start=v->loop_start;note.loop_end=v->loop_end;note.linear=v->linear;
            /* Final CONTROL establishes gains before the next read. */
            result=a->kind==PT_RENDER_SEGMENT?pt_studio_trigger_segment(mix,a->channel,&note):pt_studio_trigger(mix,a->channel,&note);break;
        case PT_RENDER_REPEAT:
            b=resolve(bindings,count,v->repeat_pcm);
            result=pt_studio_repeat(mix,a->channel,b->key,b->version,v->loop_start,v->loop_end);break;
        case PT_RENDER_STOP:pt_studio_stop(mix,a->channel);break;
        case PT_RENDER_CONTROL: {
            struct pt_studio_control c[16];memset(c,0,sizeof(c));
            c[a->channel].step=v->step;c[a->channel].gain[0]=a->gain[0];c[a->channel].gain[1]=a->gain[1];
            result=pt_studio_control(mix,(uint16_t)(1U<<a->channel),c);break;
        }
        }
        if(result!=PT_PCM_OK)goto fail;
    }
    return PT_PCM_OK;
fail:
    for(i=0;i<16;++i)pt_studio_stop(mix,i);
    return result;
}
