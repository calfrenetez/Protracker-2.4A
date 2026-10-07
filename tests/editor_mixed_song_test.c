/* SOURCE draft only; never compiled/imported/executed by this author.
 * Dedicated existing entry selector preserves genuine prior suites once.
 * Root executes the separate early SOURCE/query/readiness/retirement suites.
 * No mock READY/ACTIVE/quiet flag, synthetic issued reference, target, IRQ,
 * timer or MMIO authority is substituted for the genuine product chain.
 */
#define PT_EDITOR_MIXED_READERS_QUANTIZED_TEST_MAIN eps_quantized_main
#include "editor_mixed_readers_quantized_test.c"
#undef PT_EDITOR_MIXED_READERS_QUANTIZED_TEST_MAIN
#include "../src/editor/editor_mixed_song_internal.h"

#define EPS_MONO_FRAMES 2048U
#define EPS_MONO_CAPACITY 2112U
#define EPS_TAIL 257U
enum eps_case {EPS_EMPTY,EPS_TEMPO,EPS_MIXED,EPS_CARD16,EPS_REPLACEMENTS};
struct eps_external {
 unsigned calls,releases,alias_releases,reenter_at,alias_at;
 unsigned recycle_at,recycled_new_owners,fixture_only_retirements;
 void *alias,*pointer[PT_EDITOR_MIXED_SONG_REQUESTS];
 size_t bytes[PT_EDITOR_MIXED_SONG_REQUESTS],backing_capacity[PT_EDITOR_MIXED_SONG_REQUESTS];
 unsigned live[PT_EDITOR_MIXED_SONG_REQUESTS];
};
/* Created at its COMPLETE size initially. Never copy/realloc an attached owner.
 * emp_trial stays first solely to reuse unchanged genuine port/cache callbacks.
 * Producer and controller are distinct original subobjects of this whole P.
 */
