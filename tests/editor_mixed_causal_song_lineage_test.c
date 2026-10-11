/* Genuine three-boundary producer SOURCE fixture. Both original donor entries
 * are uncalled. No forged ACTIVE/key/receipt, live timer/device or musical PLAY. */
#include "song_lineage_fixture_support.inc"
#include "../src/editor/editor_mixed_causal_song_lineage_internal.h"

struct sl_trial;
struct sl_external {
 unsigned calls,releases,alias_releases,reenter_at;
 void *alias,*pointer[PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REQUESTS];
 size_t bytes[PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REQUESTS];
 unsigned live[PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REQUESTS];
};
struct sl_port {
 struct sl_cc_port model;
 struct sl_trial *owner;
 struct pt_mixed_causal_completed_value root,control;
 struct pt_mixed_causal_command_identity third;
 unsigned control_completed,control_publications,third_publications,binds;
 unsigned hold_control,hold_third,task_hook;
 int control_raw,third_raw,quiet_raw;
};
struct sl_trial {
 struct cp_trial base; /* Original allocator/resource helpers, old control unused. */
 struct pt_editor_mixed_causal_lineage_source_prepare joint;
 struct pt_editor_mixed_causal_song_lineage producer;
 struct sl_external external;
 struct sl_port port;
 struct pt_editor_mixed_causal_lineage_source_inputs *source;
 struct pt_editor_mixed_causal_source_borrow *borrow;
 struct pt_editor_mixed_causal_song_lineage_outputs *outputs;
 void *audit_storage,*audit_q,*q_storage,*audit_result;
 size_t audit_capacity,audit_q_capacity,q_capacity,result_capacity,output_capacity;
 int32_t *original_mono,*original_stereo,*spare_values;
 uint32_t *original_slices;unsigned spare_slot;
 struct pt_sample *real_samples;struct pt_event *real_events;
 unsigned promoting,master_observed,master_calls_at_begin,closed,kind;
 unsigned outer_enqueue_fault,outer_publish_fault;
 unsigned char tail[CS_TAIL];
 unsigned char overlap_guard[sizeof(struct pt_editor_mixed_causal_song_lineage_outputs)+CS_TAIL];
};
static unsigned sl_cases;
static struct pt_editor_mixed_causal_prepare *sl_s(struct sl_trial *e)
{return &e->joint.original.original;}
static void sl_reenter(struct sl_trial *e)
{
 uint32_t revision=e->base.editor->history.revision,generation=e->base.editor->sampler.generation;
 assert(pt_editor_mixed_causal_song_lineage_step(&e->producer,1)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);
 assert(!pt_editor_prepare_change(e->base.editor)&&!pt_editor_dispose(e->base.editor));
 assert(revision==e->base.editor->history.revision&&generation==e->base.editor->sampler.generation);
}
static void *sl_allocate(void *context,size_t n)
{
 struct cp_memory *m=context;struct sl_trial *e=(void *)m->owner;void *p;
 assert(e->base.binding->preparation_context==sl_s(e)&&e->base.binding->preparation_close);
 p=cp_new(m,n);return p;
}
static void sl_release(void *context,void *p)
{
 struct cp_memory *m=context;struct sl_trial *e=(void *)m->owner;unsigned i;
 for(i=0;i<PT_EDITOR_MIXED_READERS_ORDINARY;++i)assert(sl_s(e)->ordinary[i].data!=p);
 cp_free(m,p,0,0);
}
static void *sl_chip_new(void *context,size_t n)
{return cp_new(context,n);}
static void sl_chip_free(void *context,void *p,size_t n)
{
 struct cp_memory *m=context;struct sl_trial *e=(void *)m->owner;unsigned i;
 for(i=0;i<PT_EDITOR_MIXED_READERS_CHIP;++i)assert(sl_s(e)->chip[i].data!=p);
 cp_free(m,p,n,1);
}
static void *sl_master_new(void *context,size_t bytes)
{
 struct cp_memory *m=context;struct sl_trial *e=(void *)m->owner;
 if(e->promoting){assert(e->borrow&&e->borrow->address==sl_s(e)&&e->borrow->serial&&
    sl_s(e)->source_busy&&e->base.binding->preparation_context==sl_s(e)&&
    e->base.binding->preparation_close&&!sl_s(e)->source_activated);++e->master_observed;}
 return cp_new(m,bytes);
}
static void *sl_external_new(void *context,size_t bytes)
{
 struct sl_external *m=context;struct sl_trial *e=(void *)((char *)context- offsetof(struct sl_trial,external));
 struct cp_trial *f=&e->base;unsigned index=m->calls++,i;
 assert(index<PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REQUESTS&&bytes&&e->producer.busy&&
    sl_s(e)->source_busy&&!sl_s(e)->busy&&e->borrow->address==sl_s(e)&&
    f->binding->preparation_context==sl_s(e)&&f->binding->preparation_close);
 if(index){assert(sl_s(e)->source_activated&&e->producer.pin_count==f->editor->project->sample_count);
    for(i=0;i<f->editor->project->sample_count;++i)assert(e->producer.pin[i]&&e->producer.pin[i]==f->editor->sampler.current[i]);
 }else assert(!sl_s(e)->source_activated&&!sl_s(e)->causal&&!sl_s(e)->pool);
 if(m->reenter_at==index+1)sl_reenter(e);
 if(m->alias)return m->alias;
 m->pointer[index]=malloc(bytes);assert(m->pointer[index]);m->bytes[index]=bytes;m->live[index]=1;return m->pointer[index];
}
static void sl_external_release(void *context,void *pointer)
{
 struct sl_external *m=context;struct sl_trial *e=(void *)((char *)context- offsetof(struct sl_trial,external));unsigned i,j;
 assert(e->producer.busy&&sl_s(e)->source_busy&&!sl_s(e)->busy);
 if(pointer==m->alias){++m->alias_releases;return;}
 for(i=0;i<m->calls&&m->pointer[i]!=pointer;++i){}assert(i<m->calls&&m->live[i]);
 for(j=0;j<e->producer.requests;++j)if(e->producer.allocation[j].address==(uintptr_t)pointer)assert(!e->producer.allocation[j].live);
 m->live[i]=0;++m->releases;
 /* Original donor quarantine preserves numeric retired extents until teardown. */
}
static int sl_owned(void *context)
{
 struct cp_bus *bus=context;struct cp_trial *f=bus->owner;struct sl_trial *e=(void *)f;
 int r=f->card->healthy==1&&f->library.library&&f->library.owner==&f->card->reservation&&
    f->library.acquired==PT_AMIGUS_WAVETABLE&&f->card->reservation.reserved;
 if(e->outer_enqueue_fault&&e->producer.phase==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_ENQUEUE){
    assert(r&&e->producer.busy&&sl_s(e)->busy);e->outer_enqueue_fault=0;++e->producer.configuration.options.tick_limit;}
 return r;
}
static int sl_write(void *context,unsigned reg,uint32_t value)
{
 struct cp_bus *bus=context;struct cp_trial *f=bus->owner;struct fixture *c=f->card;unsigned i;
 assert(sl_owned(context)&&c->reservation.access);++c->writes;
 if(reg==0x14)c->address=value;
 else{assert(reg==0x10&&!(c->address&3)&&c->address<=sizeof(c->ram)-4);
    for(i=0;i<4;++i)c->ram[c->address+i]=(uint8_t)(value>>(24-8*i));}
 return 1;
}
static int sl_bind(void *context,const struct pt_mixed_causal_registration *registration)
{
 struct sl_port *p=context;struct sl_trial *e=p->owner;struct pt_editor_mixed_causal_prepare *s=sl_s(e);
 assert(e->producer.phase==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CONTROLLER&&e->producer.busy&&
    e->producer.candidate_count==3&&!e->producer.candidate_pending&&e->producer.frames==2880&&
    e->producer.intervals==4&&e->producer.audited.rewinds==1&&e->producer.activated&&
    e->outputs->sequence&&e->outputs->audit&&!e->outputs->normalizer&&
    !memcmp(e->outputs->candidate,e->producer.frozen,sizeof(e->outputs->candidate))&&
    e->producer.pin_count==e->base.editor->project->sample_count&&e->external.calls==3);
 assert(++p->binds==1&&e->base.ordinary.calls==2&&cp_live(&e->base.ordinary)==2);
 assert(s->causal&&s->queue&&!s->pool&&registration->owner==s->causal&&registration->queue==s->queue);
 assert(registration->session==31&&registration->generation==17);
 assert(!pt_mixed_readers_commands_held(s->queue)&&!pt_mixed_readers_readers_held(s->queue));
 p->model.base.registration=*registration;return 1;
}
static int sl_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{struct sl_port *p=context;return ct_clock(&p->model.base,ticks,frequency);}
static int sl_publish(void *context,struct pt_mixed_causal_owner *owner,
 const struct pt_mixed_causal_command_identity *id,const struct pt_mixed_causal_packet *packet)
{
 struct sl_port *p=context;int r=ct_publish(&p->model.base,owner,id,packet);
 if(p->owner->outer_publish_fault){assert(r);p->owner->outer_publish_fault=0;++p->owner->producer.configuration.options.tick_limit;}
 return r;
}
static int sl_publish_successor(void *context,struct pt_mixed_causal_owner *owner,const struct pt_mixed_causal_publication *d)
{struct sl_port *p=context;return ct_publish_successor(&p->model.base,owner,d);}
static int sl_completed_same(const struct pt_mixed_causal_completed_value *a,
    const struct pt_mixed_causal_completed_value *b)
{
    unsigned i;
    if(!sl_cc_identity(&a->identity,&b->identity)||a->frame!=b->frame||a->first_tick!=b->first_tick||
       a->last_tick!=b->last_tick||a->observed!=b->observed||a->issued!=b->issued||
       a->post.active_mask!=b->post.active_mask||a->post.adopted_mask!=b->post.adopted_mask)return 0;
    for(i=0;i<20;++i)if(!keys_equal(a->post.slot+i,b->post.slot+i))return 0;
    return 1;
}
static void sl_capture(struct pt_mixed_causal_completed_value *out,
    const struct pt_mixed_causal_command_identity *id,const struct pt_mixed_causal_packet *packet,
    uint64_t observed,uint64_t issued,const struct pt_mixed_causal_actual *actual)
{
    memset(out,0,sizeof(*out));out->identity=*id;out->frame=packet->frame;
    out->first_tick=packet->first;out->last_tick=packet->last;out->observed=observed;out->issued=issued;out->post=*actual;
}
static int sl_publish_control(void *context,struct pt_mixed_causal_owner *owner,
    const struct pt_mixed_causal_control_publication *d)
{
    struct sl_port *p=context;struct sl_cc_port *m=&p->model;unsigned i;int raw=p->control_raw;
    ++p->control_publications;
    if(!m->first_completed||owner!=m->base.registration.owner||d->serial!=1||
       !sl_cc_identity(&d->predecessor,&p->root.identity)||
       d->first_tick!=p->root.first_tick||d->last_tick!=p->root.last_tick||
       d->observed!=p->root.observed||d->issued!=p->root.issued||
       d->first_post.active_mask!=m->base.mask||d->first_post.adopted_mask!=m->base.mask||
       !ct_expected(&m->base,&d->packet))return 0;
    for(i=0;i<20;++i)if(!keys_equal(d->first_post.slot+i,p->root.post.slot+i)||
        !keys_equal(d->first_post.slot+i,m->base.slot+i))return 0;
    for(i=0;i<d->packet.count;++i)if(d->packet.action[i].kind!=PT_MIXED_READERS_CONTROL||
        d->packet.key[i].trigger!=p->root.identity.ticket||!ct_reader(&m->base,d->packet.key+i))return 0;
    if(raw&&!ct_publish(&m->base,owner,&d->successor,&d->packet))return 0;
    if(p->task_hook){p->task_hook=0;sl_reenter(p->owner);}return raw;
}
static int sl_publish_stop(void *context,struct pt_mixed_causal_owner *owner,
    const struct pt_mixed_causal_control_stop_publication *d)
{
    struct sl_port *p=context;struct sl_cc_port *m=&p->model;unsigned i;int raw=p->third_raw;
    ++p->third_publications;
    if(!m->first_completed||!p->control_completed||owner!=m->base.registration.owner||d->serial!=2||
       !sl_completed_same(&d->root,&p->root)||!sl_completed_same(&d->control,&p->control)||
       d->control.identity.ticket==d->root.identity.ticket||d->control.frame<=d->root.frame||
       d->packet.frame<=d->control.frame||!ct_expected(&m->base,&d->packet)||
       d->control.post.active_mask!=m->base.mask||d->control.post.adopted_mask!=m->base.mask)return 0;
    for(i=0;i<20;++i)if(!keys_equal(d->control.post.slot+i,m->base.slot+i))return 0;
    for(i=0;i<d->packet.count;++i){const struct pt_mixed_readers_action *a=d->packet.action+i;
       int zero=a->route==PT_MIXED_READERS_PAULA?
          !a->geometry.paula.data&&!a->geometry.paula.words&&!a->geometry.paula.period&&!a->geometry.paula.volume:
          !a->geometry.amigus.start&&!a->geometry.amigus.loop&&!a->geometry.amigus.end_exclusive&&
          !a->geometry.amigus.rate&&!a->geometry.amigus.control&&!a->geometry.amigus.left&&!a->geometry.amigus.right;
       if(a->kind!=PT_MIXED_READERS_STOP||!zero||d->packet.key[i].trigger!=p->root.identity.ticket||
          !ct_reader(&m->base,d->packet.key+i))return 0;
    }
    if(raw&&!ct_publish(&m->base,owner,&d->stop,&d->packet))return 0;
    if(raw)p->third=d->stop;
    if(p->task_hook){p->task_hook=0;sl_reenter(p->owner);}return raw;
}
static int sl_commit(void *context,const struct pt_mixed_causal_packet *packet,struct pt_mixed_causal_actual *actual)
{
    struct sl_port *p=context;struct ct_command *command=ct_command(&p->model.base,packet->ticket);
    struct pt_mixed_causal_command_identity id={0};uint64_t before=p->model.base.ticks;unsigned i,matched=1;int raw;
    if(command)id=command->identity;
    if(packet->action[0].kind==PT_MIXED_READERS_STOP){if(!p->control_completed)return 0;return sl_ss_commit(&p->model,packet,actual);}
    raw=sl_cc_commit(&p->model,packet,actual);
    if(raw==1&&before>=packet->first&&p->model.base.ticks<packet->last&&
       actual->active_mask==p->model.base.mask&&actual->adopted_mask==p->model.base.mask){
       for(i=0;i<20;++i)if(!keys_equal(actual->slot+i,p->model.base.slot+i))matched=0;
       if(matched){if(packet->action[0].kind==PT_MIXED_READERS_TRIGGER)sl_capture(&p->root,&id,packet,before,p->model.base.ticks,actual);
          else{sl_capture(&p->control,&id,packet,before,p->model.base.ticks,actual);p->control_completed=1;}}
    }
    return raw;
}
static int sl_command_quiet(void *context,const struct pt_mixed_causal_command_identity *id,unsigned cancel)
{
    struct sl_port *p=context;
    if(!cancel&&((p->hold_control&&id->ticket==p->control.identity.ticket)||
       (p->hold_third&&id->ticket==p->third.ticket))){++p->model.base.command_proofs;return 0;}
    return sl_cc_command_quiet(&p->model,id,cancel);
}
static int sl_reader_quiet(void *context,const struct pt_mixed_causal_reader_identity *id,unsigned cancel)
{struct sl_port *p=context;return sl_ss_reader_quiet(&p->model,id,cancel);}
static int sl_source_close(void *context,const struct pt_mixed_causal_registration *id)
{struct sl_port *p=context;return ct_source_close(&p->model.base,id);}
static int sl_source_quiet(void *context,const struct pt_mixed_causal_registration *id)
{
    struct sl_port *p=context;
    if(p->quiet_raw!=1){++p->model.base.probes;assert(ct_registration(&p->model.base.registration,id)&&ct_empty(&p->model.base));return p->quiet_raw;}
    return ct_source_quiet(&p->model.base,id);
}


