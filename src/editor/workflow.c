#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "editor.h"
#include "sample_range.h"

static void status(struct pt_editor *e,const char *s) {pt_editor_status(e,s);++e->sample_ui;}
static int active(struct pt_editor *e)
{return e->playback.active || (e->workflow.recording_busy && e->workflow.recording_busy(e->workflow.recording_context));}
static int current(struct pt_editor *e)
{return e->workflow.usage_valid && pt_sample_usage_current(&e->workflow.usage,e->project,e->history.revision,e->sampler.generation);}
static void rebuild_view(struct pt_editor *e)
{
    struct pt_editor_workflow *w=&e->workflow;unsigned i,j,n=0;
    unsigned usage_current=(unsigned)current(e);
    for(i=0;i<e->project->sample_count;++i) {
        const char *name=e->project->samples[i].name;char upper[PT_PROJECT_NAME];size_t k;
        if(w->filter && (!usage_current || !(w->usage.rows[i].flags&PT_USAGE_ELIGIBLE)))continue;
        for(k=0;k<sizeof(upper)-1 && name[k];++k)upper[k]=(char)toupper((unsigned char)name[k]);
        upper[k]=0;
        if(w->find[0] && !strstr(upper,w->find))continue;
        j=n;
        while(j && w->sort) {
            unsigned previous=w->view[j-1];int move;
            if(w->sort==1)move=strcmp(e->project->samples[previous].name,name)>0;
            else move=usage_current && w->usage.rows[previous].references<w->usage.rows[i].references;
            if(!move)break;
            w->view[j]=w->view[j-1];--j;
        }
        w->view[j]=(uint8_t)i;++n;
    }
    w->view_count=n;w->view_sample_count=e->project->sample_count;w->view_table=e->project->samples;
    for(i=0;i<n && w->view[i]+1U!=e->sample;++i) {}
    if(i<n)w->highlight=i;
    else {
        if(w->highlight>=n)w->highlight=n?n-1:0;
        if(n && e->sample!=w->view[w->highlight]+1U) {e->sample=w->view[w->highlight]+1;pt_editor_sample_all(e);}
    }
    if(w->highlight<w->first)w->first=w->highlight;
    if(w->highlight>=w->first+PT_WORKFLOW_ROWS)w->first=w->highlight-PT_WORKFLOW_ROWS+1;
}
static int scan_begin(struct pt_editor *e,unsigned for_copy)
{
    struct pt_sample_usage_options options;memset(&options,0,sizeof(options));
    options.revision=e->history.revision;options.generation=e->sampler.generation;
    if(pt_sample_usage_begin(&e->workflow.scan,e->project,&options)!=PT_USAGE_OK) {
        status(e,"USAGE SCAN REFUSED - PROJECT UNCHANGED");return 0;
    }
    e->workflow.scanning=1;e->workflow.scan_for_copy=for_copy;e->workflow.usage_valid=0;
    memset(e->workflow.usage.selected,0,sizeof(e->workflow.usage.selected));
    status(e,for_copy?"COPY: SCANNING ALL STORED PATTERNS - ESC CANCEL":"USAGE SCANNING - NO CLEANUP ITEMS SELECTED / ESC CANCEL");return 1;
}
static void manager(struct pt_editor *e)
{
    e->panel=PT_WORKFLOW_MANAGER;e->workflow.highlight=e->sample?e->sample-1:0;e->workflow.first=0;
    e->workflow.filter=0;e->workflow.sort=0;e->workflow.find[0]=0;e->workflow.searching=0;
    rebuild_view(e);(void)scan_begin(e,0);
}
static void toolbox(struct pt_editor *e)
{
    e->panel=PT_WORKFLOW_TOOLBOX;
    if(e->sample_range_slot!=e->sample)pt_editor_sample_all(e);
    e->workflow.wave_cancelled=0;
    status(e,"TOOLBOX: U SELECT LOOP / S START / E END / C COPY CURRENT RANGE");
}
void pt_editor_workflow_cancel(struct pt_editor *e)
{
    struct pt_editor_workflow *w=&e->workflow;
    pt_sampler_workflow_cancel(&w->transaction);pt_wave_summary_cancel(&w->wave_job);
    w->scanning=w->busy=w->resolving=w->pending_apply=w->wave_building=0;
}
static void cancel_input(struct pt_editor *e)
{
    unsigned wave=e->workflow.wave_building;pt_editor_workflow_cancel(e);e->workflow.wave_cancelled=wave;
    status(e,"WORKFLOW CANCELLED - UNPUBLISHED COPY/CLEANUP DISCARDED");
}
void pt_editor_workflow_loop(struct pt_editor *e,unsigned operation)
{
    struct pt_sample_range loop,view,next;enum pt_sample_range_result r;
    const struct pt_sample *s=e->sample && e->sample<=e->project->sample_count?&e->project->samples[e->sample-1]:NULL;
    r=pt_sample_range_loop(s,&loop);
    if(r!=PT_SAMPLE_RANGE_OK) {status(e,r==PT_SAMPLE_RANGE_NO_LOOP?"NO ENABLED LOOP - SAMPLE AND VIEW PRESERVED":"INVALID LOOP - SAMPLE AND VIEW PRESERVED");return;}
    if(!operation) {
        e->sample_range_slot=e->sample;e->sample_start=loop.start;e->sample_end=loop.end;e->sample_marking=0;
        status(e,"COMPLETE LOOP SELECTED - SAMPLE AND TRANSPORT PRESERVED");return;
    }
    pt_editor_wave_bounds(e,&view.start,&view.end);
    if(pt_sample_range_centre(s->pcm.frames,&view,operation==1?loop.start:loop.end,&next)!=PT_SAMPLE_RANGE_OK) {
        status(e,"LOOP VIEW REFUSED - NO CHANGE");return;
    }
    e->wave_slot=e->sample;e->wave_frames=s->pcm.frames;e->wave_start=next.start;e->wave_end=next.end;e->workflow.wave_cancelled=0;
    status(e,operation==1?"LOOP START LOCATED - ZOOM AND SELECTION PRESERVED":"EXCLUSIVE LOOP END LOCATED - ZOOM AND SELECTION PRESERVED");
}
static void transaction_result(struct pt_editor *e,enum pt_edit_result r)
{
    if(r==PT_EDIT_OK) {e->workflow.busy=1;status(e,"PREPARING TRANSACTION - ESC CANCEL / PROJECT UNCHANGED");return;}
    e->workflow.pending_apply=0;
    status(e,r==PT_EDIT_CAPACITY?"NO FREE SLOT, SAMPLE MEMORY OR UNDO CAPACITY - NO CHANGE":
        r==PT_EDIT_CONFLICT?"STALE PREVIEW OR ACTIVE OWNERS - REFRESH / NO CHANGE":"WORKFLOW REFUSED - NO CHANGE");
}
static void copy_begin_ready(struct pt_editor *e)
{
    struct pt_editor_workflow *w=&e->workflow;
    if(active(e) || e->sample!=w->copy_slot+1 || e->sample_marking || e->sample_range_slot!=e->sample ||
        e->sample_start!=w->copy_start || e->sample_end!=w->copy_end || e->history.revision!=w->copy_revision || e->sampler.generation!=w->copy_generation || !current(e)) {
        w->pending_apply=0;status(e,"COPY SOURCE/USAGE CHANGED OR PLAYBACK ACTIVE - NO CHANGE");return;
    }
    transaction_result(e,pt_sampler_copy_begin(&e->sampler,e->project,&e->history,&w->usage,w->copy_slot,w->copy_start,w->copy_end,1,&w->transaction));
}
void pt_editor_workflow_apply(struct pt_editor *e)
{
    struct pt_editor_workflow *w=&e->workflow;
    if(active(e)) {status(e,"STOP TRANSPORT/PREVIEW BEFORE APPLY OR USE STOP+APPLY");return;}
    if(!pt_editor_prepare_change(e))return;
    if(w->pending_apply==1) {
        if(!current(e)) {status(e,"CLEANUP PREVIEW STALE - REFRESH BEFORE APPLY");w->pending_apply=0;return;}
        transaction_result(e,pt_sampler_cleanup_begin(&e->sampler,e->project,&e->history,&w->usage,w->usage.selected,1,&w->transaction));
    } else if(w->pending_apply==2) {
        if(current(e))copy_begin_ready(e);else (void)scan_begin(e,1);
    }
}
static enum pt_editor_action request_apply(struct pt_editor *e,unsigned which)
{
    e->workflow.pending_apply=which;
    if(active(e)) {status(e,"STOP+APPLY WILL STOP ACTIVE TRANSPORT/PREVIEW BEFORE THIS TRANSACTION");return PT_UI_NONE;}
    pt_editor_workflow_apply(e);return PT_UI_NONE;
}
static enum pt_editor_action copy_range(struct pt_editor *e)
{
    struct pt_editor_workflow *w=&e->workflow;struct pt_sample_range range={e->sample_start,e->sample_end};
    const struct pt_sample *s=e->sample && e->sample<=e->project->sample_count?&e->project->samples[e->sample-1]:NULL;
    if(e->sample_marking || e->sample_range_slot!=e->sample || pt_sample_range_validate(s,&range)!=PT_SAMPLE_RANGE_OK) {
        status(e,"FINISH A VALID CURRENT FRAME SELECTION - COPY REFUSED");return PT_UI_NONE;
    }
    w->copy_slot=e->sample-1;w->copy_start=range.start;w->copy_end=range.end;
    w->copy_revision=e->history.revision;w->copy_generation=e->sampler.generation;
    return request_apply(e,2);
}
static void choose(struct pt_editor *e,unsigned row)
{
    struct pt_editor_workflow *w=&e->workflow;
    if(row>=w->view_count || w->view[row]>=e->project->sample_count)return;
    w->highlight=row;e->sample=w->view[row]+1;pt_editor_sample_all(e);rebuild_view(e);
    status(e,"SAMPLE SELECTED SILENTLY - EDITOR/AUDITION ARE EXPLICIT");
}
static void toggle_selected(struct pt_editor *e)
{
    struct pt_editor_workflow *w=&e->workflow;unsigned slot;
    if(!current(e) || w->highlight>=w->view_count) {status(e,"REFRESH USAGE BEFORE SELECTING CLEANUP ITEMS");return;}
    slot=w->view[w->highlight];
    if(!(w->usage.rows[slot].flags&PT_USAGE_ELIGIBLE)) {status(e,"REFERENCED/PROTECTED/EMPTY SLOT IS NOT ELIGIBLE FOR CLEANUP");return;}
    w->usage.selected[slot]^=1;status(e,"CLEANUP CHECKBOX CHANGED - PROJECT UNCHANGED");
}
static void return_origin(struct pt_editor *e)
{
    struct pt_editor_workflow *w=&e->workflow;struct pt_event_resource_origin *o=&w->return_origin;
    if(!w->return_valid || o->pattern>=e->project->pattern_count || o->row>=64 || o->track>=e->project->channels.count) {
        status(e,"NO VALID EVENT ORIGIN TO RETURN TO");return;
    }
    e->pattern=o->pattern;e->row=o->row;e->field=w->return_field;e->project->channels.selected=(uint8_t)o->track;
    if(o->order_known && o->order<e->project->order_count && e->project->orders[o->order]==o->pattern)e->position=o->order;
    pt_editor_reveal_cursor(e);e->panel=1;e->note_details=1;e->song_details=0;
    status(e,"RETURNED TO EVENT ORIGIN - TRANSPORT AND MUSIC UNCHANGED");
}
static void navigation_result(struct pt_editor *e)
{
    struct pt_editor_workflow *w=&e->workflow;struct pt_event_resource_result *r=&w->resource;
    const char *meaning=r->state==PT_EVENT_RESOURCE_EXPLICIT?"EXPLICIT EVENT INSTRUMENT":r->state==PT_EVENT_RESOURCE_RESOLVED_INHERITED?"RESOLVED INHERITED INSTRUMENT":NULL;
    if(!meaning) {status(e,r->state==PT_EVENT_RESOURCE_AMBIGUOUS?"AMBIGUOUS ENTRY HISTORY - SELECTED RESOURCE PRESERVED":
        r->state==PT_EVENT_RESOURCE_NO_RESOURCE?"NO EVENT RESOURCE - SELECTED RESOURCE PRESERVED":"UNRESOLVED CONTEXT/BUDGET - SELECTED RESOURCE PRESERVED");return;}
    w->return_origin=r->origin;w->return_valid=1;w->return_field=w->invocation_field;
    if(r->destination==PT_EVENT_RESOURCE_MIDI_ROUTING) {
        if(w->open_resource) {e->project->channels.selected=(uint8_t)r->origin.track;e->panel=4;e->channel_details=1;e->note_details=0;}
        status(e,w->open_resource?"EVENT MIDI ROUTING CONTROLS - NO PROGRAM/NOTE MESSAGE SENT":"EVENT MIDI RESOURCE IDENTIFIED - OPEN TO VIEW ROUTING");return;
    }
    if(r->destination==PT_EVENT_RESOURCE_AUDIO_MASTER && r->instrument && r->instrument<=e->project->sample_count) {
        e->sample=r->instrument;pt_editor_sample_all(e);
        if(w->open_resource) {e->panel=5;e->note_details=0;}
        status(e,e->project->samples[e->sample-1].pcm.frames?meaning:"EXPLICIT/INHERITED EVENT SLOT IS EMPTY - NO SUBSTITUTE");
    }
}
void pt_editor_event_resource(struct pt_editor *e,unsigned open)
{
    struct pt_editor_workflow *w=&e->workflow;struct pt_event_resource_origin o;
    memset(&o,0,sizeof(o));o.pattern=e->pattern;o.row=e->row;o.track=e->project->channels.selected;
    o.order=e->position;o.order_known=o.order<e->project->order_count && e->project->orders[o.order]==o.pattern;
    o.start_order=0;o.flow_mode=w->flow_mode;
    if(pt_event_resource_begin(&w->resolver,e->project,&o,e->history.revision,e->sampler.generation,200000UL)!=PT_EVENT_RESOURCE_OK) {
        status(e,"EVENT RESOURCE CONTEXT INVALID - SELECTED RESOURCE PRESERVED");return;
    }
    w->invocation_field=e->field;w->open_resource=open;w->navigation_revision=e->history.revision;w->navigation_generation=e->sampler.generation;
    w->resource_valid=0;w->resolving=1;
    if(pt_event_resource_get(&w->resolver,e->history.revision,e->sampler.generation,&w->resource)==PT_EVENT_RESOURCE_OK) {
        w->resolving=0;w->resource_valid=1;navigation_result(e);
    } else status(e,"RESOLVING EVENT INHERITANCE OFFLINE - ESC CANCEL");
}
static int search_key(struct pt_editor *e,unsigned raw)
{
    static const unsigned codes[]={0x20,0x35,0x33,0x22,0x12,0x23,0x24,0x25,0x17,0x26,0x27,0x28,0x37,0x36,0x18,0x19,0x10,0x13,0x21,0x14,0x16,0x34,0x11,0x32,0x15,0x31};
    struct pt_editor_workflow *w=&e->workflow;size_t len=strlen(w->find);unsigned i;
    if(raw==0x44 || raw==0x45)w->searching=0;
    else if(raw==0x41 || raw==0x46) {if(len)w->find[len-1]=0;}
    else if(len<16) {
        for(i=0;i<26 && raw!=codes[i];++i) {}
        if(i<26) {w->find[len]=(char)('A'+i);w->find[len+1]=0;}
        else if(raw>=1 && raw<=10) {w->find[len]=(char)(raw==10?'0':'0'+raw);w->find[len+1]=0;}
        else if(raw==0x40) {w->find[len]=' ';w->find[len+1]=0;}
    }
    w->highlight=w->first=0;rebuild_view(e);status(e,"NAME FILTER ONLY - RETURN FINISH / BACKSPACE / ESC");return 1;
}
int pt_editor_workflow_key(struct pt_editor *e,unsigned raw,unsigned qualifier,enum pt_editor_action *action)
{
    struct pt_editor_workflow *w=&e->workflow;unsigned i;
    *action=PT_UI_NONE;
    if(e->number_field || e->name_entry)return 0;
    if(w->searching)return search_key(e,raw);
    if(w->busy || (w->scanning && w->scan_for_copy)) {
        if(raw==0x45)cancel_input(e);
        else if(raw==0x59 || raw==0x40) {*action=PT_UI_STOP;cancel_input(e);}
        else status(e,"WORKFLOW BUSY - ESC CANCEL BEFORE OTHER ACTIONS");
        return 1;
    }
    if((qualifier&8) && raw==0x44) {pt_editor_event_resource(e,(qualifier&3)!=0);return 1;}
    if((qualifier&8) && (qualifier&3) && raw==0x28) {manager(e);return 1;}
    if((qualifier&8) && (qualifier&3) && raw==0x27) {toolbox(e);return 1;}
    if((qualifier&8) && raw==0x31) {pt_editor_workflow_cancel(e);w->usage_valid=w->resource_valid=0;return 0;}
    if(w->resolving && raw==0x45) {w->resolving=0;status(e,"EVENT NAVIGATION CANCELLED - TARGET PRESERVED");return 1;}
    if(e->panel!=PT_WORKFLOW_MANAGER && e->panel!=PT_WORKFLOW_TOOLBOX) {
        if((qualifier&0x30) && raw==0x41 && w->return_valid) {return_origin(e);return 1;}
        if(e->panel==6 && !(qualifier&8) && (raw==0x16 || raw==0x21 || raw==0x12 || raw==0x33)) {
            if(raw==0x33)*action=copy_range(e);else pt_editor_workflow_loop(e,raw==0x16?0:raw==0x21?1:2);return 1;
        }
        return 0;
    }
    if(raw==0x45) {pt_editor_workflow_cancel(e);e->panel=5;status(e,"SAMPLER WORKFLOW CLOSED - PROJECT PRESERVED");return 1;}
    if(raw==0x59) {*action=PT_UI_STOP;return 1;}
    if((qualifier&8) && raw==0x21)return 0;
    if(qualifier&8)return 1;
    if(e->panel==PT_WORKFLOW_TOOLBOX) {
        if(raw==0x16)pt_editor_workflow_loop(e,0);else if(raw==0x21)pt_editor_workflow_loop(e,1);else if(raw==0x12)pt_editor_workflow_loop(e,2);
        else if(raw==0x33)*action=copy_range(e);
        else if(raw==0x20) {pt_editor_sample_all(e);status(e,"WHOLE SAMPLE SELECTED");}
        else if(raw==0x0b || raw==0x0c) {if(raw==0x0b && e->sample>1)--e->sample;else if(raw==0x0c && e->sample<e->project->sample_count)++e->sample;pt_editor_sample_all(e);++e->sample_ui;}
        else if(raw==0x37)manager(e);
        else if(raw==0x19 && w->pending_apply)*action=PT_UI_WORKFLOW_STOP_APPLY;
        else if(raw==0x35)return_origin(e);
        return 1;
    }
    if(raw==0x4c)choose(e,w->highlight?w->highlight-1:0);
    else if(raw==0x4d)choose(e,w->highlight+1);
    else if(raw==0x4f)choose(e,w->highlight>PT_WORKFLOW_ROWS?w->highlight-PT_WORKFLOW_ROWS:0);
    else if(raw==0x4e && w->view_count)choose(e,w->highlight+PT_WORKFLOW_ROWS<w->view_count?w->highlight+PT_WORKFLOW_ROWS:w->view_count-1);
    else if(raw==0x40)toggle_selected(e);
    else if(raw==0x13)(void)scan_begin(e,0);
    else if(raw==0x23) {w->filter^=1;w->highlight=w->first=0;rebuild_view(e);status(e,"ELIGIBLE FILTER CHANGED - SLOT IDENTITIES PRESERVED");}
    else if(raw==0x21) {w->sort=(w->sort+1)%3;rebuild_view(e);status(e,"VIEW SORT ONLY - SLOT IDENTITIES PRESERVED");}
    else if(raw==0x36) {w->searching=1;status(e,"FIND NAME - TYPE TEXT / RETURN / ESC");}
    else if(raw==0x20 && current(e)) {for(i=0;i<w->usage.count;++i)w->usage.selected[i]=(w->usage.rows[i].flags&PT_USAGE_ELIGIBLE)!=0;status(e,"ALL ELIGIBLE CHECKED EXPLICITLY - APPLY TO REMOVE");}
    else if(raw==0x34 && current(e)) {memset(w->usage.selected,0,sizeof(w->usage.selected));status(e,"ALL CLEANUP CHECKBOXES CLEARED");}
    else if(raw==0x22)*action=request_apply(e,1);
    else if(raw==0x19 && w->pending_apply)*action=PT_UI_WORKFLOW_STOP_APPLY;
    else if(raw==0x28 || raw==0x44) {e->panel=5;status(e,"SAMPLE EDITOR OPENED - NO AUDITION");}
    else if(raw==0x57) {
        if(active(e))status(e,"AUDITION UNAVAILABLE DURING PLAYBACK - STOP FIRST");else *action=PT_UI_AUDITION;
    } else if(raw==0x14)toolbox(e);
    else if(raw==0x35)return_origin(e);
    return 1;
}
int pt_editor_workflow_click(struct pt_editor *e,int x,int y,enum pt_editor_action *action)
{
    unsigned row,column;*action=PT_UI_NONE;
    if(e->number_field || e->name_entry)return 0;
    if(e->workflow.busy || (e->workflow.scanning && e->workflow.scan_for_copy)) {
        if(x>=476 && x<599 && y>=78 && y<97)cancel_input(e);
        return 1;
    }
    if(e->panel>=5 && e->panel<=11 && y>=PT_EDITOR_BOTTOM_Y && x>=248 && x<416) {
        if(x<332)manager(e);else toolbox(e);return 1;
    }
    if(e->panel==1 && e->note_details && y>=59 && y<78 && x>=230 && x<599) {
        if(x<353)pt_editor_event_resource(e,0);else if(x<476)pt_editor_event_resource(e,1);else return_origin(e);return 1;
    }
    if(e->panel!=PT_WORKFLOW_MANAGER && e->panel!=PT_WORKFLOW_TOOLBOX)return 0;
    if(e->panel==PT_WORKFLOW_TOOLBOX) {
        if(x>=230 && x<599 && y>=21 && y<97) {
            row=(unsigned)(y-2)/19;column=(unsigned)(x-230)/123;
            if(row==1)pt_editor_workflow_loop(e,column);
            else if(row==2) {if(column==0)*action=copy_range(e);else if(column==1) {pt_editor_sample_all(e);++e->sample_ui;}else manager(e);}
            else if(row==3) {if(column==0)return_origin(e);else if(column==1 && e->workflow.pending_apply)*action=PT_UI_WORKFLOW_STOP_APPLY;else if(column==2)e->panel=5;}
            else if(row==4) {if(column==0)*action=PT_UI_STOP;else if(column==2)cancel_input(e);}
        }
        /* Waveform marking is handled by the existing half-open adapter. */
        return y>=PT_EDITOR_HEADER_Y && y<PT_EDITOR_BOTTOM_Y?0:1;
    }
    if(x>=2 && x<638 && y>=258 && y<458) {
        row=e->workflow.first+(unsigned)(y-258)/20;
        if(row<e->workflow.view_count) {choose(e,row);if(x<22)toggle_selected(e);}return 1;
    }
    if(x>=230 && x<599 && y>=21 && y<97) {
        static const unsigned keys[4][3]={{0x13,0x23,0x21},{0x20,0x34,0x22},{0x28,0x57,0x14},{0x35,0x19,0x45}};
        row=(unsigned)(y-21)/19;column=(unsigned)(x-230)/123;
        return pt_editor_workflow_key(e,keys[row][column],0,action);
    }
    if(y>=458 && y<478) {
        if(x<128)choose(e,e->workflow.highlight>PT_WORKFLOW_ROWS?e->workflow.highlight-PT_WORKFLOW_ROWS:0);
        else if(x<256 && e->workflow.view_count)choose(e,e->workflow.highlight+PT_WORKFLOW_ROWS<e->workflow.view_count?e->workflow.highlight+PT_WORKFLOW_ROWS:e->workflow.view_count-1);
        else {e->workflow.searching=1;status(e,"FIND NAME - TYPE TEXT / RETURN / ESC");}
    }
    return 1;
}
static int wave_idle(struct pt_editor *e,unsigned advance)
{
    struct pt_editor_workflow *w=&e->workflow;struct pt_sample_range view;unsigned ready=0,slot;
    const struct pt_project *p;uint64_t generation;
    if(e->panel<5 || e->panel==PT_WORKFLOW_MANAGER || w->wave_cancelled)return 0;
    p=e->panel==11?&e->sample_source.project:e->project;slot=e->panel==11?e->source_selected:e->sample;
    generation=e->panel==11?w->source_generation:e->sampler.generation;
    if(!slot || slot>p->sample_count || !p->samples[slot-1].pcm.frames)return 0;
    if(e->panel==11) {view.start=0;view.end=p->samples[slot-1].pcm.frames;}else pt_editor_wave_bounds(e,&view.start,&view.end);
    if(pt_wave_summary_current(&w->wave,p,generation) && w->wave.slot==slot-1 && w->wave.view.start==view.start && w->wave.view.end==view.end)return 0;
    if(w->wave_building && (w->wave_job.summary.project!=p || w->wave_job.summary.slot!=slot-1 ||
        w->wave_job.summary.generation!=generation || w->wave_job.summary.view.start!=view.start || w->wave_job.summary.view.end!=view.end)) {
        pt_wave_summary_cancel(&w->wave_job);w->wave_building=0;
    }
    if(!w->wave_building) {
        unsigned staging=w->wave.bins==w->wave_bins[0]?1:0;
        if(pt_wave_summary_begin(&w->wave_job,p,slot-1,generation,&view,w->wave_bins[staging],PT_WAVE_SUMMARY_BINS)!=PT_WAVE_SUMMARY_OK)return 0;
        w->wave_building=1;
    }
    /* Metadata-only scheduling may coexist with a primary phase; PCM reads wait
     * until the next input interval without transaction/resolver work. */
    if(!advance)return 0;
    {enum pt_wave_summary_result result=pt_wave_summary_step(&w->wave_job,generation,&ready);
    if(result!=PT_WAVE_SUMMARY_OK && result!=PT_WAVE_SUMMARY_PENDING) {pt_wave_summary_cancel(&w->wave_job);w->wave_building=0;return 0;}}
    if(ready) {
        if(pt_wave_summary_take(&w->wave_job,generation,&w->wave)!=PT_WAVE_SUMMARY_OK)return 0;
        w->wave_building=0;++e->sample_ui;return 1;
    }
    return 0;
}
int pt_editor_workflow_idle(struct pt_editor *e)
{
    struct pt_editor_workflow *w=&e->workflow;unsigned ready=0;enum pt_edit_result edit;
    int changed=0;unsigned primary=0;
    /* Ownership cancellation is checked every idle, independently of which
     * bounded phase is selected below. It does not inspect former source data. */
    if(w->busy && active(e)) {cancel_input(e);return 1;}
    if(e->panel==PT_WORKFLOW_MANAGER && (w->view_sample_count!=e->project->sample_count || w->view_table!=e->project->samples)) {rebuild_view(e);++e->sample_ui;changed=1;}
    if(w->usage_valid && !current(e)) {w->usage_valid=0;memset(w->usage.selected,0,sizeof(w->usage.selected));++e->sample_ui;changed=1;}
    if(w->scanning) {
        primary=1;
        if(pt_sample_usage_step(&w->scan,e->project,e->history.revision,e->sampler.generation,&w->usage,&ready)!=PT_USAGE_OK) {
            w->scanning=0;w->pending_apply=0;status(e,"USAGE SCAN STALE/REFUSED - REFRESH / NO CHANGE");changed=1;
        } else if(ready) {
            w->scanning=0;w->usage_valid=1;rebuild_view(e);
            status(e,"USAGE READY - STORED REFERENCES / NO CLEANUP ITEMS SELECTED");changed=1;
            if(w->scan_for_copy)copy_begin_ready(e);
        }
    }
    if(w->busy && !primary) {
        primary=1;edit=pt_sampler_workflow_step(w->transaction,&ready);
        if(edit!=PT_EDIT_OK) {pt_sampler_workflow_cancel(&w->transaction);w->busy=w->pending_apply=0;transaction_result(e,edit);changed=1;}
        else if(ready) {
            if(!pt_editor_prepare_change(e)) {pt_sampler_workflow_cancel(&w->transaction);w->busy=w->pending_apply=0;return 1;}
            edit=pt_sampler_workflow_commit(&w->transaction,!active(e),&w->stats);
            if(edit!=PT_EDIT_OK)pt_sampler_workflow_cancel(&w->transaction);
            w->busy=w->pending_apply=0;w->usage_valid=0;memset(w->usage.selected,0,sizeof(w->usage.selected));
            if(edit==PT_EDIT_OK) {
                char message[76];
                if(w->stats.affected_slots && w->stats.slots_freed)snprintf(message,sizeof(message),"CLEARED %u SLOTS - UNDO RETAINS MASTERS; 0 BYTES RELEASED",w->stats.slots_freed);
                else {e->sample=w->stats.destination_slot+1;snprintf(message,sizeof(message),"COPIED CURRENT RANGE TO SLOT %02X - ONE UNDO / SOURCE PRESERVED",e->sample);}
                pt_editor_sample_all(e);status(e,message);
                if(e->panel==PT_WORKFLOW_MANAGER) {rebuild_view(e);(void)scan_begin(e,0);}
            } else transaction_result(e,edit);
            changed=1;
        }
    }
    if(w->resolving && !primary) {
        primary=1;
        /* A first engine validation can scan PCM. Keep it out of an active
         * performance; cached same-version navigation remains bounded ticks. */
        if(e->playback.active && !w->resolver.validated) {
            w->resolving=0;status(e,"INHERITANCE NEEDS INITIAL STOPPED VALIDATION - TARGET PRESERVED");changed=1;
        } else if(pt_event_resource_step(&w->resolver,e->history.revision,e->sampler.generation,e->playback.active?16:256,&ready)!=PT_EVENT_RESOURCE_OK) {
            w->resolving=0;status(e,"EVENT RESOURCE STALE - INVOKE AGAIN / TARGET PRESERVED");changed=1;
        } else if(ready) {
            w->resolving=0;
            if(pt_event_resource_get(&w->resolver,e->history.revision,e->sampler.generation,&w->resource)==PT_EVENT_RESOURCE_OK) {
                w->resource_valid=1;navigation_result(e);
            }changed=1;
        }
    }
    return wave_idle(e,!primary)||changed;
}
