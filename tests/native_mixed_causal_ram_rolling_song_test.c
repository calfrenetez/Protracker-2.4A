/* Genuine four-boundary rolling producer SOURCE fixture. Both original donor entries
 * are uncalled. No forged ACTIVE/key/receipt, live timer/device or musical PLAY. */
#define PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_TEST_MAIN sl_original_producer62_not_called
#include "editor_mixed_causal_song_pair_test.c"
#undef PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_TEST_MAIN
#include "../src/native/readers_ram/native_mixed_causal_ram_port.h"
#include "../src/editor/editor_mixed_causal_song_rolling_internal.h"

struct sl_trial;
struct sl_external {
 unsigned calls,releases,alias_releases,reenter_at;
 void *alias,*pointer[PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REQUESTS];
 size_t bytes[PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REQUESTS];
 unsigned live[PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REQUESTS];
};
struct sl_adapter {
 struct sl_trial *owner;
 uint64_t ticks,ticket[4];uint32_t frequency;
 unsigned clocks,arms,proofs,shutdowns,probes,live[4],hold[4],arm_hook,clock_hook,quiet_hook,initial_clock_hook;
 int arm_raw,source_raw,quiet_raw,ticket_raw;
 unsigned custody_clears;
 uint64_t *injected64,saved64;unsigned *injected_mode,saved_mode;
};
struct sl_port {
 struct pt_private_mixed_causal_ram_port ram;
 struct sl_adapter external;
 struct pt_private_mixed_causal_ram_adapter declaration;
 unsigned binds;
};
struct sl_trial {
 struct cp_trial base; /* Original allocator/resource helpers, old control unused. */
 struct pt_editor_mixed_causal_rolling_source_prepare joint;
 struct pt_editor_mixed_causal_song_rolling producer;
 struct sl_external external;
 struct sl_port port;
 struct pt_editor_mixed_causal_rolling_source_inputs *source;
 struct pt_editor_mixed_causal_source_borrow *borrow;
 struct pt_editor_mixed_causal_song_rolling_outputs *outputs;
 void *audit_storage,*audit_q,*q_storage,*audit_result;
 size_t audit_capacity,audit_q_capacity,q_capacity,result_capacity,output_capacity;
 int32_t *original_mono,*original_stereo,*spare_values;
 uint32_t *original_slices;unsigned spare_slot;
 struct pt_sample *real_samples;struct pt_event *real_events;
 unsigned promoting,master_observed,master_calls_at_begin,closed,kind;
 unsigned outer_enqueue_fault,outer_publish_fault;
 unsigned char tail[CS_TAIL];
 unsigned char overlap_guard[sizeof(struct pt_editor_mixed_causal_song_rolling_outputs)+CS_TAIL];
};
static unsigned sl_cases;
static int sl_target_service=-1,sl_target_publish=-1;
static unsigned sl_target_state;
static struct sl_trial *sl_current;
static const char *sl_case="startup";
static struct pt_editor_mixed_causal_song_lineage_history *sl_history(struct sl_trial *e,unsigned i)
{return i<3?e->producer.original.history+i:i==3?&e->producer.fourth_history:NULL;}
static struct pt_editor_mixed_causal_prepare *sl_s(struct sl_trial *);
static void sl_check(int ok,const char *expr,unsigned line)
{
 if(ok)return;
 fprintf(stderr,"ROLLING_ASSERT case=%s line=%u expression=%s\n",sl_case,line,expr);
 fprintf(stderr,"ROLLING_TARGET service=%d publish=%d state=%u\n",sl_target_service,sl_target_publish,sl_target_state);
 if(sl_current){struct sl_trial *e=sl_current;struct pt_editor_mixed_causal_song_lineage *p=&e->producer.original;
    fprintf(stderr,"ROLLING_ATOMS result=%d first=%d phase=%u index=%u attempted=%u candidates=%u arms=%u pairs=%u effects=%u controls=%u stops=%u failed=%u suppressed=%u ticks=%llu corebusy=%u\n",
       (int)p->result,(int)p->first_error,(unsigned)p->phase,p->lineage_index,p->attempted_mask,p->candidate_count,
       e->port.external.arms,e->port.ram.pair_used,e->port.ram.effects,e->port.ram.controls,e->port.ram.stops,
       e->port.ram.failed,e->port.ram.suppressed,(unsigned long long)e->port.external.ticks,sl_s(e)->busy);
    fprintf(stderr,"ROLLING_RAM_STATE heldC=%u heldR=%u mask=%u source_attempted=%u source_closed=%u modes=%u/%u/%u proofs=%u\n",
       pt_mixed_readers_commands_held(sl_s(e)->queue),pt_mixed_readers_readers_held(sl_s(e)->queue),
       e->port.ram.mask,e->port.ram.source_attempted,e->port.ram.source_closed,e->port.ram.rolling_mode,
       e->port.ram.control_mode,e->port.ram.lineage_mode,e->port.external.proofs);
    for(unsigned i=0;i<2;++i){struct pt_private_mixed_causal_ram_command *c=e->port.ram.command+i;
       fprintf(stderr,"ROLLING_RAM_ROW row=%u ticket=%llu serial=%llu known=%u live=%u armed=%u finished=%u quiet=%u uncertain=%u ticketquiet=%d armstate=%u ticketraw=%d\n",
          i,(unsigned long long)c->identity.ticket,(unsigned long long)c->serial,c->known,c->live,c->armed,
          c->finished,c->quiet_confirmed,c->uncertain,c->ticket_outcome,c->rolling_arm_state,c->rolling_ticket_raw);}
    for(unsigned i=0;i<4;++i){struct pt_editor_mixed_causal_song_lineage_history *h=sl_history(e,i);
       fprintf(stderr,"ROLLING_HISTORY index=%u ticket=%llu slot=%u serial=%llu published=%u uncertain=%u detached=%u seen=%u hold=%u\n",i,
         (unsigned long long)h->ticket,h->reference.slot,(unsigned long long)h->reference.serial,h->published,h->uncertain,h->detached,h->service_seen,e->port.external.hold[i]);}}
 fflush(stderr);abort();
}
#undef assert
#define assert(expression) sl_check(!!(expression),#expression,__LINE__)

