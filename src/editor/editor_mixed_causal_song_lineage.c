/* Private SOURCE prototype: exactly one finite TRIGGER/CONTROL16/STOP
 * lineage at original absolute musical frames. Default pair is untouched;
 * no timer, IRQ, device, whole-song PLAY, lead, jitter or refill integration. */
#include "editor_mixed_causal_song_lineage_internal.h"
#include "../core/render_storage_internal.h"
#include "../core/paula_render_voice.h"
#include "../core/amigus_render_voice.h"
#include <limits.h>
#include <string.h>
#define ALIGN_OF(t) offsetof(struct {char prefix;t value;},value)
#define NO_COMMAND PT_SAMPLER_MIXED_COMMANDS

static int span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int apart(const void *a,size_t n,const void *b,size_t m)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return span(a,n)&&span(b,m)&&(!n||!m||(x<=y?n<=y-x:m<=x-y));}
static int inside(struct pt_sampler_storage_span s,const void *p,size_t n)
{return n&&span(s.data,s.bytes)&&span(p,n)&&n<=s.bytes&&
 (uintptr_t)p>=(uintptr_t)s.data&&(uintptr_t)p-(uintptr_t)s.data<=s.bytes-n;}
static int add_size(size_t *n,size_t more)
{if(more>SIZE_MAX-*n)return 0;*n+=more;return 1;}
static int empty_bytes(const void *v,size_t n)
{const unsigned char *p=v;size_t i;for(i=0;i<n;++i)if(p[i])return 0;return 1;}
static unsigned small_work(unsigned n){return n>256?256:n;}
static enum pt_editor_mixed_causal_song_lineage_result fail(struct pt_editor_mixed_causal_song_lineage *p,
 enum pt_editor_mixed_causal_song_lineage_result r)
{if(!p->first_error)p->first_error=r;p->result=p->first_error;p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_DRAIN;
 return p->result;}
static int identity(const struct pt_editor_mixed_causal_song_lineage *p)
{return p&&span(p,sizeof(*p))&&p->original==p&&p->adopted&&p->source&&p->borrow&&p->outputs&&p->controller&&
  p->joint&&span(p->joint,sizeof(*p->joint))&&
  (uintptr_t)p->joint%ALIGN_OF(struct pt_editor_mixed_causal_lineage_source_prepare)==0&&
  inside(p->parent,p,sizeof(*p))&&inside(p->parent,p->joint,sizeof(*p->joint))&&
  apart(p,sizeof(*p),p->joint,sizeof(*p->joint))&&
  p->controller==&p->joint->original.original&&
  p->joint->self==p->joint&&p->joint->inputs==p->source&&p->controller->source_publisher==p->borrow&&
  p->outputs==(const void *)p->extent[7].data;}
static int local_apart(const struct pt_editor_mixed_causal_song_lineage *p,uintptr_t address,size_t n)
{
 const void *v=(const void *)address;
 unsigned i;
 if(!identity(p)||!v||!n||!span(v,n)||!apart(v,n,p->parent.data,p->parent.bytes)||
    !apart(v,n,p->source,sizeof(*p->source))||!apart(v,n,p->borrow,sizeof(*p->borrow))||
    p->requests>PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REQUESTS)return 0;
 for(i=0;i<8;++i)if(!apart(v,n,p->extent[i].data,p->extent[i].bytes))return 0;
 for(i=0;i<p->requests;++i)if(!apart(v,n,(const void *)p->allocation[i].address,p->allocation[i].bytes))return 0;
 /* Original captured-only SOURCE guard survives fixed tag staleness. After
  * positive borrow close there are no controller children; captured numeric
  * producer/source extents remain guarded until its own final lifetime ends. */
 if(!p->borrow_closed&&!pt_editor_mixed_causal_source_allocation_disjoint(p->controller,p->borrow,v,n))return 0;
 return 1;
}
static int current(struct pt_editor_mixed_causal_song_lineage *p)
{
 struct pt_editor_mixed_source_observation o;
 unsigned i;
 if(!identity(p)||p->first_error||p->cancelled||p->borrow_closed)return 0;
  if(p->candidate_count>3||p->lineage_index>3||p->attempted_mask>7||
     !local_apart(p,(uintptr_t)&o,sizeof(o))){fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);return 0;}
 for(i=0;i<p->candidate_count;++i)if(memcmp(p->outputs->candidate+i,p->frozen+i,sizeof(p->frozen[i]))){
    fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);return 0;}
 o=pt_editor_mixed_causal_source_observe(p->controller,p->borrow);
 if(o.scope!=PT_EDITOR_MIXED_SOURCE_SCOPE_HELD||o.first_error||o.cancel_requested||
    !o.fixed_tags_current||memcmp(&p->configuration,&p->saved,sizeof(p->saved))){
    fail(p,o.scope==PT_EDITOR_MIXED_SOURCE_SCOPE_HELD&&!o.fixed_tags_current?
       PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_STALE:PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);return 0;}
 return 1;
}
static int external_enter(struct pt_editor_mixed_causal_song_lineage *p,unsigned cleanup)
{
 if(!identity(p)||(!cleanup&&!current(p)))return 0;
 if(!pt_editor_mixed_causal_source_enter(p->borrow)){fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);return 0;}
 return 1;
}
static int external_leave(struct pt_editor_mixed_causal_song_lineage *p,unsigned cleanup)
{
 if(!pt_editor_mixed_causal_source_leave(p->borrow)){fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);return 0;}
 return cleanup?1:current(p);
}
static int enter(struct pt_editor_mixed_causal_song_lineage *p,unsigned cleanup)
{
 if(!identity(p)||p->hook_closed)return 0;
 if(p->busy){++p->reentries;fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);return 0;}
 if(!cleanup&&!current(p))return 0;
 p->busy=1;return 1;
}

/* This wrapper has exactly three request slots. It wraps ONLY actual
 * establishment metadata and the audit's two renderer controls. Master jobs
 * retain the sampler's unchanged genuine allocator/guards. Retired numerical
 * extents remain here until final close; they are never former pointers read.
 * A non-NULL return overlapping a retired extent is unresolved custody: the
 * numerical guard cannot distinguish a stale alias from a fresh recycled owner.
 * No initializer or base release sees it, and no final quiet is manufactured.
 */
static void *allocate_external(void *context,size_t n)
{
 struct pt_editor_mixed_causal_song_lineage *p=context;
 struct pt_editor_mixed_source_observation o;
 struct pt_editor_mixed_causal_song_lineage_allocation *a;void *v;unsigned i,before=p->reentries;
 if(!identity(p)||!p->busy||!n||p->requests>=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REQUESTS||
    p->first_error||p->live_bytes>p->saved.ordinary_budget||
    n>p->saved.ordinary_budget-p->live_bytes)return NULL;
  if(!local_apart(p,(uintptr_t)&o,sizeof(o)))return NULL;
  o=pt_editor_mixed_causal_source_observe(p->controller,p->borrow);
  if(o.scope!=PT_EDITOR_MIXED_SOURCE_SCOPE_HELD||!o.source_busy||o.controller_busy||
    o.first_error||o.cancel_requested||!o.fixed_tags_current)return NULL;
 if((!p->requests&&p->phase!=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_ESTABLISH_BEGIN)||
    (p->requests&&(p->phase!=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_AUDIT_BEGIN&&
                    p->phase!=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_AUDIT_STEP)))return NULL;
 a=p->allocation+p->requests++;a->bytes=n;
 v=p->saved.external_allocator.allocate(p->saved.external_allocator.context,n);
 if(!v)return NULL;
 a->address=(uintptr_t)v;
 /* Classify FULL request before any writable child initialization/disposal. */
 if(!pt_editor_mixed_causal_source_allocation_disjoint(p->controller,p->borrow,v,n)){
    if(!span(v,n))p->quiet_ambiguous=1;
    fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);return NULL;}
 for(i=0;i+1<p->requests;++i)
    if(!apart(v,n,(const void *)p->allocation[i].address,p->allocation[i].bytes)){
       if(!p->allocation[i].live){
          a->unresolved=1;p->live_bytes+=n;p->quiet_ambiguous=1;
       }
       fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);return NULL;}
 a->live=1;p->live_bytes+=n;
 if(before!=p->reentries||!current(p)){
    /* Proved fresh actual return remains owned even after callback failure.
     * Retire BEFORE the real base release, with no later child retry. */
    a->live=0;p->live_bytes-=n;
    p->saved.external_allocator.release(p->saved.external_allocator.context,v);
    fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);return NULL;}
 return v;
}
static void release_external(void *context,void *v)
{
 struct pt_editor_mixed_causal_song_lineage *p=context;unsigned i;
 struct pt_editor_mixed_source_observation o;
 if(!identity(p)||!p->busy){if(p)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);return;}
  if(!local_apart(p,(uintptr_t)&o,sizeof(o))){fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);return;}
  o=pt_editor_mixed_causal_source_observe(p->controller,p->borrow);
  if(o.scope!=PT_EDITOR_MIXED_SOURCE_SCOPE_HELD||!o.source_busy||o.controller_busy){
    fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);return;}
 for(i=0;i<p->requests;++i)
    if(p->allocation[i].live&&p->allocation[i].address==(uintptr_t)v)break;
 if(i==p->requests){fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);return;}
 p->allocation[i].live=0;p->live_bytes-=p->allocation[i].bytes;
 p->saved.external_allocator.release(p->saved.external_allocator.context,v);
 /* Cleanup may deliberately follow cancellation/tag staleness. Its numerical
  * ownership already retired and no former source table is traversed here. */
 if(p->reentries)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);
}
static int no_external_allocations(const struct pt_editor_mixed_causal_song_lineage *p)
{unsigned i;if(p->requests>PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REQUESTS||p->quiet_ambiguous)return 0;
 for(i=0;i<p->requests;++i)if(p->allocation[i].live||p->allocation[i].unresolved)return 0;
 return p->live_bytes==p->base_bytes;}