static void sl_fill_tail(void *p,size_t used,size_t capacity,unsigned byte)
{assert(capacity>=used);memset((unsigned char *)p+used,(int)byte,capacity-used);}
static void sl_tail_same(const void *p,size_t used,size_t capacity,unsigned byte)
{size_t i;const unsigned char *v=p;for(i=used;i<capacity;++i)assert(v[i]==byte);}
static struct sl_trial *sl_make(unsigned bits,unsigned cache_bits,unsigned little,unsigned which,unsigned lead)
{
 struct sl_trial *e=calloc(1,sizeof(*e));struct cp_trial *f;struct pt_allocator a;
 struct pt_project *p;struct pt_amigus_reservation_api api;unsigned i,j;
 assert(e&&offsetof(struct sl_trial,base)==0);f=&e->base;f->bits=bits;f->cache_bits=cache_bits;f->little=little;e->kind=which;
 f->card=calloc(1,sizeof(*f->card));f->document=calloc(1,sizeof(*f->document));
 f->editor=calloc(1,sizeof(*f->editor));f->binding=calloc(1,sizeof(*f->binding));
 assert(f->card&&f->document&&f->editor&&f->binding);f->bus.owner=f;f->ordinary.owner=f;f->masters.owner=f;f->chip.owner=f;e->port.owner=e;e->port.control_raw=e->port.third_raw=e->port.quiet_raw=1;
 a=(struct pt_allocator){&f->masters,sl_master_new,cp_master_free};
 pt_document_init(f->document,&a);assert(pt_document_new(f->document,16,4*1024*1024)==PT_PROJECT_OK);
 p=&f->document->project;e->real_events=p->events;
 for(i=0;i<16;++i){p->channels.track[i].route=(which==CS_CARD16)?PT_AMIGUS:(i<4?PT_PAULA:PT_AMIGUS);
    /* Preserve genuine classic Paula's exact one-side gains. Center only the
     * card tracks; Paula's checked adapter must refuse a nonzero other side. */
    if(p->channels.track[i].route==PT_AMIGUS)p->channels.track[i].pan=128;}
 e->original_mono=cp_new(&f->masters,CS_MONO_CAPACITY*sizeof(int32_t));
 e->original_stereo=cp_new(&f->masters,64*sizeof(int32_t));
 e->original_slices=cp_new(&f->masters,2*sizeof(uint32_t));
 for(i=0;i<CS_MONO_CAPACITY;++i)e->original_mono[i]=(i&1U)?-(int32_t)(1U<<(bits-8)):(int32_t)(1U<<(bits-8));
 if(bits==24){e->original_mono[2]=257;e->original_mono[3]=-257;}
 for(j=0;j<64;++j)e->original_stereo[j]=(int32_t)((int)(j%12)-6)*(int32_t)(1U<<(bits-8));
 e->original_slices[0]=1;e->original_slices[1]=5;
 p->samples[0].pcm=(struct pt_pcm){e->original_mono,CS_MONO_CAPACITY,CS_MONO_FRAMES,8000,1,(uint8_t)bits};
 p->samples[0].volume=64;
 p->samples[1].pcm=(struct pt_pcm){e->original_stereo,64,6,8000,2,(uint8_t)bits};p->samples[1].volume=64;
 p->samples[1].slices=e->original_slices;p->samples[1].slice_count=2;
 memset(p->events,0,64*16*sizeof(*p->events));p->speed=1;p->bpm=125;
 for(i=0;i<16;++i){p->events[i]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    p->events[16+i].effect=12;p->events[16+i].parameter=32;
    p->events[32+i].kind=PT_NOTE_OFF;}
 p->events[48+15].effect=15;
 assert(pt_editor_init(f->editor,p));pt_sampler_init(&f->editor->sampler,&a,4*1024*1024);
 {struct pt_pcm spare;int32_t *values=cp_new(&f->masters,64*sizeof(int32_t));
    e->spare_slot=p->sample_count;e->spare_values=values;
    for(i=0;i<64;++i)values[i]=(int32_t)i;
    spare=(struct pt_pcm){values,64,1,8000,1,24};
    assert(pt_sampler_append_owned(&f->editor->sampler,p,&f->editor->history,&spare,&a,"unused spare")==PT_EDIT_OK&& !spare.data);}
 e->real_samples=p->samples;assert(pt_editor_mixed_attach(f->binding,f->editor));
 f->library.available=f->library.supported=f->library.count=1;f->card->healthy=1;
 api=(struct pt_amigus_reservation_api){&f->library,open_library,close_library,find,supported,reserve,release};
 assert(pt_amigus_reservation_open_resource(&f->card->reservation,&api,0,PT_AMIGUS_WAVETABLE)==PT_AMIGUS_RESERVED);
 assert(pt_amigus_wavetable_cache_attach(&f->card->cache,&f->card->reservation,0,4096,4096,&f->bus,sl_owned,sl_write));
 f->input.binding=f->binding;f->input.backend=&f->card->cache;
 f->input.causal.allocator=(struct pt_allocator){&f->ordinary,sl_allocate,sl_release};
 f->input.causal.allocator_context=(struct pt_mixed_readers_span){&f->ordinary,sizeof(f->ordinary)};
 f->input.causal.grid=(struct pt_mixed_readers_grid){100,17,709379,48000};f->input.causal.session=31;
 f->input.causal.control_budget=pt_mixed_causal_control_size();f->input.causal.queue_budget=pt_mixed_readers_control_size();
 f->input.causal.port=(struct pt_mixed_causal_port){&e->port,sizeof(e->port),PT_MIXED_CAUSAL_PORT_VERSION,
    PT_MIXED_CAUSAL_PORT_REQUIRED,sl_clock,sl_publish,sl_commit,sl_command_quiet,sl_reader_quiet,
    sl_source_close,sl_source_quiet,sl_publish_successor};
 f->input.bind_original=sl_bind;e->port.model.base.ticks=100;e->port.model.base.frequency=709379;
 e->port.model.base.commit_raw=e->port.model.base.source_raw=1;
 f->input.chip_context=&f->chip;f->input.chip_allocate=sl_chip_new;f->input.chip_release=sl_chip_free;
 f->input.factory_budget=pt_sampler_mixed_pool_size()+2*pt_sampler_mixed_command_size()+32*pt_sampler_mixed_reader_size();
 f->input.chip_budget=16384;f->input.contexts=(struct pt_sampler_storage_span){e,sizeof(*e)};
 f->causal_capacity=pt_mixed_causal_workspace_size()+CS_TAIL;f->factory_capacity=pt_sampler_mixed_workspace_size()+CS_TAIL;
 /* E/F constructors may use or clear their entire advertised scratch. Keep
  * those exact query+CS_TAIL capacities/spans and inside-tail alias guards.
  * Additional fixture-only backing is an external overrun guard, outside the
  * borrowed writable workspace; it is not a narrowed source parent. Fresh
  * advertised E/F scratch stays completely zero before genuine admission. */
 f->causal_workspace=calloc(1,f->causal_capacity+CS_TAIL);
 f->factory_workspace=calloc(1,f->factory_capacity+CS_TAIL);
 assert(f->causal_workspace&&f->factory_workspace);
 f->input.causal_workspace=f->causal_workspace;f->input.causal_capacity=f->causal_capacity;
 f->input.factory_workspace=f->factory_workspace;f->input.factory_capacity=f->factory_capacity;
 e->source=calloc(1,sizeof(*e->source));e->borrow=calloc(1,sizeof(*e->borrow));
 e->audit_capacity=pt_mixed_quantized_audit_workspace_size()+CS_TAIL;
 e->audit_q_capacity=pt_mixed_plan_normalizer_workspace_size()+CS_TAIL;e->q_capacity=e->audit_q_capacity;
 e->result_capacity=pt_mixed_quantized_audit_result_size()+CS_TAIL;e->output_capacity=sizeof(*e->outputs)+CS_TAIL;
 e->audit_storage=calloc(1,e->audit_capacity);e->audit_q=calloc(1,e->audit_q_capacity);
 e->q_storage=calloc(1,e->q_capacity);e->audit_result=calloc(1,e->result_capacity);e->outputs=calloc(1,e->output_capacity);
 assert(e->source&&e->borrow&&e->audit_storage&&e->audit_q&&e->q_storage&&e->audit_result&&e->outputs);
 e->source->original.binding=f->binding;e->source->original.preparation=&e->producer.preparation.original;e->source->preparation=&e->producer.preparation;e->source->original.contexts=f->input.contexts;
 e->source->original.immutable_count=6;e->source->original.mutable_count=2;
 e->source->original.immutable[0]=(struct pt_sampler_storage_span){f->causal_workspace,f->causal_capacity};
 e->source->original.immutable[1]=(struct pt_sampler_storage_span){f->factory_workspace,f->factory_capacity};
 e->source->original.immutable[2]=(struct pt_sampler_storage_span){f->card,sizeof(*f->card)};
 e->source->original.immutable[3]=(struct pt_sampler_storage_span){e->audit_storage,e->audit_capacity};
 e->source->original.immutable[4]=(struct pt_sampler_storage_span){e->audit_q,e->audit_q_capacity};
 e->source->original.immutable[5]=(struct pt_sampler_storage_span){e->q_storage,e->q_capacity};
 e->source->original.mutable[0]=(struct pt_sampler_storage_span){e->audit_result,e->result_capacity};
 e->source->original.mutable[1]=(struct pt_sampler_storage_span){e->outputs,e->output_capacity};
 e->source->original.causal_workspace=e->source->original.immutable[0];e->source->original.factory_workspace=e->source->original.immutable[1];
 e->source->original.backend_parent=e->source->original.immutable[2];e->producer.controller=sl_s(e);e->producer.joint=&e->joint;
 e->producer.configuration.preparation=f->input;
 e->producer.configuration.lineage=(struct pt_mixed_causal_control_stop_port){&e->port,sizeof(e->port),
    PT_MIXED_CAUSAL_CONTROL_STOP_VERSION,PT_MIXED_CAUSAL_CONTROL_STOP_REQUIRED,sl_publish_control,sl_publish_stop};
 e->producer.configuration.options=(struct pt_render_options){0};
 e->producer.configuration.options.rate=48000;e->producer.configuration.options.bits=24;
 e->producer.configuration.options.gain_q16=65536;e->producer.configuration.options.tracks=0xffff;
 e->producer.configuration.options.tick_limit=1000;e->producer.configuration.options.frame_limit=100000;
 e->producer.configuration.options.include_lead_in=(uint8_t)lead;
 e->producer.configuration.caps=(struct pt_paula_render_caps){3546895,124,65535};
 e->producer.configuration.format.bits=(uint8_t)cache_bits;e->producer.configuration.format.little_endian=(uint8_t)little;
 e->producer.configuration.external_allocator=(struct pt_allocator){&e->external,sl_external_new,sl_external_release};
 e->producer.configuration.ordinary_budget=4*1024*1024;
 e->producer.configuration.audit_budget=e->audit_capacity+e->audit_q_capacity+e->result_capacity+
    pt_render_sequence_setup_control_size()+pt_render_sequence_control_size();
 e->producer.configuration.absolute_start=960;
 sl_fill_tail(f->causal_workspace,f->causal_capacity,f->causal_capacity+CS_TAIL,0xa1);
 sl_fill_tail(f->factory_workspace,f->factory_capacity,f->factory_capacity+CS_TAIL,0xa2);
 sl_fill_tail(e->audit_storage,pt_mixed_quantized_audit_workspace_size(),e->audit_capacity,0xa3);
 sl_fill_tail(e->audit_q,pt_mixed_plan_normalizer_workspace_size(),e->audit_q_capacity,0xa4);
 sl_fill_tail(e->q_storage,pt_mixed_plan_normalizer_workspace_size(),e->q_capacity,0xa5);
 sl_fill_tail(e->audit_result,pt_mixed_quantized_audit_result_size(),e->result_capacity,0xa6);
 sl_fill_tail(e->outputs,sizeof(*e->outputs),e->output_capacity,0xa7);memset(e->tail,0xc1,sizeof(e->tail));
 memset(e->overlap_guard,0xc2,sizeof(e->overlap_guard));
 f->saved=cp_save(f,&f->saved_bytes);assert(pt_project_validate(p,NULL)==PT_PROJECT_OK);
 e->master_calls_at_begin=f->masters.calls;++sl_cases;return e;
}