static struct pt_editor_mixed_causal_prepare *sl_s(struct sl_trial *e)
{return &e->joint.original.original.original;}
static void sl_reenter(struct sl_trial *e)
{
 uint32_t revision=e->base.editor->history.revision,generation=e->base.editor->sampler.generation;
 assert(pt_editor_mixed_causal_song_rolling_step(&e->producer,1)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);
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
 assert(index<PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REQUESTS&&bytes&&e->producer.original.busy&&
    sl_s(e)->source_busy&&!sl_s(e)->busy&&e->borrow->address==sl_s(e)&&
    f->binding->preparation_context==sl_s(e)&&f->binding->preparation_close);
 if(index){assert(sl_s(e)->source_activated&&e->producer.original.pin_count==f->editor->project->sample_count);
    for(i=0;i<f->editor->project->sample_count;++i)assert(e->producer.original.pin[i]&&e->producer.original.pin[i]==f->editor->sampler.current[i]);
 }else assert(!sl_s(e)->source_activated&&!sl_s(e)->causal&&!sl_s(e)->pool);
 if(m->reenter_at==index+1)sl_reenter(e);
 if(m->alias)return m->alias;
 m->pointer[index]=malloc(bytes);assert(m->pointer[index]);m->bytes[index]=bytes;m->live[index]=1;return m->pointer[index];
}
static void sl_external_release(void *context,void *pointer)
{
 struct sl_external *m=context;struct sl_trial *e=(void *)((char *)context- offsetof(struct sl_trial,external));unsigned i,j;
 assert(e->producer.original.busy&&sl_s(e)->source_busy&&!sl_s(e)->busy);
 if(pointer==m->alias){++m->alias_releases;return;}
 for(i=0;i<m->calls&&m->pointer[i]!=pointer;++i){}assert(i<m->calls&&m->live[i]);
 for(j=0;j<e->producer.original.requests;++j)if(e->producer.original.allocation[j].address==(uintptr_t)pointer)assert(!e->producer.original.allocation[j].live);
 m->live[i]=0;++m->releases;
 /* Original donor quarantine preserves numeric retired extents until teardown. */
}
static int sl_owned(void *context)
{
 struct cp_bus *bus=context;struct cp_trial *f=bus->owner;struct sl_trial *e=(void *)f;
 int r=f->card->healthy==1&&f->library.library&&f->library.owner==&f->card->reservation&&
    f->library.acquired==PT_AMIGUS_WAVETABLE&&f->card->reservation.reserved;
 if(e->outer_enqueue_fault&&e->producer.original.phase==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_ENQUEUE){
    assert(r&&e->producer.original.busy&&sl_s(e)->busy);e->outer_enqueue_fault=0;++e->producer.original.configuration.options.tick_limit;}
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
 assert(e->producer.original.phase==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CONTROLLER&&e->producer.original.busy&&
    e->producer.original.candidate_count==4&&!e->producer.original.candidate_pending&&e->producer.original.frames==3840&&
    e->producer.original.intervals==5&&e->producer.original.audited.rewinds==1&&e->producer.original.activated&&
    e->outputs->original.sequence&&e->outputs->original.audit&&!e->outputs->original.normalizer&&
    !memcmp(e->outputs->original.candidate,e->producer.original.frozen,sizeof(e->outputs->original.candidate))&&
    e->producer.original.pin_count==e->base.editor->project->sample_count&&e->external.calls==3&&
    !memcmp(&e->producer.fourth,&e->outputs->fourth,sizeof(e->producer.fourth)));
 assert(++p->binds==1&&e->base.ordinary.calls==2&&cp_live(&e->base.ordinary)==2);
 assert(s->causal&&s->queue&&!s->pool&&registration->owner==s->causal&&registration->queue==s->queue);
 assert(registration->session==31&&registration->generation==17);
 assert(!pt_mixed_readers_commands_held(s->queue)&&!pt_mixed_readers_readers_held(s->queue));
 return pt_private_mixed_causal_ram_bind(ram,s->causal,s->queue,registration->session,registration->generation);
}
/* Only the independent adapter boundary is injected. No fixture CONTROL,
 * STOP, commit, command-quiet or reader-quiet production callback exists. */
/* Undo only the scalar actually injected by this test callback. Capture at
 * injection time; never rewind real ownership or failure/quiet classification. */