static int prune(struct pt_editor_mixed_causal_song_lineage *p)
{
 unsigned i;enum pt_editor_mixed_source_registration_observation o;
 for(i=0;i<PT_SAMPLER_MIXED_COMMANDS;++i)if(p->command[i].reference.serial){
    o=pt_editor_mixed_causal_source_command_registration(p->controller,p->borrow,p->command[i].reference);
    if(o==PT_EDITOR_MIXED_SOURCE_REGISTRATION_INVALID)return 0;
    if(o==PT_EDITOR_MIXED_SOURCE_REGISTRATION_ABSENT)memset(p->command+i,0,sizeof(p->command[i]));}
  for(i=0;i<3;++i)if(p->history[i].reference.serial&&!p->history[i].detached){
     o=pt_editor_mixed_causal_source_command_registration(p->controller,p->borrow,p->history[i].reference);
     if(o==PT_EDITOR_MIXED_SOURCE_REGISTRATION_INVALID)return 0;
     if(o==PT_EDITOR_MIXED_SOURCE_REGISTRATION_ABSENT)p->history[i].detached=1;
  }
  for(i=0;i<PT_SAMPLER_MIXED_READERS;++i)if(p->reader[i].reference.serial){
    o=pt_editor_mixed_causal_source_reader_registration(p->controller,p->borrow,p->reader[i].reference);
    if(o==PT_EDITOR_MIXED_SOURCE_REGISTRATION_INVALID)return 0;
    if(o==PT_EDITOR_MIXED_SOURCE_REGISTRATION_ABSENT)memset(p->reader+i,0,sizeof(p->reader[i]));}
 return 1;
}
static int registrations_empty(const struct pt_editor_mixed_causal_song_lineage *p)
{unsigned i;for(i=0;i<PT_SAMPLER_MIXED_COMMANDS;++i)if(p->command[i].reference.serial)return 0;
 for(i=0;i<PT_SAMPLER_MIXED_READERS;++i)if(p->reader[i].reference.serial)return 0;
 return 1;}
static int origin_equal(const struct pt_mixed_plan_origin *a,const struct pt_mixed_plan_origin *b)
{return a->present==b->present&&a->sample==b->sample&&a->channel==b->channel&&a->frame==b->frame;}
static int empty_record(const struct pt_mixed_plan_record *r)
{
 const struct pt_amigus_voice_request *t=&r->geometry.amigus.trigger;
 return r->kind==PT_MIXED_PLAN_TRIGGER&&!r->track&&!r->route&&!r->slot&&!r->sample&&!r->channel&&
 !r->first_action&&!r->control_action&&!r->geometry.amigus.bits&&!r->geometry.amigus.little_endian&&
 !t->rate_numerator&&!t->rate_denominator&&!t->offset&&!t->volume&&!t->pan&&
 !r->geometry.amigus.rate&&!r->geometry.amigus.left&&!r->geometry.amigus.right&&
 !r->image.start&&!r->image.loop&&!r->image.end_exclusive&&!r->image.rate&&
 !r->image.control&&!r->image.left&&!r->image.right;
}
static int checked_empty(struct pt_editor_mixed_causal_song_lineage *p)
{
 const struct pt_mixed_plan_quantized_batch *b=&p->outputs->normalized;unsigned i,j;
 if(b->normalized.count||b->normalized.frame!=p->target||!p->outputs->sequence||
    !current(p)||!pt_render_sequence_output_disjoint(p->outputs->sequence,
        &p->outputs->interval,sizeof(p->outputs->interval)))return 0;
 for(i=0;i<PT_MIXED_PLAN_RECORDS;++i){
    if(!empty_record(b->normalized.record+i)||b->levels[i].mode!=PT_MIXED_PLAN_LEVEL_LEGACY||
       b->levels[i].left||b->levels[i].right||!origin_equal(b->normalized.next+i,p->origins+i))return 0;}
 for(j=0;j<2;++j)for(i=0;i<PT_PROJECT_SAMPLES;++i)if(b->normalized.samples[j][i])return 0;
 return 1;
}
static int lower_trigger(struct pt_editor_mixed_causal_song_lineage *p)
{
 struct pt_editor_mixed_causal_song_lineage_outputs *u=p->outputs;unsigned i,mask=0;
 const struct pt_mixed_plan_batch *b=&u->normalized.normalized;
 if(!b->count||b->count>PT_MIXED_PLAN_RECORDS||b->frame!=p->target)return 0;
 memset(&u->batch,0,sizeof(u->batch));
 u->batch.count=b->count;
 for(i=0;i<b->count;++i){
    const struct pt_mixed_plan_record *r=b->record+i;
    const struct pt_mixed_plan_trigger_levels *l=u->normalized.levels+i;
    struct pt_editor_mixed_readers_request *q=u->batch.action+i;
    if(r->track>=PT_MIXED_PLAN_RECORDS||r->sample>=PT_PROJECT_SAMPLES||
       (mask&(1U<<r->track))||(r->route!=PT_PAULA&&r->route!=PT_AMIGUS))return 0;
    mask|=1U<<r->track;q->track=r->track;q->sample=r->sample;q->channel=r->channel;
    if(r->kind!=PT_MIXED_PLAN_TRIGGER)return 0;
    q->kind=PT_MIXED_READERS_TRIGGER;q->expected=p->pin[r->sample];
    if(!q->expected)return 0;
    if(r->route==PT_PAULA){
       if(l->mode!=PT_MIXED_PLAN_LEVEL_LEGACY||l->left||l->right)return 0;
       q->geometry.paula.period=r->geometry.paula.period;q->geometry.paula.volume=r->geometry.paula.volume;
    }else{
       q->geometry.amigus.bits=r->geometry.amigus.bits;
       q->geometry.amigus.little_endian=r->geometry.amigus.little_endian;
       q->geometry.amigus.trigger.rate_numerator=r->geometry.amigus.trigger.rate_numerator;
       q->geometry.amigus.trigger.rate_denominator=r->geometry.amigus.trigger.rate_denominator;
       q->geometry.amigus.trigger.offset=r->geometry.amigus.trigger.offset;
       q->geometry.amigus.trigger.volume=r->geometry.amigus.trigger.volume;
       q->geometry.amigus.trigger.pan=r->geometry.amigus.trigger.pan;
       q->geometry.amigus.rate=r->geometry.amigus.rate;
       q->geometry.amigus.left=r->geometry.amigus.left;q->geometry.amigus.right=r->geometry.amigus.right;
       if(l->mode!=PT_MIXED_PLAN_LEVEL_QUANTIZED||q->geometry.amigus.trigger.volume||
          q->geometry.amigus.trigger.pan)return 0;
       u->batch.levels[i].mode=PT_SAMPLER_MIXED_TRIGGER_QUANTIZED;
       u->batch.levels[i].left=l->left;u->batch.levels[i].right=l->right;
    }
 }
 for(;i<PT_MIXED_PLAN_RECORDS;++i)
    if(!empty_record(b->record+i)||u->normalized.levels[i].mode!=PT_MIXED_PLAN_LEVEL_LEGACY||
       u->normalized.levels[i].left||u->normalized.levels[i].right)return 0;
 return 1;
}
static int lower(struct pt_editor_mixed_causal_song_lineage *p)
{
 struct pt_editor_mixed_causal_song_lineage_outputs *u=p->outputs;unsigned i;
 const struct pt_mixed_plan_batch *b=&u->normalized.normalized;
 if(!p->lineage_index)return lower_trigger(p);
 if(p->lineage_index>2||!b->count||b->count>PT_MIXED_PLAN_RECORDS||b->frame!=p->target)return 0;
 memset(&u->control,0,sizeof(u->control));memset(&u->stop,0,sizeof(u->stop));
 u->control.frame=u->stop.frame=b->frame;u->control.count=u->stop.count=b->count;
 for(i=0;i<b->count;++i){const struct pt_mixed_plan_record *r=b->record+i;
    if(r->track>=PT_MIXED_PLAN_RECORDS||!p->track[r->track].serial)return 0;
    if(p->lineage_index==1){struct pt_editor_mixed_causal_lineage_control16_action *a=u->control.action+i;
       if(r->kind!=PT_MIXED_PLAN_CONTROL)return 0;
       a->reader=p->track[r->track];
       if(r->route==PT_PAULA){a->period=r->geometry.paula.period;a->volume=r->geometry.paula.volume;}
       else if(r->route==PT_AMIGUS){a->rate=r->geometry.amigus.rate;a->left=r->geometry.amigus.left;a->right=r->geometry.amigus.right;}
       else return 0;
    }else{if(r->kind!=PT_MIXED_PLAN_STOP)return 0;u->stop.reader[i]=p->track[r->track];}
 }
 return 1;
}
static const struct pt_mixed_plan_record *record_track(const struct pt_mixed_plan_quantized_batch *b,unsigned track)
{unsigned i;if(b->normalized.count>PT_MIXED_PLAN_RECORDS)return NULL;
 for(i=0;i<b->normalized.count;++i){if(b->normalized.record[i].track==track)return b->normalized.record+i;}
 return NULL;}
