/* Genuine three-boundary producer SOURCE fixture. Both original donor entries
 * are uncalled. No forged ACTIVE/key/receipt, live timer/device or musical PLAY. */
#define PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_TEST_MAIN sl_original_producer62_not_called
#include "editor_mixed_causal_song_pair_test.c"
#undef PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_TEST_MAIN
#include "../src/native/readers_ram/native_mixed_causal_ram_port.h"
#include "../src/editor/editor_mixed_causal_song_lineage_internal.h"

struct sl_trial;
struct sl_external {
 unsigned calls,releases,alias_releases,reenter_at;
 void *alias,*pointer[PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REQUESTS];
 size_t bytes[PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REQUESTS];
 unsigned live[PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REQUESTS];
};
struct sl_adapter {
 struct sl_trial *owner;
 uint64_t ticks,ticket[3];uint32_t frequency;
 unsigned clocks,arms,proofs,shutdowns,probes,live[3],hold[3],arm_hook;
 int arm_raw,source_raw,quiet_raw;
};
struct sl_port {
 struct pt_private_mixed_causal_ram_port ram;
 struct sl_adapter external;
 struct pt_private_mixed_causal_ram_adapter declaration;
 unsigned binds;
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
 struct pt_private_mixed_causal_ram_port *ram=context;struct sl_adapter *adapter=ram->adapter.context;
 struct sl_trial *e=adapter->owner;struct sl_port *p=&e->port;struct pt_editor_mixed_causal_prepare *s=sl_s(e);
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
 return pt_private_mixed_causal_ram_bind(ram,s->causal,s->queue,registration->session,registration->generation);
}
/* Only the independent adapter boundary is injected. No fixture CONTROL,
 * STOP, commit, command-quiet or reader-quiet production callback exists. */