struct eps_trial {
 struct emp_trial base;
 struct pt_editor_mixed_song producer;
 struct eps_external external;
 struct pt_editor_mixed_source_inputs *source;
 struct pt_editor_mixed_source_borrow *borrow;
 struct pt_editor_mixed_song_outputs *outputs;
 void *audit_storage,*audit_q,*q_storage,*audit_result;
 size_t audit_capacity,audit_q_capacity,q_capacity,result_capacity,output_capacity;
 int32_t *original_mono,*original_stereo;
 uint32_t *original_slices;
 struct pt_sample *real_samples;struct pt_event *real_events;
 unsigned promoting,master_reenter_at,master_observed,master_calls_at_begin;
 unsigned source_begin_calls,closed,kind,publication_outcome,outer_configuration_fault;
 unsigned char tail[EPS_TAIL];
};
static void *eps_master_new(void *context,size_t bytes)
{
 struct emp_trial *f=(void *)((char *)context- offsetof(struct emp_trial,masters));
 struct eps_trial *e=(void *)f;
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
 return emp_new(&f->masters,bytes);
}
static void *eps_external_new(void *context,size_t bytes)
{
 struct eps_external *m=context;
 struct eps_trial *e=(void *)((char *)context- offsetof(struct eps_trial,external));
 struct emp_trial *f=&e->base;unsigned index=m->calls++,i;
 assert(index<PT_EDITOR_MIXED_SONG_REQUESTS&&bytes&&e->producer.busy&&
    f->control.source_busy&&!f->control.busy&&e->borrow->address==&f->control&&
    f->binding->preparation_context==&f->control&&f->binding->preparation_close);
 if(index){
    assert(f->control.source_activated&&e->producer.pin_count==f->editor->project->sample_count);
    for(i=0;i<f->editor->project->sample_count;++i)
       assert(e->producer.pin[i]&&e->producer.pin[i]==f->editor->sampler.current[i]);
 }else assert(!f->control.source_activated&&!f->control.activation&&!f->control.pool);
 if(m->reenter_at==index+1){
    assert(pt_editor_mixed_song_step(&e->producer,1)==PT_EDITOR_MIXED_SONG_FAULT);
    assert(!pt_editor_prepare_change(f->editor));
 }
 if(m->alias&&(!m->alias_at||m->alias_at==index+1))return m->alias;
 if(m->recycle_at==index+1){
    /* A real new owner from this fixture allocator's arena, at the numerical
     * extent of its genuinely released metadata request. This is distinct
     * from the old unowned alias branch above: new request/live ownership and
     * all requested backing bytes are real, without editing product custody. */
    assert(index&&m->pointer[0]&&!m->live[0]&&m->releases&&
       bytes<=m->backing_capacity[0]&&!m->recycled_new_owners);
    m->pointer[index]=m->pointer[0];m->bytes[index]=bytes;m->live[index]=1;
    ++m->recycled_new_owners;memset(m->pointer[index],0xe7,bytes);
    return m->pointer[index];
 }
 m->backing_capacity[index]=bytes;
 if(m->recycle_at&&!index){size_t setup=pt_render_sequence_setup_control_size();
    size_t sequence=pt_render_sequence_control_size();
    if(setup>m->backing_capacity[index])m->backing_capacity[index]=setup;
    if(sequence>m->backing_capacity[index])m->backing_capacity[index]=sequence;
 }
 m->pointer[index]=malloc(m->backing_capacity[index]);assert(m->pointer[index]);
 m->bytes[index]=bytes;m->live[index]=1;return m->pointer[index];
}
static void eps_external_release(void *context,void *pointer)
{
 struct eps_external *m=context;
 struct eps_trial *e=(void *)((char *)context- offsetof(struct eps_trial,external));unsigned i,j;
 assert(e->producer.busy&&e->base.control.source_busy&&!e->base.control.busy);
 if(pointer==m->alias){++m->alias_releases;return;}
 for(i=0;i<m->calls&&m->pointer[i]!=pointer;++i){}
 assert(i<m->calls&&m->live[i]);
 for(j=0;j<e->producer.requests;++j)
    if(e->producer.allocation[j].address==(uintptr_t)pointer)assert(!e->producer.allocation[j].live);
 m->live[i]=0;++m->releases;
 /* Ordinary groups quarantine consumed backing until fixture teardown. The
  * explicit recycling group creates a separately live new allocator owner
  * from that arena; neither logical release nor recycling is a product quiet
  * certificate. The arena itself has independent fixture lifetime. */
}
static int eps_owned(void *context)
{
 struct emp_trial *f=(void *)((char *)context- offsetof(struct emp_trial,bus));
 struct eps_trial *e=(void *)f;int actual=emp_owned(context);
 if(e->outer_configuration_fault&&e->producer.phase==PT_EDITOR_MIXED_SONG_ENQUEUE){
    assert(actual&&e->producer.busy&&f->control.busy);e->outer_configuration_fault=0;
    /* Deliberately violate only the producer's retained immutable config after
     * a genuine positive card-owned callback. Never fabricate queue ownership,
     * a returned ticket, registration or normalizer origin. Lower layers use
     * their untouched copied inputs; the producer must retain their actual
     * enqueue outcome before its own separate fixed-config failure. */
    ++e->producer.configuration.options.tick_limit;
 }
 return actual;
}
static int eps_publish(void *context,struct pt_mixed_readers_activation *owner,const struct pt_mixed_activation_packet *packet)
{
 struct emp_trial *f=emp_from_port(context);struct eps_trial *e=(void *)f;
 if(!e->publication_outcome)return emp_publish(context,owner,packet);
 if(e->publication_outcome==1){++f->port.publishes;
    assert(owner==packet->registration.owner&&f->port.ticks<packet->first);return 0;}
 assert(e->publication_outcome==2);(void)emp_publish(context,owner,packet);return -1;
}
static void eps_fill_tail(void *p,size_t used,size_t capacity,unsigned byte)
{assert(capacity>=used);memset((unsigned char *)p+used,(int)byte,capacity-used);}
static void eps_tail_same(const void *p,size_t used,size_t capacity,unsigned byte)
{size_t i;const unsigned char *v=p;for(i=used;i<capacity;++i)assert(v[i]==byte);}
static void eps_same(struct eps_trial *e)
{
 struct emp_trial *f=&e->base;struct pt_project *p=f->editor->project;
 uint8_t *bytes;size_t n;unsigned i;
 p->channels.selected=0;bytes=emp_save(f,&n);
 assert(n==f->saved_bytes&&!memcmp(bytes,f->saved,n));free(bytes);
 assert(p->samples[0].pcm.bits==f->bits&&p->samples[0].pcm.channels==1&&
    p->samples[0].pcm.frames==EPS_MONO_FRAMES);
 for(i=0;i<EPS_MONO_FRAMES;++i)
    assert(p->samples[0].pcm.data[i]==e->original_mono[i]);
 assert(p->samples[1].pcm.bits==f->bits&&p->samples[1].pcm.channels==2&&p->samples[1].pcm.frames==6);
 assert(!memcmp(p->samples[1].pcm.data,e->original_stereo,12*sizeof(int32_t)));
 assert(p->samples[1].slice_count==2&&p->samples[1].slices[0]==1&&p->samples[1].slices[1]==5);
 eps_tail_same(f->activation_workspace,f->activation_capacity,f->activation_capacity+EPS_TAIL,0xa1);
 eps_tail_same(f->factory_workspace,f->factory_capacity,f->factory_capacity+EPS_TAIL,0xa2);
 eps_tail_same(e->audit_storage,pt_mixed_quantized_audit_workspace_size(),e->audit_capacity,0xa3);
 eps_tail_same(e->audit_q,pt_mixed_plan_normalizer_workspace_size(),e->audit_q_capacity,0xa4);
 eps_tail_same(e->q_storage,pt_mixed_plan_normalizer_workspace_size(),e->q_capacity,0xa5);
 eps_tail_same(e->audit_result,pt_mixed_quantized_audit_result_size(),e->result_capacity,0xa6);
 eps_tail_same(e->outputs,sizeof(*e->outputs),e->output_capacity,0xa7);
 for(i=0;i<EPS_TAIL;++i)assert(e->tail[i]==0xc1);
}
static struct eps_trial *eps_make(unsigned bits,unsigned cache_bits,unsigned little,enum eps_case which,unsigned lead)
{
 struct eps_trial *e=calloc(1,sizeof(*e));struct emp_trial *f;struct pt_allocator a;
 struct pt_project *p;struct pt_amigus_reservation_api api;unsigned i,j;
 assert(e);f=&e->base;f->bits=bits;f->cache_bits=cache_bits;f->little=little;e->kind=(unsigned)which;
 f->card=calloc(1,sizeof(*f->card));f->document=calloc(1,sizeof(*f->document));
 f->editor=calloc(1,sizeof(*f->editor));f->binding=calloc(1,sizeof(*f->binding));
 assert(f->card&&f->document&&f->editor&&f->binding);f->bus.owner=f;
 a=(struct pt_allocator){&f->masters,eps_master_new,emp_master_free};
 pt_document_init(f->document,&a);assert(pt_document_new(f->document,16,4*1024*1024)==PT_PROJECT_OK);
 p=&f->document->project;e->real_events=p->events;
 for(i=0;i<16;++i){p->channels.track[i].route=(which==EPS_CARD16||which==EPS_REPLACEMENTS)?PT_AMIGUS:(i<4?PT_PAULA:PT_AMIGUS);
    /* Preserve genuine classic Paula's exact one-side gains. Center only the
     * card tracks; Paula's checked adapter must refuse a nonzero other side. */
    if(p->channels.track[i].route==PT_AMIGUS)p->channels.track[i].pan=128;}
 e->original_mono=emp_new(&f->masters,EPS_MONO_CAPACITY*sizeof(int32_t));
 e->original_stereo=emp_new(&f->masters,64*sizeof(int32_t));
 e->original_slices=emp_new(&f->masters,2*sizeof(uint32_t));
 for(i=0;i<EPS_MONO_CAPACITY;++i)e->original_mono[i]=(i&1U)?-(int32_t)(1U<<(bits-8)):(int32_t)(1U<<(bits-8));
 if(bits==24){e->original_mono[2]=257;e->original_mono[3]=-257;}
 for(j=0;j<64;++j)e->original_stereo[j]=(int32_t)((int)(j%12)-6)*(int32_t)(1U<<(bits-8));
 e->original_slices[0]=1;e->original_slices[1]=5;
 p->samples[0].pcm=(struct pt_pcm){e->original_mono,EPS_MONO_CAPACITY,EPS_MONO_FRAMES,8000,1,(uint8_t)bits};
 p->samples[0].volume=64;
 p->samples[1].pcm=(struct pt_pcm){e->original_stereo,64,6,8000,2,(uint8_t)bits};p->samples[1].volume=64;
 p->samples[1].slices=e->original_slices;p->samples[1].slice_count=2;
 memset(p->events,0,64*16*sizeof(*p->events));p->speed=which==EPS_TEMPO?2:1;p->bpm=125;
 if(which==EPS_TEMPO){
    p->events[0]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    p->events[16+15].effect=15;p->events[16+15].parameter=137;
    p->events[32+15].effect=14;p->events[32+15].parameter=0xe1;p->events[48+15].effect=15;
 }else if(which==EPS_MIXED||which==EPS_CARD16){
    for(i=0;i<16;++i)p->events[i]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    p->events[32+15].effect=15;
 }else if(which==EPS_REPLACEMENTS){
    for(j=0;j<34;++j)for(i=0;i<16;++i)p->events[j*16+i]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    p->events[34*16+15].effect=15;
 }else p->events[32+15].effect=15;
 assert(pt_editor_init(f->editor,p));pt_sampler_init(&f->editor->sampler,&a,4*1024*1024);
 {struct pt_pcm spare;int32_t *values=emp_new(&f->masters,64*sizeof(int32_t));
    for(i=0;i<64;++i)values[i]=(int32_t)i;
    spare=(struct pt_pcm){values,64,1,8000,1,24};
    assert(pt_sampler_append_owned(&f->editor->sampler,p,&f->editor->history,&spare,&a,"unused spare")==PT_EDIT_OK&& !spare.data);}
 e->real_samples=p->samples;assert(pt_editor_mixed_attach(f->binding,f->editor));
 f->library.available=f->library.supported=f->library.count=1;f->card->healthy=1;
 api=(struct pt_amigus_reservation_api){&f->library,open_library,close_library,find,supported,reserve,release};
 assert(pt_amigus_reservation_open_resource(&f->card->reservation,&api,0,PT_AMIGUS_WAVETABLE)==PT_AMIGUS_RESERVED);
 assert(pt_amigus_wavetable_cache_attach(&f->card->cache,&f->card->reservation,0,4096,4096,&f->bus,eps_owned,emp_write));
 f->input.binding=f->binding;f->input.backend=&f->card->cache;
 f->input.activation.allocator=(struct pt_allocator){&f->ordinary,emp_allocate,emp_release};
 f->input.activation.allocator_context=(struct pt_mixed_readers_span){&f->ordinary,sizeof(f->ordinary)};
 f->input.activation.grid=(struct pt_mixed_readers_grid){100,17,709379,48000};f->input.activation.session=31;
 f->input.activation.control_budget=pt_mixed_activation_control_size();f->input.activation.queue_budget=pt_mixed_readers_control_size();
 f->input.activation.port=(struct pt_mixed_activation_port){&f->port,sizeof(f->port),PT_MIXED_ACTIVATION_PORT_VERSION,
    PT_MIXED_ACTIVATION_PORT_REQUIRED,emp_clock,eps_publish,sma_commit,emp_command_quiet,emp_reader_quiet,emp_source_close,emp_source_quiet};
 f->port.ticks=100;f->port.reader_allow=1;f->port.close_result=f->port.quiet_result=1;
 f->input.chip_context=&f->chip;f->input.chip_allocate=emp_chip_new;f->input.chip_release=emp_chip_free;
 f->input.factory_budget=pt_sampler_mixed_pool_size()+2*pt_sampler_mixed_command_size()+32*pt_sampler_mixed_reader_size();
 f->input.chip_budget=16384;f->input.contexts=(struct pt_sampler_storage_span){e,sizeof(*e)};
 f->activation_capacity=pt_mixed_activation_workspace_size()+EPS_TAIL;f->factory_capacity=pt_sampler_mixed_workspace_size()+EPS_TAIL;
 /* E/F constructors may use or clear their entire advertised scratch. Keep
  * those exact query+EPS_TAIL capacities/spans and inside-tail alias guards.
  * Additional fixture-only backing is an external overrun guard, outside the
  * borrowed writable workspace; it is not a narrowed source parent. Fresh
  * advertised E/F scratch stays completely zero before genuine admission. */
 f->activation_workspace=calloc(1,f->activation_capacity+EPS_TAIL);
 f->factory_workspace=calloc(1,f->factory_capacity+EPS_TAIL);
 assert(f->activation_workspace&&f->factory_workspace);
 f->input.activation_workspace=f->activation_workspace;f->input.activation_capacity=f->activation_capacity;
 f->input.factory_workspace=f->factory_workspace;f->input.factory_capacity=f->factory_capacity;
 e->source=calloc(1,sizeof(*e->source));e->borrow=calloc(1,sizeof(*e->borrow));
 e->audit_capacity=pt_mixed_quantized_audit_workspace_size()+EPS_TAIL;
 e->audit_q_capacity=pt_mixed_plan_normalizer_workspace_size()+EPS_TAIL;e->q_capacity=e->audit_q_capacity;
 e->result_capacity=pt_mixed_quantized_audit_result_size()+EPS_TAIL;e->output_capacity=sizeof(*e->outputs)+EPS_TAIL;
 e->audit_storage=calloc(1,e->audit_capacity);e->audit_q=calloc(1,e->audit_q_capacity);
 e->q_storage=calloc(1,e->q_capacity);e->audit_result=calloc(1,e->result_capacity);e->outputs=calloc(1,e->output_capacity);
 assert(e->source&&e->borrow&&e->audit_storage&&e->audit_q&&e->q_storage&&e->audit_result&&e->outputs);
 e->source->binding=f->binding;e->source->preparation=&e->producer.preparation;e->source->contexts=f->input.contexts;
 e->source->immutable_count=6;e->source->mutable_count=2;
 e->source->immutable[0]=(struct pt_sampler_storage_span){f->activation_workspace,f->activation_capacity};
 e->source->immutable[1]=(struct pt_sampler_storage_span){f->factory_workspace,f->factory_capacity};
 e->source->immutable[2]=(struct pt_sampler_storage_span){f->card,sizeof(*f->card)};
 e->source->immutable[3]=(struct pt_sampler_storage_span){e->audit_storage,e->audit_capacity};
 e->source->immutable[4]=(struct pt_sampler_storage_span){e->audit_q,e->audit_q_capacity};
 e->source->immutable[5]=(struct pt_sampler_storage_span){e->q_storage,e->q_capacity};
 e->source->mutable[0]=(struct pt_sampler_storage_span){e->audit_result,e->result_capacity};
 e->source->mutable[1]=(struct pt_sampler_storage_span){e->outputs,e->output_capacity};
 e->source->activation_workspace=e->source->immutable[0];e->source->factory_workspace=e->source->immutable[1];
 e->source->backend_parent=e->source->immutable[2];e->producer.controller=&f->control;
 e->producer.configuration.preparation=f->input;
 e->producer.configuration.options=(struct pt_render_options){0};
 e->producer.configuration.options.rate=48000;e->producer.configuration.options.bits=24;
 e->producer.configuration.options.gain_q16=65536;e->producer.configuration.options.tracks=0xffff;
 e->producer.configuration.options.tick_limit=1000;e->producer.configuration.options.frame_limit=100000;
 e->producer.configuration.options.include_lead_in=(uint8_t)lead;
 e->producer.configuration.caps=(struct pt_paula_render_caps){3546895,124,65535};
 e->producer.configuration.format.bits=(uint8_t)cache_bits;e->producer.configuration.format.little_endian=(uint8_t)little;
 e->producer.configuration.external_allocator=(struct pt_allocator){&e->external,eps_external_new,eps_external_release};
 e->producer.configuration.ordinary_budget=4*1024*1024;
 e->producer.configuration.audit_budget=e->audit_capacity+e->audit_q_capacity+e->result_capacity+
    pt_render_sequence_setup_control_size()+pt_render_sequence_control_size();
 e->producer.configuration.absolute_start=48000;
 eps_fill_tail(f->activation_workspace,f->activation_capacity,f->activation_capacity+EPS_TAIL,0xa1);
 eps_fill_tail(f->factory_workspace,f->factory_capacity,f->factory_capacity+EPS_TAIL,0xa2);
 eps_fill_tail(e->audit_storage,pt_mixed_quantized_audit_workspace_size(),e->audit_capacity,0xa3);
 eps_fill_tail(e->audit_q,pt_mixed_plan_normalizer_workspace_size(),e->audit_q_capacity,0xa4);
 eps_fill_tail(e->q_storage,pt_mixed_plan_normalizer_workspace_size(),e->q_capacity,0xa5);
 eps_fill_tail(e->audit_result,pt_mixed_quantized_audit_result_size(),e->result_capacity,0xa6);
 eps_fill_tail(e->outputs,sizeof(*e->outputs),e->output_capacity,0xa7);memset(e->tail,0xc1,sizeof(e->tail));
 f->saved=emp_save(f,&f->saved_bytes);assert(pt_project_validate(p,NULL)==PT_PROJECT_OK);
 e->master_calls_at_begin=f->masters.calls;return e;
}
static enum pt_editor_mixed_song_result eps_step(struct eps_trial *e,unsigned work)
{
 enum pt_editor_mixed_song_result r;unsigned phase=e->producer.phase;
 e->promoting=phase<=PT_EDITOR_MIXED_SONG_ESTABLISH_CLOSE;
 r=pt_editor_mixed_song_step(&e->producer,work);e->promoting=0;return r;
}
static void eps_begin(struct eps_trial *e)
{
 struct emp_trial *f=&e->base;unsigned before=f->masters.calls;
 assert(!f->editor->sampler.current[0]&&!f->editor->sampler.current[1]);
 assert(pt_editor_mixed_song_begin(&e->producer,e->source,e->borrow)==PT_EDITOR_MIXED_SONG_PENDING);
 ++e->source_begin_calls;
 assert(e->borrow->address==&f->control&&e->borrow->serial&&e->producer.adopted&&
    e->producer.original==&e->producer&&f->control.phase==PT_EDITOR_MIXED_READERS_SOURCE&&
    f->binding->preparation_context==&f->control&&f->binding->preparation_close&& !f->control.inputs);
 assert(before==f->masters.calls&&!f->ordinary.calls&&!e->external.calls&&!f->chip.calls&&!f->card->writes&&!f->port.reads);
 assert(pt_editor_mixed_song_step(&e->producer,0)==PT_EDITOR_MIXED_SONG_INVALID);
 assert(pt_editor_mixed_song_step(&e->producer,4097)==PT_EDITOR_MIXED_SONG_INVALID);
}
static void eps_until(struct eps_trial *e,enum pt_editor_mixed_song_phase wanted)
{
 unsigned n=0;enum pt_editor_mixed_song_result r;
 while(e->producer.phase!=wanted){r=eps_step(e,17);
    assert(r==PT_EDITOR_MIXED_SONG_PENDING&&++n<100000);}
}
static void eps_close_empty(struct eps_trial *e)
{
 unsigned i,n=0;struct emp_trial *f=&e->base;
 assert(pt_editor_mixed_song_cancel(&e->producer)!=PT_EDITOR_MIXED_SONG_INVALID);
 while(!pt_editor_mixed_song_close(&e->producer)){assert(++n<1000);
    assert(f->binding->preparation_context==&f->control);
    if(e->producer.borrow_closed)assert(!e->borrow->address&&!e->borrow->serial);
    else assert(e->borrow->address==&f->control);}
 assert(!e->outputs->audit&&!e->outputs->sequence&&!e->outputs->normalizer&&!e->producer.establish.active&&!e->producer.pin_job.owner);
 assert(!e->producer.pin_count&&!e->borrow->address&&!e->borrow->serial&&!f->binding->preparation_context);
 assert(!f->control.pool&&!f->control.activation&&!f->control.queue&&!e->external.alias_releases);
 for(i=0;i<PT_PROJECT_SAMPLES;++i)assert(!e->producer.pin[i]);
 for(i=0;i<e->external.calls;++i)assert(!e->external.live[i]);
 e->closed=1;
}
static void eps_drop_storage(struct eps_trial *e)
{
 struct emp_trial *f=&e->base;unsigned i,j;
 assert(!f->binding->preparation_context&&!e->borrow->address&&!e->borrow->serial);
 assert(pt_editor_mixed_detach(f->binding));assert(pt_editor_dispose(f->editor));
 assert(pt_amigus_wavetable_cache_detach(&f->card->cache));assert(pt_amigus_reservation_close(&f->card->reservation));
 emp_free(&f->masters,e->original_mono,EPS_MONO_CAPACITY*sizeof(int32_t),1);
 emp_free(&f->masters,e->original_stereo,64*sizeof(int32_t),1);
 emp_free(&f->masters,e->original_slices,2*sizeof(uint32_t),1);pt_document_release(f->document);
 for(i=0;i<80;++i)assert(!f->ordinary.live[i].p&&!f->chip.live[i].p&&!f->masters.live[i].p);
 assert(!f->ordinary.alias_releases&&!f->chip.alias_releases&&!e->external.alias_releases);
 for(i=0;i<e->external.calls;++i){
    assert(!e->external.live[i]);
    for(j=0;j<i&&e->external.pointer[j]!=e->external.pointer[i];++j){}
    if(j==i)free(e->external.pointer[i]);
 }
 free(f->saved);free(f->activation_workspace);free(f->factory_workspace);free(f->card);
 free(f->document);free(f->editor);free(f->binding);free(e->audit_storage);free(e->audit_q);
 free(e->q_storage);free(e->audit_result);free(e->outputs);free(e->source);
 assert(pt_editor_mixed_source_borrow_close(e->borrow));free(e->borrow);free(e);
}
static void eps_drop(struct eps_trial *e)
{assert(e->closed);eps_drop_storage(e);}
static void eps_end_unresolved_fixture_lifetime(struct eps_trial *e)
{
 struct emp_trial *f=&e->base;unsigned i,retired=0;
 assert(!e->closed&&e->producer.first_error==PT_EDITOR_MIXED_SONG_FAULT&&
    e->producer.quiet_ambiguous&&e->producer.cleanup_phase==6&&e->producer.pin_count==32&&
    !e->producer.borrow_closed&&!e->producer.hook_closed&&e->borrow->address==&f->control&&
    f->binding->preparation_context==&f->control&&
    pt_editor_mixed_source_children_closed(&f->control,e->borrow));
 /* END OF TEST LIFETIME ONLY. No producer call or product recovery follows.
  * This harness owns the entire fake allocator arena and captured resources.
  * It explicitly retires its fresh recycled arena owner without invoking the
  * product's allocator release. Unknown product custody/barrier/accounting
  * remains unchanged, and no CLOSED result or successful producer close is
  * manufactured. Fixture teardown is not application acceptance. */
 for(i=0;i<e->external.calls;++i)if(e->external.live[i]){
    assert(e->external.recycle_at==i+1&&e->external.pointer[i]==e->external.pointer[0]&&
       e->producer.allocation[i].unresolved&&!e->producer.allocation[i].live);
    e->external.live[i]=0;++e->external.fixture_only_retirements;++retired;
 }
 assert(retired==e->external.recycled_new_owners);
 for(i=0;i<PT_PROJECT_SAMPLES;++i)if(e->producer.pin[i]){
    assert(pt_editor_mixed_source_enter(e->borrow));
    pt_sampler_unpin(e->producer.pin[i]);
    assert(pt_editor_mixed_source_leave(e->borrow));
 }
 /* These genuine original SOURCE operations only dispose the now-ended
  * fixture's captured lifetime. Product pin slots and quiet-ambiguity flag are
  * deliberately not cleared, and the producer is never used again. */
 assert(pt_editor_mixed_source_borrow_close(e->borrow)&&!e->borrow->address&&!e->borrow->serial);
 assert(pt_editor_mixed_readers_prepare_close(&f->control)&&!f->binding->preparation_context);
 assert(!e->closed&&e->producer.pin_count==32&&e->producer.quiet_ambiguous&&
    !e->producer.borrow_closed&&!e->producer.hook_closed&&
    e->producer.phase==PT_EDITOR_MIXED_SONG_DRAIN&&e->producer.result==PT_EDITOR_MIXED_SONG_FAULT);
 eps_drop_storage(e);
}
static void eps_preparation(unsigned bits,unsigned cache_bits,unsigned little)
{
 struct eps_trial *e=eps_make(bits,cache_bits,little,EPS_MIXED,0);unsigned i;
 eps_begin(e);eps_until(e,PT_EDITOR_MIXED_SONG_AUDIT_BEGIN);
 assert(e->producer.pin_count==e->base.editor->project->sample_count&&e->producer.activated&&
    e->external.calls==1&&e->external.releases==1&&!e->outputs->audit&&e->master_observed);
 for(i=0;i<e->base.editor->project->sample_count;++i)
    assert(e->producer.pin[i]==e->base.editor->sampler.current[i]&&e->producer.pin[i]);
 eps_same(e);eps_until(e,PT_EDITOR_MIXED_SONG_NEXT);
 assert(e->external.calls==3&&e->producer.requests==3&&e->outputs->audit&&e->outputs->sequence&&
    e->producer.audited.rewinds==1&&e->producer.audited.frames==1920&&e->producer.audited.intervals==3);
 assert(e->base.ordinary.calls==3&&!e->base.chip.calls&&!e->base.card->writes&&!e->base.port.reads);
 eps_same(e);eps_close_empty(e);eps_same(e);eps_drop(e);
}
static void eps_empty_boundaries(void)
{
 static const unsigned frames[]={0,960,960};static const uint64_t end[]={48000,48960,49920};
 struct eps_trial *e=eps_make(24,16,0,EPS_EMPTY,0);unsigned boundary=0,n=0;
 eps_begin(e);eps_until(e,PT_EDITOR_MIXED_SONG_NEXT);
 while(e->producer.result!=PT_EDITOR_MIXED_SONG_DONE){
    enum pt_editor_mixed_song_phase phase=e->producer.phase;
    assert(eps_step(e,7)==PT_EDITOR_MIXED_SONG_PENDING||e->producer.result==PT_EDITOR_MIXED_SONG_DONE);
    if(phase==PT_EDITOR_MIXED_SONG_LOWER){
       assert(boundary<3&&e->outputs->interval.frames==frames[boundary]&&e->producer.target==end[boundary]&&
          !e->outputs->normalized.normalized.count&&!e->outputs->normalizer);
       assert(e->outputs->interval.emit==1&&e->outputs->interval.end==(boundary==2));++boundary;
    }
    assert(++n<100000);
 }
 assert(boundary==3&&e->producer.frames==1920&&e->producer.intervals==3&&!e->base.port.publishes&&
    !e->base.port.commits&&!e->base.chip.calls&&!e->base.card->writes);
 eps_same(e);eps_close_empty(e);eps_drop(e);
}
static void eps_cancel_phase(enum pt_editor_mixed_song_phase phase)
{
 struct eps_trial *e=eps_make(16,8,0,EPS_MIXED,0);
 eps_begin(e);eps_until(e,phase);eps_close_empty(e);eps_same(e);eps_drop(e);
}
static void eps_master_failure(void)
{
 struct eps_trial *e=eps_make(24,16,0,EPS_MIXED,0);unsigned n=0;
 e->master_reenter_at=1;eps_begin(e);
 while(!e->producer.first_error){(void)eps_step(e,17);assert(++n<10000);}
 assert(e->master_observed==1&&!e->base.control.pool&&!e->base.control.activation&&e->borrow->address==&e->base.control);
 eps_close_empty(e);eps_same(e);eps_drop(e);
}
static void eps_external_failure(unsigned reentry)
{
 struct eps_trial *e=eps_make(8,16,0,EPS_EMPTY,0);uint8_t *before=NULL;size_t n;
 if(reentry)e->external.reenter_at=1;
 else{e->external.alias=(uint8_t *)e->outputs+sizeof(*e->outputs);n=e->output_capacity;
    before=malloc(n);assert(before);memcpy(before,e->outputs,n);}
 eps_begin(e);assert(eps_step(e,17)!=PT_EDITOR_MIXED_SONG_PENDING);
 if(reentry)assert(e->external.calls==1&&e->external.releases==1&&!e->external.live[0]);
 else assert(e->external.calls==1&&!e->external.releases&&!e->external.alias_releases&&
    !memcmp(before,e->outputs,e->output_capacity));
 assert(e->base.binding->preparation_context==&e->base.control&&e->borrow->address);
 eps_close_empty(e);eps_same(e);free(before);eps_drop(e);
}
struct eps_boundary {unsigned frames,count,end;uint64_t end_frame;};
/* Independent literal renderer oracle copied as values from the established
 * strict whole-audit tempo/F00 case, rather than recomputed by this producer.
 * All emit values are 1. emit0 requires row_range, which the accepted whole
 * audit refuses; no unreachable emit0 runtime case is manufactured here. */