static int zero_image(const struct pt_amigus_voice_plan *v)
{return !v->start&&!v->loop&&!v->end_exclusive&&!v->rate&&!v->control&&!v->left&&!v->right;}
static int zero_trigger(const struct pt_amigus_voice_request *v)
{return !v->rate_numerator&&!v->rate_denominator&&!v->offset&&!v->volume&&!v->pan;}
/* U.normalized is mutable. Validate its current CONTROL conversion against
 * the referenced renderer action before any prior-image redundancy decision.
 * Only numeric resolved step/gains are converted; no foreign PCM is read. */
static int control_from_action(const struct pt_editor_mixed_causal_song_lineage *p,
 const struct pt_mixed_plan_record *r)
{
 struct {uint32_t rate;uint16_t period,left,right;uint8_t volume;} local;
 const struct pt_render_action *a;
 const struct pt_project *project=p->source->original.binding->editor->project;
 if(!local_apart(p,(uintptr_t)&local,sizeof(local))||
    r->first_action>=p->outputs->plan.count)return 0;
 a=p->outputs->plan.action+r->first_action;
 if(a->kind!=PT_RENDER_CONTROL||a->channel!=r->track||r->sample>=project->sample_count||
    a->voice.pcm!=&project->samples[r->sample].pcm||a->voice.active!=1||
    a->voice.segment||a->voice.repeat_pcm)return 0;
 if(r->route==PT_PAULA){
    if(!pt_paula_render_control(a->voice.step,p->saved.options.rate,a->gain,r->slot,
          &p->saved.caps,&local.period,&local.volume))return 0;
    return local.period==r->geometry.paula.period&&local.volume==r->geometry.paula.volume;
 }
 if(r->route!=PT_AMIGUS||!pt_amigus_render_control(a->voice.step,p->saved.options.rate,
       a->gain,&local.rate,&local.left,&local.right))return 0;
 return local.rate==r->geometry.amigus.rate&&local.left==r->geometry.amigus.left&&
    local.right==r->geometry.amigus.right;
}
/* Independent full semantic check before storing descriptive snapshots.
 * Every next-origin and source mask, including untouched tracks, is checked. */
static int snapshot_checked_batch(struct pt_editor_mixed_causal_song_lineage *p,
 const struct pt_mixed_plan_quantized_batch *b,unsigned raw)
{
 struct {struct pt_mixed_plan_origin next[PT_MIXED_PLAN_RECORDS];uint8_t samples[2][PT_PROJECT_SAMPLES];} local;
 unsigned i,seen=0,slots=0,stage=p->candidate_count;
 enum pt_mixed_plan_kind kind=stage==0?PT_MIXED_PLAN_TRIGGER:stage==1?PT_MIXED_PLAN_CONTROL:PT_MIXED_PLAN_STOP;
 if(!local_apart(p,(uintptr_t)&local,sizeof(local))||b->normalized.frame!=p->target||
    b->normalized.count>PT_MIXED_PLAN_RECORDS||(p->outputs->interval.end&&b->normalized.count)||
    (b->normalized.count&&stage>=3))return 0;
 memcpy(local.next,p->origins,sizeof(local.next));memset(local.samples,0,sizeof(local.samples));
 for(i=0;i<b->normalized.count;++i){const struct pt_mixed_plan_record *r=b->normalized.record+i;
     const struct pt_mixed_plan_trigger_levels *level=b->levels+i;unsigned index;
     unsigned control=raw&&stage&&stage<3&&r->kind==PT_MIXED_PLAN_CONTROL;
    const struct pt_channels *channels=&p->source->original.binding->editor->project->channels;
    unsigned t,expected_slot=0;
     if((r->kind!=kind&&!control)||r->track>=PT_MIXED_PLAN_RECORDS||r->track>=channels->count||
       r->sample>=p->source->original.binding->editor->project->sample_count||r->channel||
       (seen&(1U<<r->track))||(r->route!=PT_PAULA&&r->route!=PT_AMIGUS)||
       channels->track[r->track].route!=r->route)return 0;
    for(t=0;t<r->track;++t)if(channels->track[t].route==r->route)++expected_slot;
    if(r->slot!=expected_slot)return 0;
    index=r->route==PT_PAULA?r->slot:4+r->slot;
    if((r->route==PT_PAULA?r->slot>=4:r->slot>=16)||(slots&(1U<<index)))return 0;
    slots|=1U<<index;seen|=1U<<r->track;
    if(r->first_action>=p->outputs->plan.count||p->outputs->plan.action[r->first_action].channel!=r->track||
        p->outputs->plan.action[r->first_action].kind!=(control?PT_RENDER_CONTROL:
          stage==0?PT_RENDER_TRIGGER:stage==1?PT_RENDER_CONTROL:PT_RENDER_STOP))return 0;
    if(r->kind==PT_MIXED_PLAN_CONTROL&&!control_from_action(p,r))return 0;
    if(!stage){
       if(r->control_action!=UINT_MAX&&(r->control_action>=p->outputs->plan.count||
          p->outputs->plan.action[r->control_action].kind!=PT_RENDER_CONTROL||
          p->outputs->plan.action[r->control_action].channel!=r->track))return 0;
       if(r->route==PT_PAULA){if(level->mode!=PT_MIXED_PLAN_LEVEL_LEGACY||level->left||level->right)return 0;}
       else if(level->mode!=PT_MIXED_PLAN_LEVEL_QUANTIZED||
          r->geometry.amigus.bits!=p->saved.format.bits||r->geometry.amigus.little_endian!=p->saved.format.little_endian||
          r->geometry.amigus.trigger.volume||r->geometry.amigus.trigger.pan)return 0;
       local.next[r->track]=(struct pt_mixed_plan_origin){1,r->sample,r->channel,p->target};
       local.samples[r->route==PT_PAULA?0:1][r->sample]=1;
    }else{const struct pt_mixed_plan_record *first=record_track(p->frozen,r->track);
        const struct pt_mixed_plan_record *controlled=stage==2&&r->kind==PT_MIXED_PLAN_STOP?
           record_track(p->frozen+1,r->track):first;
       if(!first||!controlled||first->sample!=r->sample||first->channel!=r->channel||
          first->route!=r->route||first->slot!=r->slot||controlled->sample!=r->sample||
          controlled->channel!=r->channel||controlled->route!=r->route||controlled->slot!=r->slot||
          !p->origins[r->track].present||p->origins[r->track].sample!=r->sample||
          p->origins[r->track].channel!=r->channel||p->origins[r->track].frame!=p->frozen[0].normalized.frame||
          r->control_action!=UINT_MAX||level->mode!=PT_MIXED_PLAN_LEVEL_LEGACY||level->left||level->right||
          !zero_image(&r->image))return 0;
       if(r->route==PT_AMIGUS&&(!zero_trigger(&r->geometry.amigus.trigger)||
          r->geometry.amigus.bits||r->geometry.amigus.little_endian))return 0;
        if(stage==2&&r->kind==PT_MIXED_PLAN_STOP){
           if(r->route==PT_PAULA){if(r->geometry.paula.period||r->geometry.paula.volume)return 0;}
          else if(r->geometry.amigus.rate||r->geometry.amigus.left||r->geometry.amigus.right)return 0;
          memset(local.next+r->track,0,sizeof(local.next[r->track]));
       }
    }
 }
 for(;i<PT_MIXED_PLAN_RECORDS;++i)if(!empty_record(b->normalized.record+i)||
    b->levels[i].mode!=PT_MIXED_PLAN_LEVEL_LEGACY||b->levels[i].left||b->levels[i].right)return 0;
 for(i=0;i<PT_MIXED_PLAN_RECORDS;++i)if(!origin_equal(local.next+i,b->normalized.next+i))return 0;
 return !memcmp(local.samples,b->normalized.samples,sizeof(local.samples));
}
static int snapshot_checked(struct pt_editor_mixed_causal_song_lineage *p)
{return snapshot_checked_batch(p,&p->outputs->normalized,0);}
/* Captured descriptive geometry is neither a live key nor a readiness proof.
 * The whole prior batches were checked and remain exact current SOURCE images.
 * Only the effective rate/levels can prove that a raw CONTROL does no work. */