static void sl_inject64(struct sl_adapter *a,uint64_t *field)
{assert(!a->injected64&&!a->injected_mode);a->injected64=field;a->saved64=*field;++*field;}
static void sl_inject_mode(struct sl_adapter *a,unsigned *field,unsigned value)
{assert(!a->injected64&&!a->injected_mode);a->injected_mode=field;a->saved_mode=*field;*field=value;}
static void sl_undo_injected(struct sl_adapter *a)
{
 assert((a->injected64!=NULL)!=(a->injected_mode!=NULL));
 if(a->injected64){*a->injected64=a->saved64;a->injected64=NULL;}
 else{*a->injected_mode=a->saved_mode;a->injected_mode=NULL;}
}
static void sl_clear_custody(struct sl_adapter *a,struct pt_private_mixed_causal_ram_command *c)
{
 struct pt_mixed_causal_command_identity identity=c->identity;
 memset(c,0,sizeof(*c));c->identity=identity;c->known=c->quiet_confirmed=1;
 c->rolling_arm_state=PT_PRIVATE_CAUSAL_RAM_ARM_NOT_ENTERED;++a->custody_clears;
}
static int sl_adapter_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{
 struct sl_adapter *a=context;struct pt_private_mixed_causal_ram_port *p=&a->owner->port.ram;
 ++a->clocks;*ticks=a->ticks;*frequency=a->frequency;
 if(a->initial_clock_hook&&!p->pair_used&&p->command[0].known&&
    p->command[0].rolling_arm_state==PT_PRIVATE_CAUSAL_RAM_ARM_NOT_ENTERED){
    unsigned hook=a->initial_clock_hook;a->initial_clock_hook=0;
    if(hook==1)sl_inject64(a,&p->command[0].identity.ticket);
    else if(hook==2)sl_inject_mode(a,&p->command[0].rolling_arm_state,PT_PRIVATE_CAUSAL_RAM_ARM_ENTERED);
    else if(hook==3)sl_inject_mode(a,&p->rolling_mode,0);
    else if(hook==4)assert(pt_private_mixed_causal_ram_dispatch(p,p->command[0].identity.ticket)==PT_MIXED_CAUSAL_INVALID);
    else if(hook==5)return -1;
    else{assert(hook==6);*ticks=oracle(960);}}
 if(a->clock_hook&&p->rolling_before.serial){unsigned hook=a->clock_hook;a->clock_hook=0;
    if(hook==1)sl_inject64(a,&p->control_completed.observed);
    else if(hook==2)sl_inject64(a,&p->command[(unsigned)((p->rolling_serial+1)&1U)].serial);
    else if(hook==3)sl_inject64(a,&p->completed.observed);
    else if(hook==4)sl_inject_mode(a,&p->rolling_mode,0);else if(hook==5)sl_inject_mode(a,&p->control_mode,0);else sl_inject_mode(a,&p->lineage_mode,1);}
 return 1;
}
static int sl_adapter_arm(void *context,const struct pt_mixed_causal_registration *r,
 uint64_t ticket,uint64_t tick,uint32_t frequency)
{
 struct sl_adapter *a=context;struct pt_private_mixed_causal_ram_port *p=&a->owner->port.ram;
 unsigned i=a->arms;int raw=a->arm_raw;
 assert(i<4&&ct_registration(r,&p->registration)&&frequency==a->frequency&&ticket&&tick==oracle(960U*(i+1)));
 a->ticket[i]=ticket;a->live[i]=raw!=0||a->arm_hook;++a->arms;
 if(a->arm_hook){unsigned hook=a->arm_hook;a->arm_hook=0;
    if(hook==1)assert(pt_private_mixed_causal_ram_dispatch(p,ticket)==PT_MIXED_CAUSAL_INVALID);
    else if(hook==2)sl_inject64(a,&p->control_completed.observed);
    else if(hook==3)sl_inject64(a,&p->command[(unsigned)(p->rolling_serial&1U)].serial);
    else if(hook==4)sl_inject_mode(a,&p->rolling_mode,0);else if(hook==5)sl_inject_mode(a,&p->control_mode,0);
    else if(hook==6)sl_inject_mode(a,&p->lineage_mode,1);
    else if(hook==7)sl_inject_mode(a,&p->command[(unsigned)(p->rolling_serial&1U)].rolling_arm_state,
        PT_PRIVATE_CAUSAL_RAM_ARM_NOT_ENTERED);
    else if(hook==8){sl_inject64(a,&p->command[(unsigned)(p->rolling_serial&1U)].identity.ticket);*a->injected64=0;}
    else{assert(hook==9);sl_clear_custody(a,p->command+(unsigned)(p->rolling_serial&1U));}}
 return raw;
}
static int sl_adapter_quiet(void *context,const struct pt_mixed_causal_registration *r,uint64_t ticket,unsigned cancel)
{
 struct sl_adapter *a=context;unsigned i;
 assert(cancel<=1&&ct_registration(r,&a->owner->port.ram.registration));++a->proofs;
 for(i=0;i<a->arms&&a->ticket[i]!=ticket;++i){}assert(i<a->arms);
 if(a->hold[i])return 0;
 if(a->quiet_hook){struct pt_private_mixed_causal_ram_port *p=&a->owner->port.ram;
    unsigned hook=a->quiet_hook;a->quiet_hook=0;
    if(hook==1)sl_inject64(a,&p->completed.observed);
    else if(hook==2)sl_inject64(a,&p->command[i&1U].serial);
    else if(hook==3)sl_inject_mode(a,&p->rolling_mode,0);else if(hook==4)sl_inject_mode(a,&p->control_mode,0);
    else if(hook==5)sl_inject_mode(a,&p->lineage_mode,1);
    else if(hook==6||hook==7)sl_inject_mode(a,&p->command[i&1U].rolling_arm_state,
        hook==6?PT_PRIVATE_CAUSAL_RAM_ARM_NOT_ENTERED:0);
    else if(hook==8){sl_inject64(a,&p->command[i&1U].identity.ticket);*a->injected64=0;}
    else{assert(hook==9);sl_clear_custody(a,p->command+(i&1U));}}
 if(a->ticket_raw==1)a->live[i]=0;
 return a->ticket_raw;
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
 assert(e&&offsetof(struct sl_trial,base)==0);sl_current=e;f=&e->base;f->bits=bits;f->cache_bits=cache_bits;f->little=little;e->kind=which;
 f->card=calloc(1,sizeof(*f->card));f->document=calloc(1,sizeof(*f->document));
 f->editor=calloc(1,sizeof(*f->editor));f->binding=calloc(1,sizeof(*f->binding));
 assert(f->card&&f->document&&f->editor&&f->binding);f->bus.owner=f;f->ordinary.owner=f;f->masters.owner=f;f->chip.owner=f;e->port.external.owner=e;
 e->port.external.ticks=100;e->port.external.frequency=709379;
 e->port.external.arm_raw=e->port.external.source_raw=e->port.external.quiet_raw=e->port.external.ticket_raw=1;
 e->port.declaration=(struct pt_private_mixed_causal_ram_adapter){&e->port.external,sizeof(e->port.external),
    sl_adapter_clock,sl_adapter_arm,sl_adapter_quiet,sl_adapter_close,sl_adapter_source_quiet};
 assert(pt_private_mixed_causal_ram_init_rolling(&e->port.ram,31,17,709379,&e->port.declaration,128,8));
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
    p->events[32+i].effect=12;p->events[32+i].parameter=16;
    p->events[48+i].kind=PT_NOTE_OFF;}
 p->events[64+15].effect=15;
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
 e->source->original.original.binding=f->binding;
 e->source->original.original.preparation=&e->producer.preparation.original.original;
 e->source->original.preparation=&e->producer.preparation.original;
 e->source->preparation=&e->producer.preparation;e->source->original.original.contexts=f->input.contexts;
 e->source->original.original.immutable_count=6;e->source->original.original.mutable_count=2;
 e->source->original.original.immutable[0]=(struct pt_sampler_storage_span){f->causal_workspace,f->causal_capacity};
 e->source->original.original.immutable[1]=(struct pt_sampler_storage_span){f->factory_workspace,f->factory_capacity};
 e->source->original.original.immutable[2]=(struct pt_sampler_storage_span){f->card,sizeof(*f->card)};
 e->source->original.original.immutable[3]=(struct pt_sampler_storage_span){e->audit_storage,e->audit_capacity};
 e->source->original.original.immutable[4]=(struct pt_sampler_storage_span){e->audit_q,e->audit_q_capacity};
 e->source->original.original.immutable[5]=(struct pt_sampler_storage_span){e->q_storage,e->q_capacity};
 e->source->original.original.mutable[0]=(struct pt_sampler_storage_span){e->audit_result,e->result_capacity};
 e->source->original.original.mutable[1]=(struct pt_sampler_storage_span){e->outputs,e->output_capacity};
 e->source->original.original.causal_workspace=e->source->original.original.immutable[0];e->source->original.original.factory_workspace=e->source->original.original.immutable[1];
 e->source->original.original.backend_parent=e->source->original.original.immutable[2];e->producer.original.controller=sl_s(e);e->producer.original.joint=&e->joint.original;e->producer.joint=&e->joint;
 e->producer.original.configuration.preparation=f->input;
 e->producer.port=pt_private_mixed_causal_ram_rolling_api(&e->port.ram);
 assert(!pt_private_mixed_causal_ram_lineage_api(&e->port.ram).publish_stop_after_control);
 assert(!pt_private_mixed_causal_ram_control_api(&e->port.ram).publish_control);
 e->producer.original.configuration.options=(struct pt_render_options){0};
 e->producer.original.configuration.options.rate=48000;e->producer.original.configuration.options.bits=24;
 e->producer.original.configuration.options.gain_q16=65536;e->producer.original.configuration.options.tracks=0xffff;
 e->producer.original.configuration.options.tick_limit=1000;e->producer.original.configuration.options.frame_limit=100000;
 e->producer.original.configuration.options.include_lead_in=(uint8_t)lead;
 e->producer.original.configuration.caps=(struct pt_paula_render_caps){3546895,124,65535};
 e->producer.original.configuration.format.bits=(uint8_t)cache_bits;e->producer.original.configuration.format.little_endian=(uint8_t)little;
 e->producer.original.configuration.external_allocator=(struct pt_allocator){&e->external,sl_external_new,sl_external_release};
 e->producer.original.configuration.ordinary_budget=4*1024*1024;
 e->producer.original.configuration.audit_budget=e->audit_capacity+e->audit_q_capacity+e->result_capacity+
    pt_render_sequence_setup_control_size()+pt_render_sequence_control_size();
 e->producer.original.configuration.absolute_start=960;
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
 enum pt_editor_mixed_causal_song_lineage_result r;unsigned phase=e->producer.original.phase;
 e->promoting=phase<=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_ESTABLISH_CLOSE;
 r=pt_editor_mixed_causal_song_rolling_step(&e->producer,work);e->promoting=0;return r;
}
static void sl_begin(struct sl_trial *e)
{
 struct cp_trial *f=&e->base;unsigned masters=f->masters.calls;
 assert(offsetof(struct sl_trial,base)==0&&sizeof(*e)>sizeof(e->joint)+sizeof(e->producer));
 assert(!f->editor->sampler.current[0]&&!f->editor->sampler.current[1]&&masters==e->master_calls_at_begin);
 assert(pt_editor_mixed_causal_song_rolling_begin(&e->producer,e->source,e->borrow)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PENDING);
 assert(e->borrow->address==sl_s(e)&&e->borrow->serial&&e->producer.original.original==&e->producer.original&&
    f->binding->preparation_context==sl_s(e)&&f->binding->preparation_close&&!sl_s(e)->inputs&&
    !sl_s(e)->source_activated&&e->producer.original.adopted);
 assert(masters==f->masters.calls&&!f->ordinary.calls&&!e->external.calls&&!f->chip.calls&&!f->card->writes&&
    !e->port.binds&&!e->port.external.clocks&&!e->port.ram.publishes);
 assert(pt_editor_mixed_causal_song_rolling_step(&e->producer,0)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_INVALID);
 assert(pt_editor_mixed_causal_song_rolling_step(&e->producer,4097)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_INVALID);
}
static void sl_preflight(struct sl_trial *e)
{
 unsigned n=0,nonempty=0,i;sl_begin(e);
 while(e->producer.original.phase!=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CONTROLLER){
    if(e->producer.original.phase==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_LOWER&&e->outputs->original.normalized.normalized.count){
       static const unsigned kind[4]={PT_MIXED_PLAN_TRIGGER,PT_MIXED_PLAN_CONTROL,PT_MIXED_PLAN_CONTROL,PT_MIXED_PLAN_STOP};
       const struct pt_mixed_plan_quantized_batch *b=&e->outputs->original.normalized;
       assert(nonempty<4&&b->normalized.frame==960U*(nonempty+1)&&e->producer.original.target==b->normalized.frame);
       for(i=0;i<b->normalized.count;++i)assert(b->normalized.record[i].kind==kind[nonempty]);++nonempty;
    }
    assert(sl_step(e,17)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PENDING&&++n<100000);
    assert(!e->base.ordinary.calls&&!e->port.binds&&!e->port.external.clocks&&!e->port.ram.publishes&&
       !e->base.chip.calls&&!e->base.card->writes&&!sl_s(e)->causal&&!sl_s(e)->pool);
 }
 assert(nonempty==4&&e->producer.original.candidate_count==4&&e->producer.original.frames==3840&&e->producer.original.intervals==5&&
    e->producer.original.target==4800&&e->producer.original.audited.frames==3840&&e->producer.original.audited.intervals==5&&
    e->producer.original.audited.rewinds==1&&!memcmp(e->producer.original.audited.samples,e->producer.original.samples,sizeof(e->producer.original.samples))&&
    !memcmp(e->producer.original.frozen,e->outputs->original.candidate,sizeof(e->producer.original.frozen))&&e->external.calls==3&&
    e->producer.original.requests==3&&e->external.releases==2&&e->master_observed&&e->producer.original.activated&&
    !memcmp(&e->producer.fourth,&e->outputs->fourth,sizeof(e->producer.fourth)));
 for(i=0;i<e->base.editor->project->sample_count;++i)assert(e->producer.original.pin[i]&&e->producer.original.pin[i]==e->base.editor->sampler.current[i]);
 assert(e->base.editor->sampler.allocator.context==&e->base.masters&&e->base.editor->sampler.allocator.allocate==sl_master_new&&
    e->base.editor->sampler.allocator.release==cp_master_free);sl_same(e);
}
static void sl_to_publication(struct sl_trial *e,unsigned index)
{
 unsigned n=0;assert(e->producer.original.lineage_index==index);
 while(e->producer.original.phase!=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PUBLICATION){
    enum pt_editor_mixed_causal_song_lineage_result r=sl_step(e,17);
    assert((r==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PENDING||r==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PUBLISH)&&++n<100000);}
 assert(e->producer.original.result==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PUBLISH&&(*sl_history(e,index)).ticket&&
    (*sl_history(e,index)).reference.serial&&(e->producer.original.attempted_mask&(1U<<index)));
}
static void sl_publish_current(struct sl_trial *e,unsigned index)
{
 assert(e->producer.original.lineage_index==index&&pt_editor_mixed_causal_song_rolling_publish(&e->producer)==PT_MIXED_READERS_OK);
 assert((*sl_history(e,index)).published&&!(*sl_history(e,index)).uncertain&&e->producer.original.lineage_index==index+1);
}
static void sl_wait(struct sl_trial *e,unsigned index)
{
 unsigned n=0;unsigned char ram[sizeof(e->base.card->ram)];
 struct pt_mixed_plan_quantized_batch snapshots[3];struct pt_editor_mixed_reader_ref refs[32];
 unsigned chip=e->base.chip.calls,writes=e->base.card->writes,master=e->base.masters.calls,ordinary=e->base.ordinary.calls,i;
 memcpy(ram,e->base.card->ram,sizeof(ram));memcpy(snapshots,e->producer.original.frozen,sizeof(snapshots));
 for(i=0;i<32;++i)refs[i]=e->producer.original.reader[i].reference;
 while(e->producer.original.result!=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_WAIT_ACTIVE){
    assert(sl_step(e,17)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PENDING||e->producer.original.result==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_WAIT_ACTIVE);
    assert(++n<100000);}
 assert(e->producer.original.lineage_index==index&&e->producer.original.target==960U*(index+1)&&
    e->producer.original.phase==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_BATCH_BEGIN&&e->producer.original.attempted_mask==(1U<<index)-1&&
    ordinary==e->base.ordinary.calls&&chip==e->base.chip.calls&&writes==e->base.card->writes&&master==e->base.masters.calls&&
    !memcmp(ram,e->base.card->ram,sizeof(ram))&&!memcmp(snapshots,e->producer.original.frozen,sizeof(snapshots)));
 for(i=0;i<32;++i)assert(refs[i].serial==e->producer.original.reader[i].reference.serial&&refs[i].slot==e->producer.original.reader[i].reference.slot);
 assert(sl_step(e,17)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_WAIT_ACTIVE&&e->producer.original.target==960U*(index+1));
}
static struct pt_private_mixed_causal_ram_command *sl_ram_command(struct sl_trial *e,unsigned history)
{
 unsigned i;for(i=0;i<2;++i)if(e->port.ram.command[i].known&&
    e->port.ram.command[i].identity.ticket==(*sl_history(e,history)).ticket)return e->port.ram.command+i;
 return NULL;
}
static void sl_fire(struct sl_trial *e,unsigned index)
{
 struct pt_private_mixed_causal_ram_port *p=&e->port.ram;uint64_t ticket=(*sl_history(e,index)).ticket;
 unsigned before=p->effects+p->controls+p->stops;
 assert(ticket&&(*sl_history(e,index)).published);e->port.external.ticks=oracle(960U*(index+1))-1;
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
    assert(a->kind==(index==0?PT_MIXED_READERS_TRIGGER:index<3?PT_MIXED_READERS_CONTROL:PT_MIXED_READERS_STOP)&&
       c->packet.key[i].trigger==(*sl_history(e,0)).ticket&&c->packet.key[i].route==a->route&&c->packet.key[i].slot==a->slot);
    if(!index&&a->route==PT_MIXED_READERS_PAULA){assert(a->geometry.paula.words==CS_MONO_FRAMES/2);
       for(j=0;j<CS_MONO_FRAMES;++j)assert(a->geometry.paula.data[j]==(e->base.bits==24&&(j==2||j==3)?0U:(j&1U)?255U:1U));}
    if(!index&&a->route==PT_MIXED_READERS_AMIGUS){const struct pt_mixed_readers_card *card=c->packet.card+i;
       assert(card->logical_bytes==CS_MONO_FRAMES*2&&card->full_capacity>=card->logical_bytes);
       for(j=0;j<CS_MONO_FRAMES;++j){uint16_t w=e->base.bits==24&&j==2?1U:e->base.bits==24&&j==3?65535U:(j&1U)?65280U:256U;
          assert(e->base.card->ram[card->address+2*j]==(uint8_t)(w>>8)&&e->base.card->ram[card->address+2*j+1]==(uint8_t)w);}}
    if(index&&index<3&&a->route==PT_MIXED_READERS_AMIGUS){const struct pt_mixed_plan_quantized_batch *b=e->producer.original.frozen+index;
       assert(a->geometry.amigus.left==b->normalized.record[i].geometry.amigus.left&&
          a->geometry.amigus.right==b->normalized.record[i].geometry.amigus.right&&
          (a->geometry.amigus.left>255||a->geometry.amigus.right>255));}
 }
}
static void sl_commands(struct sl_trial *e,unsigned cancel)
{
 unsigned i;for(i=0;i<4;++i)if((*sl_history(e,i)).reference.serial&&(*sl_history(e,i)).ticket&&!(*sl_history(e,i)).detached){
    enum pt_mixed_readers_result r=pt_editor_mixed_causal_song_rolling_service_command(&e->producer,i,cancel);
    assert(r==PT_MIXED_READERS_OK||r==PT_MIXED_READERS_PENDING||r==PT_MIXED_READERS_BACKEND);}
}
static void sl_readers(struct sl_trial *e,unsigned cancel)
{
 unsigned i;for(i=0;i<32;++i)if(e->producer.original.reader[i].reference.serial){
    struct pt_editor_mixed_reader_ref r=e->producer.original.reader[i].reference;
    if(sl_s(e)->reader[r.slot].ticket){enum pt_mixed_readers_result result=pt_editor_mixed_causal_song_rolling_service_reader(&e->producer,i,cancel);
       assert(result==PT_MIXED_READERS_OK||result==PT_MIXED_READERS_PENDING||result==PT_MIXED_READERS_BACKEND);}}
}
static void sl_close(struct sl_trial *e)
{
 unsigned i,n=0;assert(pt_editor_mixed_causal_song_rolling_cancel(&e->producer)!=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_INVALID);
 sl_commands(e,1);sl_readers(e,1);
 while(!pt_editor_mixed_causal_song_rolling_close(&e->producer)){
    assert(++n<1000&&e->base.binding->preparation_context==sl_s(e));
    if(e->producer.original.borrow_closed)assert(!e->borrow->address&&!e->borrow->serial);else assert(e->borrow->address==sl_s(e));
    sl_commands(e,1);sl_readers(e,1);}
 assert(!e->outputs->original.audit&&!e->outputs->original.sequence&&!e->outputs->original.normalizer&&!e->producer.original.establish.active&&!e->producer.original.pin_job.owner&&
    !e->producer.original.pin_count&&!e->borrow->address&&!e->borrow->serial&&!e->base.binding->preparation_context&&
    !sl_s(e)->pool&&!sl_s(e)->causal&&!sl_s(e)->queue&&!e->external.alias_releases);
 for(i=0;i<32;++i)assert(!e->producer.original.pin[i]);for(i=0;i<e->external.calls;++i)assert(!e->external.live[i]);e->closed=1;
}
static void sl_drop(struct sl_trial *e)
{
 struct cp_trial *f=&e->base;unsigned i;
 assert((e->closed||!e->producer.original.adopted)&&!f->binding->preparation_context&&!e->borrow->address&&!e->borrow->serial);
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
 assert(pt_editor_mixed_causal_source_borrow_close(e->borrow));free(e->borrow);sl_current=NULL;free(e);
}


