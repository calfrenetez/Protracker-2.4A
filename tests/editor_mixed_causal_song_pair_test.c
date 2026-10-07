/* Genuine before-master typed song-pair fixture. The original 43-case donor
 * main and all inherited entries are renamed and UNCALLED. No mirrored core,
 * synthetic master/ACTIVE/READY proof, native/device/IRQ or musical PLAY. */
#define PT_EDITOR_MIXED_CAUSAL_PREPARE_TEST_MAIN cs_typed43_donor_not_called
#include "editor_mixed_causal_prepare_test.c"
#undef PT_EDITOR_MIXED_CAUSAL_PREPARE_TEST_MAIN
#include "../src/editor/editor_mixed_causal_song_pair_internal.h"

#define CS_MONO_FRAMES 2048U
#define CS_MONO_CAPACITY 2112U
#define CS_TAIL 257U
enum cs_kind {CS_MIXED,CS_CARD16,CS_EMPTY,CS_ONE,CS_THIRD,CS_CONTROL,CS_STOP};
struct cs_external {
 unsigned calls,releases,alias_releases,reenter_at;
 void *alias,*pointer[PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_REQUESTS];
 size_t bytes[PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_REQUESTS];
 unsigned live[PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_REQUESTS];
};
struct cs_trial {
 struct cp_trial base;
 struct pt_editor_mixed_causal_song_pair producer;
 struct cs_external external;
 struct pt_editor_mixed_causal_source_inputs *source;
 struct pt_editor_mixed_causal_source_borrow *borrow;
 struct pt_editor_mixed_causal_song_pair_outputs *outputs;
 void *audit_storage,*audit_q,*q_storage,*audit_result;
 size_t audit_capacity,audit_q_capacity,q_capacity,result_capacity,output_capacity;
 int32_t *original_mono,*original_stereo,*spare_values;uint32_t *original_slices;unsigned spare_slot;
 struct pt_sample *real_samples;struct pt_event *real_events;
 unsigned promoting,master_reenter_at,master_observed,master_calls_at_begin;
 unsigned closed,kind,publication_outcome,outer_configuration_fault;
 unsigned char tail[CS_TAIL];
 /* Complete readable backing for the P/U overlap refusal, independent of
  * unrelated queried owner sizes. Its full capacity is part of declared P. */
 unsigned char overlap_guard[sizeof(struct pt_editor_mixed_causal_song_pair_outputs)+CS_TAIL];
};
static unsigned cs_cases;
static void *cs_master_new(void *context,size_t bytes)
{
 struct cp_memory *m=context;struct cp_trial *f=m->owner;struct cs_trial *e=(void *)f;
 if(e->promoting){
    assert(e->borrow&&e->borrow->address==&f->control&&e->borrow->serial&&
       f->control.source_busy&&f->binding->preparation_context==&f->control&&
       f->binding->preparation_close&&!f->control.source_activated);
    ++e->master_observed;
    if(e->master_reenter_at==e->master_observed){
       uint32_t rev=f->editor->history.revision,gen=f->editor->sampler.generation;
       assert(!pt_editor_prepare_change(f->editor)&&!pt_editor_dispose(f->editor));
       assert(rev==f->editor->history.revision&&gen==f->editor->sampler.generation);
       assert(f->control.source_cancel_requested&&f->control.first_error&&e->borrow->address==&f->control);
    }
 }
 return cp_new(m,bytes);
}
static void *cs_external_new(void *context,size_t bytes)
{
 struct cs_external *m=context;struct cs_trial *e=(void *)((char *)context- offsetof(struct cs_trial,external));
 struct cp_trial *f=&e->base;unsigned index=m->calls++,i;
 assert(index<PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_REQUESTS&&bytes&&e->producer.busy&&
    f->control.source_busy&&!f->control.busy&&e->borrow->address==&f->control&&
    f->binding->preparation_context==&f->control&&f->binding->preparation_close);
 if(index){assert(f->control.source_activated&&e->producer.pin_count==f->editor->project->sample_count);
    for(i=0;i<f->editor->project->sample_count;++i)assert(e->producer.pin[i]&&e->producer.pin[i]==f->editor->sampler.current[i]);
 }else assert(!f->control.source_activated&&!f->control.causal&&!f->control.pool);
 if(m->reenter_at==index+1){assert(pt_editor_mixed_causal_song_pair_step(&e->producer,1)==PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_FAULT);
    assert(!pt_editor_prepare_change(f->editor));}
 if(m->alias)return m->alias;
 m->pointer[index]=malloc(bytes);assert(m->pointer[index]);m->bytes[index]=bytes;m->live[index]=1;return m->pointer[index];
}
static void cs_external_release(void *context,void *pointer)
{
 struct cs_external *m=context;struct cs_trial *e=(void *)((char *)context- offsetof(struct cs_trial,external));unsigned i,j;
 assert(e->producer.busy&&e->base.control.source_busy&&!e->base.control.busy);
 if(pointer==m->alias){++m->alias_releases;return;}
 for(i=0;i<m->calls&&m->pointer[i]!=pointer;++i){}assert(i<m->calls&&m->live[i]);
 for(j=0;j<e->producer.requests;++j)if(e->producer.allocation[j].address==(uintptr_t)pointer)assert(!e->producer.allocation[j].live);
 m->live[i]=0;++m->releases;
 /* Fixture arena backing is quarantined through teardown after the genuine
  * logical release; this prevents allocator-address recycling from hiding the
  * product's explicit unresolved-retired-extent protocol. No quiet is inferred. */
}
static int cs_owned(void *context)
{
 struct cp_bus *bus=context;struct cp_trial *f=bus->owner;struct cs_trial *e=(void *)f;int actual=cp_owned(context);
 if(e->outer_configuration_fault&&e->producer.phase==PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_ENQUEUE){
    assert(actual&&e->producer.busy&&f->control.busy);e->outer_configuration_fault=0;
    ++e->producer.configuration.options.tick_limit;
 }
 return actual;
}
static int cs_bind(void *context,const struct pt_mixed_causal_registration *registration)
{
 struct cp_port *port=context;struct cs_trial *e=(void *)port->owner;
 assert(e->producer.phase==PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_CONTROLLER&&e->producer.busy&&
    e->producer.candidate_count==2&&!e->producer.candidate_pending&&e->producer.frames==1920&&
    e->producer.intervals==3&&e->producer.audited.rewinds==1&&e->producer.activated&&
    e->outputs->sequence&&e->outputs->audit&&!e->outputs->normalizer&&
    !memcmp(e->outputs->candidate,e->producer.frozen,sizeof(e->outputs->candidate))&&
    e->producer.pin_count==e->base.editor->project->sample_count&&e->external.calls==3);
 return cp_bind(context,registration);
}
static int cs_publish(void *context,struct pt_mixed_causal_owner *owner,
 const struct pt_mixed_causal_command_identity *identity,const struct pt_mixed_causal_packet *packet)
{
 struct cp_port *p=context;struct cs_trial *e=(void *)p->owner;
 if(!e->publication_outcome)return cp_publish(context,owner,identity,packet);
 if(e->publication_outcome==1){++p->model.publications;assert(owner==packet->registration.owner&&p->model.ticks<packet->first);return 0;}
 (void)cp_publish(context,owner,identity,packet);return e->publication_outcome==2?-1:2;
}
static int cs_publish_successor(void *context,struct pt_mixed_causal_owner *owner,const struct pt_mixed_causal_publication *publication)
{return cp_publish_successor(context,owner,publication);}
static void cs_fill_tail(void *p,size_t used,size_t capacity,unsigned byte)
{assert(capacity>=used);memset((unsigned char *)p+used,(int)byte,capacity-used);}
static void cs_tail_same(const void *p,size_t used,size_t capacity,unsigned byte)
{size_t i;const unsigned char *v=p;for(i=used;i<capacity;++i)assert(v[i]==byte);}
static struct cs_trial *cs_make(unsigned bits,unsigned cache_bits,unsigned little,unsigned which,unsigned lead)
{
 struct cs_trial *e=calloc(1,sizeof(*e));struct cp_trial *f;struct pt_allocator a;
 struct pt_project *p;struct pt_amigus_reservation_api api;unsigned i,j;
 assert(e&&offsetof(struct cs_trial,base)==0);f=&e->base;f->bits=bits;f->cache_bits=cache_bits;f->little=little;e->kind=which;
 f->card=calloc(1,sizeof(*f->card));f->document=calloc(1,sizeof(*f->document));
 f->editor=calloc(1,sizeof(*f->editor));f->binding=calloc(1,sizeof(*f->binding));
 assert(f->card&&f->document&&f->editor&&f->binding);f->bus.owner=f;f->ordinary.owner=f;f->masters.owner=f;f->chip.owner=f;f->port.owner=f;f->port.bind_raw=1;f->port.quiet_raw=1;
 a=(struct pt_allocator){&f->masters,cs_master_new,cp_master_free};
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
 for(j=0;j<2;++j)for(i=0;i<16;++i)
    p->events[j*16+i]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
 p->events[32+15].effect=15;
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
 assert(pt_amigus_wavetable_cache_attach(&f->card->cache,&f->card->reservation,0,4096,4096,&f->bus,cs_owned,cp_write));
 f->input.binding=f->binding;f->input.backend=&f->card->cache;
 f->input.causal.allocator=(struct pt_allocator){&f->ordinary,cp_allocate,cp_release};
 f->input.causal.allocator_context=(struct pt_mixed_readers_span){&f->ordinary,sizeof(f->ordinary)};
 f->input.causal.grid=(struct pt_mixed_readers_grid){100,17,709379,48000};f->input.causal.session=31;
 f->input.causal.control_budget=pt_mixed_causal_control_size();f->input.causal.queue_budget=pt_mixed_readers_control_size();
 f->input.causal.port=(struct pt_mixed_causal_port){&f->port,sizeof(f->port),PT_MIXED_CAUSAL_PORT_VERSION,
    PT_MIXED_CAUSAL_PORT_REQUIRED,cp_clock,cs_publish,cp_commit,cp_command_quiet,cp_reader_quiet,
    cp_source_close,cp_source_quiet,cs_publish_successor};
 f->input.bind_original=cs_bind;f->port.model.ticks=100;f->port.model.frequency=709379;
 f->port.model.commit_raw=f->port.model.source_raw=1;
 f->input.chip_context=&f->chip;f->input.chip_allocate=cp_chip_new;f->input.chip_release=cp_chip_free;
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
 e->source->binding=f->binding;e->source->preparation=&e->producer.preparation;e->source->contexts=f->input.contexts;
 e->source->immutable_count=6;e->source->mutable_count=2;
 e->source->immutable[0]=(struct pt_sampler_storage_span){f->causal_workspace,f->causal_capacity};
 e->source->immutable[1]=(struct pt_sampler_storage_span){f->factory_workspace,f->factory_capacity};
 e->source->immutable[2]=(struct pt_sampler_storage_span){f->card,sizeof(*f->card)};
 e->source->immutable[3]=(struct pt_sampler_storage_span){e->audit_storage,e->audit_capacity};
 e->source->immutable[4]=(struct pt_sampler_storage_span){e->audit_q,e->audit_q_capacity};
 e->source->immutable[5]=(struct pt_sampler_storage_span){e->q_storage,e->q_capacity};
 e->source->mutable[0]=(struct pt_sampler_storage_span){e->audit_result,e->result_capacity};
 e->source->mutable[1]=(struct pt_sampler_storage_span){e->outputs,e->output_capacity};
 e->source->causal_workspace=e->source->immutable[0];e->source->factory_workspace=e->source->immutable[1];
 e->source->backend_parent=e->source->immutable[2];e->producer.controller=&f->control;
 e->producer.configuration.preparation=f->input;
 e->producer.configuration.options=(struct pt_render_options){0};
 e->producer.configuration.options.rate=48000;e->producer.configuration.options.bits=24;
 e->producer.configuration.options.gain_q16=65536;e->producer.configuration.options.tracks=0xffff;
 e->producer.configuration.options.tick_limit=1000;e->producer.configuration.options.frame_limit=100000;
 e->producer.configuration.options.include_lead_in=(uint8_t)lead;
 e->producer.configuration.caps=(struct pt_paula_render_caps){3546895,124,65535};
 e->producer.configuration.format.bits=(uint8_t)cache_bits;e->producer.configuration.format.little_endian=(uint8_t)little;
 e->producer.configuration.external_allocator=(struct pt_allocator){&e->external,cs_external_new,cs_external_release};
 e->producer.configuration.ordinary_budget=4*1024*1024;
 e->producer.configuration.audit_budget=e->audit_capacity+e->audit_q_capacity+e->result_capacity+
    pt_render_sequence_setup_control_size()+pt_render_sequence_control_size();
 e->producer.configuration.absolute_start=960;
 cs_fill_tail(f->causal_workspace,f->causal_capacity,f->causal_capacity+CS_TAIL,0xa1);
 cs_fill_tail(f->factory_workspace,f->factory_capacity,f->factory_capacity+CS_TAIL,0xa2);
 cs_fill_tail(e->audit_storage,pt_mixed_quantized_audit_workspace_size(),e->audit_capacity,0xa3);
 cs_fill_tail(e->audit_q,pt_mixed_plan_normalizer_workspace_size(),e->audit_q_capacity,0xa4);
 cs_fill_tail(e->q_storage,pt_mixed_plan_normalizer_workspace_size(),e->q_capacity,0xa5);
 cs_fill_tail(e->audit_result,pt_mixed_quantized_audit_result_size(),e->result_capacity,0xa6);
 cs_fill_tail(e->outputs,sizeof(*e->outputs),e->output_capacity,0xa7);memset(e->tail,0xc1,sizeof(e->tail));
 memset(e->overlap_guard,0xc2,sizeof(e->overlap_guard));
 f->saved=cp_save(f,&f->saved_bytes);assert(pt_project_validate(p,NULL)==PT_PROJECT_OK);
 e->master_calls_at_begin=f->masters.calls;++cs_cases;return e;
}
static enum pt_editor_mixed_causal_song_pair_result cs_step(struct cs_trial *e,unsigned work)
{
 enum pt_editor_mixed_causal_song_pair_result r;unsigned phase=e->producer.phase;
 e->promoting=phase<=PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_ESTABLISH_CLOSE;
 r=pt_editor_mixed_causal_song_pair_step(&e->producer,work);e->promoting=0;return r;
}
static void cs_resave(struct cs_trial *e)
{free(e->base.saved);e->base.saved=cp_save(&e->base,&e->base.saved_bytes);}
static void cs_same(struct cs_trial *e)
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
 cs_tail_same(f->causal_workspace,f->causal_capacity,f->causal_capacity+CS_TAIL,0xa1);
 cs_tail_same(f->factory_workspace,f->factory_capacity,f->factory_capacity+CS_TAIL,0xa2);
 cs_tail_same(e->audit_storage,pt_mixed_quantized_audit_workspace_size(),e->audit_capacity,0xa3);
 cs_tail_same(e->audit_q,pt_mixed_plan_normalizer_workspace_size(),e->audit_q_capacity,0xa4);
 cs_tail_same(e->q_storage,pt_mixed_plan_normalizer_workspace_size(),e->q_capacity,0xa5);
 cs_tail_same(e->audit_result,pt_mixed_quantized_audit_result_size(),e->result_capacity,0xa6);
 cs_tail_same(e->outputs,sizeof(*e->outputs),e->output_capacity,0xa7);
 for(i=0;i<CS_TAIL;++i)assert(e->tail[i]==0xc1);
 cs_tail_same(e->overlap_guard,0,sizeof(e->overlap_guard),0xc2);
}
static void cs_begin(struct cs_trial *e)
{
 struct cp_trial *f=&e->base;unsigned masters=f->masters.calls;
 assert(offsetof(struct cs_trial,base)==0&&sizeof(*e)>sizeof(f->control)+sizeof(e->producer));
 assert(!f->editor->sampler.current[0]&&!f->editor->sampler.current[1]&&f->masters.calls==e->master_calls_at_begin);
 assert(pt_editor_mixed_causal_song_pair_begin(&e->producer,e->source,e->borrow)==PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_PENDING);
 assert(e->borrow->address==&f->control&&e->borrow->serial&&e->producer.original==&e->producer&&
    f->binding->preparation_context==&f->control&&f->binding->preparation_close&&
    !f->control.inputs&&!f->control.source_activated&&e->producer.adopted);
 assert(masters==f->masters.calls&&!f->ordinary.calls&&!e->external.calls&&!f->chip.calls&&
    !f->card->writes&&!f->port.bind_calls&&!f->port.model.clocks&&!f->port.model.publications);
 assert(pt_editor_mixed_causal_song_pair_step(&e->producer,0)==PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_INVALID);
 assert(pt_editor_mixed_causal_song_pair_step(&e->producer,4097)==PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_INVALID);
}
static void cs_until(struct cs_trial *e,enum pt_editor_mixed_causal_song_pair_phase wanted)
{
 unsigned n=0;enum pt_editor_mixed_causal_song_pair_result r;
 while(e->producer.phase!=wanted){r=cs_step(e,17);assert(++n<100000);
    if(r==PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_PUBLISH){
       if(wanted==PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_PUBLICATION)break;
       assert(pt_editor_mixed_causal_song_pair_publish(&e->producer)==PT_MIXED_READERS_OK);
    }else assert(r==PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_PENDING||r==PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_PUBLISHED);
 }
 assert(e->producer.phase==wanted);
}
static void cs_command_services(struct cs_trial *e,unsigned cancel)
{
 unsigned i;for(i=0;i<2;++i)if(e->producer.command[i].reference.serial&&e->producer.command[i].ticket){
    enum pt_mixed_readers_result r=pt_editor_mixed_causal_song_pair_service_command(&e->producer,i,cancel);
    assert(r==PT_MIXED_READERS_OK||r==PT_MIXED_READERS_PENDING||r==PT_MIXED_READERS_BACKEND);
 }
}
static void cs_reader_services(struct cs_trial *e,unsigned cancel)
{
 unsigned i;for(i=0;i<32;++i)if(e->producer.reader[i].reference.serial){
    struct pt_editor_mixed_reader_ref r=e->producer.reader[i].reference;
    /* Untransferred partial readers are local controller cleanup; this fixture
     * never fabricates a ticket to force a domain proof. */
    if(e->base.control.reader[r.slot].ticket){
       enum pt_mixed_readers_result result=pt_editor_mixed_causal_song_pair_service_reader(&e->producer,i,cancel);
       assert(result==PT_MIXED_READERS_OK||result==PT_MIXED_READERS_PENDING||result==PT_MIXED_READERS_BACKEND);
    }
 }
}
static void cs_close(struct cs_trial *e)
{
 unsigned i,n=0;struct cp_trial *f=&e->base;
 assert(pt_editor_mixed_causal_song_pair_cancel(&e->producer)!=PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_INVALID);
 cs_command_services(e,1);cs_reader_services(e,1);
 while(!pt_editor_mixed_causal_song_pair_close(&e->producer)){assert(++n<1000);
    assert(f->binding->preparation_context==&f->control);
    if(e->producer.borrow_closed)assert(!e->borrow->address&&!e->borrow->serial);
    else assert(e->borrow->address==&f->control);
    cs_command_services(e,1);cs_reader_services(e,1);
 }
 assert(!e->outputs->audit&&!e->outputs->sequence&&!e->outputs->normalizer&&!e->producer.establish.active&&!e->producer.pin_job.owner);
 assert(!e->producer.pin_count&&!e->borrow->address&&!e->borrow->serial&&!f->binding->preparation_context);
 assert(!f->control.pool&&!f->control.causal&&!f->control.queue&&!e->external.alias_releases);
 for(i=0;i<32;++i)assert(!e->producer.pin[i]);
 for(i=0;i<e->external.calls;++i)assert(!e->external.live[i]);
 e->closed=1;
}
static void cs_drop(struct cs_trial *e)
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
static int cs_empty_record(const struct pt_mixed_plan_record *r)
{
 const struct pt_amigus_voice_request *t=&r->geometry.amigus.trigger;
 /* Empty declared semantics, not C padding: the genuine normalizer contract
  * does not grant padding bytes authority. Full candidate copies are compared
  * only against their original byte-identical copied before-images. */
 return r->kind==PT_MIXED_PLAN_TRIGGER&&!r->track&&!r->route&&!r->slot&&!r->sample&&!r->channel&&
    !r->first_action&&!r->control_action&&!r->geometry.amigus.bits&&!r->geometry.amigus.little_endian&&
    !t->rate_numerator&&!t->rate_denominator&&!t->offset&&!t->volume&&!t->pan&&
    !r->geometry.amigus.rate&&!r->geometry.amigus.left&&!r->geometry.amigus.right&&
    !r->image.start&&!r->image.loop&&!r->image.end_exclusive&&!r->image.rate&&
    !r->image.control&&!r->image.left&&!r->image.right;
}
static void cs_snapshot_oracle(struct cs_trial *e,unsigned boundary)
{
 static const unsigned frames[3]={0,960,960},count[3]={16,16,0};
 static const uint64_t target[3]={960,1920,2880};
 struct pt_mixed_plan_quantized_batch *b=&e->outputs->normalized;unsigned i,j,paula=0,card=0;
 assert(boundary<3&&e->outputs->interval.frames==frames[boundary]&&e->outputs->interval.emit==1&&
    e->outputs->interval.end==(boundary==2)&&b->normalized.count==count[boundary]&&
    b->normalized.frame==target[boundary]&&e->producer.target==target[boundary]);
 for(i=0;i<16;++i){unsigned route=e->base.editor->project->channels.track[i].route;
    if(boundary<2){const struct pt_mixed_plan_record *r=b->normalized.record+i;
       assert(r->kind==PT_MIXED_PLAN_TRIGGER&&r->track==i&&r->route==route&&r->sample==0&&!r->channel&&
          r->slot==(route==PT_PAULA?paula++:card++)&&r->first_action<e->outputs->plan.count);
       assert(b->normalized.next[i].present&&b->normalized.next[i].sample==0&&!b->normalized.next[i].channel&&
          b->normalized.next[i].frame==target[boundary]);
       if(route==PT_PAULA)assert(b->levels[i].mode==PT_MIXED_PLAN_LEVEL_LEGACY&&!b->levels[i].left&&!b->levels[i].right);
       else assert(b->levels[i].mode==PT_MIXED_PLAN_LEVEL_QUANTIZED&&
          b->levels[i].left==r->image.left&&b->levels[i].right==r->image.right&&!r->geometry.amigus.trigger.volume&&!r->geometry.amigus.trigger.pan);
    }else{assert(cs_empty_record(b->normalized.record+i)&&b->levels[i].mode==PT_MIXED_PLAN_LEVEL_LEGACY&&
       !b->levels[i].left&&!b->levels[i].right&&b->normalized.next[i].present&&b->normalized.next[i].frame==1920&&
       b->normalized.next[i].sample==0&&!b->normalized.next[i].channel);}
 }
 for(j=0;j<2;++j)for(i=0;i<32;++i)assert(b->normalized.samples[j][i]==(boundary<2&&!i&&(j?card:paula)?1:0));
}
static void cs_preflight(struct cs_trial *e)
{
 unsigned n=0,boundary=0,i;cs_begin(e);
 while(e->producer.phase!=PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_CONTROLLER){
    unsigned phase=e->producer.phase;
    if(phase==PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_LOWER){cs_snapshot_oracle(e,boundary);++boundary;}
    assert(cs_step(e,17)==PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_PENDING&&++n<100000);
    assert(!e->base.ordinary.calls&&!e->base.port.bind_calls&&!e->base.port.model.clocks&&
       !e->base.port.model.publications&&!e->base.chip.calls&&!e->base.card->writes&&!e->base.control.causal&&!e->base.control.pool);
 }
 assert(boundary==3&&e->producer.candidate_count==2&&e->producer.frames==1920&&e->producer.intervals==3&&
    e->producer.audited.frames==1920&&e->producer.audited.intervals==3&&e->producer.audited.rewinds==1&&
    !memcmp(e->producer.audited.samples,e->producer.samples,sizeof(e->producer.samples))&&
    !memcmp(e->producer.frozen,e->outputs->candidate,sizeof(e->producer.frozen))&&e->external.calls==3&&
    e->producer.requests==3&&e->external.releases==2&&e->master_observed&&e->producer.activated);
 for(i=0;i<e->base.editor->project->sample_count;++i)assert(e->producer.pin[i]&&e->producer.pin[i]==e->base.editor->sampler.current[i]);
 assert(e->base.editor->sampler.allocator.context==&e->base.masters&&e->base.editor->sampler.allocator.allocate==cs_master_new&&
    e->base.editor->sampler.allocator.release==cp_master_free);
 cs_same(e);
}
static void cs_packet(struct cs_trial *e,unsigned ci)
{
 static const uint64_t first[2]={14288,28476},last[2]={14303,28490};
 struct cp_trial *f=&e->base;struct ct_command *c=ct_command(&f->port.model,e->producer.command[ci].ticket);unsigned i,j;
 assert(ci<2&&f->bits==24&&f->cache_bits==16&&!f->little);
 assert(c&&c->packet.frame==(ci?1920U:960U)&&c->packet.first==first[ci]&&c->packet.last==last[ci]&&
    first[ci]==oracle(c->packet.frame)&&last[ci]==oracle(c->packet.frame+1)&&
    c->packet.registration.owner==f->control.causal&&c->packet.registration.queue==f->control.queue&&c->packet.count==16);
 for(i=0;i<16;++i){const struct pt_mixed_readers_action *a=c->packet.action+i;
    assert(a->kind==PT_MIXED_READERS_TRIGGER&&c->packet.key[i].route==a->route&&c->packet.key[i].slot==a->slot);
    if(a->route==PT_MIXED_READERS_PAULA){assert(a->geometry.paula.words==CS_MONO_FRAMES/2);
       for(j=0;j<CS_MONO_FRAMES;++j)assert(a->geometry.paula.data[j]==
          (j==2||j==3?0U:(j&1U)?255U:1U));
    }else{const struct pt_mixed_readers_card *card=c->packet.card+i;
       assert(a->route==PT_MIXED_READERS_AMIGUS&&card->logical_bytes==CS_MONO_FRAMES*f->cache_bits/8&&card->full_capacity>=card->logical_bytes&&
          a->geometry.amigus.left==e->producer.frozen[ci].levels[i].left&&a->geometry.amigus.right==e->producer.frozen[ci].levels[i].right);
       /* Independent literal oracle for the fixture's +65536/-65536 and
        * +257/-257 24-bit values: 16-bit BE is 0100/ff00 and 0001/ffff.
        * No test-only production converter or sibling suite is imported. */
       for(j=0;j<CS_MONO_FRAMES;++j){uint16_t w=j==2?1U:j==3?65535U:(j&1U)?65280U:256U;
          assert(f->card->ram[card->address+2*j]==(uint8_t)(w>>8)&&
             f->card->ram[card->address+2*j+1]==(uint8_t)w);}

    }
 }
}
static void cs_success(unsigned card_only,unsigned order)
{
 struct cs_trial *e=cs_make(24,16,0,card_only?CS_CARD16:CS_MIXED,0);struct cp_trial *f=&e->base;
 unsigned i,reads;uint64_t first,second;cs_preflight(e);cs_until(e,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_END);
 assert(e->producer.result==PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_PUBLISHED&&e->producer.pair_index==2&&
    f->port.bind_calls==1&&f->port.model.publications==2&&!f->port.model.commits&&
    pt_mixed_readers_commands_held(f->control.queue)==2&&pt_mixed_readers_readers_held(f->control.queue)==32);
 cs_packet(e,0);cs_packet(e,1);first=e->producer.command[0].ticket;second=e->producer.command[1].ticket;assert(first&&second&&first!=second);
 memset(&e->outputs->plan,0xca,sizeof(e->outputs->plan));memset(&e->outputs->normalized,0xcb,sizeof(e->outputs->normalized));
 memset(e->q_storage,0xcc,pt_mixed_plan_normalizer_workspace_size());
 f->port.model.ticks=14287;assert(pt_mixed_causal_fire(f->control.causal,second)==PT_MIXED_CAUSAL_EARLY);
 assert(pt_mixed_causal_fire(f->control.causal,first)==PT_MIXED_CAUSAL_EARLY&&!f->port.model.effects);
 f->port.model.ticks=14288;assert(pt_mixed_causal_fire(f->control.causal,first)==PT_MIXED_CAUSAL_COMMITTED&&f->port.model.effects==16);
 for(i=0;i<16;++i)assert(pt_editor_mixed_causal_song_pair_service_reader(&e->producer,i,0)==PT_MIXED_READERS_PENDING);
 if(!order)assert(pt_editor_mixed_causal_song_pair_service_command(&e->producer,0,0)==PT_MIXED_READERS_OK&&
    !e->producer.command[0].reference.serial&&pt_mixed_readers_readers_held(f->control.queue)==32);
 f->port.model.ticks=28475;assert(pt_mixed_causal_fire(f->control.causal,second)==PT_MIXED_CAUSAL_EARLY&&f->port.model.effects==16);
 f->port.model.ticks=28476;assert(pt_mixed_causal_fire(f->control.causal,second)==PT_MIXED_CAUSAL_COMMITTED&&f->port.model.effects==32);
 for(i=16;i<32;++i)assert(pt_editor_mixed_causal_song_pair_service_reader(&e->producer,i,0)==PT_MIXED_READERS_PENDING);
 if(!order)cs_command_services(e,0);
 reads=f->port.model.reader_proofs;cs_reader_services(e,1);assert(f->port.model.reader_proofs==reads+32);
 if(order){cs_command_services(e,0);reads=f->port.model.reader_proofs;cs_reader_services(e,1);assert(f->port.model.reader_proofs==reads);}
 assert(!pt_mixed_readers_commands_held(f->control.queue)&&!pt_mixed_readers_readers_held(f->control.queue)&&ct_empty(&f->port.model));
 for(i=0;i<20;++i)assert(!f->port.model.slot[i].serial);
 cs_same(e);cs_close(e);assert(f->port.model.shutdowns==1);cs_same(e);cs_drop(e);
}
static void cs_cancel_phase(enum pt_editor_mixed_causal_song_pair_phase phase)
{
 struct cs_trial *e=cs_make(16,8,0,CS_MIXED,0);cs_begin(e);cs_until(e,phase);cs_close(e);cs_same(e);cs_drop(e);
}
static void cs_song_refusal(unsigned kind)
{
 struct cs_trial *e=cs_make(24,16,0,CS_MIXED,0);struct pt_project *p=e->base.editor->project;unsigned i,n=0;
 if(kind==0)memset(p->events,0,32*sizeof(*p->events));
 else if(kind==1){memset(p->events+16,0,16*sizeof(*p->events));p->events[16+15].effect=15;}
 else if(kind==2){for(i=0;i<16;++i)p->events[32+i]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};p->events[48+15].effect=15;}
 else if(kind==3)memset(p->events+16,0,16*sizeof(*p->events));
 else if(kind==4){memset(p->events+16,0,16*sizeof(*p->events));for(i=0;i<16;++i){p->events[16+i].effect=14;p->events[16+i].parameter=0xc0;}}
 else{assert(kind==5);e->producer.configuration.absolute_start=UINT64_MAX-100;}
 cs_resave(e);cs_begin(e);
 while(!e->producer.first_error){(void)cs_step(e,17);assert(++n<100000);}
 assert(e->producer.result==PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_REFUSED||e->producer.result==PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_FAULT);
 assert(!e->base.ordinary.calls&&!e->base.port.bind_calls&&!e->base.port.model.publications&&!e->base.control.pool&&
    !e->base.control.causal&&!e->base.chip.calls&&!e->base.card->writes&&e->borrow->address==&e->base.control);
 cs_close(e);cs_same(e);cs_drop(e);
}
static void cs_external_fault(unsigned kind)
{
 struct cs_trial *e=cs_make(24,16,0,CS_MIXED,0);uint8_t *before=NULL;unsigned n=0;
 if(kind==0){e->external.alias=(uint8_t *)e->outputs+sizeof(*e->outputs);before=malloc(e->output_capacity);assert(before);memcpy(before,e->outputs,e->output_capacity);}
 else if(kind==1)e->external.reenter_at=1;
 else{assert(kind==2);e->master_reenter_at=1;}
 cs_begin(e);while(!e->producer.first_error){(void)cs_step(e,17);assert(++n<10000);}
 assert(!e->base.control.pool&&!e->base.control.causal&&!e->base.port.bind_calls&&e->borrow->address==&e->base.control);
 if(kind==0)assert(e->external.calls==1&&!e->external.releases&&!e->external.alias_releases&&!memcmp(before,e->outputs,e->output_capacity));
 if(kind==1)assert(e->external.calls==1&&e->external.releases==1&&!e->external.live[0]);
 if(kind==2)assert(e->master_observed==1&&e->base.control.source_cancel_requested);
 cs_close(e);cs_same(e);free(before);cs_drop(e);
}
static void cs_bind_fault(int raw)
{
 struct cs_trial *e=cs_make(24,16,0,CS_MIXED,0);unsigned n=0;cs_preflight(e);e->base.port.bind_raw=raw;
 while(!e->producer.first_error){(void)cs_step(e,17);assert(++n<10000);}
 assert(e->base.port.bind_calls==1&&e->base.control.original_binding_called&&!e->base.control.original_binding_confirmed&&
    e->base.control.original_binding_outcome==raw&&e->base.ordinary.calls==2&&e->base.control.causal&&e->base.control.queue&&
    !e->base.control.pool&&!e->base.chip.calls&&!e->base.card->writes&&!e->base.port.model.publications);
 cs_close(e);cs_same(e);cs_drop(e);
}
static void cs_publish_fault(unsigned kind)
{
 struct cs_trial *e=cs_make(24,16,0,CS_MIXED,0);struct cp_trial *f=&e->base;
 struct pt_mixed_causal_owner *causal;struct pt_mixed_readers_output *queue;
 struct pt_sampler_mixed_pool *pool;struct pt_editor_mixed_command_ref original;
 enum pt_mixed_readers_result result;uint64_t ticket,borrow_serial;
 cs_begin(e);cs_until(e,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_PUBLICATION);
 ticket=e->outputs->ticket;assert(ticket&&e->producer.command[0].ticket==ticket);
 original=e->producer.command[0].reference;causal=f->control.causal;queue=f->control.queue;pool=f->control.pool;
 borrow_serial=e->borrow->serial;e->publication_outcome=kind;
 result=pt_editor_mixed_causal_song_pair_publish(&e->producer);assert(result!=PT_MIXED_READERS_OK);
 assert(e->producer.command[0].ticket==ticket&&f->control.causal&&f->control.pool&&
    e->producer.pin_count==f->editor->project->sample_count&&!f->port.model.commits);
 if(kind==1){
    /* A clean raw0 retained no backend packet or effect. Actual queue/reader
     * transfer remains owned and PENDING at the original publication phase;
     * the existing cs_close below explicitly cancels, it does not republish. */
    assert(result==PT_MIXED_READERS_PENDING&&e->producer.scheduled_result==PT_MIXED_READERS_PENDING&&
       !e->producer.first_error&&!e->producer.cancelled&&!f->control.first_error&&!f->control.source_cancel_requested&&
       e->producer.phase==PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_PUBLICATION&&e->producer.result==PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_PUBLISH&&
       !e->producer.pair_index&&e->producer.working_command==0&&!e->producer.command[0].published&&!e->producer.command[0].uncertain);
    assert(e->outputs->ticket==ticket&&original.serial&&e->producer.command[0].reference.slot==original.slot&&
       e->producer.command[0].reference.serial==original.serial&&f->control.command[original.slot].serial==original.serial&&
       f->control.command[original.slot].transferred&&f->control.command[original.slot].ticket==ticket&&
       f->control.causal==causal&&f->control.queue==queue&&f->control.pool==pool&&
       e->borrow->address==&f->control&&e->borrow->serial==borrow_serial&&borrow_serial&&
       f->binding->preparation_context==&f->control&&f->binding->preparation_close);
    assert(pt_mixed_readers_commands_held(queue)==1&&pt_mixed_readers_readers_held(queue)==16&&
       !ct_command(&f->port.model,ticket)&&ct_empty(&f->port.model)&&f->port.model.publications==1&&
       !f->port.model.effects&&!f->port.model.commits&&!f->port.model.command_proofs&&!f->port.model.reader_proofs&&!f->port.model.shutdowns);
 }else assert(e->producer.first_error&&ct_command(&f->port.model,ticket));
 cs_close(e);
 if(kind==1)assert(e->producer.first_error==PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_CANCELLED&&
    f->port.model.publications==1&&!f->port.model.effects&&!f->port.model.commits);
 cs_same(e);cs_drop(e);
}
static void cs_enqueue_outer_fault(void)
{
 struct cs_trial *e=cs_make(24,16,0,CS_MIXED,0);cs_begin(e);cs_until(e,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_ENQUEUE);
 e->outer_configuration_fault=1;assert(cs_step(e,17)==PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_FAULT);
 assert(!e->outer_configuration_fault&&e->producer.scheduled_result==PT_MIXED_READERS_OK&&e->outputs->ticket&&
    e->producer.command[0].ticket==e->outputs->ticket&&e->base.control.command[0].transferred&&
    e->base.control.command[0].ticket==e->outputs->ticket&&!e->base.port.model.publications);
 cs_close(e);cs_same(e);cs_drop(e);
}
static void cs_source_unknown(int raw)
{
 struct cs_trial *e=cs_make(24,16,0,CS_MIXED,0);unsigned n=0,pins,probes;cs_begin(e);cs_until(e,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_LOAD);
 e->base.port.model.source_raw=raw;e->base.port.quiet_raw=0;pins=e->producer.pin_count;
 assert(pt_editor_mixed_causal_song_pair_cancel(&e->producer)==PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_CANCELLED);
 while(!e->base.port.model.shutdowns){assert(!pt_editor_mixed_causal_song_pair_close(&e->producer)&&++n<1000);}
 assert(e->base.port.model.shutdowns==1&&e->producer.pin_count==pins&&e->borrow->address==&e->base.control&&
    e->base.control.causal&&!e->base.control.pool&&!e->producer.borrow_closed&&!e->producer.hook_closed);
 probes=e->base.port.model.probes;assert(!pt_editor_mixed_causal_song_pair_close(&e->producer)&&
    e->base.port.model.shutdowns==1&&e->base.port.model.probes==probes+1&&e->producer.pin_count==pins);
 /* A later separately explicit read-only proof may settle the original source.
  * It does not retry shutdown, upgrade raw failure or claim a device stop. */
 e->base.port.quiet_raw=1;cs_close(e);assert(e->base.port.model.shutdowns==1);cs_same(e);cs_drop(e);
}
static void cs_begin_refusal(unsigned kind)
{
 struct cs_trial *e=cs_make(24,16,0,CS_MIXED,0);struct pt_editor_mixed_causal_song_pair before;
 struct pt_editor_mixed_causal_prepare controller;struct pt_editor_mixed_causal_source_borrow borrow;
 if(kind==0)e->source->mutable[1].bytes=sizeof(*e->outputs)-1;
 else if(kind==1){
    /* e is readable for at least output_capacity because overlap_guard alone
     * has that capacity, and e is the complete original P parent. No producer/
     * U size relation or writable alias exemption is assumed. */
    assert(e->output_capacity==sizeof(e->overlap_guard));e->source->mutable[1].data=e;}
 else{assert(kind==2);e->source->contexts.bytes=sizeof(e->base);}
 memcpy(&before,&e->producer,sizeof(before));memcpy(&controller,&e->base.control,sizeof(controller));memcpy(&borrow,e->borrow,sizeof(borrow));
 assert(pt_editor_mixed_causal_song_pair_begin(&e->producer,e->source,e->borrow)==PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_INVALID&&
    !memcmp(&before,&e->producer,sizeof(before))&&!memcmp(&controller,&e->base.control,sizeof(controller))&&
    !memcmp(&borrow,e->borrow,sizeof(borrow))&&!e->base.binding->preparation_context&&!e->external.calls&&!e->base.ordinary.calls);
 cs_same(e);cs_drop(e);
}
static void cs_budget_refusal(unsigned kind)
{
 struct cs_trial *e=cs_make(24,16,0,CS_MIXED,0);unsigned n=0;
 if(!kind){e->producer.configuration.ordinary_budget=0;
    assert(pt_editor_mixed_causal_song_pair_begin(&e->producer,e->source,e->borrow)==PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_CAPACITY&&
       e->producer.adopted&&e->borrow->address==&e->base.control&&!e->external.calls&&!e->base.ordinary.calls&&!e->producer.pin_count);
 }else{assert(kind==1);--e->producer.configuration.audit_budget;cs_begin(e);
    while(!e->producer.first_error){(void)cs_step(e,17);assert(++n<10000);}
    assert(e->producer.first_error==PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_CAPACITY&&e->external.calls==1&&
       !e->outputs->audit&&!e->outputs->sequence&&!e->base.ordinary.calls&&!e->base.port.bind_calls);
 }
 cs_close(e);cs_same(e);cs_drop(e);
}
static void cs_original_guard(unsigned kind)
{
 struct cs_trial *e=cs_make(24,16,0,CS_MIXED,0);cs_begin(e);
 if(kind==0){struct pt_editor_mixed_causal_song_pair copy,original;memcpy(&copy,&e->producer,sizeof(copy));memcpy(&original,&e->producer,sizeof(original));
    assert(pt_editor_mixed_causal_song_pair_step(&copy,1)==PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_INVALID&&
       !memcmp(&original,&e->producer,sizeof(original)));}
 else if(kind==1){struct pt_editor_mixed_causal_source_borrow copy,original;memcpy(&copy,e->borrow,sizeof(copy));memcpy(&original,e->borrow,sizeof(original));
    assert(!pt_editor_mixed_causal_source_enter(&copy)&&!pt_editor_mixed_causal_source_borrow_close(&copy)&&!memcmp(&original,e->borrow,sizeof(original)));}
 else{assert(kind==2);cs_until(e,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_CONTROLLER);++e->outputs->candidate[1].normalized.record[15].track;
    assert(cs_step(e,1)==PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_FAULT&&!e->base.ordinary.calls&&!e->base.port.bind_calls);}
 cs_close(e);cs_same(e);cs_drop(e);
}
#ifndef PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_TEST_MAIN
#define PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_TEST_MAIN main
#endif
int PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_TEST_MAIN(void)
{
 static const enum pt_editor_mixed_causal_song_pair_phase cancelled[]={
  PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_ESTABLISH_BEGIN,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_ESTABLISH_STEP,
  PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_PIN_BEGIN,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_PIN_STEP,
  PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_PIN_CURRENT,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_PIN_FINISH,
  PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_ESTABLISH_CLOSE,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_ACTIVATE,
  PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_AUDIT_BEGIN,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_AUDIT_STEP,
  PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_AUDIT_GET,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_AUDIT_TAKE,
  PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_AUDIT_TRANSFER_CHECK,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_NEXT,
  PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_FORECAST_BEGIN,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_FORECAST_STEP,
  PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_Q_BEGIN,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_Q_STEP,
  PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_Q_GET,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_Q_CLOSE,
  PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_LOWER,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_CONSUME,
  PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_COMMIT,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_CONTROLLER,
  PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_LOAD,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_BATCH_BEGIN,
  PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_BATCH_STEP,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_REFERENCES,
  PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_ENQUEUE,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_PUBLICATION,
  PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_END};
 unsigned i,order;
 for(i=0;i<2;++i)for(order=0;order<2;++order)cs_success(i,order);
 assert(sizeof(cancelled)/sizeof(cancelled[0])==31);
 for(i=0;i<31;++i)cs_cancel_phase(cancelled[i]);
 for(i=0;i<6;++i)cs_song_refusal(i);
 for(i=0;i<3;++i)cs_external_fault(i);
 cs_bind_fault(0);cs_bind_fault(-1);cs_bind_fault(2);
 for(i=1;i<=3;++i)cs_publish_fault(i);
 cs_enqueue_outer_fault();cs_source_unknown(0);cs_source_unknown(-1);cs_source_unknown(2);
 for(i=0;i<3;++i)cs_begin_refusal(i);
 for(i=0;i<3;++i)cs_original_guard(i);
 for(i=0;i<2;++i)cs_budget_refusal(i);
 assert(cs_cases==62&&cp_cases==0&&ct_cases==0);
 puts("EDITOR MIXED CAUSAL SONG PAIR PASS:62 genuine before-master cases;4 mixed4Paula+12card/16card pure-TRIGGER pairs with24-bit masters/cache16 and independent C/R/source proof orders,31 named-phase cancellations,27 bounded shape/budget/alias/reentry/raw/identity refusals;early SOURCE before establishment and persistent pins,one activate refresh,READY masks/take-once full EOF replay,original960/1920 windows and empty2880 boundary,two copied complete snapshots before bind/factory/cache/publication,actual transfer and retained unknown source,exact saves/full capacities/tails/3 external requests/C2R32; SOFTWARE_ONLY");
 return 0;
}