static void sl_resave(struct sl_trial *e)
{free(e->base.saved);e->base.saved=cp_save(&e->base,&e->base.saved_bytes);}
static void sl_same(struct sl_trial *e)
{
 struct cp_trial *f=&e->base;struct pt_project *p=f->editor->project;size_t n;uint8_t *saved;unsigned i;
 p->channels.selected=0;saved=cp_save(f,&n);assert(n==f->saved_bytes&&!memcmp(saved,f->saved,n));free(saved);
 assert(p->samples[0].pcm.bits==f->bits&&p->samples[0].pcm.frames==CS_MONO_FRAMES&&p->samples[0].pcm.channels==1);
 for(i=0;i<CS_MONO_FRAMES;++i)assert(p->samples[0].pcm.data[i]==e->original_mono[i]);
 for(i=0;i<CS_MONO_CAPACITY;++i){int32_t literal=(i&1U)?-(int32_t)(1U<<(f->bits-8)):(int32_t)(1U<<(f->bits-8));
    if(f->bits==24&&i==2)literal=257;else if(f->bits==24&&i==3)literal=-257;
    assert(e->original_mono[i]==literal);}
 assert(p->samples[1].pcm.bits==f->bits&&p->samples[1].pcm.frames==6&&p->samples[1].pcm.channels==2&&
    !memcmp(p->samples[1].pcm.data,e->original_stereo,12*sizeof(int32_t))&&
    p->samples[1].slice_count==2&&p->samples[1].slices[0]==1&&p->samples[1].slices[1]==5);
 for(i=0;i<64;++i)assert(e->original_stereo[i]==(int32_t)((int)(i%12)-6)*(int32_t)(1U<<(f->bits-8)));
 assert(e->original_slices[0]==1&&e->original_slices[1]==5);
 assert(p->samples[e->spare_slot].pcm.data==e->spare_values&&p->samples[e->spare_slot].pcm.capacity==64&&
    p->samples[e->spare_slot].pcm.bits==24&&p->samples[e->spare_slot].pcm.frames==1);
 for(i=0;i<64;++i)assert(e->spare_values[i]==(int32_t)i);
 sl_tail_same(f->causal_workspace,f->causal_capacity,f->causal_capacity+CS_TAIL,0xa1);
 sl_tail_same(f->factory_workspace,f->factory_capacity,f->factory_capacity+CS_TAIL,0xa2);
 sl_tail_same(e->audit_storage,pt_mixed_quantized_audit_workspace_size(),e->audit_capacity,0xa3);
 sl_tail_same(e->audit_q,pt_mixed_plan_normalizer_workspace_size(),e->audit_q_capacity,0xa4);
 sl_tail_same(e->q_storage,pt_mixed_plan_normalizer_workspace_size(),e->q_capacity,0xa5);
 sl_tail_same(e->audit_result,pt_mixed_quantized_audit_result_size(),e->result_capacity,0xa6);
 sl_tail_same(e->outputs,sizeof(*e->outputs),e->output_capacity,0xa7);
 for(i=0;i<CS_TAIL;++i)assert(e->tail[i]==0xc1);
 sl_tail_same(e->overlap_guard,0,sizeof(e->overlap_guard),0xc2);
}