static void sl_detach(struct sl_trial *e,unsigned i)
{
 struct pt_editor_mixed_causal_song_lineage_history *h=sl_history(e,i);
 struct pt_editor_mixed_command_ref original=h->reference;
 e->port.external.hold[i]=0;
 assert(pt_editor_mixed_causal_song_rolling_service_command(&e->producer,i,0)==PT_MIXED_READERS_OK);
 assert(h->detached&&!sl_s(e)->command[original.slot].handle.address&&
    !sl_s(e)->command[original.slot].handle.token);
 assert(e->joint.rolling.row[original.slot].detached&&e->joint.rolling.row[original.slot].closed&&
    e->joint.rolling.row[original.slot].ticket==h->ticket&&e->joint.rolling.row[original.slot].ref.serial==original.serial);
}
/* Actual queue lifecycle, never a forged key or receipt. A musical STOP owns
 * its original16 before enqueue, closes further admission during enqueue, and
 * still publishes that exact already-admitted command through genuine core. */
static void sl_admission(struct sl_trial *e,unsigned closed)
{
 unsigned i,n=0;struct pt_mixed_readers_key key,saved;
 const struct pt_mixed_causal_actual *root=&e->port.ram.completed.post;
 assert(root->active_mask==root->adopted_mask);
 for(i=0;i<20;++i)if(root->active_mask&(1U<<i)){
    enum pt_mixed_readers_result r;memset(&key,0xa5,sizeof(key));saved=key;
    r=pt_mixed_readers_reader_key(sl_s(e)->queue,root->slot[i].trigger,root->slot[i].action,&key);
    assert(r==(closed?PT_MIXED_READERS_STALE:PT_MIXED_READERS_OK));
    if(closed)assert(!memcmp(&key,&saved,sizeof(key)));
    else assert(!memcmp(&key,root->slot+i,sizeof(key)));
    ++n;
 }
 assert(n==16&&pt_mixed_readers_readers_held(sl_s(e)->queue)==16);
}
/* Genuine preparation to each reuse boundary; no supplied completion/keys. */
static struct sl_trial *sl_prefix(unsigned bits,unsigned card,unsigned boundary)
{
 struct sl_trial *e=sl_make(bits,16,0,card?CS_CARD16:CS_MIXED,0);
 struct pt_private_mixed_causal_ram_port *p=&e->port.ram;
 sl_preflight(e);sl_to_publication(e,0);sl_publish_current(e,0);sl_packet(e,0);
 sl_wait(e,1);sl_fire(e,0);e->port.external.hold[0]=1;
 assert(pt_editor_mixed_causal_song_rolling_service_command(&e->producer,0,0)==PT_MIXED_READERS_PENDING);
 assert(sl_history(e,0)->service_seen&&!sl_history(e,0)->detached&&p->completed.valid);
 sl_to_publication(e,1);if(boundary==1)return e;
 sl_publish_current(e,1);sl_packet(e,1);sl_wait(e,2);sl_fire(e,1);
 assert(p->control_completed.valid&&p->control_completed.identity.ticket==sl_history(e,1)->ticket&&p->controls==16);
 assert(sl_step(e,17)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_WAIT_ACTIVE&&p->pair_used==2);
 sl_detach(e,0);sl_to_publication(e,2);
 if(boundary==2)return e;
 sl_publish_current(e,2);sl_packet(e,2);sl_wait(e,3);sl_fire(e,2);sl_admission(e,0);
 e->port.external.hold[1]=1;
 assert(pt_editor_mixed_causal_song_rolling_service_command(&e->producer,1,0)==PT_MIXED_READERS_PENDING);
 assert(sl_step(e,17)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_WAIT_ACTIVE&&p->pair_used==3);
 sl_detach(e,1);sl_to_publication(e,3);sl_admission(e,1);return e;
}
static void sl_four(unsigned bits,unsigned card,unsigned source_hold)
{
 struct sl_trial *e=sl_prefix(bits,card,2);struct pt_private_mixed_causal_ram_port *p=&e->port.ram;
 struct pt_private_mixed_causal_ram_completed root=p->completed,first_control=p->control_completed;
 struct pt_private_mixed_causal_ram_reader readers[32];struct pt_editor_mixed_reader_ref refs[32];
 unsigned char ram[sizeof(e->base.card->ram)];unsigned i,chips=e->base.chip.calls,writes=e->base.card->writes,masters=e->base.masters.calls;
 memcpy(readers,p->reader,sizeof(readers));memcpy(ram,e->base.card->ram,sizeof(ram));
 for(i=0;i<32;++i)refs[i]=e->producer.original.reader[i].reference;
 assert(sl_history(e,2)->reference.slot==sl_history(e,0)->reference.slot&&
    sl_history(e,2)->reference.serial!=sl_history(e,0)->reference.serial);
 sl_publish_current(e,2);sl_packet(e,2);sl_wait(e,3);sl_fire(e,2);sl_admission(e,0);
 assert(p->controls==32&&p->effects==16&&!p->stops&&p->control_completed.identity.ticket==sl_history(e,2)->ticket&&
    p->control_completed.frame==2880&&p->control_completed.observed==oracle(2880)&&p->control_completed.issued==oracle(2880));
 assert(!memcmp(&root,&p->completed,sizeof(root))&&memcmp(&first_control,&p->control_completed,sizeof(first_control)));
 assert(pt_editor_mixed_causal_prepare_service_command(sl_s(e),sl_history(e,0)->reference,0,NULL)==PT_MIXED_READERS_INVALID);
 e->port.external.hold[1]=1;
 assert(pt_editor_mixed_causal_song_rolling_service_command(&e->producer,1,0)==PT_MIXED_READERS_PENDING);
 assert(sl_step(e,17)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_WAIT_ACTIVE&&e->producer.original.attempted_mask==7);
 sl_detach(e,1);sl_to_publication(e,3);sl_admission(e,1);
 assert(sl_history(e,3)->reference.slot==sl_history(e,1)->reference.slot&&
    sl_history(e,3)->reference.serial!=sl_history(e,1)->reference.serial);
 sl_publish_current(e,3);sl_packet(e,3);
 assert(p->pair_used==4&&p->command[0].identity.ticket==sl_history(e,2)->ticket&&
    p->command[1].identity.ticket==sl_history(e,3)->ticket&&e->producer.original.attempted_mask==15&&
    pt_mixed_readers_commands_held(sl_s(e)->queue)==2);
 assert(pt_editor_mixed_causal_prepare_service_command(sl_s(e),sl_history(e,1)->reference,0,NULL)==PT_MIXED_READERS_INVALID);
 for(i=0;i<32;++i){assert(refs[i].slot==e->producer.original.reader[i].reference.slot&&refs[i].serial==e->producer.original.reader[i].reference.serial);
    if(readers[i].live){assert(p->reader[i].live&&p->reader[i].active&&
       !memcmp(&readers[i].key,&p->reader[i].key,sizeof(readers[i].key))&&
       !memcmp(&readers[i].card,&p->reader[i].card,sizeof(readers[i].card)));
       if(readers[i].action.route==PT_MIXED_READERS_PAULA)assert(readers[i].action.geometry.paula.data==p->reader[i].action.geometry.paula.data&&
          readers[i].action.geometry.paula.words==p->reader[i].action.geometry.paula.words);
       else assert(readers[i].action.geometry.amigus.start==p->reader[i].action.geometry.amigus.start&&
          readers[i].action.geometry.amigus.loop==p->reader[i].action.geometry.amigus.loop&&
          readers[i].action.geometry.amigus.end_exclusive==p->reader[i].action.geometry.amigus.end_exclusive);}}
 assert(e->base.chip.calls==chips&&e->base.card->writes==writes&&e->base.masters.calls==masters&&!memcmp(ram,e->base.card->ram,sizeof(ram)));
 sl_fire(e,3);assert(p->stops==16&&!p->mask&&p->controls==32&&p->effects==16&&
    pt_mixed_readers_readers_held(sl_s(e)->queue)==16);
 e->port.external.hold[2]=1;
 assert(pt_editor_mixed_causal_song_rolling_service_reader(&e->producer,0,0)==PT_MIXED_READERS_PENDING&&!p->source_attempted);
 e->port.external.hold[2]=0;sl_commands(e,0);sl_readers(e,0);sl_commands(e,0);sl_readers(e,0);
 assert(!pt_mixed_readers_commands_held(sl_s(e)->queue)&&!pt_mixed_readers_readers_held(sl_s(e)->queue));
 if(source_hold){e->port.external.source_raw=e->port.external.quiet_raw=0;
    assert(pt_editor_mixed_causal_song_rolling_cancel(&e->producer)!=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_INVALID);
    for(i=0;!p->source_attempted&&i<1000;++i)assert(!pt_editor_mixed_causal_song_rolling_close(&e->producer));
    assert(i<1000&&!p->source_closed&&e->producer.original.pin_count&&e->borrow->address==sl_s(e)&&e->base.binding->preparation_context==sl_s(e));
    assert(!pt_editor_mixed_causal_song_rolling_close(&e->producer));e->port.external.quiet_raw=1;}
 sl_close(e);sl_same(e);sl_drop(e);
}
static void sl_refuse_history(unsigned boundary,unsigned root)
{
 struct sl_trial *e=sl_prefix(16,0,boundary);struct pt_private_mixed_causal_ram_port *p=&e->port.ram;
 struct pt_private_mixed_causal_ram_completed *h=root?&p->completed:&p->control_completed,saved=*h;
 unsigned arms=e->port.external.arms,used=p->pair_used;enum pt_mixed_readers_result result;
 h->observed++;result=pt_editor_mixed_causal_song_rolling_publish(&e->producer);
 assert(result==PT_MIXED_READERS_PENDING&&e->port.external.arms==arms&&p->pair_used==used&&!p->failed&&!p->suppressed&&
    !sl_history(e,boundary)->published&&!sl_history(e,boundary)->uncertain);
 *h=saved;sl_close(e);sl_same(e);sl_drop(e);
}
static void sl_refuse_arm(unsigned boundary,unsigned which)
{
 struct sl_trial *e=boundary?sl_prefix(16,0,boundary):sl_make(16,16,0,CS_MIXED,0);
 struct pt_private_mixed_causal_ram_port *p=&e->port.ram;
 enum pt_mixed_readers_result result;unsigned arms=e->port.external.arms;
 if(!boundary){sl_preflight(e);sl_to_publication(e,0);}
 e->port.external.arm_raw=which==1?-1:0;e->port.external.arm_hook=which==2;
 result=pt_editor_mixed_causal_song_rolling_publish(&e->producer);
 assert(result!=PT_MIXED_READERS_OK&&e->port.external.arms==arms+1&&p->pair_used==boundary+1&&!sl_history(e,boundary)->published);
 if(!which){assert(result==PT_MIXED_READERS_PENDING&&!p->failed&&!p->suppressed&&!sl_history(e,boundary)->uncertain);
    assert(pt_editor_mixed_causal_song_rolling_publish(&e->producer)==PT_MIXED_READERS_INVALID&&e->port.external.arms==arms+1);}
 else assert(result==PT_MIXED_READERS_BACKEND&&p->failed&&p->suppressed&&sl_history(e,boundary)->uncertain);
 e->port.external.arm_hook=0;e->port.external.arm_raw=1;sl_close(e);sl_same(e);sl_drop(e);
}
static void sl_admin_cancel(void)
{
 struct sl_trial *e=sl_prefix(24,0,2);unsigned stops=e->port.ram.stops;
 assert(pt_editor_mixed_causal_song_rolling_cancel(&e->producer)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CANCELLED);
 assert(e->port.ram.stops==stops&&!e->port.ram.source_attempted&&e->producer.original.pin_count);
 sl_close(e);sl_same(e);sl_drop(e);
}
static void sl_callback_mutation(unsigned boundary,unsigned arm,unsigned which)
{
 struct sl_trial *e=sl_prefix(16,0,boundary);struct pt_private_mixed_causal_ram_port *p=&e->port.ram;
 enum pt_mixed_readers_result r;unsigned arms=e->port.external.arms,used=p->pair_used;
 if(arm)e->port.external.arm_hook=which+2;else e->port.external.clock_hook=which+1;
 r=pt_editor_mixed_causal_song_rolling_publish(&e->producer);
 assert(r==PT_MIXED_READERS_BACKEND&&p->failed&&p->suppressed&&
    e->port.external.arms==arms+arm&&p->pair_used==used+arm&&
    !sl_history(e,boundary)->published&&sl_history(e,boundary)->uncertain&&
    e->producer.original.pin_count==e->base.editor->project->sample_count);
 if(!arm){struct pt_private_mixed_causal_ram_command *c=sl_ram_command(e,boundary);
    unsigned proofs=e->port.external.proofs;struct pt_mixed_causal_command_identity identity;
    struct pt_mixed_causal_port api=pt_private_mixed_causal_ram_api(p);
    assert(c&&c->rolling_arm_state==PT_PRIVATE_CAUSAL_RAM_ARM_NOT_ENTERED&&c->known&&
       !c->live&&!c->armed&&!c->owner&&c->quiet_confirmed&&c->uncertain&&!c->finished);
    identity=c->identity;
    /* No adapter ticket_quiet is legal for a ticket never passed to arm_at. */
    assert(api.command_quiet(api.context,&identity,1)==1&&e->port.external.proofs==proofs&&
       p->failed&&p->suppressed&&!sl_history(e,boundary)->detached);}
 sl_undo_injected(&e->port.external);e->port.external.hold[0]=0;sl_close(e);sl_same(e);sl_drop(e);
}
static void sl_quiet_mutation(unsigned which)
{
 struct sl_trial *e=sl_make(16,16,0,CS_MIXED,0);struct pt_private_mixed_causal_ram_port *p=&e->port.ram;
 struct pt_editor_mixed_command_ref ref;sl_preflight(e);sl_to_publication(e,0);
 sl_publish_current(e,0);sl_packet(e,0);sl_wait(e,1);sl_fire(e,0);
 ref=sl_history(e,0)->reference;e->port.external.quiet_hook=which+1;
 assert(pt_editor_mixed_causal_song_rolling_service_command(&e->producer,0,0)==PT_MIXED_READERS_BACKEND&&
    p->failed&&p->suppressed&&p->command[0].uncertain&&!sl_history(e,0)->detached&&
    sl_s(e)->command[ref.slot].handle.address&&e->producer.original.pin_count&&
    !e->joint.rolling.row[ref.slot].closed&&p->command[0].rolling_ticket_raw==1&&
    p->command[0].ticket_outcome==-1);
 sl_undo_injected(&e->port.external);sl_close(e);sl_same(e);sl_drop(e);
}
static void sl_arm_state_mutation(unsigned which)
{
 struct sl_trial *e=sl_prefix(16,0,2);struct pt_private_mixed_causal_ram_port *p=&e->port.ram;
 unsigned arms=e->port.external.arms,proofs=e->port.external.proofs;
 struct pt_private_mixed_causal_ram_command *c;enum pt_mixed_readers_result r;
 e->port.external.arm_raw=which==0?1:which==1?0:-1;e->port.external.arm_hook=7;
 r=pt_editor_mixed_causal_song_rolling_publish(&e->producer);c=sl_ram_command(e,2);
 assert(r==PT_MIXED_READERS_BACKEND&&p->failed&&p->suppressed&&c&&c->live&&c->armed&&
    c->uncertain&&!c->quiet_confirmed&&c->rolling_arm_state==PT_PRIVATE_CAUSAL_RAM_ARM_ENTERED&&
    e->port.external.arms==arms+1&&e->port.external.proofs==proofs&&
    !sl_history(e,2)->published&&sl_history(e,2)->uncertain&&e->producer.original.pin_count);
 sl_undo_injected(&e->port.external);e->port.external.arm_raw=1;e->port.external.hold[2]=1;
 r=pt_editor_mixed_causal_song_rolling_service_command(&e->producer,2,1);
 assert(r==PT_MIXED_READERS_BACKEND&&e->port.external.proofs==proofs+1&&!sl_history(e,2)->detached&&
    c->live&&c->uncertain&&c->rolling_arm_state==PT_PRIVATE_CAUSAL_RAM_ARM_ENTERED&&
    sl_s(e)->command[sl_history(e,2)->reference.slot].handle.address);
 e->port.external.hold[2]=0;sl_close(e);sl_same(e);sl_drop(e);
}
static void sl_initial_clock_refusal(unsigned which)
{
 struct sl_trial *e=sl_make(16,16,0,CS_MIXED,0);struct pt_private_mixed_causal_ram_port *p=&e->port.ram;
 enum pt_mixed_readers_result r;struct pt_private_mixed_causal_ram_command *c;unsigned proofs;
 sl_preflight(e);sl_to_publication(e,0);e->port.external.initial_clock_hook=which+1;
 r=pt_editor_mixed_causal_song_rolling_publish(&e->producer);c=sl_ram_command(e,0);proofs=e->port.external.proofs;
 assert(!e->port.external.initial_clock_hook&&!e->port.external.arms&&!p->pair_used&&!p->rolling_serial&&
    !p->effects&&!p->controls&&!p->stops&&!p->source_attempted&&!sl_history(e,0)->published&&
    e->producer.original.pin_count==e->base.editor->project->sample_count);
 if(which==5){assert(r==PT_MIXED_READERS_PENDING&&!p->failed&&!p->suppressed&&
    !sl_history(e,0)->uncertain&&!c);}
 else{struct pt_mixed_causal_port api=pt_private_mixed_causal_ram_api(p);struct pt_mixed_causal_command_identity identity;
    assert(r==PT_MIXED_READERS_BACKEND&&p->failed&&p->suppressed&&sl_history(e,0)->uncertain&&
       c&&c->known&&c->uncertain&&!c->live&&!c->armed&&!c->owner&&!c->finished&&c->quiet_confirmed&&
       c->rolling_arm_state==PT_PRIVATE_CAUSAL_RAM_ARM_NOT_ENTERED);
    identity=c->identity;assert(api.command_quiet(api.context,&identity,1)==1&&e->port.external.proofs==proofs&&
       p->failed&&p->suppressed&&!sl_history(e,0)->detached);}
 if(e->port.external.injected64||e->port.external.injected_mode)sl_undo_injected(&e->port.external);
 sl_close(e);sl_same(e);sl_drop(e);
}
static void sl_owned_row_mutation(unsigned arm,unsigned shape,unsigned outcome)
{
 struct sl_trial *e=arm?sl_prefix(16,0,2):sl_make(16,16,0,CS_MIXED,0);
 struct pt_private_mixed_causal_ram_port *p=&e->port.ram;struct sl_adapter *a=&e->port.external;
 unsigned index=arm?2:0,proofs=a->proofs;struct pt_private_mixed_causal_ram_command *c;
 enum pt_mixed_readers_result r;struct pt_editor_mixed_command_ref ref;
 int raw=outcome==0?1:outcome==1?0:-1;
 if(arm){a->arm_raw=raw;a->arm_hook=shape?9:8;
    r=pt_editor_mixed_causal_song_rolling_publish(&e->producer);}
 else{sl_preflight(e);sl_to_publication(e,0);sl_publish_current(e,0);sl_packet(e,0);sl_wait(e,1);sl_fire(e,0);
    proofs=a->proofs;a->ticket_raw=raw;a->quiet_hook=shape?9:8;
    r=pt_editor_mixed_causal_song_rolling_service_command(&e->producer,0,0);}
 c=sl_ram_command(e,index);ref=sl_history(e,index)->reference;
 assert(r==PT_MIXED_READERS_BACKEND&&p->failed&&p->suppressed&&c&&c->known&&c->live&&
    c->uncertain&&!c->quiet_confirmed&&c->owner==sl_s(e)->causal&&c->packet.count==16&&
    c->packet.ticket==sl_history(e,index)->ticket&&c->identity.ticket==sl_history(e,index)->ticket&&
    c->rolling_arm_state==PT_PRIVATE_CAUSAL_RAM_ARM_ENTERED&&!sl_history(e,index)->detached&&
    sl_s(e)->command[ref.slot].handle.address&&e->producer.original.pin_count&&!p->source_attempted&&
    a->proofs==proofs+(arm?0:1)&&a->custody_clears==shape&&
    (arm?c->arm_outcome==raw:(c->rolling_ticket_raw==raw&&c->ticket_outcome==-1)));
 if(a->injected64||a->injected_mode)sl_undo_injected(a);
 a->arm_raw=a->ticket_raw=1;
 /* Pending real quiet retains the saved state; only later exact quiet1 may
  * consume it. No test clears any real uncertainty, ownership or fault. */
 proofs=a->proofs;a->hold[index]=1;
 r=pt_editor_mixed_causal_song_rolling_service_command(&e->producer,index,1);
 assert(r==PT_MIXED_READERS_BACKEND&&a->proofs==proofs+1&&c->live&&c->uncertain&&
    !c->quiet_confirmed&&!sl_history(e,index)->detached&&e->producer.original.pin_count&&
    sl_s(e)->command[ref.slot].handle.address&&c->rolling_ticket_raw==0&&c->ticket_outcome==0);
 a->hold[index]=0;sl_close(e);sl_same(e);sl_drop(e);
}
static void sl_persistent_profile_quiet_mutation(void)
{
 struct sl_trial *e=sl_prefix(16,0,2);struct pt_private_mixed_causal_ram_port *p=&e->port.ram;
 struct sl_adapter *a=&e->port.external;struct pt_private_mixed_causal_ram_command *c;
 struct pt_editor_mixed_command_ref ref;enum pt_mixed_readers_result r;unsigned *injected,saved,proofs;
 a->arm_hook=4;r=pt_editor_mixed_causal_song_rolling_publish(&e->producer);c=sl_ram_command(e,2);
 assert(r==PT_MIXED_READERS_BACKEND&&!p->rolling_mode&&p->failed&&p->suppressed&&c&&c->live&&
    c->uncertain&&c->rolling_arm_state==PT_PRIVATE_CAUSAL_RAM_ARM_ENTERED&&sl_history(e,2)->uncertain);
 /* Keep the deliberately corrupted profile through the next genuine quiet.
  * Save only its injected byte identity/value; do not rewind the whole port. */
 injected=a->injected_mode;saved=a->saved_mode;assert(injected==&p->rolling_mode&&saved==1);a->injected_mode=NULL;
 ref=sl_history(e,2)->reference;proofs=a->proofs;a->quiet_hook=7;
 r=pt_editor_mixed_causal_song_rolling_service_command(&e->producer,2,1);
 assert(r==PT_MIXED_READERS_BACKEND&&a->proofs==proofs+1&&!p->rolling_mode&&p->failed&&p->suppressed&&
    c->live&&c->uncertain&&!c->quiet_confirmed&&c->rolling_arm_state==PT_PRIVATE_CAUSAL_RAM_ARM_ENTERED&&
    !sl_history(e,2)->detached&&sl_s(e)->command[ref.slot].handle.address&&e->producer.original.pin_count&&!p->source_attempted&&
    c->rolling_ticket_raw==1&&c->ticket_outcome==-1);
 sl_undo_injected(a);*injected=saved;sl_close(e);sl_same(e);sl_drop(e);
}
/* These reachable states use actual factory-owned targets and genuine core
 * service/publication. No test callback fabricates CONTROL/STOP or receipts.
 * Superseding TRIGGER is outside this deliberately closed rolling profile. */