static int control_matches_previous(const struct pt_editor_mixed_causal_song_lineage *p,
 const struct pt_mixed_plan_record *r)
{
 const struct pt_mixed_plan_record *first=record_track(p->frozen,r->track);
 const struct pt_mixed_plan_record *prior=p->candidate_count==2?
    record_track(p->frozen+1,r->track):NULL;
 const struct pt_mixed_plan_trigger_levels *levels;unsigned i;
 if(!first||first->kind!=PT_MIXED_PLAN_TRIGGER||first->sample!=r->sample||
    first->channel!=r->channel||first->route!=r->route||first->slot!=r->slot)return -1;
 if(prior){
    if(prior->kind!=PT_MIXED_PLAN_CONTROL||prior->sample!=r->sample||
       prior->channel!=r->channel||prior->route!=r->route||prior->slot!=r->slot)return -1;
    i=(unsigned)(prior-p->frozen[1].normalized.record);levels=p->frozen[1].levels+i;
    if(levels->mode!=PT_MIXED_PLAN_LEVEL_LEGACY||levels->left||levels->right)return -1;
    if(r->route==PT_PAULA)return r->geometry.paula.period==prior->geometry.paula.period&&
       r->geometry.paula.volume==prior->geometry.paula.volume;
    return r->geometry.amigus.rate==prior->geometry.amigus.rate&&
       r->geometry.amigus.left==prior->geometry.amigus.left&&
       r->geometry.amigus.right==prior->geometry.amigus.right;
 }
 i=(unsigned)(first-p->frozen[0].normalized.record);levels=p->frozen[0].levels+i;
 if(r->route==PT_PAULA){
    if(levels->mode!=PT_MIXED_PLAN_LEVEL_LEGACY||levels->left||levels->right)return -1;
    return r->geometry.paula.period==first->geometry.paula.period&&
       r->geometry.paula.volume==first->geometry.paula.volume;
 }
 if(levels->mode!=PT_MIXED_PLAN_LEVEL_QUANTIZED||levels->left!=first->image.left||
    levels->right!=first->image.right)return -1;
 /* TRIGGER stores its actual quantized rate in image, not CONTROL geometry. */
 return r->geometry.amigus.rate==first->image.rate&&
    r->geometry.amigus.left==levels->left&&r->geometry.amigus.right==levels->right;
}
static int reduce_controls(struct pt_editor_mixed_causal_song_lineage *p)
{
 struct pt_mixed_plan_quantized_batch local;
 const struct pt_mixed_plan_quantized_batch *b=&p->outputs->normalized;
 unsigned i,count=0;int same;
 /* Numeric whole-local admission precedes the first scratch initialization.
  * Validate every raw field before allowing any apparently redundant omission. */
 if(!local_apart(p,(uintptr_t)&local,sizeof(local))||!snapshot_checked_batch(p,b,1))return 0;
 memcpy(&local,b,sizeof(local));
 for(i=0;i<b->normalized.count;++i){
    const struct pt_mixed_plan_record *r=b->normalized.record+i;
    if(p->candidate_count&&p->candidate_count<3&&r->kind==PT_MIXED_PLAN_CONTROL){
       same=control_matches_previous(p,r);
       if(same<0)return 0;
       if(same)continue;
    }
    if(count!=i){
       memcpy(local.normalized.record+count,r,sizeof(*r));
       memcpy(local.levels+count,b->levels+i,sizeof(local.levels[count]));
    }
    ++count;
 }
 local.normalized.count=count;
 memset(local.normalized.record+count,0,
    (PT_MIXED_PLAN_RECORDS-count)*sizeof(local.normalized.record[0]));
 memset(local.levels+count,0,(PT_MIXED_PLAN_RECORDS-count)*sizeof(local.levels[0]));
 /* Origins, next/source masks, original action indices and absolute frame stay
  * unchanged. A changed CONTROL mixed with STOP still refuses the exact shape. */
 if(!snapshot_checked_batch(p,&local,0)||!current(p))return 0;
 memcpy(&p->outputs->normalized,&local,sizeof(local));return 1;
}
static int pressure(struct pt_editor_mixed_causal_song_lineage *p)
{
 unsigned i,free_c=0,free_r=0,needed=0;
 if(!prune(p))return -1;
 for(i=0;i<PT_SAMPLER_MIXED_COMMANDS;++i)if(!p->command[i].reference.serial)++free_c;
 for(i=0;i<PT_SAMPLER_MIXED_READERS;++i)if(!p->reader[i].reference.serial)++free_r;
  if(!p->lineage_index)needed=p->outputs->batch.count;
 return free_c&&needed<=free_r;
}
static void audit_inputs(struct pt_editor_mixed_causal_song_lineage *p)
{
 unsigned i;struct pt_editor_mixed_causal_lineage_source_inputs const *s=p->source;
 p->audit_contexts[0]=(struct pt_mixed_plan_span){p->parent.data,p->parent.bytes};
 p->audit_contexts[1]=(struct pt_mixed_plan_span){s,sizeof(*s)};
 p->audit_contexts[2]=(struct pt_mixed_plan_span){p->borrow,sizeof(*p->borrow)};
 for(i=0;i<3;++i)p->audit_contexts[i+3]=(struct pt_mixed_plan_span){p->extent[i].data,p->extent[i].bytes};
 p->audit_contexts[6]=(struct pt_mixed_plan_span){p->extent[5].data,p->extent[5].bytes};
 p->audit_inputs=(struct pt_mixed_quantized_audit_inputs){
    p->source->original.binding->editor->project,&p->saved.options,&p->saved.caps,&p->saved.format,
    &p->allocator,p->audit_contexts,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_AUDIT_CONTEXTS,
    (void *)p->extent[4].data,p->extent[4].bytes,(void *)p->extent[6].data,p->extent[6].bytes,
    p->saved.audit_budget,p->revision,p->generation};
}
static void normalizer_inputs(struct pt_editor_mixed_causal_song_lineage *p)
{
 unsigned i;const struct pt_editor_mixed_causal_lineage_source_inputs *s=p->source;
 p->normalizer_contexts[0]=(struct pt_mixed_plan_span){p->parent.data,p->parent.bytes};
 p->normalizer_contexts[1]=(struct pt_mixed_plan_span){s,sizeof(*s)};
 p->normalizer_contexts[2]=(struct pt_mixed_plan_span){p->borrow,sizeof(*p->borrow)};
 for(i=0;i<5;++i)p->normalizer_contexts[3+i]=(struct pt_mixed_plan_span){p->extent[i].data,p->extent[i].bytes};
 p->normalizer_contexts[8]=(struct pt_mixed_plan_span){p->extent[6].data,p->extent[6].bytes};
 p->normalizer_contexts[9]=(struct pt_mixed_plan_span){p->outputs->sequence,pt_render_sequence_control_size()};
 p->normalizer_inputs=(struct pt_mixed_plan_inputs){p->source->original.binding->editor->project,
    &p->outputs->plan,p->origins,&p->saved.caps,&p->saved.format,p->normalizer_contexts,
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_Q_CONTEXTS,p->saved.options.rate,p->target,p->revision,p->generation};
}