static int sl_adapter_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{
 struct sl_adapter *a=context;++a->clocks;*ticks=a->ticks;*frequency=a->frequency;return 1;
}
static int sl_adapter_arm(void *context,const struct pt_mixed_causal_registration *r,
 uint64_t ticket,uint64_t tick,uint32_t frequency)
{
 struct sl_adapter *a=context;struct pt_private_mixed_causal_ram_port *p=&a->owner->port.ram;
 unsigned i=a->arms;int raw=a->arm_raw;
 assert(i<3&&ct_registration(r,&p->registration)&&frequency==a->frequency&&ticket&&tick==oracle(960U*(i+1)));
 a->ticket[i]=ticket;a->live[i]=raw!=0||a->arm_hook;++a->arms;
 if(a->arm_hook){a->arm_hook=0;assert(pt_private_mixed_causal_ram_dispatch(p,ticket)==PT_MIXED_CAUSAL_INVALID);}
 return raw;
}
static int sl_adapter_quiet(void *context,const struct pt_mixed_causal_registration *r,uint64_t ticket,unsigned cancel)
{
 struct sl_adapter *a=context;unsigned i;
 assert(cancel<=1&&ct_registration(r,&a->owner->port.ram.registration));++a->proofs;
 for(i=0;i<a->arms&&a->ticket[i]!=ticket;++i){}assert(i<a->arms);
 if(a->hold[i])return 0;
 a->live[i]=0;return 1;
}
static int sl_adapter_close(void *context,const struct pt_mixed_causal_registration *r)
{
 struct sl_adapter *a=context;unsigned i;
 assert(ct_registration(r,&a->owner->port.ram.registration));for(i=0;i<a->arms;++i)assert(!a->live[i]);
 assert(++a->shutdowns==1);return a->source_raw;
}
static int sl_adapter_source_quiet(void *context,const struct pt_mixed_causal_registration *r)
{
 struct sl_adapter *a=context;unsigned i;
 assert(a->shutdowns==1&&ct_registration(r,&a->owner->port.ram.registration));for(i=0;i<a->arms;++i)assert(!a->live[i]);
 ++a->probes;return a->quiet_raw;
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
 assert(f->card&&f->document&&f->editor&&f->binding);f->bus.owner=f;f->ordinary.owner=f;f->masters.owner=f;f->chip.owner=f;e->port.external.owner=e;
 e->port.external.ticks=100;e->port.external.frequency=709379;
 e->port.external.arm_raw=e->port.external.source_raw=e->port.external.quiet_raw=1;
 e->port.declaration=(struct pt_private_mixed_causal_ram_adapter){&e->port.external,sizeof(e->port.external),
    sl_adapter_clock,sl_adapter_arm,sl_adapter_quiet,sl_adapter_close,sl_adapter_source_quiet};
 assert(pt_private_mixed_causal_ram_init_lineage(&e->port.ram,31,17,709379,&e->port.declaration,128,8));
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
 f->input.causal.port=pt_private_mixed_causal_ram_api(&e->port.ram);
 f->input.bind_original=sl_bind;
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
 e->producer.configuration.lineage=pt_private_mixed_causal_ram_lineage_api(&e->port.ram);
 assert(!pt_private_mixed_causal_ram_control_api(&e->port.ram).publish_control);
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
    !e->port.binds&&!e->port.external.clocks&&!e->port.ram.publishes);
 assert(pt_editor_mixed_causal_song_lineage_step(&e->producer,0)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_INVALID);
 assert(pt_editor_mixed_causal_song_lineage_step(&e->producer,4097)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_INVALID);
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
    assert(!e->base.ordinary.calls&&!e->port.binds&&!e->port.external.clocks&&!e->port.ram.publishes&&
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
static struct pt_private_mixed_causal_ram_command *sl_ram_command(struct sl_trial *e,unsigned history)
{
 unsigned i;for(i=0;i<2;++i)if(e->port.ram.command[i].known&&
    e->port.ram.command[i].identity.ticket==e->producer.history[history].ticket)return e->port.ram.command+i;
 return NULL;
}
static void sl_fire(struct sl_trial *e,unsigned index)
{
 struct pt_private_mixed_causal_ram_port *p=&e->port.ram;uint64_t ticket=e->producer.history[index].ticket;
 unsigned before=p->effects+p->controls+p->stops;
 assert(ticket&&e->producer.history[index].published);e->port.external.ticks=oracle(960U*(index+1))-1;
 assert(pt_private_mixed_causal_ram_dispatch(p,ticket)==PT_MIXED_CAUSAL_EARLY&&before==p->effects+p->controls+p->stops);
 e->port.external.ticks=oracle(960U*(index+1));assert(pt_private_mixed_causal_ram_dispatch(p,ticket)==PT_MIXED_CAUSAL_COMMITTED);
 assert(p->trace_count==3&&p->dispatch_result==PT_MIXED_CAUSAL_COMMITTED&&p->ledger_result==PT_MIXED_CAUSAL_COMMITTED);
}
static void sl_packet(struct sl_trial *e,unsigned index)
{
 struct pt_private_mixed_causal_ram_command *c=sl_ram_command(e,index);unsigned i,j;
 assert(c&&c->packet.frame==960U*(index+1)&&c->packet.first==oracle(c->packet.frame)&&
    c->packet.last==oracle(c->packet.frame+1)&&c->packet.count==16&&
    c->packet.registration.owner==sl_s(e)->causal&&c->packet.registration.queue==sl_s(e)->queue);
 for(i=0;i<c->packet.count;++i){const struct pt_mixed_readers_action *a=c->packet.action+i;
    assert(a->kind==(index==0?PT_MIXED_READERS_TRIGGER:index==1?PT_MIXED_READERS_CONTROL:PT_MIXED_READERS_STOP)&&
       c->packet.key[i].trigger==e->producer.history[0].ticket&&c->packet.key[i].route==a->route&&c->packet.key[i].slot==a->slot);
    if(!index&&a->route==PT_MIXED_READERS_PAULA){assert(a->geometry.paula.words==CS_MONO_FRAMES/2);
       for(j=0;j<CS_MONO_FRAMES;++j)assert(a->geometry.paula.data[j]==(e->base.bits==24&&(j==2||j==3)?0U:(j&1U)?255U:1U));}
    if(!index&&a->route==PT_MIXED_READERS_AMIGUS){const struct pt_mixed_readers_card *card=c->packet.card+i;
       assert(card->logical_bytes==CS_MONO_FRAMES*2&&card->full_capacity>=card->logical_bytes);
       for(j=0;j<CS_MONO_FRAMES;++j){uint16_t w=e->base.bits==24&&j==2?1U:e->base.bits==24&&j==3?65535U:(j&1U)?65280U:256U;
          assert(e->base.card->ram[card->address+2*j]==(uint8_t)(w>>8)&&e->base.card->ram[card->address+2*j+1]==(uint8_t)w);}}
    if(index==1&&a->route==PT_MIXED_READERS_AMIGUS){const struct pt_mixed_plan_quantized_batch *b=e->producer.frozen+1;
       assert(a->geometry.amigus.left==b->normalized.record[i].geometry.amigus.left&&
          a->geometry.amigus.right==b->normalized.record[i].geometry.amigus.right&&
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
 sl_commands(e,1);sl_readers(e,1);
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

/* Positive composition uses the genuine producer/factory/controller/core and
 * real RAM port callbacks. C1 allocation actually frees before C3; ASan in a
 * separately authorized future run detects any retired event/holder traversal. */
static struct sl_trial *sl_three(unsigned bits,unsigned card,unsigned order,unsigned source_hold)
{
 struct sl_trial *e=sl_make(bits,16,0,card?CS_CARD16:CS_MIXED,0);struct pt_private_mixed_causal_ram_port *p=&e->port.ram;
 struct pt_editor_mixed_reader_record readers[32];struct pt_private_mixed_causal_ram_reader saved_ram_readers[32];
 struct pt_private_mixed_causal_ram_completed root,control;
 struct pt_editor_mixed_reader_ref refs[32];unsigned char ram[sizeof(e->base.card->ram)];
 unsigned i,first_slot,ordinary,chips,masters,writes,held;uint64_t first,second,third;
 sl_preflight(e);sl_to_publication(e,0);sl_publish_current(e,0);sl_packet(e,0);
 first=e->producer.history[0].ticket;first_slot=e->producer.history[0].reference.slot;
 assert(pt_mixed_readers_commands_held(sl_s(e)->queue)==1&&pt_mixed_readers_readers_held(sl_s(e)->queue)==16&&!p->effects);
 sl_wait(e,1);sl_fire(e,0);root=p->completed;
 assert(root.valid&&root.identity.ticket==first&&root.frame==960&&root.observed==oracle(960)&&root.issued==oracle(960));
 e->port.external.hold[0]=1;
 assert(pt_editor_mixed_causal_song_lineage_service_command(&e->producer,0,0)==PT_MIXED_READERS_PENDING&&
    e->producer.history[0].service_seen&&!e->producer.history[0].detached&&!p->root_command_quiet);
 memcpy(readers,sl_s(e)->reader,sizeof(readers));memcpy(saved_ram_readers,p->reader,sizeof(saved_ram_readers));
 memcpy(ram,e->base.card->ram,sizeof(ram));for(i=0;i<32;++i)refs[i]=e->producer.reader[i].reference;
 ordinary=e->base.ordinary.calls;chips=e->base.chip.calls;masters=e->base.masters.calls;writes=e->base.card->writes;
 sl_to_publication(e,1);sl_publish_current(e,1);sl_packet(e,1);second=e->producer.history[1].ticket;
 assert(second!=first&&pt_mixed_readers_commands_held(sl_s(e)->queue)==2&&!p->control_completed.valid&&
    e->base.ordinary.calls==ordinary+1&&!memcmp(readers,sl_s(e)->reader,sizeof(readers)));
 sl_wait(e,2);sl_fire(e,1);control=p->control_completed;
 assert(control.valid&&control.identity.ticket==second&&control.frame==1920&&p->controls==16&&p->effects==16&&
    control.observed==oracle(1920)&&control.issued==oracle(1920));
 /* Actual CONTROL alone cannot admit STOP while C1 service is still pending. */
 assert(sl_step(e,17)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_WAIT_ACTIVE&&e->producer.attempted_mask==3&&p->pair_used==2);
 e->port.external.hold[0]=0;
 assert(pt_editor_mixed_causal_song_lineage_service_command(&e->producer,0,0)==PT_MIXED_READERS_OK&&
    e->producer.history[0].detached&&e->joint.original.root_detached&&e->joint.original.root_closed&&p->root_command_quiet&&
    !sl_s(e)->command[first_slot].handle.address&&!sl_s(e)->command[first_slot].handle.token&&
    pt_mixed_readers_commands_held(sl_s(e)->queue)==1);
 assert(!memcmp(&root,&p->completed,sizeof(root)));
 ordinary=e->base.ordinary.calls;sl_to_publication(e,2);
 assert(e->producer.history[2].reference.slot==first_slot&&
    e->producer.history[2].reference.serial!=e->producer.history[0].reference.serial);
 sl_publish_current(e,2);sl_packet(e,2);third=e->producer.history[2].ticket;
 assert(third!=first&&third!=second&&p->pair_used==3&&p->command[0].lineage_phase==3&&
    p->command[0].identity.ticket==third&&p->command[1].identity.ticket==second&&
    pt_mixed_readers_commands_held(sl_s(e)->queue)==2&&e->producer.lineage_index==3&&e->producer.attempted_mask==7&&
    e->base.ordinary.calls==ordinary+1&&e->base.chip.calls==chips&&e->base.masters.calls==masters&&
    e->base.card->writes==writes&&!memcmp(ram,e->base.card->ram,sizeof(ram)));
 assert(!memcmp(&root,&p->completed,sizeof(root))&&!memcmp(&control,&p->control_completed,sizeof(control)));
 for(i=0;i<32;++i){assert(refs[i].slot==e->producer.reader[i].reference.slot&&refs[i].serial==e->producer.reader[i].reference.serial);
    if(saved_ram_readers[i].live){assert(p->reader[i].live&&p->reader[i].active&&
       !memcmp(&saved_ram_readers[i].key,&p->reader[i].key,sizeof(p->reader[i].key))&&
       !memcmp(&saved_ram_readers[i].card,&p->reader[i].card,sizeof(p->reader[i].card)));
       if(p->reader[i].action.route==PT_MIXED_READERS_PAULA)assert(p->reader[i].action.geometry.paula.data==saved_ram_readers[i].action.geometry.paula.data&&
          p->reader[i].action.geometry.paula.words==saved_ram_readers[i].action.geometry.paula.words);
       else assert(p->reader[i].action.geometry.amigus.start==saved_ram_readers[i].action.geometry.amigus.start&&
          p->reader[i].action.geometry.amigus.loop==saved_ram_readers[i].action.geometry.amigus.loop&&
          p->reader[i].action.geometry.amigus.end_exclusive==saved_ram_readers[i].action.geometry.amigus.end_exclusive);}}
 memcpy(saved_ram_readers,p->reader,sizeof(saved_ram_readers));sl_fire(e,2);
 assert(p->stops==16&&!p->mask&&p->effects==16&&p->controls==16&&pt_mixed_readers_readers_held(sl_s(e)->queue)==16);
 for(i=0;i<32;++i)if(saved_ram_readers[i].live){saved_ram_readers[i].active=0;
    assert(!memcmp(saved_ram_readers+i,p->reader+i,sizeof(p->reader[i])));}
 /* Even inactive STOP readers retain geometry/leases while C2's expected-only
  * ticket proof is pending. No completion/count/zero-mask supplies quiet. */
 e->port.external.hold[1]=1;held=pt_mixed_readers_readers_held(sl_s(e)->queue);
 assert(pt_editor_mixed_causal_song_lineage_service_reader(&e->producer,0,0)==PT_MIXED_READERS_PENDING&&
    pt_mixed_readers_readers_held(sl_s(e)->queue)==held&&!p->source_attempted);
 e->port.external.hold[1]=0;
 if(!order)sl_commands(e,0);sl_readers(e,0);if(order)sl_commands(e,0);
 assert(!pt_mixed_readers_commands_held(sl_s(e)->queue)&&!pt_mixed_readers_readers_held(sl_s(e)->queue));
 for(i=0;i<32;++i)assert(!p->reader[i].live);for(i=0;i<20;++i)assert(!p->slot[i].serial);
 assert(!memcmp(&root,&p->completed,sizeof(root))&&!memcmp(&control,&p->control_completed,sizeof(control)));
 assert(e->base.chip.calls==chips&&e->base.masters.calls==masters&&e->base.card->writes==writes&&
    !memcmp(ram,e->base.card->ram,sizeof(ram)));sl_same(e);
 if(source_hold){
    e->port.external.source_raw=0;e->port.external.quiet_raw=source_hold==1?0:source_hold==2?-1:2;
    assert(pt_editor_mixed_causal_song_lineage_cancel(&e->producer)!=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_INVALID);
    for(i=0;!p->source_attempted&&i<1000;++i)assert(!pt_editor_mixed_causal_song_lineage_close(&e->producer));
    assert(i<1000&&p->source_attempted&&!p->source_closed&&e->port.external.shutdowns==1);
    assert(!pt_editor_mixed_causal_song_lineage_close(&e->producer)&&!p->source_closed&&
       e->borrow->address==sl_s(e)&&e->borrow->serial&&e->producer.pin_count&&
       e->base.binding->preparation_context==sl_s(e));e->port.external.quiet_raw=1;
 }
 sl_close(e);assert(p->source_closed&&e->port.external.shutdowns==1);sl_same(e);return e;
}
static void sl_history_refusal(unsigned which)
{
 struct sl_trial *e=sl_make(24,16,0,CS_MIXED,0);struct pt_private_mixed_causal_ram_port *p=&e->port.ram;
 unsigned arms;enum pt_mixed_readers_result r;
 sl_preflight(e);sl_to_publication(e,0);sl_publish_current(e,0);sl_fire(e,0);
 assert(pt_editor_mixed_causal_song_lineage_service_command(&e->producer,0,0)==PT_MIXED_READERS_OK);
 sl_to_publication(e,1);sl_publish_current(e,1);sl_fire(e,1);sl_to_publication(e,2);arms=e->port.external.arms;
 if(which)++p->control_completed.frame;else ++p->completed.frame;
 r=pt_editor_mixed_causal_song_lineage_publish(&e->producer);
 /* Clean history refusal is queue backpressure, not uncertain publication. */
 assert(r==PT_MIXED_READERS_PENDING&&e->port.external.arms==arms&&p->pair_used==2&&!p->stops&&
    !p->failed&&!p->suppressed&&!e->producer.history[2].published&&!e->producer.history[2].uncertain&&
    e->producer.lineage_index==2&&e->producer.phase==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PUBLICATION&&
    e->producer.working_command<PT_SAMPLER_MIXED_COMMANDS&&
    !e->producer.command[e->producer.working_command].published&&
    !e->producer.command[e->producer.working_command].uncertain);
 sl_close(e);sl_same(e);sl_drop(e);
}
static void sl_arm_refusal(unsigned which)
{
 struct sl_trial *e=sl_make(24,16,0,CS_MIXED,0);struct pt_private_mixed_causal_ram_port *p=&e->port.ram;
 enum pt_mixed_readers_result r;
 sl_preflight(e);sl_to_publication(e,0);sl_publish_current(e,0);sl_fire(e,0);
 assert(pt_editor_mixed_causal_song_lineage_service_command(&e->producer,0,0)==PT_MIXED_READERS_OK);
 sl_to_publication(e,1);sl_publish_current(e,1);sl_fire(e,1);sl_to_publication(e,2);
 e->port.external.arm_raw=which==1?-1:0;e->port.external.arm_hook=which==2;
 r=pt_editor_mixed_causal_song_lineage_publish(&e->producer);
 assert(r!=PT_MIXED_READERS_OK&&e->port.external.arms==3&&p->pair_used==3&&!p->stops&&
    !e->producer.history[2].published);
 if(which)assert(r==PT_MIXED_READERS_BACKEND&&p->failed&&p->suppressed&&
    p->command[0].live&&p->command[0].uncertain&&e->producer.history[2].uncertain);
 /* Clean0 consumes the once-only C3 row and proves it quiet. PENDING does
  * not authorize rearm: this fixture cancels/drains without publishing again. */
 else assert(r==PT_MIXED_READERS_PENDING&&!p->failed&&!p->suppressed&&
    !p->command[0].live&&!p->command[0].armed&&p->command[0].quiet_confirmed&&
    !e->producer.history[2].uncertain&&e->producer.lineage_index==2&&
    e->producer.phase==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PUBLICATION&&
    e->producer.working_command<PT_SAMPLER_MIXED_COMMANDS&&
    !e->producer.command[e->producer.working_command].published&&
    !e->producer.command[e->producer.working_command].uncertain);
 sl_close(e);sl_same(e);sl_drop(e);
}
static void sl_capabilities(void)
{
 struct pt_private_mixed_causal_ram_port *p=calloc(3,sizeof(*p));struct sl_adapter external={0};
 struct pt_private_mixed_causal_ram_adapter a={&external,sizeof(external),sl_adapter_clock,sl_adapter_arm,
    sl_adapter_quiet,sl_adapter_close,sl_adapter_source_quiet};assert(p);
 assert(pt_private_mixed_causal_ram_init(p,31,17,709379,&a,128,8)&&
    !pt_private_mixed_causal_ram_control_api(p).publish_control&&!pt_private_mixed_causal_ram_lineage_api(p).publish_control);
 assert(pt_private_mixed_causal_ram_init_control(p+1,31,17,709379,&a,128,8)&&
    pt_private_mixed_causal_ram_control_api(p+1).publish_control&&!pt_private_mixed_causal_ram_lineage_api(p+1).publish_control);
 assert(pt_private_mixed_causal_ram_init_lineage(p+2,31,17,709379,&a,128,8)&&
    !pt_private_mixed_causal_ram_control_api(p+2).publish_control&&pt_private_mixed_causal_ram_lineage_api(p+2).publish_control&&
    pt_private_mixed_causal_ram_lineage_api(p+2).publish_stop_after_control);free(p);
}
int main(void)
{
 unsigned bits,card,order,i;
 for(bits=16;bits<=24;bits+=8)for(card=0;card<2;++card)for(order=0;order<2;++order){
    struct sl_trial *e=sl_three(bits,card,order,0);sl_drop(e);}
 for(i=1;i<=3;++i){struct sl_trial *e=sl_three(24,0,0,i);sl_drop(e);}
 for(i=0;i<2;++i)sl_history_refusal(i);for(i=0;i<3;++i)sl_arm_refusal(i);sl_capabilities();
 assert(sl_cases==16&&cs_cases==0&&cp_cases==0&&ct_cases==0);
 puts("FINITE RAM TRIGGER-CONTROL16-STOP SOURCE FIXTURE PASS:17 cases;16 genuine producer/factory/core compositions and1 disjoint capability check;8 mixed/all-card master16/24 cache16 sequences;exact960/1920/2880;two C slots and actual C1 disposal before C3;copied root/control histories;original16 persistent readers and full16 levels;expected-only pending quiet and both C/R orders;3 pending/unknown source retentions;2 genuine-history mismatches;clean0/unknown/reentrant0 third arms;SOFTWARE_ONLY_NO_NATIVE_ACTIVATION");
 return 0;
}