static void sl_target_refusal(unsigned which)
{
 struct sl_trial *e=sl_prefix(16,0,3);struct pt_private_mixed_causal_ram_port *p=&e->port.ram;
 struct pt_mixed_readers_key key=p->completed.post.slot[0];
 struct pt_mixed_readers_reader_receipt receipt;enum pt_mixed_readers_result r;
 unsigned arms=e->port.external.arms,used=p->pair_used,publishes=p->publishes,stops=p->stops;
 memset(&receipt,0,sizeof(receipt));sl_target_service=sl_target_publish=-1;sl_target_state=0;
 if(which==2){
    sl_publish_current(e,3);sl_packet(e,3);sl_fire(e,3);
    arms=e->port.external.arms;used=p->pair_used;publishes=p->publishes;stops=p->stops;
    e->port.external.hold[3]=1;
    r=pt_mixed_causal_service_reader(sl_s(e)->causal,key.trigger,key.action,0,&receipt);
    sl_target_service=(int)r;sl_target_state=(unsigned)receipt.state;
    assert(r==PT_MIXED_READERS_PENDING&&receipt.state==PT_MIXED_READER_DRAINING&&
       receipt.adoption==PT_MIXED_ADOPTED&&!memcmp(&receipt.key,&key,sizeof(key))&&stops==16);
 }else if(which<2){
    if(!which)e->port.external.hold[2]=1;
    r=pt_mixed_causal_service_reader(sl_s(e)->causal,key.trigger,key.action,1,&receipt);
    sl_target_service=(int)r;sl_target_state=(unsigned)receipt.state;
    assert(r==(which?PT_MIXED_READERS_OK:PT_MIXED_READERS_PENDING)&&
       receipt.state==(which?PT_MIXED_READER_RETIRED:PT_MIXED_READER_ACTIVE)&&
       receipt.adoption==PT_MIXED_ADOPTED&&!memcmp(&receipt.key,&key,sizeof(key))&&!stops);
 }else{assert(which==3);e->base.card->healthy=0;}
 r=pt_mixed_causal_rolling_publish(sl_s(e)->causal,sl_history(e,3)->ticket);sl_target_publish=(int)r;
 assert(r==(which==3?PT_MIXED_READERS_STALE:PT_MIXED_READERS_INVALID)&&
    e->port.external.arms==arms&&p->pair_used==used&&p->publishes==publishes&&p->stops==stops&&
    !p->source_attempted&&e->producer.original.pin_count==e->base.editor->project->sample_count);
 if(which==3)e->base.card->healthy=1; /* Cleanup only; failed admission is never retried. */
 e->port.external.hold[2]=e->port.external.hold[3]=0;
 sl_close(e);sl_same(e);sl_drop(e);
}
/* A private including fixture may select only this dormant donor entry.
 * The ordinary rolling68 entry and every assertion remain the default. */