enum pt_editor_mixed_causal_song_lineage_result pt_editor_mixed_causal_song_lineage_begin(struct pt_editor_mixed_causal_song_lineage *p,
 const struct pt_editor_mixed_causal_lineage_source_inputs *s,struct pt_editor_mixed_causal_source_borrow *b)
{
 enum pt_editor_mixed_readers_result r;unsigned i;size_t total=0;
 struct pt_editor_mixed_causal_song_lineage_outputs *u;
 if(!p||!s||!b||!span(p,sizeof(*p))||!span(s,sizeof(*s))||!span(b,sizeof(*b))||
    (uintptr_t)p%ALIGN_OF(struct pt_editor_mixed_causal_song_lineage)||
    (uintptr_t)s%ALIGN_OF(struct pt_editor_mixed_causal_lineage_source_inputs)||
    (uintptr_t)b%ALIGN_OF(struct pt_editor_mixed_causal_source_borrow)||p->adopted||p->phase||p->busy||
    !p->joint||!p->controller||
    (uintptr_t)p->joint%ALIGN_OF(struct pt_editor_mixed_causal_lineage_source_prepare)||
    !inside(s->original.contexts,p->joint,sizeof(*p->joint))||
    p->controller!=&p->joint->original.original||
    !inside(s->original.contexts,p,sizeof(*p))||
    (uintptr_t)p->controller%ALIGN_OF(struct pt_editor_mixed_causal_prepare)||
    !apart(p,sizeof(*p),p->joint,sizeof(*p->joint))||
    s->preparation!=&p->preparation||s->original.preparation!=&p->preparation.original||s->original.immutable_count!=6||s->original.mutable_count!=2||
    s->original.causal_workspace.data!=s->original.immutable[0].data||s->original.causal_workspace.bytes!=s->original.immutable[0].bytes||
    s->original.factory_workspace.data!=s->original.immutable[1].data||s->original.factory_workspace.bytes!=s->original.immutable[1].bytes||
    s->original.backend_parent.data!=s->original.immutable[2].data||s->original.backend_parent.bytes!=s->original.immutable[2].bytes||
    s->original.mutable[1].bytes<sizeof(*u)||!span(s->original.mutable[1].data,s->original.mutable[1].bytes)||
    (uintptr_t)s->original.mutable[1].data%ALIGN_OF(struct pt_editor_mixed_causal_song_lineage_outputs)||
    s->original.immutable[3].bytes<pt_mixed_quantized_audit_workspace_size()||
    (uintptr_t)s->original.immutable[3].data%pt_mixed_quantized_audit_workspace_alignment()||
    s->original.immutable[4].bytes<pt_mixed_plan_normalizer_workspace_size()||
    (uintptr_t)s->original.immutable[4].data%pt_mixed_plan_normalizer_workspace_alignment()||
    s->original.immutable[5].bytes<pt_mixed_plan_normalizer_workspace_size()||
    (uintptr_t)s->original.immutable[5].data%pt_mixed_plan_normalizer_workspace_alignment()||
    s->original.mutable[0].bytes<pt_mixed_quantized_audit_result_size()||
    (uintptr_t)s->original.mutable[0].data%pt_mixed_quantized_audit_result_alignment()||
    !p->configuration.external_allocator.allocate||!p->configuration.external_allocator.release||
    (p->configuration.external_allocator.context&&!inside(s->original.contexts,p->configuration.external_allocator.context,1))||
    p->configuration.preparation.binding!=s->original.binding||
    p->configuration.preparation.contexts.data!=s->original.contexts.data||
    p->configuration.preparation.contexts.bytes!=s->original.contexts.bytes||
    p->configuration.options.rate!=p->configuration.preparation.causal.grid.rate||
    p->configuration.absolute_start==UINT64_MAX)return PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_INVALID;
 u=(void *)s->original.mutable[1].data;
 /* U's original owner slots and the whole unused producer state must be zero
  * BEFORE hook adoption, so a refused begin never follows invented child values.
  * Protect incoming full outputs against actual complete source storage before
  * that zero read; no caller-provided narrow parent is exempted. */
 if(!s->original.binding||!span(s->original.binding,sizeof(*s->original.binding))||
    (uintptr_t)s->original.binding%ALIGN_OF(struct pt_editor_mixed)||
    !s->original.binding->editor||!span(s->original.binding->editor,sizeof(*s->original.binding->editor))||
    (uintptr_t)s->original.binding->editor%ALIGN_OF(struct pt_editor)||
    !s->original.binding->editor->project||!span(s->original.binding->editor->project,sizeof(*s->original.binding->editor->project))||
    (uintptr_t)s->original.binding->editor->project%ALIGN_OF(struct pt_project)||
    !apart(u,s->original.mutable[1].bytes,s->original.contexts.data,s->original.contexts.bytes)||
    !apart(u,s->original.mutable[1].bytes,s,sizeof(*s))||!apart(u,s->original.mutable[1].bytes,b,sizeof(*b))||
    !apart(u,s->original.mutable[1].bytes,s->original.binding,sizeof(*s->original.binding))||
    !apart(u,s->original.mutable[1].bytes,s->original.binding->editor,sizeof(*s->original.binding->editor))||
    !pt_sampler_output_disjoint(&s->original.binding->editor->sampler,u,s->original.mutable[1].bytes)||
    !pt_render_project_storage_output_disjoint(s->original.binding->editor->project,u,s->original.mutable[1].bytes)||
    !pt_sampler_output_disjoint(&s->original.binding->editor->sampler,p,sizeof(*p))||
    !pt_render_project_storage_output_disjoint(s->original.binding->editor->project,p,sizeof(*p)))
    return PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_INVALID;
 for(i=0;i<6;++i)if(!apart(u,s->original.mutable[1].bytes,s->original.immutable[i].data,s->original.immutable[i].bytes))
    return PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_INVALID;
 if(!apart(u,s->original.mutable[1].bytes,s->original.mutable[0].data,s->original.mutable[0].bytes)||!empty_bytes(u,sizeof(*u))||
    !empty_bytes(&p->saved,sizeof(*p)-offsetof(struct pt_editor_mixed_causal_song_lineage,saved)))
    return PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_INVALID;
 /* No external source callback has occurred. Genuine SOURCE admission performs
  * complete pair/source guards before the first owned write. */
  r=pt_editor_mixed_causal_lineage_source_begin(p->joint,s,b);
 if(r!=PT_EDITOR_MIXED_READERS_PENDING)return PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_INVALID;
 p->adopted=1;p->original=p;p->source=s;p->borrow=b;p->outputs=u;p->parent=s->original.contexts;
 memcpy(&p->saved,&p->configuration,sizeof(p->saved));
 for(i=0;i<6;++i)p->extent[i]=s->original.immutable[i];
 p->extent[6]=s->original.mutable[0];p->extent[7]=s->original.mutable[1];
 if(!add_size(&total,p->parent.bytes)||!add_size(&total,sizeof(*s))||!add_size(&total,sizeof(*b)))
    return fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CAPACITY);
 for(i=0;i<8;++i)if(!add_size(&total,p->extent[i].bytes))return fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CAPACITY);
 if(total>p->saved.ordinary_budget)return fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CAPACITY);
 p->base_bytes=p->live_bytes=total;p->allocator=(struct pt_allocator){p,allocate_external,release_external};
 p->establish_parents[0]=p->parent;
 p->establish_parents[1]=(struct pt_sampler_storage_span){s,sizeof(*s)};
 p->establish_parents[2]=(struct pt_sampler_storage_span){b,sizeof(*b)};
 for(i=0;i<8;++i)p->establish_parents[i+3]=p->extent[i];
 p->revision=s->original.binding->editor->history.revision;p->generation=s->original.binding->editor->sampler.generation;
 p->working_command=NO_COMMAND;p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_ESTABLISH_BEGIN;
 p->result=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PENDING;return p->result;
}