static enum pt_editor_mixed_causal_song_lineage_result sl_step(struct sl_trial *e,unsigned work)
{
 enum pt_editor_mixed_causal_song_lineage_result r;unsigned phase=e->producer.phase;
 e->promoting=phase<=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_ESTABLISH_CLOSE;
 r=pt_editor_mixed_causal_song_lineage_step(&e->producer,work);e->promoting=0;return r;
}
static void sl_begin(struct sl_trial *e)
{
 struct cp_trial *f=&e->base;unsigned masters=f->masters.calls;
 assert(offsetof(struct sl_trial,base)==0&&sizeof(*e)>sizeof(e->joint)+sizeof(e->producer));
 assert(!f->editor->sampler.current[0]&&!f->editor->sampler.current[1]&&masters==e->master_calls_at_begin);
 assert(pt_editor_mixed_causal_song_lineage_begin(&e->producer,e->source,e->borrow)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PENDING);
 assert(e->borrow->address==sl_s(e)&&e->borrow->serial&&e->producer.original==&e->producer&&
    f->binding->preparation_context==sl_s(e)&&f->binding->preparation_close&&!sl_s(e)->inputs&&
    !sl_s(e)->source_activated&&e->producer.adopted);
 assert(masters==f->masters.calls&&!f->ordinary.calls&&!e->external.calls&&!f->chip.calls&&!f->card->writes&&
    !e->port.binds&&!e->port.model.base.clocks&&!e->port.model.base.publications);
 assert(pt_editor_mixed_causal_song_lineage_step(&e->producer,0)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_INVALID);
 assert(pt_editor_mixed_causal_song_lineage_step(&e->producer,4097)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_INVALID);
}
static void sl_until(struct sl_trial *e,enum pt_editor_mixed_causal_song_lineage_phase wanted)
{
 unsigned n=0;enum pt_editor_mixed_causal_song_lineage_result r;
 while(e->producer.phase!=wanted){r=sl_step(e,17);assert(++n<100000);
    assert(r==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PENDING||
       (r==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PUBLISH&&wanted==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PUBLICATION));}
}
static void sl_preflight(struct sl_trial *e)
{
 unsigned n=0,nonempty=0,i;sl_begin(e);
 while(e->producer.phase!=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CONTROLLER){
    if(e->producer.phase==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_LOWER&&e->outputs->normalized.normalized.count){
       static const unsigned kind[3]={PT_MIXED_PLAN_TRIGGER,PT_MIXED_PLAN_CONTROL,PT_MIXED_PLAN_STOP};
       const struct pt_mixed_plan_quantized_batch *b=&e->outputs->normalized;
       assert(nonempty<3&&b->normalized.frame==960U*(nonempty+1)&&e->producer.target==b->normalized.frame);
       for(i=0;i<b->normalized.count;++i)assert(b->normalized.record[i].kind==kind[nonempty]);++nonempty;
    }
    assert(sl_step(e,17)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PENDING&&++n<100000);
    assert(!e->base.ordinary.calls&&!e->port.binds&&!e->port.model.base.clocks&&!e->port.model.base.publications&&
       !e->base.chip.calls&&!e->base.card->writes&&!sl_s(e)->causal&&!sl_s(e)->pool);
 }
 assert(nonempty==3&&e->producer.candidate_count==3&&e->producer.frames==2880&&e->producer.intervals==4&&
    e->producer.target==3840&&e->producer.audited.frames==2880&&e->producer.audited.intervals==4&&
    e->producer.audited.rewinds==1&&!memcmp(e->producer.audited.samples,e->producer.samples,sizeof(e->producer.samples))&&
    !memcmp(e->producer.frozen,e->outputs->candidate,sizeof(e->producer.frozen))&&e->external.calls==3&&
    e->producer.requests==3&&e->external.releases==2&&e->master_observed&&e->producer.activated);
 for(i=0;i<e->base.editor->project->sample_count;++i)assert(e->producer.pin[i]&&e->producer.pin[i]==e->base.editor->sampler.current[i]);
 assert(e->base.editor->sampler.allocator.context==&e->base.masters&&e->base.editor->sampler.allocator.allocate==sl_master_new&&
    e->base.editor->sampler.allocator.release==cp_master_free);sl_same(e);
}
static void sl_to_publication(struct sl_trial *e,unsigned index)
{
 unsigned n=0;assert(e->producer.lineage_index==index);
 while(e->producer.phase!=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PUBLICATION){
    enum pt_editor_mixed_causal_song_lineage_result r=sl_step(e,17);
    assert((r==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PENDING||r==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PUBLISH)&&++n<100000);}
 assert(e->producer.result==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PUBLISH&&e->producer.history[index].ticket&&
    e->producer.history[index].reference.serial&&(e->producer.attempted_mask&(1U<<index)));
}
static void sl_publish_current(struct sl_trial *e,unsigned index)
{
 assert(e->producer.lineage_index==index&&pt_editor_mixed_causal_song_lineage_publish(&e->producer)==PT_MIXED_READERS_OK);
 assert(e->producer.history[index].published&&!e->producer.history[index].uncertain&&e->producer.lineage_index==index+1);
}
static void sl_wait(struct sl_trial *e,unsigned index)
{
 unsigned n=0;unsigned char ram[sizeof(e->base.card->ram)];
 struct pt_mixed_plan_quantized_batch snapshots[3];struct pt_editor_mixed_reader_ref refs[32];
 unsigned chip=e->base.chip.calls,writes=e->base.card->writes,master=e->base.masters.calls,ordinary=e->base.ordinary.calls,i;
 memcpy(ram,e->base.card->ram,sizeof(ram));memcpy(snapshots,e->producer.frozen,sizeof(snapshots));
 for(i=0;i<32;++i)refs[i]=e->producer.reader[i].reference;
 while(e->producer.result!=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_WAIT_ACTIVE){
    assert(sl_step(e,17)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PENDING||e->producer.result==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_WAIT_ACTIVE);
    assert(++n<100000);}
 assert(e->producer.lineage_index==index&&e->producer.target==960U*(index+1)&&
    e->producer.phase==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_BATCH_BEGIN&&e->producer.attempted_mask==(1U<<index)-1&&
    ordinary==e->base.ordinary.calls&&chip==e->base.chip.calls&&writes==e->base.card->writes&&master==e->base.masters.calls&&
    !memcmp(ram,e->base.card->ram,sizeof(ram))&&!memcmp(snapshots,e->producer.frozen,sizeof(snapshots)));
 for(i=0;i<32;++i)assert(refs[i].serial==e->producer.reader[i].reference.serial&&refs[i].slot==e->producer.reader[i].reference.slot);
 assert(sl_step(e,17)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_WAIT_ACTIVE&&e->producer.target==960U*(index+1));
}
static void sl_fire(struct sl_trial *e,unsigned index)
{
 struct ct_port *p=&e->port.model.base;uint64_t ticket=e->producer.history[index].ticket;unsigned before=p->effects+e->port.model.controls+e->port.model.stops;
 assert(ticket&&e->producer.history[index].published);p->ticks=oracle(960U*(index+1))-1;
 assert(pt_mixed_causal_fire(sl_s(e)->causal,ticket)==PT_MIXED_CAUSAL_EARLY&&before==p->effects+e->port.model.controls+e->port.model.stops);
 p->ticks=oracle(960U*(index+1));assert(pt_mixed_causal_fire(sl_s(e)->causal,ticket)==PT_MIXED_CAUSAL_COMMITTED);
}
static void sl_packet(struct sl_trial *e,unsigned index)
{
 struct ct_port *p=&e->port.model.base;const struct ct_command *c=ct_command(p,e->producer.history[index].ticket);
 unsigned i,j;assert(c&&c->packet.frame==960U*(index+1)&&c->packet.first==oracle(c->packet.frame)&&
    c->packet.last==oracle(c->packet.frame+1)&&c->packet.registration.owner==sl_s(e)->causal&&
    c->packet.registration.queue==sl_s(e)->queue&&c->packet.count==e->producer.frozen[index].normalized.count);
 for(i=0;i<c->packet.count;++i){const struct pt_mixed_readers_action *a=c->packet.action+i;
    assert(a->kind==(index==0?PT_MIXED_READERS_TRIGGER:index==1?PT_MIXED_READERS_CONTROL:PT_MIXED_READERS_STOP)&&
       c->packet.key[i].route==a->route&&c->packet.key[i].slot==a->slot&&c->packet.key[i].trigger==e->producer.history[0].ticket);
    if(!index&&a->route==PT_MIXED_READERS_PAULA){assert(a->geometry.paula.words==CS_MONO_FRAMES/2);
       for(j=0;j<CS_MONO_FRAMES;++j)assert(a->geometry.paula.data[j]==(e->base.bits==24&&(j==2||j==3)?0U:(j&1U)?255U:1U));}
    if(!index&&a->route==PT_MIXED_READERS_AMIGUS){const struct pt_mixed_readers_card *card=c->packet.card+i;
       const struct pt_mixed_plan_quantized_batch *b=e->producer.frozen;
       assert(card->logical_bytes==CS_MONO_FRAMES*2&&card->full_capacity>=card->logical_bytes);
       assert(b->levels[i].mode==PT_MIXED_PLAN_LEVEL_QUANTIZED&&
          a->geometry.amigus.left==b->levels[i].left&&a->geometry.amigus.right==b->levels[i].right&&
          (a->geometry.amigus.left>255||a->geometry.amigus.right>255));
       for(j=0;j<CS_MONO_FRAMES;++j){uint16_t w=e->base.bits==24&&j==2?1U:e->base.bits==24&&j==3?65535U:(j&1U)?65280U:256U;
          assert(e->base.card->ram[card->address+2*j]==(uint8_t)(w>>8)&&e->base.card->ram[card->address+2*j+1]==(uint8_t)w);}}
    if(index==1&&a->route==PT_MIXED_READERS_AMIGUS){
       const struct pt_mixed_plan_quantized_batch *b=e->producer.frozen+1;
       const struct pt_mixed_plan_record *r=b->normalized.record+i;
       assert(a->geometry.amigus.rate==r->geometry.amigus.rate&&
          a->geometry.amigus.left==r->geometry.amigus.left&&a->geometry.amigus.right==r->geometry.amigus.right&&
          b->levels[i].mode==PT_MIXED_PLAN_LEVEL_LEGACY&&!b->levels[i].left&&!b->levels[i].right&&
          (a->geometry.amigus.left>255||a->geometry.amigus.right>255));}
 }
}
static void sl_commands(struct sl_trial *e,unsigned cancel)
{
 unsigned i;for(i=0;i<3;++i)if(e->producer.history[i].reference.serial&&e->producer.history[i].ticket&&!e->producer.history[i].detached){
    enum pt_mixed_readers_result r=pt_editor_mixed_causal_song_lineage_service_command(&e->producer,i,cancel);
    assert(r==PT_MIXED_READERS_OK||r==PT_MIXED_READERS_PENDING||r==PT_MIXED_READERS_BACKEND);}
}
static void sl_readers(struct sl_trial *e,unsigned cancel)
{
 unsigned i;for(i=0;i<32;++i)if(e->producer.reader[i].reference.serial){
    struct pt_editor_mixed_reader_ref r=e->producer.reader[i].reference;
    if(sl_s(e)->reader[r.slot].ticket){enum pt_mixed_readers_result result=pt_editor_mixed_causal_song_lineage_service_reader(&e->producer,i,cancel);
       assert(result==PT_MIXED_READERS_OK||result==PT_MIXED_READERS_PENDING||result==PT_MIXED_READERS_BACKEND);}}
}
static void sl_close(struct sl_trial *e)
{
 unsigned i,n=0;assert(pt_editor_mixed_causal_song_lineage_cancel(&e->producer)!=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_INVALID);
 e->port.model.quiet_mask=e->port.model.stopped_mask;sl_commands(e,1);sl_readers(e,1);
 while(!pt_editor_mixed_causal_song_lineage_close(&e->producer)){
    assert(++n<1000&&e->base.binding->preparation_context==sl_s(e));
    if(e->producer.borrow_closed)assert(!e->borrow->address&&!e->borrow->serial);else assert(e->borrow->address==sl_s(e));
    sl_commands(e,1);sl_readers(e,1);}
 assert(!e->outputs->audit&&!e->outputs->sequence&&!e->outputs->normalizer&&!e->producer.establish.active&&!e->producer.pin_job.owner&&
    !e->producer.pin_count&&!e->borrow->address&&!e->borrow->serial&&!e->base.binding->preparation_context&&
    !sl_s(e)->pool&&!sl_s(e)->causal&&!sl_s(e)->queue&&!e->external.alias_releases);
 for(i=0;i<32;++i)assert(!e->producer.pin[i]);for(i=0;i<e->external.calls;++i)assert(!e->external.live[i]);e->closed=1;
}
static void sl_drop(struct sl_trial *e)
{
 struct cp_trial *f=&e->base;unsigned i;
 assert((e->closed||!e->producer.adopted)&&!f->binding->preparation_context&&!e->borrow->address&&!e->borrow->serial);
 assert(pt_editor_mixed_detach(f->binding)&&pt_editor_dispose(f->editor));
 assert(pt_amigus_wavetable_cache_detach(&f->card->cache)&&pt_amigus_reservation_close(&f->card->reservation));
 cp_free(&f->masters,e->original_mono,CS_MONO_CAPACITY*sizeof(int32_t),1);
 cp_free(&f->masters,e->original_stereo,64*sizeof(int32_t),1);
 cp_free(&f->masters,e->original_slices,2*sizeof(uint32_t),1);pt_document_release(f->document);
 assert(!cp_live(&f->ordinary)&&!cp_live(&f->chip)&&!cp_live(&f->masters));
 assert(!f->ordinary.alias_releases&&!f->chip.alias_releases&&!e->external.alias_releases);
 for(i=0;i<e->external.calls;++i){assert(!e->external.live[i]);free(e->external.pointer[i]);}
 free(f->saved);free(f->causal_workspace);free(f->factory_workspace);free(f->card);free(f->document);free(f->editor);free(f->binding);
 free(e->audit_storage);free(e->audit_q);free(e->q_storage);free(e->audit_result);free(e->outputs);free(e->source);
 assert(pt_editor_mixed_causal_source_borrow_close(e->borrow));free(e->borrow);free(e);
}