static const struct eps_boundary eps_tempo_trimmed[]={
 {0,0,0,0},{0,1,0,0},{960,1,0,960},{960,1,0,1920},
 {875,1,0,2795},{876,1,0,3671},{876,1,0,4547},{876,1,0,5423},
 {876,1,0,6299},{876,0,1,7175}
};
static const struct eps_boundary eps_tempo_lead[]={
 {960,0,0,960},{960,1,0,1920},{960,1,0,2880},{960,1,0,3840},
 {875,1,0,4715},{876,1,0,5591},{876,1,0,6467},{876,1,0,7343},
 {876,1,0,8219},{876,0,1,9095}
};
static const struct eps_boundary eps_sixteen[]={
 {0,16,0,0},{960,16,0,960},{960,0,1,1920}
};
struct eps_wait_snapshot {
 struct pt_render_interval interval;
 struct pt_render_plan plan;
 struct pt_mixed_plan_quantized_batch normalized;
 struct pt_editor_mixed_readers_quantized_batch batch;
 struct pt_mixed_plan_origin origins[PT_MIXED_PLAN_RECORDS];
 uint64_t frames,intervals,target;unsigned action;
};
static void eps_snapshot(struct eps_trial *e,struct eps_wait_snapshot *s)
{
 s->interval=e->outputs->interval;s->plan=e->outputs->plan;s->normalized=e->outputs->normalized;
 s->batch=e->outputs->batch;memcpy(s->origins,e->producer.origins,sizeof(s->origins));
 s->frames=e->producer.frames;s->intervals=e->producer.intervals;s->target=e->producer.target;s->action=e->producer.action;
}
static void eps_wait_unchanged(struct eps_trial *e)
{
 struct eps_wait_snapshot before,after;unsigned i;
 unsigned reads=e->base.port.reads,publishes=e->base.port.publishes,commits=e->base.port.commits;
 unsigned cs=e->base.port.command_calls,rs=e->base.port.reader_calls,alloc=e->base.ordinary.calls;
 enum pt_editor_mixed_song_result waiting=e->producer.result;
 assert(waiting==PT_EDITOR_MIXED_SONG_WAIT_ACTIVE||waiting==PT_EDITOR_MIXED_SONG_WAIT_CAPACITY);
 memset(&before,0,sizeof(before));memset(&after,0,sizeof(after));eps_snapshot(e,&before);
 for(i=0;i<2;++i)assert(eps_step(e,256)==waiting);
 eps_snapshot(e,&after);assert(!memcmp(&before,&after,sizeof(before)));
 assert(reads==e->base.port.reads&&publishes==e->base.port.publishes&&commits==e->base.port.commits&&
    cs==e->base.port.command_calls&&rs==e->base.port.reader_calls&&alloc==e->base.ordinary.calls);
}
static void eps_command_services(struct eps_trial *e)
{
 unsigned i;
 for(i=0;i<PT_SAMPLER_MIXED_COMMANDS;++i)if(e->producer.command[i].reference.serial){
    assert(e->producer.command[i].ticket&&e->producer.command[i].published);
    assert(pt_editor_mixed_song_service_command(&e->producer,i,0)==PT_MIXED_READERS_OK);
    assert(!e->producer.command[i].reference.serial);
 }
}
static void eps_observe_readers(struct eps_trial *e)
{
 unsigned i;
 /* Explicit genuine observations establish real adoption, without pretending
  * the old public PENDING output is an ACTIVE receipt. Their retained issued
  * refs are preserved. No cancellation or terminal proof is requested. */
 for(i=0;i<PT_SAMPLER_MIXED_READERS;++i)if(e->producer.reader[i].reference.serial){
    struct pt_editor_mixed_reader_ref ref=e->producer.reader[i].reference;
    memset(&e->outputs->reader_receipt,0xb8,sizeof(e->outputs->reader_receipt));
    assert(pt_editor_mixed_song_service_reader(&e->producer,i,0)==PT_MIXED_READERS_PENDING);
    assert(ref.slot==e->producer.reader[i].reference.slot&&ref.serial==e->producer.reader[i].reference.serial);
    /* The pending public operation never publishes an output receipt. */
    {unsigned j;const uint8_t *v=(void *)&e->outputs->reader_receipt;
       for(j=0;j<sizeof(e->outputs->reader_receipt);++j)assert(v[j]==0xb8);}
 }
}
static void eps_reader_services(struct eps_trial *e,unsigned cancel)
{
 unsigned i;
 for(i=0;i<PT_SAMPLER_MIXED_READERS;++i)if(e->producer.reader[i].reference.serial){
    assert(pt_editor_mixed_song_service_reader(&e->producer,i,cancel)==PT_MIXED_READERS_OK);
    assert(!e->producer.reader[i].reference.serial);
 }
}
static void eps_packet(struct eps_trial *e,const struct pt_mixed_activation_packet *packet)
{
 struct emp_trial *f=&e->base;struct pt_editor_mixed_song_outputs *u=e->outputs;
 unsigned i,j;
 assert(packet->registration.owner==f->control.activation&&packet->registration.queue==f->control.queue&&
    packet->ticket==u->ticket&&packet->frame==e->producer.target&&packet->first==oracle(e->producer.target)&&
    packet->last==oracle(e->producer.target+1)&&packet->count==u->batch.count);
 for(i=0;i<packet->count;++i){
    const struct pt_editor_mixed_readers_request *x=u->batch.action+i;
    const struct pt_mixed_readers_action *a=packet->action+i;
    const struct pt_sampler_mixed_trigger_levels *levels=u->batch.levels+i;
    assert(a->kind==x->kind&&a->slot==u->normalized.normalized.record[i].slot&&
       packet->key[i].route==a->route&&packet->key[i].slot==a->slot);
    if(a->route==PT_MIXED_READERS_PAULA){
       assert(levels->mode==PT_SAMPLER_MIXED_TRIGGER_LEGACY&&!levels->left&&!levels->right);
       if(a->kind==PT_MIXED_READERS_TRIGGER){
          assert(a->geometry.paula.words==EPS_MONO_FRAMES/2&&a->geometry.paula.period==x->geometry.paula.period&&
             a->geometry.paula.volume==x->geometry.paula.volume);
          for(j=0;j<EPS_MONO_FRAMES;++j)assert(a->geometry.paula.data[j]==smf_convert8(e->original_mono[j],f->bits));
       }
    }else if(a->kind==PT_MIXED_READERS_TRIGGER){
       const struct pt_mixed_readers_card *card=packet->card+i;struct pt_amigus_voice_plan expected;
       struct pt_amigus_trigger_levels_request request={x->geometry.amigus.trigger,levels->left,levels->right};
       struct pt_playback_format format={f->cache_bits,x->channel,f->little,0};
       assert(a->route==PT_MIXED_READERS_AMIGUS&&levels->mode==PT_SAMPLER_MIXED_TRIGGER_QUANTIZED&&
          !request.geometry.volume&&!request.geometry.pan&&card->logical_bytes==EPS_MONO_FRAMES*f->cache_bits/8&&
          card->full_capacity>=card->logical_bytes);
       assert(pt_amigus_trigger_levels_prepare(f->editor->project->samples+x->sample,&format,&request,
          card->address,card->logical_bytes,&expected));emq_plan_equal(&a->geometry.amigus,&expected);
       assert(a->geometry.amigus.left==levels->left&&a->geometry.amigus.right==levels->right);
       for(j=0;j<EPS_MONO_FRAMES;++j){int32_t v=e->original_mono[j];
          if(f->cache_bits==8)assert(f->card->ram[card->address+j]==smf_convert8(v,f->bits));
          else{uint16_t word=smf_convert16(v,f->bits);
             assert(f->card->ram[card->address+2*j]==(uint8_t)(f->little?word:word>>8));
             assert(f->card->ram[card->address+2*j+1]==(uint8_t)(f->little?word>>8:word));}}
    }else assert(levels->mode==PT_SAMPLER_MIXED_TRIGGER_LEGACY&&!levels->left&&!levels->right);
 }
 for(;i<16;++i)assert(u->batch.levels[i].mode==PT_SAMPLER_MIXED_TRIGGER_LEGACY&&
    !u->batch.levels[i].left&&!u->batch.levels[i].right);
}
static void eps_publish_fire(struct eps_trial *e)
{
 struct emp_trial *f=&e->base;struct pt_mixed_readers_activation *owner=f->control.activation;
 uint64_t ticket=e->outputs->ticket;struct sma_command *copied;unsigned publishes=f->port.publishes,commits=f->port.commits;
 assert(e->producer.result==PT_EDITOR_MIXED_SONG_PUBLISH&&ticket&&!e->outputs->normalizer);
 assert(pt_editor_mixed_song_publish(&e->producer)==PT_MIXED_READERS_OK&&f->port.publishes==publishes+1);
 copied=sma_command(&f->port,ticket);assert(copied);eps_packet(e,&copied->packet);
 /* These exact results and used scratch have been consumed/closed. Expire
  * them without touching any live owner, genuine interval or borrowed source.
  * Fire receives only its original activation owner and copied ticket. */
 memset(&e->outputs->plan,0xca,sizeof(e->outputs->plan));
 memset(&e->outputs->normalized,0xcb,sizeof(e->outputs->normalized));
 memset(e->q_storage,0xcc,pt_mixed_plan_normalizer_workspace_size());
 f->port.ticks=copied->packet.first;
 assert(pt_mixed_activation_fire(owner,ticket)==PT_MIXED_ACTIVATION_COMMITTED&&f->port.commits==commits+1);
 assert(owner==f->control.activation&&e->producer.result==PT_EDITOR_MIXED_SONG_PENDING);
 /* Empty used scratch before the next genuine in-place constructor. Its full
  * tail stays immutable and is never treated as disposable used workspace. */
 memset(e->q_storage,0,pt_mixed_plan_normalizer_workspace_size());
}
static void eps_timeline(unsigned bits,unsigned cache,unsigned little,enum eps_case which,unsigned lead,unsigned hold_c)
{
 struct eps_trial *e=eps_make(bits,cache,little,which,lead);unsigned boundary=0,n=0,waited_c=0;
 const struct eps_boundary *table=which==EPS_TEMPO?(lead?eps_tempo_lead:eps_tempo_trimmed):eps_sixteen;
 unsigned count=which==EPS_TEMPO?10:3,expected_publishes=which==EPS_TEMPO?8:2;
 eps_begin(e);eps_until(e,PT_EDITOR_MIXED_SONG_NEXT);
 while(e->producer.result!=PT_EDITOR_MIXED_SONG_DONE){
    enum pt_editor_mixed_song_phase phase=e->producer.phase;enum pt_editor_mixed_song_result r=eps_step(e,17);
    if(phase==PT_EDITOR_MIXED_SONG_LOWER){
       assert(boundary<count&&e->outputs->interval.frames==table[boundary].frames&&
          e->outputs->interval.end==table[boundary].end&&e->outputs->interval.emit==1&&
          e->outputs->normalized.normalized.count==table[boundary].count&&
          e->producer.target==48000+table[boundary].end_frame&&!e->outputs->normalizer);
       ++boundary;
    }
    if(r==PT_EDITOR_MIXED_SONG_PUBLISH){eps_publish_fire(e);
       if(hold_c)eps_observe_readers(e);else eps_command_services(e);
    }else if(r==PT_EDITOR_MIXED_SONG_WAIT_CAPACITY){
       assert(hold_c&&which==EPS_TEMPO);eps_wait_unchanged(e);eps_command_services(e);++waited_c;
    }else assert(r==PT_EDITOR_MIXED_SONG_PENDING||r==PT_EDITOR_MIXED_SONG_DONE);
    assert(++n<100000);
 }
 assert(boundary==count&&e->producer.intervals==count&&e->producer.frames==table[count-1].end_frame&&
    e->producer.audited.frames==e->producer.frames&&e->producer.audited.rewinds==1&&
    e->base.port.publishes==expected_publishes&&e->base.port.commits==expected_publishes);
 if(hold_c)assert(waited_c==3);
 eps_command_services(e);eps_reader_services(e,1);eps_same(e);eps_close_empty(e);eps_drop(e);
}
static void eps_active_backpressure(void)
{
 struct eps_trial *e=eps_make(24,16,0,EPS_MIXED,0);unsigned n=0,waits=0,earlier;
 struct pt_editor_mixed_command_ref earlier_ref;uint64_t earlier_ticket;
 eps_begin(e);eps_until(e,PT_EDITOR_MIXED_SONG_NEXT);
 while(e->producer.result!=PT_EDITOR_MIXED_SONG_PUBLISH){assert(eps_step(e,17)==PT_EDITOR_MIXED_SONG_PENDING||
    e->producer.result==PT_EDITOR_MIXED_SONG_PUBLISH);assert(++n<100000);}
 eps_publish_fire(e); /* Deliberately retain C and real UNADOPTED R records. */
 earlier=e->producer.working_command;assert(earlier<PT_SAMPLER_MIXED_COMMANDS);
 earlier_ref=e->producer.command[earlier].reference;earlier_ticket=e->producer.command[earlier].ticket;
 assert(earlier_ref.serial&&earlier_ticket==e->outputs->ticket&&e->producer.command[earlier].published&&
    !e->producer.command[earlier].uncertain);
 while(e->producer.result!=PT_EDITOR_MIXED_SONG_PUBLISH){
    enum pt_editor_mixed_song_result r=eps_step(e,17);
    if(r==PT_EDITOR_MIXED_SONG_WAIT_ACTIVE){unsigned i=e->producer.action;
       struct pt_editor_mixed_reader_ref ref=e->outputs->batch.action[i].reader;
       assert(e->producer.phase==PT_EDITOR_MIXED_SONG_READINESS&&i<16&&ref.serial);
       eps_wait_unchanged(e);
       assert(pt_editor_mixed_song_service_reader(&e->producer,ref.slot,0)==PT_MIXED_READERS_PENDING);
       assert(e->producer.result==PT_EDITOR_MIXED_SONG_PENDING&&++waits<=16);
    }else assert(r==PT_EDITOR_MIXED_SONG_PENDING||r==PT_EDITOR_MIXED_SONG_PUBLISH);
    assert(++n<100000);
 }
 assert(waits==16&&e->producer.target==48960&&e->producer.frames==0&&e->producer.intervals==1);
 /* The second original C is genuinely enqueued, but is not published yet.
  * Service only the saved earlier published C. Never weaken the all-published
  * helper or treat an issued ticket/registration as publication authority. */
 {
    unsigned working=e->producer.working_command,port_index,allocations=e->base.ordinary.calls;
    uint64_t ticket=e->outputs->ticket,first=oracle(e->producer.target),last=oracle(e->producer.target+1);
    enum pt_editor_mixed_song_phase phase=e->producer.phase;
    enum pt_editor_mixed_song_result result=e->producer.result;
    struct pt_editor_mixed_song_command pending;
    struct pt_editor_mixed_command_record original_pending;
    struct eps_wait_snapshot before,after;struct sma_port expected_port;
    struct sma_command *copied=sma_command(&e->base.port,earlier_ticket);
    assert(working<PT_SAMPLER_MIXED_COMMANDS&&working!=earlier&&copied&&copied->committed);
    assert(phase==PT_EDITOR_MIXED_SONG_PUBLICATION&&result==PT_EDITOR_MIXED_SONG_PUBLISH&&
       ticket&&ticket!=earlier_ticket&&e->base.port.ticks<first&&first<last);
    memcpy(&pending,e->producer.command+working,sizeof(pending));
    memcpy(&original_pending,e->base.control.command+working,sizeof(original_pending));
    assert(pending.reference.serial&&pending.ticket==ticket&&!pending.published&&!pending.uncertain&&
       pending.reference.slot==e->outputs->command.slot&&pending.reference.serial==e->outputs->command.serial);
    assert(e->producer.command[earlier].reference.slot==earlier_ref.slot&&
       e->producer.command[earlier].reference.serial==earlier_ref.serial&&
       e->producer.command[earlier].ticket==earlier_ticket&&e->producer.command[earlier].published);
    assert(pt_editor_mixed_source_command_registration(&e->base.control,e->borrow,earlier_ref)==
       PT_EDITOR_MIXED_SOURCE_REGISTRATION_PRESENT);
    assert(pt_editor_mixed_source_command_registration(&e->base.control,e->borrow,pending.reference)==
       PT_EDITOR_MIXED_SOURCE_REGISTRATION_PRESENT&&!sma_command(&e->base.port,ticket));
    memset(&before,0,sizeof(before));memset(&after,0,sizeof(after));eps_snapshot(e,&before);
    memcpy(&expected_port,&e->base.port,sizeof(expected_port));
    port_index=(unsigned)(copied-e->base.port.command);assert(port_index<PT_SAMPLER_MIXED_COMMANDS);
    assert(pt_editor_mixed_song_service_command(&e->producer,earlier,0)==PT_MIXED_READERS_OK);
    assert(!e->producer.command[earlier].reference.serial&&
       pt_editor_mixed_source_command_registration(&e->base.control,e->borrow,earlier_ref)==
       PT_EDITOR_MIXED_SOURCE_REGISTRATION_ABSENT);
    assert(!memcmp(&pending,e->producer.command+working,sizeof(pending))&&
       !memcmp(&original_pending,e->base.control.command+working,sizeof(original_pending))&&
       pt_editor_mixed_source_command_registration(&e->base.control,e->borrow,pending.reference)==
       PT_EDITOR_MIXED_SOURCE_REGISTRATION_PRESENT);
    eps_snapshot(e,&after);assert(!memcmp(&before,&after,sizeof(before)));
    assert(e->producer.working_command==working&&e->producer.phase==phase&&e->producer.result==result&&
       e->outputs->ticket==ticket&&e->outputs->command.slot==pending.reference.slot&&
       e->outputs->command.serial==pending.reference.serial&&oracle(e->producer.target)==first&&
       oracle(e->producer.target+1)==last&&e->base.ordinary.calls==allocations);
    /* Only the actual earlier C callback/consumption may change the fixture
     * port. This local expected copy never alters the original live port. */
    ++expected_port.command_calls;memset(expected_port.command+port_index,0,sizeof(expected_port.command[0]));
    assert(!memcmp(&expected_port,&e->base.port,sizeof(expected_port))&&!sma_command(&e->base.port,ticket));
 }
 eps_publish_fire(e);eps_command_services(e);
 while(e->producer.result!=PT_EDITOR_MIXED_SONG_DONE){assert(eps_step(e,17)==PT_EDITOR_MIXED_SONG_PENDING||
    e->producer.result==PT_EDITOR_MIXED_SONG_DONE);assert(++n<100000);}
 eps_reader_services(e,1);eps_same(e);eps_close_empty(e);eps_drop(e);
}
static void eps_replacement_backpressure(void)
{
 struct eps_trial *e=eps_make(24,16,0,EPS_REPLACEMENTS,0);unsigned n=0,issued=0,waits=0,i,retired=0;
 struct pt_editor_mixed_reader_ref previous[16]={{0,0}};
 eps_begin(e);eps_until(e,PT_EDITOR_MIXED_SONG_NEXT);
 while(e->producer.result!=PT_EDITOR_MIXED_SONG_DONE){
    enum pt_editor_mixed_song_result r=eps_step(e,17);
    if(r==PT_EDITOR_MIXED_SONG_PUBLISH){
       assert(e->outputs->batch.count==16&&e->producer.target==48000+(uint64_t)issued*960);
       for(i=0;i<16;++i){struct pt_editor_mixed_reader_ref ref=e->producer.track[i];
          assert(ref.serial&&(!previous[i].serial||ref.slot!=previous[i].slot||ref.serial!=previous[i].serial));
          previous[i]=ref;}
       eps_publish_fire(e);eps_command_services(e);++issued;
    }else if(r==PT_EDITOR_MIXED_SONG_WAIT_CAPACITY){
       struct eps_wait_snapshot before,after;unsigned freed=0;
       assert(issued>=2&&e->producer.phase==PT_EDITOR_MIXED_SONG_BATCH_BEGIN);
       assert(pt_editor_mixed_song_get(&e->producer).reader_count==32);
       eps_wait_unchanged(e);memset(&before,0,sizeof(before));memset(&after,0,sizeof(after));eps_snapshot(e,&before);
       for(i=0;i<32;++i)if(e->producer.reader[i].reference.serial){unsigned j,current=0;
          for(j=0;j<16;++j)if(e->producer.track[j].slot==e->producer.reader[i].reference.slot&&
             e->producer.track[j].serial==e->producer.reader[i].reference.serial)current=1;
          if(!current){assert(pt_editor_mixed_song_service_reader(&e->producer,i,0)==PT_MIXED_READERS_OK);
             assert(!e->producer.reader[i].reference.serial);++freed;}}
       eps_snapshot(e,&after);assert(freed==16&&!memcmp(&before,&after,sizeof(before)));
       ++waits;retired+=freed;
    }else assert(r==PT_EDITOR_MIXED_SONG_PENDING||r==PT_EDITOR_MIXED_SONG_DONE);
    assert(++n<1000000);
 }
 assert(issued==34&&waits==32&&retired==512&&e->producer.frames==32640&&e->producer.intervals==35&&
    e->producer.audited.rewinds==1&&e->base.port.publishes==34&&e->base.port.commits==34);
 eps_reader_services(e,1);eps_same(e);eps_close_empty(e);eps_drop(e);
}
static void eps_first_published(struct eps_trial *e,unsigned fire)
{
 unsigned n=0;eps_begin(e);eps_until(e,PT_EDITOR_MIXED_SONG_NEXT);
 while(e->producer.result!=PT_EDITOR_MIXED_SONG_PUBLISH){assert(eps_step(e,17)==PT_EDITOR_MIXED_SONG_PENDING||
    e->producer.result==PT_EDITOR_MIXED_SONG_PUBLISH);assert(++n<100000);}
 if(fire)eps_publish_fire(e);
}
static void eps_retirement_order(unsigned reader_first,unsigned release_fault)
{
 struct eps_trial *e=eps_make(24,16,0,EPS_MIXED,0);unsigned i,rs;
 eps_first_published(e,1);rs=e->base.port.reader_calls;
 if(!reader_first)eps_command_services(e);
 if(reader_first){
    for(i=0;i<32;++i)if(e->producer.reader[i].reference.serial){
       struct pt_editor_mixed_reader_ref ref=e->producer.reader[i].reference;
       assert(pt_editor_mixed_song_service_reader(&e->producer,i,1)==PT_MIXED_READERS_OK);
       assert(e->producer.reader[i].retirement_consumed&&e->producer.reader[i].reference.serial==ref.serial&&
          pt_editor_mixed_source_reader_registration(&e->base.control,e->borrow,ref)==PT_EDITOR_MIXED_SOURCE_REGISTRATION_PRESENT);
    }
    assert(e->base.port.reader_calls==rs+16&&pt_editor_mixed_song_get(&e->producer).reader_count==16);
    rs=e->base.port.reader_calls;
    /* Explicit re-service of the SAME retained real refs performs genuine
     * settlement/close work but makes no second backend retirement call. */
    for(i=0;i<32;++i)if(e->producer.reader[i].reference.serial){
       assert(pt_editor_mixed_song_service_reader(&e->producer,i,1)==PT_MIXED_READERS_OK&&
          e->producer.reader[i].retirement_consumed);}
    assert(rs==e->base.port.reader_calls);eps_command_services(e);
    assert(pt_editor_mixed_song_get(&e->producer).reader_count==16);
 }
 if(release_fault)e->base.ordinary.hook=1;
 for(i=0;i<32;++i)if(e->producer.reader[i].reference.serial){
    unsigned calls=e->base.port.reader_calls;
    struct pt_editor_mixed_reader_ref ref=e->producer.reader[i].reference;
    enum pt_mixed_readers_result r=pt_editor_mixed_song_service_reader(&e->producer,i,1);
    assert(r==(release_fault?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK));
    assert(e->base.port.reader_calls==calls+(reader_first?0:1)&&!e->producer.reader[i].reference.serial&&
       pt_editor_mixed_source_reader_registration(&e->base.control,e->borrow,ref)==PT_EDITOR_MIXED_SOURCE_REGISTRATION_ABSENT);
 }
 assert(!pt_editor_mixed_song_get(&e->producer).reader_count&&!pt_editor_mixed_song_get(&e->producer).command_count);
 if(release_fault)assert(e->base.reentered==1&&e->producer.first_error&&e->borrow->address==&e->base.control);
 eps_same(e);eps_close_empty(e);eps_drop(e);
}
static void eps_pending_retirement(void)
{
 struct eps_trial *e=eps_make(16,8,0,EPS_MIXED,0);unsigned i,slot=32,calls,pins;
 struct pt_editor_mixed_reader_ref original;
 eps_first_published(e,1);eps_command_services(e);
 for(i=0;i<32;++i)if(e->producer.reader[i].reference.serial){slot=i;break;}assert(slot<32);
 original=e->producer.reader[slot].reference;pins=e->producer.pin_count;e->base.port.reader_allow=0;
 calls=e->base.port.reader_calls;
 assert(pt_editor_mixed_song_service_reader(&e->producer,slot,1)==PT_MIXED_READERS_PENDING&&
    e->base.port.reader_calls==calls+1&&!e->producer.reader[slot].retirement_consumed&&
    e->producer.reader[slot].reference.serial==original.serial&&e->producer.pin_count==pins&&
    e->borrow->address==&e->base.control);
 calls=e->base.port.reader_calls;(void)pt_editor_mixed_song_get(&e->producer);(void)pt_editor_mixed_song_get(&e->producer);
 assert(e->base.port.reader_calls==calls&&e->producer.pin_count==pins);
 /* Genuine clean pending has consumed no terminal domain settlement. A
  * separate explicit task call after the port's pending gate opens can make
  * at most one actual R operation; neither diagnostics nor elapsed time do. */
 e->base.port.reader_allow=1;
 assert(pt_editor_mixed_song_service_reader(&e->producer,slot,1)==PT_MIXED_READERS_OK&&
    e->base.port.reader_calls==calls+1&&!e->producer.reader[slot].reference.serial);
 eps_reader_services(e,1);eps_close_empty(e);eps_same(e);eps_drop(e);
}
static void eps_local_unpublished(void)
{
 struct eps_trial *e=eps_make(16,8,1,EPS_MIXED,0);unsigned i,n=0;
 eps_first_published(e,0);
 assert(e->outputs->ticket&&!e->base.port.publishes&&!e->base.port.reader_calls&&!e->base.port.command_calls);
 (void)pt_editor_mixed_song_cancel(&e->producer);
 for(i=0;i<32;++i)if(e->producer.reader[i].reference.serial){
    assert(pt_editor_mixed_song_service_reader(&e->producer,i,1)==PT_MIXED_READERS_BACKEND);
    assert(!e->producer.reader[i].reference.serial);}
 /* Queue stop consumed the local unpublished C original. Its actual absent
  * record is pruned by genuine close, without a backend domain certificate. */
 while(!pt_editor_mixed_song_close(&e->producer))assert(++n<1000);
 assert(!pt_editor_mixed_song_get(&e->producer).command_count&&!e->base.port.publishes&&
    !e->base.port.reader_calls&&!e->base.port.command_calls&&!e->producer.pin_count);
 e->closed=1;eps_same(e);eps_drop(e);
}
static void eps_source_quiet_pending(void)
{
 struct eps_trial *e=eps_make(8,16,0,EPS_CARD16,0);unsigned n=0,pins;
 eps_first_published(e,1);eps_command_services(e);eps_reader_services(e,1);
 e->base.port.close_result=e->base.port.quiet_result=0;
 (void)pt_editor_mixed_song_cancel(&e->producer);pins=e->producer.pin_count;
 while(!e->base.port.close_calls){assert(!pt_editor_mixed_song_close(&e->producer)&&++n<1000);}
 assert(e->base.port.close_calls==1&&e->base.port.callback_owner&&e->producer.cleanup_phase==6&&
    e->producer.pin_count==pins&&pins&&e->borrow->address==&e->base.control);
 assert(!pt_editor_mixed_song_close(&e->producer)&&e->base.port.close_calls==1&&e->base.port.quiet_calls&&
    e->producer.pin_count==pins&&e->base.binding->preparation_context==&e->base.control);
 eps_same(e);
 /* Declared ordinary-memory fixture's independent external callback release,
  * then the actual port source_quiet operation. This is no physical quiet,
  * native completion or voice-stop evidence and does not set a product flag. */
 e->base.port.callback_owner=NULL;e->base.port.quiet_result=1;
 while(!pt_editor_mixed_song_close(&e->producer))assert(++n<1000);
 assert(e->base.port.close_calls==1&&!e->producer.pin_count&&!e->borrow->address&&!e->base.binding->preparation_context);
 e->closed=1;eps_same(e);eps_drop(e);
}
static void eps_stale_captured_cleanup(void)
{
 struct eps_trial *e=eps_make(24,16,0,EPS_CARD16,0);struct pt_project before;unsigned i,n=0;
 eps_first_published(e,1);before=*e->base.editor->project;
 ++e->base.editor->history.revision;
 e->base.editor->project->samples=(void *)(UINTPTR_MAX-7);
 e->base.editor->project->events=(void *)(UINTPTR_MAX-7);
 e->base.editor->project->extensions=(void *)(UINTPTR_MAX-7);
 assert(eps_step(e,1)==PT_EDITOR_MIXED_SONG_STALE&&e->producer.first_error&&e->borrow->address);
 /* This producer walks only its retained ORIGINAL numerical controls/refs
  * after fixed-tag failure. No former project source descriptor is followed. */
 for(i=0;i<32;++i)if(e->producer.reader[i].reference.serial){
    assert(pt_editor_mixed_song_service_reader(&e->producer,i,1)==PT_MIXED_READERS_BACKEND&&
       e->producer.reader[i].retirement_consumed);}
 for(i=0;i<2;++i)if(e->producer.command[i].reference.serial){
    assert(pt_editor_mixed_song_service_command(&e->producer,i,1)==PT_MIXED_READERS_BACKEND);
    assert(!e->producer.command[i].reference.serial);}
 for(i=0;i<32;++i)if(e->producer.reader[i].reference.serial){unsigned calls=e->base.port.reader_calls;
    assert(pt_editor_mixed_song_service_reader(&e->producer,i,1)==PT_MIXED_READERS_BACKEND&&
       !e->producer.reader[i].reference.serial&&calls==e->base.port.reader_calls);}
 while(!pt_editor_mixed_song_close(&e->producer))assert(++n<1000);
 *e->base.editor->project=before;e->closed=1;eps_same(e);eps_drop(e);
}
static void eps_options_refusal(unsigned which)
{
 struct eps_trial *e=eps_make(8,8,0,EPS_EMPTY,0);unsigned n=0;
 if(!which)e->producer.configuration.options.row_range=1;
 else if(which==1)e->producer.configuration.options.pattern_only=1;
 else e->producer.configuration.options.tracks=15;
 eps_begin(e);
 while(!e->producer.first_error){(void)eps_step(e,17);assert(++n<100000);}
 assert(e->producer.phase==PT_EDITOR_MIXED_SONG_DRAIN&&e->producer.pin_count==32&&e->producer.activated&&
    !e->base.port.publishes&&!e->base.port.commits&&!e->base.port.reads);
 eps_close_empty(e);eps_same(e);eps_drop(e);
}
static void eps_deadline_overflow(void)
{
 struct eps_trial *e=eps_make(8,8,0,EPS_EMPTY,0);unsigned n=0;
 e->producer.configuration.absolute_start=UINT64_MAX-2;eps_begin(e);
 while(!e->producer.first_error){(void)eps_step(e,17);assert(++n<100000);}
 assert(e->producer.first_error==PT_EDITOR_MIXED_SONG_REFUSED&&e->producer.target==UINT64_MAX-2&&
    e->producer.frames==0&&e->producer.intervals==1&&!e->base.port.reads&&!e->base.port.publishes);
 eps_close_empty(e);eps_same(e);eps_drop(e);
}
static void eps_cancel_issued_phase(enum pt_editor_mixed_song_phase wanted)
{
 struct eps_trial *e=eps_make(16,16,1,EPS_MIXED,0);unsigned i,n=0;
 eps_first_published(e,1);eps_command_services(e);
 if(wanted==PT_EDITOR_MIXED_SONG_CONSUME||wanted==PT_EDITOR_MIXED_SONG_COMMIT){
    while(e->producer.result!=PT_EDITOR_MIXED_SONG_PUBLISH){assert(eps_step(e,17)==PT_EDITOR_MIXED_SONG_PENDING||
       e->producer.result==PT_EDITOR_MIXED_SONG_PUBLISH);assert(++n<100000);}
    assert(e->outputs->interval.frames==960);eps_publish_fire(e);eps_command_services(e);
 }
 while(e->producer.phase!=wanted){enum pt_editor_mixed_song_result r=eps_step(e,17);
    assert(r==PT_EDITOR_MIXED_SONG_PENDING||(r==PT_EDITOR_MIXED_SONG_PUBLISH&&e->producer.phase==wanted));
    assert(++n<100000);}
 assert(e->producer.pin_count==32&&e->borrow->address==&e->base.control);
 (void)pt_editor_mixed_song_cancel(&e->producer);
 for(i=0;i<32;++i)if(e->producer.reader[i].reference.serial){
    assert(pt_editor_mixed_song_service_reader(&e->producer,i,1)==PT_MIXED_READERS_BACKEND);
    assert(e->producer.reader[i].retirement_consumed||!e->producer.reader[i].reference.serial);}
 for(i=0;i<2;++i)if(e->producer.command[i].reference.serial&&e->producer.command[i].published)
    assert(pt_editor_mixed_song_service_command(&e->producer,i,1)==PT_MIXED_READERS_BACKEND);
 for(i=0;i<32;++i)if(e->producer.reader[i].reference.serial){unsigned calls=e->base.port.reader_calls;
    assert(pt_editor_mixed_song_service_reader(&e->producer,i,1)==PT_MIXED_READERS_BACKEND&&
       calls==e->base.port.reader_calls&&!e->producer.reader[i].reference.serial);}
 eps_close_empty(e);eps_same(e);eps_drop(e);
}
static void eps_clean_pending_publication(void)
{
 struct eps_trial *e=eps_make(16,8,1,EPS_MIXED,0);unsigned attempts,reads,rs,cs;
 uint64_t original;eps_first_published(e,0);original=e->outputs->ticket;e->publication_outcome=1;
 assert(pt_editor_mixed_song_publish(&e->producer)==PT_MIXED_READERS_PENDING&&
    e->producer.result==PT_EDITOR_MIXED_SONG_PUBLISH&&e->outputs->ticket==original);
 attempts=e->base.port.publishes;reads=e->base.port.reads;rs=e->base.port.reader_calls;cs=e->base.port.command_calls;
 assert(eps_step(e,256)==PT_EDITOR_MIXED_SONG_PUBLISH&&pt_editor_mixed_song_get(&e->producer).result==PT_EDITOR_MIXED_SONG_PUBLISH);
 assert(attempts==e->base.port.publishes&&reads==e->base.port.reads&&rs==e->base.port.reader_calls&&cs==e->base.port.command_calls);
 /* The caller explicitly chooses a separate retry after genuine clean0, on
  * this exact unchanged ticket/window. No automatic rebase or clock shift. */
 e->publication_outcome=0;eps_publish_fire(e);eps_command_services(e);eps_reader_services(e,1);
 eps_close_empty(e);eps_same(e);eps_drop(e);
}
static void eps_failed_publication(unsigned unknown)
{
 struct eps_trial *e=eps_make(24,16,0,EPS_MIXED,0);unsigned i,n=0;
 uint64_t original;eps_first_published(e,0);original=e->outputs->ticket;
 if(unknown)e->publication_outcome=2;else e->base.port.ticks=oracle(e->producer.target);
 assert(pt_editor_mixed_song_publish(&e->producer)==(unknown?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_LATE)&&
    e->producer.first_error&&e->producer.phase==PT_EDITOR_MIXED_SONG_DRAIN&&e->outputs->ticket==original);
 assert(eps_step(e,17)==e->producer.first_error&&e->base.port.publishes==(unknown?1U:0U)&&!e->base.port.commits);
 (void)pt_editor_mixed_song_cancel(&e->producer);
 for(i=0;i<32;++i)if(e->producer.reader[i].reference.serial)
    assert(pt_editor_mixed_song_service_reader(&e->producer,i,1)==PT_MIXED_READERS_BACKEND);
 if(unknown){
    for(i=0;i<2;++i)if(e->producer.command[i].reference.serial&&e->producer.command[i].ticket)
       assert(pt_editor_mixed_song_service_command(&e->producer,i,1)==PT_MIXED_READERS_BACKEND);
    for(i=0;i<32;++i)if(e->producer.reader[i].reference.serial){unsigned calls=e->base.port.reader_calls;
       assert(pt_editor_mixed_song_service_reader(&e->producer,i,1)==PT_MIXED_READERS_BACKEND&&
          !e->producer.reader[i].reference.serial&&calls==e->base.port.reader_calls);}
 }
 while(!pt_editor_mixed_song_close(&e->producer))assert(++n<1000);
 assert(!e->base.port.commits&&!e->producer.pin_count);e->closed=1;eps_same(e);eps_drop(e);
}
static void eps_actual_enqueue_outer_fault(void)
{
 struct eps_trial *e=eps_make(24,16,0,EPS_CARD16,0);unsigned i,n=0;
 eps_begin(e);eps_until(e,PT_EDITOR_MIXED_SONG_ENQUEUE);e->outer_configuration_fault=1;
 assert(eps_step(e,17)==PT_EDITOR_MIXED_SONG_FAULT&&!e->outer_configuration_fault&&
    e->producer.scheduled_result==PT_MIXED_READERS_OK&&e->outputs->ticket&&e->producer.first_error);
 for(i=0;i<16;++i)assert(e->producer.origins[i].present&&e->producer.track[i].serial&&
    e->producer.track[i].serial==e->producer.prospective[i].serial&&e->producer.samples[1][0]);
 assert(e->producer.command[e->producer.working_command].ticket==e->outputs->ticket&&!e->base.port.publishes);
 (void)pt_editor_mixed_song_cancel(&e->producer);
 for(i=0;i<32;++i)if(e->producer.reader[i].reference.serial)
    assert(pt_editor_mixed_song_service_reader(&e->producer,i,1)==PT_MIXED_READERS_BACKEND);
 while(!pt_editor_mixed_song_close(&e->producer))assert(++n<1000);
 e->closed=1;eps_same(e);eps_drop(e);
}
static void eps_late_external_failure(unsigned request,unsigned alias)
{
 struct eps_trial *e=eps_make(24,16,0,EPS_EMPTY,0);unsigned i,n=0;
 assert(request==2||request==3);eps_begin(e);
 if(alias){eps_until(e,PT_EDITOR_MIXED_SONG_AUDIT_BEGIN);
    e->external.alias_at=request;
    /* Newly promoted complete master capacities are captured at the one real
     * activate refresh. The second/third renderer control must reject the
     * entire current source tail before initialization or base release. */
    e->external.alias=e->base.editor->project->samples[0].pcm.data+
       e->base.editor->project->samples[0].pcm.capacity-1;
 }else e->external.reenter_at=request;
 while(!e->producer.first_error){(void)eps_step(e,17);assert(++n<100000);}
 assert(e->external.calls==request&&e->producer.requests==request&&e->producer.pin_count==32&&
    e->borrow->address==&e->base.control&&!e->base.port.reads&&!e->base.port.publishes);
 eps_close_empty(e);eps_same(e);
 for(i=0;i<request;++i)assert(!e->external.live[i]);
 assert(!e->external.alias_releases&&e->external.releases==request-(alias?1U:0U));eps_drop(e);
}
static void eps_unresolved_hold(struct eps_trial *e,unsigned request)
{
 struct emp_trial *f=&e->base;unsigned i,n=0,releases;
 uintptr_t pins[PT_PROJECT_SAMPLES];size_t unresolved=e->producer.allocation[request-1].bytes;
 assert(request>=2&&request<=3&&e->producer.requests==request&&e->external.calls==request&&
    e->producer.allocation[request-1].unresolved&&!e->producer.allocation[request-1].live&&
    e->producer.quiet_ambiguous&&e->producer.pin_count==32&&e->producer.first_error==PT_EDITOR_MIXED_SONG_FAULT);
 for(i=0;i<32;++i){assert(e->producer.pin[i]);pins[i]=(uintptr_t)e->producer.pin[i];}
 while(e->producer.cleanup_phase<6){assert(!pt_editor_mixed_song_close(&e->producer)&&++n<64);}
 assert(!e->outputs->audit&&!e->outputs->sequence&&!e->outputs->normalizer&&!e->producer.establish.active&&
    !e->producer.pin_job.owner&&!e->outputs->temporary_pin&&!e->outputs->persistent_pin);
 releases=e->external.releases;
 assert(e->producer.live_bytes==e->producer.base_bytes+unresolved);
 for(i=0;i<16;++i){
    assert(!pt_editor_mixed_song_close(&e->producer)&&e->producer.cleanup_phase==6&&
       e->producer.phase==PT_EDITOR_MIXED_SONG_DRAIN&&e->producer.result==PT_EDITOR_MIXED_SONG_FAULT&&
       e->producer.quiet_ambiguous&&e->producer.pin_count==32&&
       !e->producer.borrow_closed&&!e->producer.hook_closed&&e->borrow->address==&f->control&&
       e->borrow->serial&&f->binding->preparation_context==&f->control&&f->binding->preparation_close);
    assert(e->external.calls==request&&e->external.releases==releases&&!e->external.alias_releases&&
       !e->external.fixture_only_retirements&&!f->port.publishes&&!f->port.commits&&!e->closed&&
       e->producer.live_bytes==e->producer.base_bytes+unresolved);
    assert(!pt_editor_prepare_change(f->editor)&&!pt_editor_dispose(f->editor));
 }
 for(i=0;i<32;++i)assert((uintptr_t)e->producer.pin[i]==pins[i]);
 assert(pt_editor_mixed_source_children_closed(&f->control,e->borrow));
 eps_same(e);
}
static void eps_retired_numeric_alias(void)
{
 struct eps_trial *e=eps_make(16,8,0,EPS_EMPTY,0);unsigned n=0;
 eps_begin(e);eps_until(e,PT_EDITOR_MIXED_SONG_AUDIT_BEGIN);
 assert(e->external.calls==1&&e->external.releases==1&&e->external.pointer[0]&&!e->external.live[0]);
 e->external.alias=e->external.pointer[0];e->external.alias_at=2;
 while(!e->producer.first_error){(void)eps_step(e,17);assert(++n<100000);}
 assert(e->external.calls==2&&e->producer.requests==2&&!e->external.alias_releases);
 /* This retained historical alias still creates no new mock owner. Product
  * cannot use that private fixture fact to prove its return unowned or quiet. */
 assert(!e->external.recycled_new_owners&&!e->external.live[1]);
 eps_unresolved_hold(e,2);assert(e->external.releases==1);
 eps_end_unresolved_fixture_lifetime(e);
}
static void eps_recycled_external_owner(unsigned request)
{
 struct eps_trial *e=eps_make(24,16,0,EPS_EMPTY,0);unsigned n=0,index=request-1;
 assert(request==2||request==3);e->external.recycle_at=request;eps_begin(e);
 while(!e->producer.first_error){(void)eps_step(e,17);assert(++n<100000);}
 assert(e->external.calls==request&&e->external.recycled_new_owners==1&&
    !e->external.alias&&!e->external.alias_releases&&!e->external.live[0]&&
    e->external.live[index]&&e->external.pointer[index]==e->external.pointer[0]&&
    e->external.bytes[index]==e->producer.allocation[index].bytes&&
    e->producer.allocation[index].address==(uintptr_t)e->external.pointer[index]);
 eps_tail_same(e->external.pointer[index],0,e->external.bytes[index],0xe7);
 eps_unresolved_hold(e,request);
 /* Full new backing remains live and untouched after all actual child closes
  * and repeated refused producer closes. No second allocation/release occurs. */
 assert(e->external.live[index]&&e->external.recycled_new_owners==1&&
    !e->external.fixture_only_retirements&&e->external.releases==request-1);
 eps_tail_same(e->external.pointer[index],0,e->external.bytes[index],0xe7);
 eps_end_unresolved_fixture_lifetime(e);
}
static void eps_admission_refusals(unsigned which)
{
 struct eps_trial *e=eps_make(8,8,0,EPS_EMPTY,0);struct pt_editor_mixed_source_inputs original=*e->source;
 struct pt_editor_mixed_readers_prepare *controller=e->producer.controller;
 size_t n=e->output_capacity;uint8_t *copy=malloc(n);assert(copy);memcpy(copy,e->outputs,n);
 if(!which)e->source->mutable[1]=(struct pt_sampler_storage_span){e->original_mono+EPS_MONO_CAPACITY-1,n};
 else if(which==1)e->source->immutable[5]=(struct pt_sampler_storage_span){(char *)e->outputs+sizeof(*e->outputs),e->q_capacity};
 else if(which==2)e->source->contexts=(struct pt_sampler_storage_span){&e->producer,sizeof(e->producer)};
 else e->producer.controller=(void *)&e->producer;
 assert(pt_editor_mixed_song_begin(&e->producer,e->source,e->borrow)==PT_EDITOR_MIXED_SONG_INVALID&&
    !e->producer.adopted&&!e->producer.original&&!e->borrow->address&&!e->borrow->serial&&
    !e->base.binding->preparation_context&&!e->external.calls&&!e->base.ordinary.calls&&!e->base.port.reads&&
    !memcmp(copy,e->outputs,n));
 *e->source=original;e->producer.controller=controller;free(copy);
 eps_begin(e);eps_close_empty(e);eps_same(e);eps_drop(e);
}
static void eps_ordinary_full_budget(void)
{
 struct eps_trial *e=eps_make(8,8,0,EPS_EMPTY,0);size_t whole=sizeof(*e)+sizeof(*e->source)+sizeof(*e->borrow);
 unsigned i;
 for(i=0;i<6;++i)whole+=e->source->immutable[i].bytes;
 for(i=0;i<2;++i)whole+=e->source->mutable[i].bytes;
 e->producer.configuration.ordinary_budget=whole-1;
 assert(pt_editor_mixed_song_begin(&e->producer,e->source,e->borrow)==PT_EDITOR_MIXED_SONG_CAPACITY&&
    e->producer.adopted&&e->borrow->address==&e->base.control&&!e->external.calls&&!e->base.ordinary.calls);
 eps_close_empty(e);eps_same(e);eps_drop(e);
}
#ifndef PT_EDITOR_MIXED_SONG_TEST_MAIN
#define PT_EDITOR_MIXED_SONG_TEST_MAIN main
#endif
int PT_EDITOR_MIXED_SONG_TEST_MAIN(void)
{
 unsigned bits,cache,little,phase;
 assert(eps_quantized_main()==0);
 for(bits=8;bits<=24;bits+=8)for(cache=8;cache<=16;cache+=8)for(little=0;little<2;++little)
    eps_preparation(bits,cache,little);
 eps_empty_boundaries();
 for(phase=PT_EDITOR_MIXED_SONG_ESTABLISH_BEGIN;phase<=PT_EDITOR_MIXED_SONG_LOWER;++phase)
    eps_cancel_phase((enum pt_editor_mixed_song_phase)phase);
 eps_master_failure();eps_external_failure(0);eps_external_failure(1);
 for(bits=8;bits<=24;bits+=8)for(cache=8;cache<=16;cache+=8)for(little=0;little<2;++little){
    eps_timeline(bits,cache,little,EPS_MIXED,0,0);eps_timeline(bits,cache,little,EPS_CARD16,0,0);}
 eps_timeline(24,16,0,EPS_TEMPO,0,0);eps_timeline(24,16,0,EPS_TEMPO,1,0);
 eps_timeline(24,16,0,EPS_TEMPO,0,1);eps_active_backpressure();eps_replacement_backpressure();
 eps_retirement_order(0,0);eps_retirement_order(1,0);eps_retirement_order(0,1);eps_retirement_order(1,1);
 eps_pending_retirement();
 eps_local_unpublished();eps_source_quiet_pending();eps_stale_captured_cleanup();
 eps_options_refusal(0);eps_options_refusal(1);eps_options_refusal(2);eps_deadline_overflow();
 for(phase=PT_EDITOR_MIXED_SONG_READINESS;phase<=PT_EDITOR_MIXED_SONG_COMMIT;++phase)
    eps_cancel_issued_phase((enum pt_editor_mixed_song_phase)phase);
 eps_clean_pending_publication();eps_failed_publication(0);eps_failed_publication(1);eps_actual_enqueue_outer_fault();
 eps_late_external_failure(2,0);eps_late_external_failure(3,0);eps_late_external_failure(2,1);eps_late_external_failure(3,1);
 eps_retired_numeric_alias();eps_recycled_external_owner(2);eps_recycled_external_owner(3);
 eps_admission_refusals(0);eps_admission_refusals(1);eps_admission_refusals(2);eps_admission_refusals(3);
 eps_ordinary_full_budget();
 puts("EDITOR MIXED SONG DRAFT PASS: genuine early establishment and all-slot persistent pins; one refresh; SAME strict audit sequence once rewound; typed full-capacity parents/tails; exact empty/tempo/lead/zero/F00 boundaries; quantized mixed4/12 and card16 cache geometry; copied-only fire after expired Q scratch; real ACTIVE and C2/R32 backpressure; incremental early cancellation and allocation reentry; unresolved recycled allocator custody retains full budget/pins/barrier without product recovery; fixture-only lifetime teardown; SOFTWARE_ONLY");
 /* Every assertion is authored SOURCE only, not an observed PASS. Execution
  * requires root's committed dependency pins, complete independent SOURCE
  * review and a separately scoped custody/qualifier packet. */
 return 0;
}