enum pt_editor_mixed_causal_song_lineage_result pt_editor_mixed_causal_song_lineage_step(struct pt_editor_mixed_causal_song_lineage *p,unsigned work)
{
 struct pt_editor_mixed_causal_song_lineage_outputs *u;struct pt_editor *e;
 enum pt_establish_result er;enum pt_edit_result pr;enum pt_render_result rr;
 enum pt_mixed_quantized_audit_result ar;enum pt_mixed_plan_result qr;
 enum pt_editor_mixed_readers_result cr;enum pt_mixed_readers_result mr;unsigned i,j,n;
 if(!work||work>4096)return PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_INVALID;
 if(!enter(p,0))return p&&identity(p)?p->result:PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_INVALID;
 u=p->outputs;e=p->source->original.binding->editor;
 if(p->result==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_WAIT_ACTIVE||p->result==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_WAIT_CAPACITY||
    p->phase==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PUBLICATION||p->phase==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_END)goto done;
 p->result=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PENDING;
 switch(p->phase){
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_ESTABLISH_BEGIN:
    if(!external_enter(p,0))break;
    er=pt_sampler_establish_begin(&p->establish,&e->sampler,e->project,&p->allocator,
       p->establish_parents,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PARENTS,p->revision);
    if(!external_leave(p,0))break;
    if(er!=PT_ESTABLISH_PENDING)fail(p,er==PT_ESTABLISH_CAPACITY?PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CAPACITY:PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);
    else p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_ESTABLISH_STEP;
    break;
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_ESTABLISH_STEP:
    if(!external_enter(p,0))break;
    er=pt_sampler_establish_step(&p->establish,p->revision,p->generation,work);
    if(!external_leave(p,0))break;
    if(er==PT_ESTABLISH_READY)p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PIN_BEGIN;
    else if(er!=PT_ESTABLISH_PENDING)fail(p,er==PT_ESTABLISH_CAPACITY?PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CAPACITY:PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);
    break;
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PIN_BEGIN:
    if(p->pin_slot==e->project->sample_count){p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_ESTABLISH_CLOSE;break;}
    if(p->pin_slot>=PT_PROJECT_SAMPLES||u->temporary_pin||u->persistent_pin||p->pin_job.owner){fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);break;}
    memset(&u->temporary_pcm,0,sizeof(u->temporary_pcm));memset(&u->persistent_pcm,0,sizeof(u->persistent_pcm));u->ready=0;
    if(!external_enter(p,0))break;
    pr=pt_sampler_pin_job_begin(&p->pin_job,&e->sampler,e->project,p->pin_slot,p->generation);
    if(!external_leave(p,0))break;
    if(pr!=PT_EDIT_OK)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);else p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PIN_STEP;break;
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PIN_STEP:
    if(!external_enter(p,0))break;
    u->ready=0;
    pr=pt_sampler_pin_job_step(&p->pin_job,work,&u->temporary_pcm,&u->temporary_pin,&u->ready);
    if(!external_leave(p,0))break;
    if(pr!=PT_EDIT_OK)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);
    else if(u->ready){if(!u->temporary_pin)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);else p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PIN_CURRENT;}
    break;
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PIN_CURRENT:
    if(!external_enter(p,0))break;
    pr=pt_sampler_pin_current(&e->sampler,e->project,p->pin_slot,p->generation,
       u->temporary_pin,&u->persistent_pcm,&u->persistent_pin);
    if(!external_leave(p,0))break;
    if(pr!=PT_EDIT_OK||!u->persistent_pin)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);
    else p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PIN_FINISH;
    break;
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PIN_FINISH:
    if(!u->persistent_pin||!u->temporary_pin||p->pin[p->pin_slot]){fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);break;}
    if(!external_enter(p,0))break;
    p->pin[p->pin_slot]=u->persistent_pin;u->persistent_pin=NULL;++p->pin_count;
    {struct pt_sample_version *temporary=u->temporary_pin;u->temporary_pin=NULL;
       pt_sampler_pin_job_cancel(&p->pin_job);pt_sampler_unpin(temporary);}
    if(!external_leave(p,0))break;
    ++p->pin_slot;p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PIN_BEGIN;break;
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_ESTABLISH_CLOSE:
    if(!external_enter(p,0))break;
    n=(unsigned)pt_sampler_establish_cancel(&p->establish);
    if(!external_leave(p,0))break;
    if(!n||p->establish.active)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);else p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_ACTIVATE;break;
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_ACTIVATE:
    if(p->activated||p->pin_count!=e->project->sample_count){fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);break;}
    p->preparation.original=p->saved.preparation;p->preparation.lineage=p->saved.lineage;
    cr=pt_editor_mixed_causal_lineage_source_activate(p->joint,p->borrow,&p->preparation);
    if(!current(p))break;
    if(cr!=PT_EDITOR_MIXED_READERS_PENDING)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);
    else{p->activated=1;audit_inputs(p);p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_AUDIT_BEGIN;}break;
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_AUDIT_BEGIN:
    if(!external_enter(p,0))break;
    ar=pt_mixed_quantized_audit_begin_in_workspace((void *)p->extent[3].data,p->extent[3].bytes,&p->audit_inputs,&u->audit);
    if(!external_leave(p,0))break;
    if(ar!=PT_MIXED_QAUDIT_PENDING||!u->audit)fail(p,ar==PT_MIXED_QAUDIT_CAPACITY?PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CAPACITY:PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);
    else p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_AUDIT_STEP;
    break;
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_AUDIT_STEP:
    if(!external_enter(p,0))break;
    ar=pt_mixed_quantized_audit_step(u->audit,p->revision,p->generation,small_work(work));
    if(!external_leave(p,0))break;
    if(ar==PT_MIXED_QAUDIT_READY)p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_AUDIT_GET;
    else if(ar!=PT_MIXED_QAUDIT_PENDING)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);
    break;
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_AUDIT_GET:
    if(!external_enter(p,0))break;
    ar=pt_mixed_quantized_audit_get(u->audit,p->revision,p->generation,&u->audit_report);
    if(!external_leave(p,0))break;
    if(ar!=PT_MIXED_QAUDIT_READY||u->audit_report.rewinds)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);
    else{p->audited=u->audit_report;p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_AUDIT_TAKE;}break;
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_AUDIT_TAKE:
    if(u->sequence){fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);break;}
    if(!external_enter(p,0))break;
    ar=pt_mixed_quantized_audit_take(u->audit,p->revision,p->generation,&u->sequence);
    if(!external_leave(p,0))break;
    if(ar!=PT_MIXED_QAUDIT_TAKEN||!u->sequence)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);
    else p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_AUDIT_TRANSFER_CHECK;
    break;
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_AUDIT_TRANSFER_CHECK:
    if(!external_enter(p,0))break;
    ar=pt_mixed_quantized_audit_get(u->audit,p->revision,p->generation,&u->audit_report);
    if(!external_leave(p,0))break;
    if(ar!=PT_MIXED_QAUDIT_TAKEN||u->audit_report.rewinds!=1||u->audit_report.frames!=p->audited.frames||
       u->audit_report.intervals!=p->audited.intervals)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);
    else{p->audited.rewinds=1;p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_NEXT;}break;
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CONTROLLER:
    cr=pt_editor_mixed_causal_prepare_advance_validation(p->controller,work);
    if(!current(p))break;
    if(cr==PT_EDITOR_MIXED_READERS_OPEN)p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_LOAD;
    else if(cr!=PT_EDITOR_MIXED_READERS_PENDING)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);
    break;
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_LOAD:
    if(p->lineage_index>=3||p->candidate_count!=3){fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);break;}
    memcpy(&u->normalized,p->frozen+p->lineage_index,sizeof(u->normalized));
    p->target=u->normalized.normalized.frame;
    if(!lower(p)){fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);break;}
    p->action=0;p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_BATCH_BEGIN;break;
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_NEXT:
    if(!external_enter(p,0))break;
    if(!pt_render_sequence_output_disjoint(u->sequence,&u->interval,sizeof(u->interval))){
       (void)external_leave(p,1);fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);break;}
    rr=pt_render_sequence_next(u->sequence,&u->interval);
    if(!external_leave(p,0))break;
    if(rr!=PT_RENDER_OK||u->interval.emit>1||u->interval.end>1||
       p->frames>UINT64_MAX-u->interval.frames||p->frames+u->interval.frames>=UINT64_MAX-p->saved.absolute_start){
       fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);break;}
    p->target=p->saved.absolute_start+p->frames+u->interval.frames;
    p->remaining=u->interval.frames;p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FORECAST_BEGIN;break;
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FORECAST_BEGIN:
    if(!external_enter(p,0))break;
    rr=pt_render_lookahead_begin(&p->lookahead,u->sequence);
    if(!external_leave(p,0))break;
    if(rr!=PT_RENDER_OK)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);else p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FORECAST_STEP;break;
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FORECAST_STEP:
    if(!external_enter(p,0))break;
    u->ready=0;
    if(!pt_render_sequence_output_disjoint(u->sequence,&u->plan,sizeof(u->plan))||
       !pt_render_sequence_output_disjoint(u->sequence,&u->ready,sizeof(u->ready))){
       (void)external_leave(p,1);fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);break;}
    rr=pt_render_lookahead_step(&p->lookahead,small_work(work),&u->plan,&u->ready);
    if(!external_leave(p,0))break;
    if(rr!=PT_RENDER_OK)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);
    else if(u->ready){normalizer_inputs(p);p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_Q_BEGIN;}
    break;
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_Q_BEGIN:
    if(!external_enter(p,0))break;
    qr=pt_mixed_plan_normalizer_begin_quantized_in_workspace((void *)p->extent[5].data,p->extent[5].bytes,&p->normalizer_inputs,&u->normalizer);
    if(!external_leave(p,0))break;
    if(qr!=PT_MIXED_PLAN_PENDING||!u->normalizer)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);else p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_Q_STEP;break;
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_Q_STEP:
    if(!external_enter(p,0))break;
    qr=pt_mixed_plan_normalizer_step(u->normalizer,p->revision,p->generation,small_work(work));
    if(!external_leave(p,0))break;
    if(qr==PT_MIXED_PLAN_READY)p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_Q_GET;
    else if(qr!=PT_MIXED_PLAN_PENDING)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);
    break;
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_Q_GET:
    if(!external_enter(p,0))break;
    qr=pt_mixed_plan_normalizer_get_quantized(u->normalizer,p->revision,p->generation,&u->normalized);
    if(!external_leave(p,0))break;
    if(qr!=PT_MIXED_PLAN_READY)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);else p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_Q_CLOSE;break;
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_Q_CLOSE:
    {
     struct pt_mixed_plan_quantized_batch checked;
     /* GET is repeatable while READY and authenticates the captured whole plan,
      * source and exact full batch. U.plan/U.normalized remain mutable between
      * yielded phases: never trust their bounds or supplied subset at CLOSE. */
     if(!local_apart(p,(uintptr_t)&checked,sizeof(checked))){
        fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);break;}
     if(!external_enter(p,0))break;
     qr=pt_mixed_plan_normalizer_get_quantized(u->normalizer,p->revision,p->generation,&checked);
     if(!external_leave(p,0))break;
     if(qr!=PT_MIXED_PLAN_READY||memcmp(&checked,&u->normalized,sizeof(checked))){
        fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);break;}
     if(!external_enter(p,0))break;
     n=(unsigned)pt_mixed_plan_normalizer_close(&u->normalizer);
     if(!external_leave(p,0))break;
     if(!n||u->normalizer)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);
     else if(!reduce_controls(p))fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);
     else p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_LOWER;
     break;
    }
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_LOWER:
    if(!snapshot_checked(p)){fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);break;}
    if(!u->normalized.normalized.count){
       if(!checked_empty(p))fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);
       else p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CONSUME;
       break;}
    if(p->candidate_count>=3||
       (p->candidate_count&&p->target<=p->frozen[p->candidate_count-1].normalized.frame)){
       fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);break;}
    p->candidate_pending=1;p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CONSUME;break;
  case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_BATCH_BEGIN:
    if(p->lineage_index>=3||(p->attempted_mask&(1U<<p->lineage_index))){fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);break;}
    if(p->lineage_index==1){
       if(!p->history[0].service_seen||!p->history[0].ticket||
          !pt_mixed_causal_factory_control_stop_first_current(p->controller->causal,p->history[0].ticket)){
          p->result=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_WAIT_ACTIVE;break;}
    }else if(p->lineage_index==2){
       if(!prune(p)){fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);break;}
       if(!p->history[0].detached||!p->joint->original.root_detached||!p->joint->original.root_closed||
          !p->history[1].ticket||!pt_mixed_causal_factory_control_stop_control_current(p->controller->causal,p->history[1].ticket)){
          p->result=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_WAIT_ACTIVE;break;}
    }
    {int available=pressure(p);if(available<0){fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);break;}
       if(!available){p->result=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_WAIT_CAPACITY;break;}}
    {struct {
        struct pt_editor_mixed_readers_quantized_batch trigger;
        struct pt_editor_mixed_causal_lineage_control16_batch control;
        struct pt_editor_mixed_causal_lineage_stop_batch stop;
        struct pt_editor_mixed_command_ref command;
     } local;
     if(!local_apart(p,(uintptr_t)&local,sizeof(local))){fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);break;}
     memcpy(&local.trigger,&u->batch,sizeof(local.trigger));memcpy(&local.control,&u->control,sizeof(local.control));
     memcpy(&local.stop,&u->stop,sizeof(local.stop));local.command=(struct pt_editor_mixed_command_ref){0,0};
     p->attempted_mask|=1U<<p->lineage_index;
     cr=!p->lineage_index?pt_editor_mixed_causal_prepare_batch_begin_quantized(p->controller,p->target,&local.trigger,&local.command):
        p->lineage_index==1?pt_editor_mixed_causal_lineage_control16_prepare_batch_begin(&p->joint->original,&local.control,&local.command):
        pt_editor_mixed_causal_lineage_stop_prepare_batch_begin(&p->joint->original,&local.stop,&local.command);
     /* Retain actual original issued custody before an independent outer fault.
      * The mutable U parent was admitted at begin, never passed to controller. */
     u->command=local.command;
     if(local.command.serial){i=local.command.slot;
        if(i>=PT_SAMPLER_MIXED_COMMANDS||p->command[i].reference.serial||p->history[p->lineage_index].reference.serial){
           fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);break;}
        p->command[i].reference=local.command;p->history[p->lineage_index].reference=local.command;p->working_command=i;}
    }
    if(!current(p))break;
    if(cr!=PT_EDITOR_MIXED_READERS_PENDING||p->working_command==NO_COMMAND)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);
    else p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_BATCH_STEP;
    break;
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_BATCH_STEP:
    cr=pt_editor_mixed_causal_prepare_batch_advance(p->controller,p->command[p->working_command].reference);
    if(!current(p))break;
    if(cr==PT_EDITOR_MIXED_READERS_OPEN){p->action=0;p->phase=p->lineage_index?PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_ENQUEUE:PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFERENCES;}
    else if(cr!=PT_EDITOR_MIXED_READERS_PENDING)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);
    break;
  case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFERENCES:
    if(p->lineage_index){fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);break;}
    if(p->action==u->batch.count){p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_ENQUEUE;break;}
    i=p->action++;
    {struct pt_editor_mixed_reader_ref local;unsigned track=u->batch.action[i].track;
     if(!local_apart(p,(uintptr_t)&local,sizeof(local))||track>=PT_MIXED_PLAN_RECORDS){fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);break;}
     local=(struct pt_editor_mixed_reader_ref){0,0};
     cr=pt_editor_mixed_causal_prepare_reader_reference(p->controller,p->command[p->working_command].reference,i,&local);
     u->reader=local;
     if(local.serial){j=local.slot;
        if(j>=PT_SAMPLER_MIXED_READERS||p->reader[j].reference.serial||p->track[track].serial){fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);break;}
        p->reader[j].reference=local;p->track[track]=local;}
    }
    if(!current(p))break;
    if(cr!=PT_EDITOR_MIXED_READERS_OPEN||!u->reader.serial)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);
    break;
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_ENQUEUE:
    {uint64_t local;
     if(!local_apart(p,(uintptr_t)&local,sizeof(local))){fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);break;}
     local=0;mr=pt_editor_mixed_causal_prepare_enqueue(p->controller,p->command[p->working_command].reference,&local);
     u->ticket=local;
    }
    p->scheduled_result=mr;
    if(mr==PT_MIXED_READERS_OK&&u->ticket){
       p->command[p->working_command].ticket=u->ticket;p->history[p->lineage_index].ticket=u->ticket;
       /* Actual transfer is retained even through an independent outer fault.
        * Descriptive replay state was already committed at genuine EOF; it is
        * never replaced with a predicted ACTIVE key or factory reference. */
    }
    if(!current(p))break;
    if(mr!=PT_MIXED_READERS_OK||!u->ticket)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);
    else{p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PUBLICATION;p->result=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PUBLISH;}break;
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CONSUME:
    if(!p->remaining){p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_COMMIT;break;}
    n=small_work(work);if(n>p->remaining)n=p->remaining;
    if(!external_enter(p,0))break;
    rr=pt_render_sequence_consume(u->sequence,n);
    if(!external_leave(p,0))break;
    if(rr!=PT_RENDER_OK)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);else p->remaining-=n;break;
 case PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_COMMIT:
    if(!external_enter(p,0))break;
    rr=pt_render_lookahead_commit(&p->lookahead);
    if(!external_leave(p,0))break;
    if(rr!=PT_RENDER_OK||p->intervals==UINT64_MAX){fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);break;}
    for(i=0;i<PT_MIXED_PLAN_RECORDS;++i)p->origins[i]=u->normalized.normalized.next[i];
    for(j=0;j<2;++j)for(i=0;i<PT_PROJECT_SAMPLES;++i)p->samples[j][i]|=u->normalized.normalized.samples[j][i];
    if(p->candidate_pending){
       if(p->candidate_count>=3){fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);break;}
       memcpy(p->frozen+p->candidate_count,&u->normalized,sizeof(u->normalized));
       memcpy(u->candidate+p->candidate_count,&u->normalized,sizeof(u->normalized));
       ++p->candidate_count;p->candidate_pending=0;
    }
    p->frames+=u->interval.frames;++p->intervals;p->working_command=NO_COMMAND;
    if(u->interval.end){
       if(p->frames!=p->audited.frames||p->intervals!=p->audited.intervals||
          memcmp(p->samples,p->audited.samples,sizeof(p->samples)))fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);
       else if(p->candidate_count!=3)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);
       else p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CONTROLLER;
    }else p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_NEXT;break;
 default:fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);break;
 }