static void sl_success(unsigned bits,unsigned card_only,unsigned order)
{
 struct sl_trial *e=sl_make(bits,16,0,card_only?CS_CARD16:CS_MIXED,0);struct ct_port *p=&e->port.model.base;
 struct pt_editor_mixed_reader_record readers[32];unsigned char ram[sizeof(e->base.card->ram)];
 struct pt_editor_mixed_reader_ref root_ref[32];unsigned i,ordinary,chips,masters,writes,proofs;uint64_t first,control,third;unsigned first_slot;
 sl_preflight(e);sl_to_publication(e,0);sl_publish_current(e,0);sl_packet(e,0);
 assert(pt_mixed_readers_commands_held(sl_s(e)->queue)==1&&pt_mixed_readers_readers_held(sl_s(e)->queue)==16&&!p->commits);
 first=e->producer.history[0].ticket;first_slot=e->producer.history[0].reference.slot;
 sl_wait(e,1);sl_fire(e,0);e->port.model.hold_first=1;
 assert(pt_editor_mixed_causal_song_lineage_service_command(&e->producer,0,0)==PT_MIXED_READERS_PENDING&&
    e->producer.history[0].service_seen&&!e->producer.history[0].detached);
 memcpy(readers,sl_s(e)->reader,sizeof(readers));memcpy(ram,e->base.card->ram,sizeof(ram));
 for(i=0;i<32;++i)root_ref[i]=e->producer.reader[i].reference;
 ordinary=e->base.ordinary.calls;chips=e->base.chip.calls;masters=e->base.masters.calls;writes=e->base.card->writes;
 sl_to_publication(e,1);sl_publish_current(e,1);sl_packet(e,1);control=e->producer.history[1].ticket;
 assert(control&&control!=first&&pt_mixed_readers_commands_held(sl_s(e)->queue)==2&&pt_mixed_readers_readers_held(sl_s(e)->queue)==16&&
    e->base.ordinary.calls==ordinary+1&&chips==e->base.chip.calls&&masters==e->base.masters.calls&&writes==e->base.card->writes&&
    !memcmp(readers,sl_s(e)->reader,sizeof(readers))&&!memcmp(ram,e->base.card->ram,sizeof(ram)));
 sl_wait(e,2);sl_fire(e,1);e->port.model.hold_first=0;
 assert(pt_editor_mixed_causal_song_lineage_service_command(&e->producer,0,0)==PT_MIXED_READERS_OK&&
    e->producer.history[0].detached&&e->joint.original.root_detached&&e->joint.original.root_closed&&
    !sl_s(e)->command[first_slot].handle.address&&!sl_s(e)->command[first_slot].handle.token&&pt_mixed_readers_commands_held(sl_s(e)->queue)==1);
 ordinary=e->base.ordinary.calls;sl_to_publication(e,2);sl_publish_current(e,2);sl_packet(e,2);
 third=e->producer.history[2].ticket;assert(third&&third!=first&&third!=control&&e->producer.lineage_index==3&&
    e->producer.result==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PUBLISHED&&e->producer.attempted_mask==7&&
    e->producer.history[2].reference.slot==first_slot&&e->producer.history[2].reference.serial!=e->producer.history[0].reference.serial&&
    pt_mixed_readers_commands_held(sl_s(e)->queue)==2&&pt_mixed_readers_readers_held(sl_s(e)->queue)==16&&
    e->base.ordinary.calls==ordinary+1&&chips==e->base.chip.calls&&masters==e->base.masters.calls&&writes==e->base.card->writes&&
    !memcmp(ram,e->base.card->ram,sizeof(ram)));
 for(i=0;i<32;++i)assert(root_ref[i].slot==e->producer.reader[i].reference.slot&&root_ref[i].serial==e->producer.reader[i].reference.serial);
 assert(sl_step(e,17)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PUBLISHED&&e->producer.attempted_mask==7&&e->producer.target==2880);
 sl_fire(e,2);assert(p->effects==16&&e->port.model.controls==16&&e->port.model.stops==16&&!p->mask);
 e->port.model.quiet_mask=e->port.model.stopped_mask;
 if(!order)sl_commands(e,0);proofs=p->reader_proofs;sl_readers(e,1);assert(p->reader_proofs==proofs+16);
 if(order){sl_commands(e,0);proofs=p->reader_proofs;sl_readers(e,1);assert(p->reader_proofs==proofs);}
 assert(!pt_mixed_readers_commands_held(sl_s(e)->queue)&&!pt_mixed_readers_readers_held(sl_s(e)->queue)&&ct_empty(p));
 for(i=0;i<20;++i)assert(!p->slot[i].serial);sl_same(e);sl_close(e);assert(p->shutdowns==1);sl_same(e);sl_drop(e);
}
/* Proper subsets come from real project effects, not altered normalized data. */
static void sl_success_subset(void)
{
 struct sl_trial *e=sl_make(24,16,0,CS_MIXED,0);struct pt_project *project=e->base.editor->project;
 struct ct_port *p=&e->port.model.base;struct pt_editor_mixed_reader_record before[32];
 unsigned char ram[sizeof(e->base.card->ram)];unsigned i,chip,masters,writes;
 for(i=6;i<16;++i)memset(project->events+16+i,0,sizeof(*project->events));
 for(i=0;i<16;++i)if(i!=0&&i!=5)memset(project->events+32+i,0,sizeof(*project->events));
 sl_resave(e);sl_preflight(e);
 assert(e->producer.frozen[0].normalized.count==16&&e->producer.frozen[1].normalized.count==6&&
    e->producer.frozen[2].normalized.count==2&&e->producer.frozen[2].normalized.record[0].track==0&&
    e->producer.frozen[2].normalized.record[1].track==5);
 sl_to_publication(e,0);sl_publish_current(e,0);sl_packet(e,0);sl_fire(e,0);e->port.model.hold_first=1;
 assert(pt_editor_mixed_causal_song_lineage_service_command(&e->producer,0,0)==PT_MIXED_READERS_PENDING&&e->producer.history[0].service_seen);
 memcpy(before,sl_s(e)->reader,sizeof(before));memcpy(ram,e->base.card->ram,sizeof(ram));
 chip=e->base.chip.calls;masters=e->base.masters.calls;writes=e->base.card->writes;
 sl_to_publication(e,1);sl_publish_current(e,1);sl_packet(e,1);
 assert(!memcmp(before,sl_s(e)->reader,sizeof(before))&&pt_mixed_readers_commands_held(sl_s(e)->queue)==2&&
    pt_mixed_readers_readers_held(sl_s(e)->queue)==16);
 sl_wait(e,2);sl_fire(e,1);e->port.model.hold_first=0;
 assert(pt_editor_mixed_causal_song_lineage_service_command(&e->producer,0,0)==PT_MIXED_READERS_OK&&
    e->producer.history[0].detached&&e->joint.original.root_detached&&e->joint.original.root_closed);
 sl_to_publication(e,2);sl_publish_current(e,2);sl_packet(e,2);sl_fire(e,2);
 assert(p->effects==16&&e->port.model.controls==6&&e->port.model.stops==2&&
    e->producer.attempted_mask==7&&pt_mixed_readers_commands_held(sl_s(e)->queue)==2&&
    pt_mixed_readers_readers_held(sl_s(e)->queue)==16&&chip==e->base.chip.calls&&masters==e->base.masters.calls&&
    writes==e->base.card->writes&&!memcmp(ram,e->base.card->ram,sizeof(ram)));
 /* Only STOP's two selected voices stop here; remaining original readers are
  * separately cancelled with genuine domain receipts, never declared quiet. */
 assert(p->mask&&e->port.model.stopped_mask==((1U<<0)|(1U<<5)));
 e->port.model.quiet_mask=e->port.model.stopped_mask;sl_commands(e,0);sl_readers(e,1);
 assert(!pt_mixed_readers_commands_held(sl_s(e)->queue)&&!pt_mixed_readers_readers_held(sl_s(e)->queue)&&ct_empty(p));
 sl_same(e);sl_close(e);assert(p->shutdowns==1);sl_same(e);sl_drop(e);
}
static void sl_cancel_phase(enum pt_editor_mixed_causal_song_lineage_phase phase)
{struct sl_trial *e=sl_make(24,16,0,CS_MIXED,0);sl_begin(e);sl_until(e,phase);sl_close(e);sl_same(e);sl_drop(e);}
static void sl_shape_refusal(unsigned kind)
{
 struct sl_trial *e=sl_make(24,16,0,CS_MIXED,0);struct pt_project *p=e->base.editor->project;unsigned i,n=0;
 if(kind==0)for(i=0;i<16;++i)memset(p->events+16+i,0,sizeof(*p->events));
 else if(kind==1)for(i=0;i<16;++i)p->events[16+i]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
 else if(kind==2)for(i=0;i<16;++i)p->events[32+i]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
 else if(kind==3){p->events[48]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};p->events[63].effect=0;p->events[79].effect=15;}
 else if(kind==4){p->events[16]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};}
 else if(kind==5){memset(p->events,0,16*sizeof(*p->events));}
 else if(kind==6)e->producer.configuration.absolute_start=UINT64_MAX-960;
 else{assert(kind==7);p->events[31].effect=0;} /* STOP track15 outside CONTROL subset. */
 sl_resave(e);sl_begin(e);
 while(!e->producer.first_error){(void)sl_step(e,17);assert(++n<100000);}
 assert((e->producer.first_error==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED||e->producer.first_error==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT)&&
    !e->base.ordinary.calls&&!e->port.binds&&!e->port.model.base.publications&&!e->base.card->writes&&!e->base.chip.calls&&
    !sl_s(e)->pool&&!sl_s(e)->causal&&e->producer.attempted_mask==0);
 sl_close(e);sl_same(e);sl_drop(e);
}
/* Numeric whole-local admission precedes initialization, never a pointee read. */
static void sl_local_numeric(struct sl_trial *e,uintptr_t address,size_t bytes)
{assert(pt_editor_mixed_causal_source_allocation_disjoint(sl_s(e),e->borrow,(const void *)address,bytes));}
static void sl_begin_refusal(unsigned kind)
{
 struct sl_trial *e=sl_make(24,16,0,CS_MIXED,0);
 if(kind==3){
    struct pt_editor_mixed_causal_source_borrow copied,before;
    struct pt_editor_mixed_causal_song_lineage original;
    sl_begin(e);
    sl_local_numeric(e,(uintptr_t)&copied,sizeof(copied));
    sl_local_numeric(e,(uintptr_t)&before,sizeof(before));
    sl_local_numeric(e,(uintptr_t)&original,sizeof(original));
    copied=*e->borrow;before=*e->borrow;original=e->producer;
    assert(copied.address&&copied.serial&&
       pt_editor_mixed_causal_lineage_source_activate(&e->joint,&copied,&e->producer.preparation)==PT_EDITOR_MIXED_READERS_INVALID&&
       e->borrow->address==before.address&&e->borrow->serial==before.serial&&
       !memcmp(&original,&e->producer,sizeof(original))&&!sl_s(e)->source_activated&&
       !e->external.calls&&!e->base.ordinary.calls&&!e->base.chip.calls&&!e->base.card->writes);
    sl_close(e);sl_same(e);sl_drop(e);return;
 }
 {
    struct pt_editor_mixed_causal_song_lineage before;
    struct pt_editor_mixed_causal_lineage_source_inputs saved=*e->source;
    if(kind==0)e->source->original.mutable[1]=(struct pt_sampler_storage_span){e->overlap_guard,sizeof(*e->outputs)+CS_TAIL};
    else if(kind==1)e->source->original.immutable[5]=(struct pt_sampler_storage_span){(char *)e->source->original.immutable[4].data+e->audit_q_capacity-CS_TAIL,CS_TAIL};
    else{assert(kind==2);e->producer.joint=(void *)((char *)&e->joint+1);}
    before=e->producer;
    assert(pt_editor_mixed_causal_song_lineage_begin(&e->producer,e->source,e->borrow)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_INVALID&&
       !memcmp(&before,&e->producer,sizeof(before))&&!e->base.binding->preparation_context&&!e->base.ordinary.calls&&
       !e->external.calls&&!e->base.chip.calls&&!e->base.card->writes);
    *e->source=saved;e->producer=before;e->producer.joint=&e->joint;sl_same(e);sl_drop(e);
 }
}
static void sl_current_refusal(unsigned kind)
{
 struct sl_trial *e=sl_make(24,16,0,CS_MIXED,0);unsigned n=0;sl_preflight(e);
 if(kind==0)++e->producer.configuration.options.tick_limit;
 else if(kind==1)++e->outputs->candidate[1].normalized.frame;
 else{assert(kind==2);++e->base.editor->history.revision;}
 while(!e->producer.first_error){(void)sl_step(e,17);assert(++n<100000);}
 assert(!e->port.binds&&!e->base.ordinary.calls&&!e->base.card->writes&&!e->base.chip.calls);
 sl_close(e);sl_same(e);sl_drop(e);
}
static void sl_budget_refusal(unsigned kind)
{
 struct sl_trial *e=sl_make(24,16,0,CS_MIXED,0);unsigned n=0;
 if(!kind)e->producer.configuration.ordinary_budget=1;else e->producer.configuration.audit_budget=1;
 {enum pt_editor_mixed_causal_song_lineage_result r=pt_editor_mixed_causal_song_lineage_begin(&e->producer,e->source,e->borrow);
    assert(r==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PENDING||r==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CAPACITY);}
 while(!e->producer.first_error){(void)sl_step(e,17);assert(++n<100000);}
 assert(e->producer.first_error==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CAPACITY||e->producer.first_error==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED);
 assert(!e->base.ordinary.calls&&!e->port.binds&&!e->base.chip.calls&&!e->base.card->writes);sl_close(e);sl_same(e);sl_drop(e);
}
static void sl_external_fault(unsigned kind)
{
 struct sl_trial *e=sl_make(24,16,0,CS_MIXED,0);unsigned n=0;unsigned char before[CS_TAIL];
 if(!kind){e->external.alias=(char *)e->outputs+sizeof(*e->outputs);memcpy(before,e->external.alias,sizeof(before));}
 else{assert(kind==1);e->external.reenter_at=1;}
 sl_begin(e);while(!e->producer.first_error){(void)sl_step(e,17);assert(++n<100000);}
 assert(e->external.calls==1&&!e->base.ordinary.calls&&!e->port.binds&&!e->base.chip.calls&&!e->base.card->writes);
 if(!kind)assert(!memcmp(before,e->external.alias,sizeof(before))&&!e->external.alias_releases);
 else assert(e->external.pointer[0]&&!e->external.live[0]&&e->external.releases==1);
 sl_close(e);sl_same(e);sl_drop(e);
}
static void sl_outer_fault(unsigned publication)
{
 struct sl_trial *e=sl_make(24,16,0,CS_MIXED,0);struct pt_editor_mixed_command_ref ref;uint64_t ticket;
 sl_preflight(e);sl_until(e,publication?PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PUBLICATION:PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_ENQUEUE);
 ref=e->producer.history[0].reference;assert(ref.serial);
 if(publication){ticket=e->producer.history[0].ticket;e->outer_publish_fault=1;
    assert(ticket&&pt_editor_mixed_causal_song_lineage_publish(&e->producer)==PT_MIXED_READERS_OK&&
       e->producer.history[0].published&&e->producer.history[0].ticket==ticket);}
 else{e->outer_enqueue_fault=1;assert(sl_step(e,17)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);
    ticket=e->producer.history[0].ticket;assert(ticket&&e->producer.scheduled_result==PT_MIXED_READERS_OK);}
 assert(e->producer.first_error&&e->producer.history[0].reference.serial==ref.serial&&
    sl_s(e)->command[ref.slot].ticket==ticket&&sl_s(e)->command[ref.slot].transferred&&
    pt_mixed_readers_commands_held(sl_s(e)->queue)==1);
 sl_close(e);sl_same(e);sl_drop(e);
}
static void sl_source_unknown(int raw)
{
 struct sl_trial *e=sl_make(24,16,0,CS_MIXED,0);unsigned n=0,pins,probes;sl_begin(e);sl_until(e,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_LOAD);
 e->port.model.base.source_raw=raw;e->port.quiet_raw=0;pins=e->producer.pin_count;
 assert(pt_editor_mixed_causal_song_lineage_cancel(&e->producer)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CANCELLED);
 while(!e->port.model.base.shutdowns){assert(!pt_editor_mixed_causal_song_lineage_close(&e->producer)&&++n<1000);}
 assert(e->port.model.base.shutdowns==1&&e->producer.pin_count==pins&&e->borrow->address==sl_s(e)&&sl_s(e)->causal&&
    !sl_s(e)->pool&&!e->producer.borrow_closed&&!e->producer.hook_closed);
 probes=e->port.model.base.probes;assert(!pt_editor_mixed_causal_song_lineage_close(&e->producer)&&
    e->port.model.base.shutdowns==1&&e->port.model.base.probes==probes+1&&e->producer.pin_count==pins);
 /* Separate later provider proof; shutdown is never retried or upgraded. */
 e->port.quiet_raw=1;sl_close(e);assert(e->port.model.base.shutdowns==1);sl_same(e);sl_drop(e);
}
#ifndef PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_TEST_MAIN
#define PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_TEST_MAIN main
#endif
int PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_TEST_MAIN(void)
{
 static const enum pt_editor_mixed_causal_song_lineage_phase cancelled[]={
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_ESTABLISH_BEGIN,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_ACTIVATE,
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_AUDIT_TAKE,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_NEXT,
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_BATCH_STEP};
 unsigned bits,card,order,i;
 for(bits=16;bits<=24;bits+=8)for(card=0;card<2;++card)for(order=0;order<2;++order)sl_success(bits,card,order);
 sl_success_subset();
 for(i=0;i<5;++i)sl_cancel_phase(cancelled[i]);for(i=0;i<8;++i)sl_shape_refusal(i);
 for(i=0;i<4;++i)sl_begin_refusal(i);for(i=0;i<3;++i)sl_current_refusal(i);
 for(i=0;i<2;++i)sl_budget_refusal(i);for(i=0;i<2;++i)sl_external_fault(i);
 sl_outer_fault(0);sl_outer_fault(1);sl_source_unknown(0);sl_source_unknown(-1);sl_source_unknown(2);
 assert(sl_cases==38&&cs_cases==0&&cp_cases==0&&ct_cases==0);
 puts("EDITOR MIXED CAUSAL SONG LINEAGE PASS:38 genuine three-boundary cases;8 mixed4Paula+12card/16card TRIGGER-CONTROL16-STOP sequences with16/24-bit masters/cache16,exact960/1920/2880 frames,two live C slots and actual C1 service/disposal before C3 reuse,both C/R retirement orders,1 strict CONTROL6/STOP2 subset of original16,real audit/take-once/EOF,WAIT_ACTIVE without rebase,full16-bit card levels,29 source/cancel/shape/budget/alias/stale/reentry/retained-lower-OK/unknown-source cases,original master/save/cache/full-capacity tails;WAIT_CAPACITY source-arithmetic only; SOFTWARE_ONLY");
 return 0;
}