#ifndef PT_PRIVATE_ROLLING_RAM_FIXTURE_ENTRY
#define PT_PRIVATE_ROLLING_RAM_FIXTURE_ENTRY main
#endif
int PT_PRIVATE_ROLLING_RAM_FIXTURE_ENTRY(void)
{
 unsigned b,r;sl_case="mixed16";sl_four(16,0,0);sl_case="mixed24-sourcehold";sl_four(24,0,1);
 sl_case="card16";sl_four(16,1,0);sl_case="card24-sourcehold";sl_four(24,1,1);
 for(b=2;b<=3;++b)for(r=0;r<2;++r){sl_case=r?"root-history-refusal":"latest-history-refusal";sl_refuse_history(b,r);}
 for(b=2;b<=3;++b)for(r=0;r<3;++r){sl_case=r==0?"clean-arm0":r==1?"unknown-arm":"reentrant-arm0";sl_refuse_arm(b,r);}
 sl_case="administrative-cancel";sl_admin_cancel();
 for(b=1;b<=3;++b)for(r=0;r<2;++r){sl_case="clock-custody-mutation";sl_callback_mutation(b,0,r);
    sl_case="arm-custody-mutation";sl_callback_mutation(b,1,r);}
 for(r=0;r<2;++r){sl_case="quiet-custody-mutation";sl_quiet_mutation(r);}
 for(r=0;r<3;++r){sl_case="clock-mode-mutation";sl_callback_mutation(2,0,r+3);
    sl_case="arm-mode-mutation";sl_callback_mutation(2,1,r+2);
    sl_case="quiet-mode-mutation";sl_quiet_mutation(r+2);}
 for(r=0;r<3;++r){sl_case="arm-entry-state-mutation";sl_arm_state_mutation(r);}
 sl_case="quiet-entry-state-mutation";sl_quiet_mutation(5);
 for(r=0;r<6;++r){sl_case="initial-clock-refusal";sl_initial_clock_refusal(r);}
 for(r=0;r<3;++r){sl_case="initial-arm-refusal";sl_refuse_arm(0,r);}
 sl_case="persistent-profile-quiet-state-mutation";sl_persistent_profile_quiet_mutation();
 for(b=0;b<2;++b)for(r=0;r<3;++r){sl_case="arm-owned-row-erasure";sl_owned_row_mutation(1,b,r);
    sl_case="quiet-owned-row-erasure";sl_owned_row_mutation(0,b,r);}
 sl_case="cancelled-target-not-quiet";sl_target_refusal(0);
 sl_case="retired-target";sl_target_refusal(1);
 sl_case="draining-target";sl_target_refusal(2);
 sl_case="refusing-current-owner";sl_target_refusal(3);
 assert(sl_cases==68);
 puts("ROLLING RAM T16-C16-C16-S16 HOST FIXTURE PASS:68 cases;genuine producer-factory-controller-core;both C row reuse after actual quiet-detach-NULL disposal;root-latest actual histories;full20 registry;original16 readers and masters-cache leases;ownSTOP admission closed;root-empty-reused no-arm tombstones and sticky attempted-arm quiet through corrupted profile;cancelled-retired-draining-owner-current refusals;exact960-1920-2880-3840;SOFTWARE_ONLY");return 0;
}