done:
 p->busy=0;return p->result;
}

struct pt_editor_mixed_causal_song_lineage_status pt_editor_mixed_causal_song_lineage_get(const struct pt_editor_mixed_causal_song_lineage *p)
{
  struct pt_editor_mixed_causal_song_lineage_status s;unsigned i;
  if(!identity(p)||!local_apart(p,(uintptr_t)&s,sizeof(s)))return (struct pt_editor_mixed_causal_song_lineage_status){0};
  memset(&s,0,sizeof(s));
 s.result=p->result;s.first_error=p->first_error;s.phase=p->phase;s.scheduled_result=p->scheduled_result;
 s.consumed_frames=p->frames;s.intervals=p->intervals;s.target=p->target;s.pin_count=p->pin_count;s.allocation_requests=p->requests;
 for(i=0;i<PT_SAMPLER_MIXED_COMMANDS;++i)if(p->command[i].reference.serial)++s.command_count;
 for(i=0;i<PT_SAMPLER_MIXED_READERS;++i)if(p->reader[i].reference.serial)++s.reader_count;
 return s;
}
enum pt_mixed_readers_result pt_editor_mixed_causal_song_lineage_publish(struct pt_editor_mixed_causal_song_lineage *p)
{
 enum pt_mixed_readers_result r;struct pt_editor_mixed_causal_song_lineage_command *c;
 if(!enter(p,0))return PT_MIXED_READERS_INVALID;
 if(p->phase!=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PUBLICATION||p->working_command>=PT_SAMPLER_MIXED_COMMANDS){
    p->busy=0;return PT_MIXED_READERS_INVALID;}
 c=p->command+p->working_command;
 r=pt_editor_mixed_causal_prepare_publish(p->controller,c->reference);p->scheduled_result=r;
  if(r==PT_MIXED_READERS_OK){c->published=1;p->history[p->lineage_index].published=1;}
  else if(r!=PT_MIXED_READERS_PENDING){c->uncertain=1;p->history[p->lineage_index].uncertain=1;}
  if(!current(p)){p->busy=0;return r==PT_MIXED_READERS_OK?r:PT_MIXED_READERS_BACKEND;}
 if(r==PT_MIXED_READERS_OK){
    p->working_command=NO_COMMAND;++p->lineage_index;
    p->phase=p->lineage_index==3?PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_END:PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_LOAD;
    p->result=p->lineage_index==3?PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PUBLISHED:PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PENDING;
 }
 else if(r!=PT_MIXED_READERS_PENDING)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);
 p->busy=0;return r;
}
enum pt_mixed_readers_result pt_editor_mixed_causal_song_lineage_service_command(struct pt_editor_mixed_causal_song_lineage *p,unsigned i,unsigned cancel)
{
 enum pt_mixed_readers_result r;struct pt_editor_mixed_causal_song_lineage_history *h;
 if(i>=3||cancel>1||!enter(p,1))return PT_MIXED_READERS_INVALID;
 h=p->history+i;
 if(!h->reference.serial||!h->ticket||h->detached){p->busy=0;return PT_MIXED_READERS_INVALID;}
 /* NULL is essential: a PENDING committed C1 service really adopts/times the
  * original queue readers and is not an invented output receipt. */
 r=pt_editor_mixed_causal_prepare_service_command(p->controller,h->reference,cancel,NULL);p->scheduled_result=r;
 if(!i&&!cancel&&(r==PT_MIXED_READERS_PENDING||r==PT_MIXED_READERS_OK)&&
    p->controller->causal&&pt_mixed_causal_factory_control_stop_first_current(p->controller->causal,h->ticket))h->service_seen=1;
 if(!prune(p))fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);
 if(r!=PT_MIXED_READERS_OK&&r!=PT_MIXED_READERS_PENDING)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);
 if(!p->first_error&&(p->result==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_WAIT_CAPACITY||p->result==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_WAIT_ACTIVE))
    p->result=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PENDING;
 p->busy=0;return r;
}
enum pt_mixed_readers_result pt_editor_mixed_causal_song_lineage_service_reader(struct pt_editor_mixed_causal_song_lineage *p,unsigned i,unsigned cancel)
{
 struct pt_mixed_reader_retirement observed;enum pt_mixed_readers_result r;
 if(i>=PT_SAMPLER_MIXED_READERS||cancel>1||!enter(p,1))return PT_MIXED_READERS_INVALID;
 if(!p->reader[i].reference.serial||p->reader[i].uncertain){
    p->busy=0;return PT_MIXED_READERS_INVALID;}
  if(!local_apart(p,(uintptr_t)&observed,sizeof(observed))){fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);p->busy=0;return PT_MIXED_READERS_INVALID;}
  observed=pt_editor_mixed_causal_source_retire_original_reader(p->controller,p->borrow,p->reader[i].reference,cancel);
 r=observed.result;
 p->scheduled_result=r;
 /* Retain exact genuine domain terminal settlement even through sticky outer
  * errors. It is not valid proof, quiet, C detach, device completion or owner
  * consumption. A later explicit call on this SAME retained ref can attempt
  * real close after C detach; the private operation suppresses backend repeat.
  * A terminal error without settlement remains uncertain and is not retried. */
 if(observed.retirement_consumed)p->reader[i].retirement_consumed=1;
 else if(r!=PT_MIXED_READERS_OK&&r!=PT_MIXED_READERS_PENDING)p->reader[i].uncertain=1;
 if(!prune(p))fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);
 if(r!=PT_MIXED_READERS_OK&&r!=PT_MIXED_READERS_PENDING)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);
 if(!p->first_error&&(p->result==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_WAIT_CAPACITY||p->result==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_WAIT_ACTIVE))
    p->result=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PENDING;
 p->busy=0;return r;
}
enum pt_editor_mixed_causal_song_lineage_result pt_editor_mixed_causal_song_lineage_cancel(struct pt_editor_mixed_causal_song_lineage *p)
{
 if(!enter(p,1))return identity(p)?p->result:PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_INVALID;
 p->cancelled=1;if(!p->first_error)p->first_error=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CANCELLED;
 p->result=p->first_error;p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_DRAIN;
 pt_render_lookahead_cancel(&p->lookahead);
 (void)pt_editor_mixed_causal_prepare_cancel(p->controller);
 p->busy=0;return p->result;
}
int pt_editor_mixed_causal_song_lineage_close(struct pt_editor_mixed_causal_song_lineage *p)
{
 struct pt_editor_mixed_causal_song_lineage_outputs *u;unsigned n=0;
 if(!identity(p))return 0;
 if(p->hook_closed)return 1;
 if(!enter(p,1))return 0;
 u=p->outputs;
 p->cancelled=1;if(!p->first_error)p->first_error=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CANCELLED;
 p->result=p->first_error;p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_DRAIN;
 switch(p->cleanup_phase){
 case 0:
    pt_render_lookahead_cancel(&p->lookahead);
    (void)pt_editor_mixed_causal_prepare_cancel(p->controller);++p->cleanup_phase;break;
 case 1:
    if(u->normalizer){if(!external_enter(p,1))break;
       n=(unsigned)pt_mixed_plan_normalizer_close(&u->normalizer);
       (void)external_leave(p,1);if(!n)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);
       if(u->normalizer)break;}
    ++p->cleanup_phase;break;
 case 2:
    if(u->sequence){struct pt_render_sequence *sequence=u->sequence;
       if(!external_enter(p,1))break;
       /* Clear ORIGINAL actual slot before one void close/release callback. */
       u->sequence=NULL;p->sequence_closed=1;pt_render_sequence_close(sequence);
       (void)external_leave(p,1);}
    ++p->cleanup_phase;break;
 case 3:
    if(u->audit){if(!external_enter(p,1))break;
       (void)pt_mixed_quantized_audit_close(&u->audit);(void)external_leave(p,1);
       if(u->audit)break;}
    ++p->cleanup_phase;break;
 case 4:
    if(p->pin_job.owner){if(!external_enter(p,1))break;
       pt_sampler_pin_job_cancel(&p->pin_job);(void)external_leave(p,1);
       if(p->pin_job.owner)break;}
    if(u->temporary_pin){struct pt_sample_version *v=u->temporary_pin;
       if(!external_enter(p,1))break;
       u->temporary_pin=NULL;pt_sampler_unpin(v);
       (void)external_leave(p,1);break;}
    if(u->persistent_pin){struct pt_sample_version *v=u->persistent_pin;
       if(!external_enter(p,1))break;
       u->persistent_pin=NULL;pt_sampler_unpin(v);
       (void)external_leave(p,1);break;}
    ++p->cleanup_phase;break;
 case 5:
    if(p->establish.active){if(!external_enter(p,1))break;
       n=(unsigned)pt_sampler_establish_cancel(&p->establish);(void)external_leave(p,1);
       if(!n)fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);
       if(p->establish.active)break;}
    ++p->cleanup_phase;break;
 case 6:
    /* One explicit close may independently attempt source_close/source_quiet;
     * no producer domain proof polling or shutdown retry is performed here. */
    (void)pt_editor_mixed_causal_prepare_close(p->controller);
    if(!prune(p)){fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);break;}
    if(!registrations_empty(p)||!no_external_allocations(p)||u->normalizer||u->sequence||u->audit||
       p->establish.active||p->pin_job.owner||u->temporary_pin||u->persistent_pin||
       !pt_editor_mixed_causal_source_children_closed(p->controller,p->borrow))break;
    ++p->cleanup_phase;break;
 case 7:
    while(p->cleanup_slot<PT_PROJECT_SAMPLES&&!p->pin[p->cleanup_slot])++p->cleanup_slot;
    if(p->cleanup_slot<PT_PROJECT_SAMPLES){struct pt_sample_version *v=p->pin[p->cleanup_slot];
       if(!external_enter(p,1))break;
       p->pin[p->cleanup_slot++]=NULL;--p->pin_count;
       pt_sampler_unpin(v);(void)external_leave(p,1);break;}
    if(p->pin_count){fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);break;}
    ++p->cleanup_phase;break;
 case 8:
    if(!pt_editor_mixed_causal_source_borrow_close(p->borrow)||p->borrow->address||p->borrow->serial)break;
    p->borrow_closed=1;++p->cleanup_phase;break;
 case 9:
    if(!pt_editor_mixed_causal_prepare_close(p->controller))break;
    p->hook_closed=1;p->phase=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FINISHED;p->result=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CLOSED;n=1;break;
 default:fail(p,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);break;
 }
 p->busy=0;return n&&p->hook_closed;
}
