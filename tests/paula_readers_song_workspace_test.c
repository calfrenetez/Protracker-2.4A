/* Existing genuine fixture resolves in this same tests directory.
 * Model callbacks are serialized host work, never native activation. */
#define main inherited_readers_fixture_main
#include "sampler_paula_readers_test.c"
#undef main
#include "../src/editor/paula_preflight.h"
#include "../src/core/render_lookahead.h"
#include "../src/editor/paula_song.h"
/* Legacy and new public declarations must coexist in this real fixture TU. */
static struct pt_render_sequence *fixture_transferred_sequence;
static int fixture_observe_preflight_transfer(struct pt_paula_preflight *work,struct pt_render_sequence **out)
{int result=pt_paula_preflight_transfer(work,out);if(result)fixture_transferred_sequence=*out;return result;}
/* Observe only the output of the genuine actual audit transfer; do not guess
 * allocator ordering or reconstruct a sequence from any report/owner layout. */
#define pt_paula_preflight_transfer fixture_observe_preflight_transfer
#include "../src/editor/paula_readers_song.c"
#undef pt_paula_preflight_transfer

#define SONG_RATE 48000U
#define SONG_FREQUENCY 1000000U
#define SONG_START 10000U
static const unsigned song_track[4]={0,2,4,6};
static const struct pt_paula_render_caps song_caps={3546895,124,65535};

struct song_fixture {
 struct fixture genuine;
 struct pt_sample_version *prepared[2];
 struct pt_sample sample_before[2];
 void *master_before[2];size_t master_bytes[2];
 struct pt_sample *table_before;
 int32_t original_before[VALUES];
 struct pt_pattern_history history;struct pt_pattern_command history_command[4];struct pt_event_change history_change[4];
 unsigned history_active,padding_slot;int32_t *padding_data;void *padding_before;size_t padding_bytes;
};

/* Required masters are promoted BEFORE borrowed startup/sequence owners exist.
 * Pins belong to the fixture and are independent of producer/backend pins. */
static struct song_fixture *song_fixture_new(unsigned bits,unsigned promote)
{
 struct song_fixture *s=calloc(1,sizeof(*s));struct fixture *f;struct pt_project *p;unsigned i;
 assert(s);f=&s->genuine;f->allocator=(struct pt_allocator){&f->fast,allocate,release};
 pt_document_init(&f->document,&f->allocator);
 assert(pt_document_new(&f->document,16,SIZE_MAX)==PT_PROJECT_OK);p=&f->document.project;
 for(i=0;i<VALUES;++i)f->master.values[i]=((int32_t)(i%200)-100)*(1L<<(bits-8))+(bits==8?0:(int32_t)(i&63));
 f->master.values[0]=-(1L<<(bits-1));f->master.values[1]=(1L<<(bits-1))-1;
 f->master.values[2]=bits==8?0x12:bits==16?0x1234:0x123456;f->master.values[3]=-f->master.values[2];
 memcpy(s->original_before,f->master.values,sizeof(s->original_before));
 for(i=0;i<16;++i)p->channels.track[i].route=PT_AMIGUS;
 for(i=0;i<4;++i){p->channels.track[song_track[i]].route=PT_PAULA;p->channels.track[song_track[i]].pan=i==0||i==3?0:255;}
 p->samples[0].pcm=(struct pt_pcm){f->master.values,VALUES,16384,8000,1,(uint8_t)bits};
 p->samples[0].volume=64;p->samples[1]=p->samples[0];
 for(i=0;i<4;++i)p->events[song_track[i]]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
 p->events[0].effect=14;p->events[0].parameter=0x11;
 p->events[2].effect=12;p->events[2].parameter=32;
 p->events[16+15].effect=15;p->events[16+15].parameter=150;
 p->events[16*2+15].effect=14;p->events[16*2+15].parameter=0xe1;
 p->events[16*4]=(struct pt_event){320,0,PT_NOTE_PERIOD,2,0,0,0,0};
 p->events[16*7+15].effect=15;p->events[16*7+15].parameter=0;
 pt_sampler_init(&f->sampler,&f->allocator,1024*1024);
 if(promote)for(i=0;i<2;++i){struct pt_pcm current;
   assert(pt_sampler_pin(&f->sampler,p,i,f->sampler.generation,&current,&s->prepared[i])==PT_EDIT_OK);
   assert(s->prepared[i]==f->sampler.current[i]);
 }
 s->table_before=p->samples;for(i=0;i<2;++i){s->sample_before[i]=p->samples[i];
   s->master_bytes[i]=p->samples[i].pcm.capacity*sizeof(int32_t);s->master_before[i]=malloc(s->master_bytes[i]);
   assert(s->master_before[i]);memcpy(s->master_before[i],p->samples[i].pcm.data,s->master_bytes[i]);
 }
 f->backend.now=100;return s;
}

static unsigned fixture_command_polls,fixture_reader_polls;
static unsigned fixture_submit_mode;
static struct pt_paula_readers_song *fixture_clock_owner;
static struct song_fixture *fixture_clock_source;
static unsigned fixture_clock_action,fixture_clock_reentries;
static int fixture_song_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{struct backend *b=context;*ticks=b->now;*frequency=SONG_FREQUENCY;return 1;}
static int fixture_song_hook_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{
 unsigned action=fixture_clock_action;fixture_clock_action=0;
 if(action==1){struct pt_paula_readers_song_status out,before;memset(&out,0x5a,sizeof(out));before=out;
   assert(pt_paula_readers_song_get(fixture_clock_owner,1,&out)==PT_PAULA_READERS_SONG_BUSY&&!memcmp(&out,&before,sizeof(out)));++fixture_clock_reentries;}
 if(action==2)++fixture_clock_source->genuine.document.project.bpm;
 return fixture_song_clock(context,ticks,frequency);
}
static enum pt_readers_reply fixture_poll_command(void *c,uint64_t t,struct pt_readers_command_receipt *out)
{++fixture_command_polls;return poll_command(c,t,out);}
static enum pt_readers_reply fixture_cancel_command(void *c,uint64_t t,struct pt_readers_command_receipt *out)
{++fixture_command_polls;return cancel_command(c,t,out);}
static enum pt_readers_reply fixture_poll_reader(void *c,const struct pt_readers_domain *d,struct pt_readers_reader_receipt *out)
{++fixture_reader_polls;return poll_reader(c,d,out);}
static enum pt_readers_reply fixture_cancel_reader(void *c,const struct pt_readers_domain *d,struct pt_readers_reader_receipt *out)
{++fixture_reader_polls;return cancel_reader(c,d,out);}
/* Mode1 is an actual callback refusal with no borrowed domains. Mode2 first
 * records genuine acceptance and its exact borrowed references, then reports
 * uncertainty. Existing model polls therefore retain and later prove the real
 * domains, rather than fabricating receipts after an unrecorded submission. */
static int fixture_song_fault_submit(void *context,const struct pt_readers_event *event)
{
 struct backend *b=context;unsigned mode=fixture_submit_mode;int result;fixture_submit_mode=0;
 if(mode==1){++b->submissions;return 0;}
 result=submit(context,event);
 if(mode==2){assert(result==1);b->uncertain=1;return -1;}
 return result;
}

static struct pt_render_options song_options(void)
{
 struct pt_render_options o;memset(&o,0,sizeof(o));o.rate=SONG_RATE;o.bits=24;
 o.tracks=(1U<<0)|(1U<<2)|(1U<<4)|(1U<<6);o.gain_q16=65536;
 o.tick_limit=200;o.frame_limit=200000;return o;
}

/* Fieldwise descriptor comparison avoids relying on uninitialized padding.
 * Complete version-span beforeimages will cover owned master/marker capacity. */
static void song_masters_unchanged(const struct song_fixture *s)
{
 const struct fixture *f=&s->genuine;const struct pt_project *p=&f->document.project;unsigned i;
 assert(p->samples==s->table_before&&!memcmp(f->master.values,s->original_before,sizeof(s->original_before)));
 for(i=0;i<2;++i){const struct pt_sample *a=p->samples+i,*b=s->sample_before+i;
   assert(a->pcm.data==b->pcm.data&&a->pcm.capacity==b->pcm.capacity&&a->pcm.frames==b->pcm.frames&&a->pcm.rate==b->pcm.rate&&a->pcm.channels==b->pcm.channels&&a->pcm.bits==b->pcm.bits);
   assert(a->volume==b->volume&&a->finetune==b->finetune);
   assert(a->loop==b->loop&&a->loop_start==b->loop_start&&a->loop_end==b->loop_end&&a->crossfade==b->crossfade&&a->interpolation==b->interpolation&&a->slices==b->slices&&a->slice_count==b->slice_count&&!memcmp(a->name,b->name,sizeof(a->name)));
   assert(!memcmp(a->pcm.data,s->master_before[i],s->master_bytes[i]));
   if(s->prepared[i])assert(f->sampler.current[i]==s->prepared[i]);
 }
 if(s->padding_data)assert(!memcmp(s->padding_data,s->padding_before,s->padding_bytes)&&p->samples[s->padding_slot].pcm.data==s->padding_data&&p->samples[s->padding_slot].pcm.capacity*sizeof(int32_t)==s->padding_bytes);
}

/* Real owned recording adoption preserves unused capacity in a genuine current
 * version. Padding is then inside the producer's immutable borrowed spans. */
static void song_add_owned_padding(struct song_fixture *s)
{
 struct fixture *f=&s->genuine;struct pt_project *p=&f->document.project;struct pt_pcm pcm;unsigned i;
 assert(!s->history_active&&pt_pattern_history_init(&s->history,p,s->history_command,4,s->history_change,4)==PT_EDIT_OK);
 s->history_active=1;s->padding_slot=p->sample_count;s->padding_bytes=17000*sizeof(int32_t);
 s->padding_data=allocate(&f->fast,s->padding_bytes);assert(s->padding_data);
 for(i=0;i<17000;++i)s->padding_data[i]=i<16384?f->master.values[i]:(int32_t)0x5a6b7c8d;
 pcm=(struct pt_pcm){s->padding_data,17000,16384,8000,1,p->samples[0].pcm.bits};
 assert(pt_sampler_append_owned(&f->sampler,p,&s->history,&pcm,&f->allocator,"unused owned padding")==PT_EDIT_OK&&!pcm.data&&f->sampler.current[s->padding_slot]);
 s->padding_before=malloc(s->padding_bytes);assert(s->padding_before);memcpy(s->padding_before,s->padding_data,s->padding_bytes);s->table_before=p->samples;
}

static void song_fixture_free(struct song_fixture *s)
{
 struct fixture *f=&s->genuine;unsigned i;song_masters_unchanged(s);
 /* Oracle/coordinator domains must have been explicitly drained and closed. */
 assert(!f->pool&&!f->queue);
 for(i=0;i<2;++i)if(s->prepared[i])pt_sampler_unpin(s->prepared[i]);
 for(i=0;i<2;++i)free(s->master_before[i]);
 if(s->history_active)pt_pattern_history_release(&s->history);
 free(s->padding_before);
 pt_sampler_release(&f->sampler);pt_document_release(&f->document);
 assert(!f->fast.live&&!f->chip.live&&!f->fast.bytes&&!f->chip.bytes&&!f->sampler.bytes);free(s);
}

struct song_oracle {
 struct song_fixture *source;struct pt_render_sequence *sequence;
 struct pt_paula_preflight_report report;uint64_t frame;
 struct pt_readers_key keys[PT_CHANNEL_LIMIT],held_key[8];
 struct pt_paula_readers_reader *held[8];unsigned count,ended,controls,boundaries;
};
static struct song_oracle *song_oracle_open(unsigned bits)
{
 struct song_oracle *o=calloc(1,sizeof(*o));struct fixture *f;struct pt_render_options options=song_options();
 struct pt_paula_readers_config c;struct pt_scheduled_grid grid={100,7,SONG_FREQUENCY,SONG_RATE};struct pt_readers_backend b;
 assert(o);o->source=song_fixture_new(bits,1);f=&o->source->genuine;o->frame=SONG_START;
 c=(struct pt_paula_readers_config){2,8,1024*1024,131072,7,&f->chip,chip_allocate,chip_release};
 assert(pt_paula_readers_open(&f->allocator,&f->sampler,&f->document.project,&c,&f->pool)==PT_PAULA_READERS_OK);
 f->backend.pool=f->pool;
 b=(struct pt_readers_backend){&f->backend,sizeof(f->backend),{7,8,4},PT_READERS_VERSION,3,8,fixture_song_clock,submit,poll_command,cancel_command,poll_reader,cancel_reader};
 assert(pt_readers_open(&f->allocator,&grid,1001,&b,2,8,&f->queue)==PT_SCHEDULED_OK);
 assert(pt_paula_preflight_take(&f->document.project,&options,NULL,&song_caps,1,&f->allocator,&o->report,&o->sequence)==PT_PAULA_COMPATIBLE&&o->sequence);
 return o;
}
static void song_oracle_ready(struct pt_paula_readers_command *c)
{
 unsigned ready=0,n=0;enum pt_paula_readers_result r;
 do{r=pt_paula_readers_step(c,&ready);assert(++n<4096&&(r==PT_PAULA_READERS_PENDING||r==PT_PAULA_READERS_OK));}while(!ready);
}
/* This reads the genuine lowerer's normalized output; it performs no folding,
 * renderer interpretation, period rounding or packed-sample conversion itself. */
static struct pt_scheduled_batch song_oracle_batch(struct pt_paula_readers_command *c)
{
 struct pt_scheduled_batch b;unsigned i;memset(&b,0,sizeof(b));b.frame=c->frame;b.generation=7;b.count=c->count;
 for(i=0;i<c->count;++i){struct command_source *s=c->source+i;struct pt_scheduled_action *a=b.action+i;
   a->kind=s->request.kind;a->slot=(unsigned)c->pool->map[s->request.track];a->period=c->lower[i].period;a->volume=c->lower[i].volume;
   if(a->kind==PT_SCHEDULED_TRIGGER){struct pt_paula_readers_view v;assert(pt_paula_readers_view(c,i,&v)==PT_PAULA_READERS_OK);a->data=v.data+c->lower[i].offset;a->words=(uint16_t)(c->lower[i].length/2);}
 }return b;
}
static unsigned song_oracle_next(struct song_oracle *o,struct pt_scheduled_batch *out)
{
 struct fixture *f=&o->source->genuine;struct pt_render_interval span;struct pt_render_lookahead ahead;
 struct pt_render_plan plan;struct pt_paula_readers_command *c=NULL;unsigned ready=0,n=0,i;uint32_t frames;uint64_t ticket;
 assert(!o->ended);memset(&ahead,0,sizeof(ahead));
 assert(pt_render_sequence_next(o->sequence,&span)==PT_RENDER_OK&&span.emit);
 assert(pt_render_lookahead_begin(&ahead,o->sequence)==PT_RENDER_OK);
 do{assert(pt_render_lookahead_step(&ahead,256,&plan,&ready)==PT_RENDER_OK&&++n<4096);}while(!ready);
 assert(o->frame<=UINT64_MAX-span.frames);o->frame+=span.frames;++o->boundaries;
 assert(pt_paula_readers_lower_begin(f->pool,f->queue,o->frame,SONG_RATE,&plan,&song_caps,o->keys,&c)==PT_PAULA_READERS_OK);
 if(c){song_oracle_ready(c);*out=song_oracle_batch(c);assert(pt_paula_readers_lower_enqueue(c,&ticket)==PT_SCHEDULED_OK);
   observe_fire(f,ticket);
   for(i=0;i<c->count;++i){struct command_source *s=c->source+i;
     if(s->request.kind==PT_SCHEDULED_TRIGGER){assert(o->count<8);o->held[o->count]=reader_handle(c,i);o->held_key[o->count]=active_key(o->held[o->count]);o->keys[s->request.track]=o->held_key[o->count++];}
     else if(s->request.kind==PT_SCHEDULED_CONTROL)++o->controls;
   }
   detach_collect(f,ticket);assert(pt_paula_readers_command_close(c));
 }else{memset(out,0,sizeof(*out));out->generation=7;out->frame=o->frame;}
 frames=span.frames;while(frames){unsigned work=frames>256?256:frames;assert(pt_render_sequence_consume(o->sequence,work)==PT_RENDER_OK);frames-=work;}
 assert(pt_render_lookahead_commit(&ahead)==PT_RENDER_OK);o->ended=span.end;song_masters_unchanged(o->source);return out->count;
}
static struct pt_scheduled_batch song_oracle_next_nonempty(struct song_oracle *o)
{struct pt_scheduled_batch b;while(!song_oracle_next(o,&b)){assert(!o->ended);}return b;}
static void song_batch_same(const struct pt_scheduled_batch *actual,const struct pt_scheduled_batch *expected)
{
 unsigned i;assert(actual->frame==expected->frame&&actual->generation==expected->generation&&actual->count==expected->count);
 for(i=0;i<actual->count;++i){const struct pt_scheduled_action *a=actual->action+i,*b=expected->action+i;
   assert(a->kind==b->kind&&a->slot==b->slot&&a->period==b->period&&a->volume==b->volume&&a->words==b->words);
   if(a->kind==PT_SCHEDULED_TRIGGER)assert(a->data&&b->data&&!memcmp(a->data,b->data,(size_t)a->words*2));
   else assert(!a->data&&!a->words);
 }
}
static void song_oracle_close(struct song_oracle *o)
{
 struct fixture *f=&o->source->genuine;struct pt_render_plan stop;struct pt_paula_readers_command *c=NULL;uint64_t ticket;unsigned i;
 assert(o->ended&&o->controls>80&&o->frame-SONG_START==o->report.frames);
 memset(&stop,0,sizeof(stop));stop.count=4;for(i=0;i<4;++i){stop.action[i].kind=PT_RENDER_STOP;stop.action[i].channel=song_track[i];}
 assert(pt_paula_readers_lower_begin(f->pool,f->queue,o->frame,SONG_RATE,&stop,&song_caps,o->keys,&c)==PT_PAULA_READERS_OK&&c);
 song_oracle_ready(c);assert(pt_paula_readers_lower_enqueue(c,&ticket)==PT_SCHEDULED_OK);observe_fire(f,ticket);detach_collect(f,ticket);assert(pt_paula_readers_command_close(c));
 for(i=0;i<o->count;++i){retire_collect(f,o->held_key[i]);assert(pt_paula_readers_reader_close(o->held[i]));}
 pt_render_sequence_close(o->sequence);assert(pt_readers_close(f->queue)&&pt_paula_readers_close(f->pool));f->queue=NULL;f->pool=NULL;
 song_fixture_free(o->source);free(o);
}

static struct pt_paula_readers_song_config fixture_song_config(struct song_fixture *source)
{
 struct fixture *f=&source->genuine;struct pt_paula_readers_song_config c;memset(&c,0,sizeof(c));
 c.render=song_options();c.caps=song_caps;
 c.readers=(struct pt_paula_readers_config){2,8,1024*1024,131072,7,&f->chip,chip_allocate,chip_release};
 c.grid=(struct pt_scheduled_grid){100,7,SONG_FREQUENCY,SONG_RATE};
 c.backend=(struct pt_readers_backend){&f->backend,sizeof(f->backend),{7,8,4},PT_READERS_VERSION,3,8,fixture_song_hook_clock,fixture_song_fault_submit,fixture_poll_command,fixture_cancel_command,fixture_poll_reader,fixture_cancel_reader};
 c.session=1001;c.absolute_start=SONG_START;c.control_budget=4*1024*1024;return c;
}

/* Reuse every real v5 oracle/backend case below through caller-owned scratch.
 * The original fixture remains a separate, unchanged legacy-begin test group.
 * Dirty scratch is not a completion certificate; the genuine begin populates it.
 * Freeing it before any step lets the sanitizer detect escaped references. */
struct fixture_workspace {uint8_t *allocation,*data;size_t capacity,allocated;};
static unsigned fixture_workspace_begins,fixture_workspace_successes;
static struct fixture_workspace fixture_workspace_new(size_t extra)
{
 struct fixture_workspace w;size_t size=pt_paula_readers_song_begin_workspace_size();
 size_t alignment=pt_paula_readers_song_begin_workspace_alignment(),skip;
 assert(size&&alignment&&size<=SIZE_MAX-extra);
 w.capacity=size+extra;assert(w.capacity<=SIZE_MAX-alignment-pt_paula_readers_song_control_size());
 w.allocated=w.capacity+alignment+pt_paula_readers_song_control_size();
 w.allocation=malloc(w.allocated);assert(w.allocation);
 skip=(alignment-(uintptr_t)w.allocation%alignment)%alignment;w.data=w.allocation+skip;
 assert((uintptr_t)w.data%alignment==0&&w.capacity<=w.allocated-skip);
 memset(w.allocation,0xc7,w.allocated);return w;
}
static void fixture_workspace_discard(struct fixture_workspace *w)
{memset(w->allocation,0xe7,w->allocated);free(w->allocation);memset(w,0,sizeof(*w));}
static enum pt_paula_readers_song_result fixture_workspace_begin(const struct pt_allocator *a,
 struct pt_sampler *sampler,struct pt_project *project,const struct pt_paula_readers_song_config *config,
 uint32_t revision,struct pt_paula_readers_song **out)
{
 struct fixture_workspace w=fixture_workspace_new(4096);enum pt_paula_readers_song_result r;unsigned i;
 ++fixture_workspace_begins;
 r=pt_paula_readers_song_begin_in_workspace(a,sampler,project,config,revision,w.data,w.capacity,out);
 if(r==PT_PAULA_READERS_SONG_PENDING){assert(*out);++fixture_workspace_successes;
   /* Secondary source-structure observation, not a replacement readiness proof. */
   assert(!(*out)->begin_control_count);
   for(i=0;i<5;++i)assert(!(*out)->begin_control[i].data&&!(*out)->begin_control[i].bytes);
 }
 fixture_workspace_discard(&w);return r;
}

static struct pt_paula_readers_song *fixture_song_begin(struct song_fixture *source)
{
 struct fixture *f=&source->genuine;struct pt_paula_readers_song *s=NULL;
 struct pt_paula_readers_song_config c=fixture_song_config(source);
 fixture_transferred_sequence=NULL;
 assert(fixture_workspace_begin(&f->allocator,&f->sampler,&f->document.project,&c,1,&s)==PT_PAULA_READERS_SONG_PENDING&&s);
 assert(!f->backend.submissions&&!f->backend.effects&&!f->chip.calls);return s;
}
static struct pt_paula_readers_song_status fixture_song_get(struct pt_paula_readers_song *s)
{
 struct pt_paula_readers_song_status out;enum pt_paula_readers_song_result r;
 memset(&out,0xa5,sizeof(out));r=pt_paula_readers_song_get(s,1,&out);
 assert(r==PT_PAULA_READERS_SONG_PENDING||r==PT_PAULA_READERS_SONG_DONE||r==PT_PAULA_READERS_SONG_OK||r==PT_PAULA_READERS_SONG_WAIT_ACTIVE||r==PT_PAULA_READERS_SONG_WAIT_PRESSURE);
 assert(out.result==r);return out;
}
static enum pt_paula_readers_song_result fixture_song_step(struct song_fixture *source,struct pt_paula_readers_song *s)
{
 struct fixture *f=&source->genuine;struct pt_paula_readers_song_status out;
 unsigned submissions=f->backend.submissions,effects=f->backend.effects,cp=fixture_command_polls,rp=fixture_reader_polls;
 enum pt_paula_readers_song_result r=pt_paula_readers_song_step(s,1,256,&out);
 assert(r==PT_PAULA_READERS_SONG_PENDING||r==PT_PAULA_READERS_SONG_DONE||r==PT_PAULA_READERS_SONG_WAIT_ACTIVE||r==PT_PAULA_READERS_SONG_WAIT_PRESSURE);
 assert(f->backend.submissions==submissions&&f->backend.effects==effects&&out.result==r&&fixture_command_polls==cp&&fixture_reader_polls==rp);
 f->pool=s->pool;f->queue=s->queue;f->backend.pool=s->pool;song_masters_unchanged(source);return r;
}
static unsigned fixture_command_index(const struct pt_paula_readers_song *s,uint64_t ticket)
{unsigned i;for(i=0;i<PT_PAULA_READERS_SONG_COMMANDS;++i)if(s->command[i].holder&&s->command[i].ticket==ticket)return i;assert(0);return 0;}
static unsigned fixture_reader_index(const struct pt_paula_readers_song *s,const struct pt_readers_key *key)
{unsigned i;for(i=0;i<PT_PAULA_READERS_SONG_READERS;++i)if(s->reader[i].holder&&s->reader[i].holder->token==key->owner&&s->reader[i].trigger==key->trigger&&s->reader[i].action==key->action)return i;assert(0);return 0;}
static void fixture_detach(struct song_fixture *source,struct pt_paula_readers_song *s,uint64_t ticket)
{
 struct fixture *f=&source->genuine;struct pt_readers_command_receipt out;unsigned i=fixture_command_index(s,ticket);
 detach(&f->backend,ticket);assert(pt_paula_readers_song_service_command(s,i,0,&out)==PT_SCHEDULED_OK);
 assert(!s->command[i].holder);song_masters_unchanged(source);
}
static void fixture_fire(struct song_fixture *source,struct pt_paula_readers_song *s,uint64_t ticket)
{
 struct fixture *f=&source->genuine;struct pt_readers_command_receipt out;unsigned i=fixture_command_index(s,ticket);
 unsigned fast=f->fast.calls,chip=f->chip.calls;f->backend.now=mc(&f->backend,ticket)->event.scheduled.first;fire(&f->backend,ticket);
 assert(f->fast.calls==fast&&f->chip.calls==chip); /* Actual host activation allocates/converts nothing. */
 assert(pt_paula_readers_song_service_command(s,i,0,&out)==PT_SCHEDULED_PENDING);
 assert(out.origin==PT_READERS_BACKEND_ACTUAL&&out.ticket==ticket);
}
static void fixture_wait_active(struct song_fixture *source,struct pt_paula_readers_song *s)
{
 unsigned n=0,submissions=source->genuine.backend.submissions;uint64_t frame;
 while(fixture_song_step(source,s)!=PT_PAULA_READERS_SONG_WAIT_ACTIVE)assert(++n<4096);
 frame=fixture_song_get(s).boundary_frame;
 for(n=0;n<4;++n){assert(fixture_song_step(source,s)==PT_PAULA_READERS_SONG_WAIT_ACTIVE);assert(fixture_song_get(s).boundary_frame==frame);}
 assert(source->genuine.backend.submissions==submissions);
}
static void fixture_retire(struct song_fixture *source,struct pt_paula_readers_song *s,const struct pt_readers_key *key)
{
 struct pt_readers_reader_receipt out;unsigned i=fixture_reader_index(s,key);
 retired(&source->genuine.backend,key);assert(pt_paula_readers_song_service_reader(s,i,0,&out)==PT_SCHEDULED_OK);
 assert(same_key(&out.key,key)&&out.state==PT_READERS_RETIRED);
}
static void fixture_finish_owner(struct song_fixture *source,struct pt_paula_readers_song **s)
{
 assert(pt_paula_readers_song_close(s)&&!*s);source->genuine.pool=NULL;source->genuine.queue=NULL;
 song_fixture_free(source);
}
static void song_whole_exact(unsigned bits)
{
 struct song_fixture *source=song_fixture_new(bits,1);struct fixture *f=&source->genuine;
 struct song_oracle *oracle=song_oracle_open(bits);struct pt_paula_readers_song *s=fixture_song_begin(source);
 struct pt_paula_readers_song_status status;
 uint64_t held_first=0,held_second=0;unsigned batches=0,control_batches=0,steps=0,pressure=0,replacement=0,i;
 while(!(status=fixture_song_get(s)).done){
   enum pt_paula_readers_song_result r;
   if(status.phase!=PT_PAULA_READERS_SONG_PUBLISH){
     unsigned initial=status.phase==PT_PAULA_READERS_SONG_INITIAL||status.phase==PT_PAULA_READERS_SONG_RETAIN||status.phase==PT_PAULA_READERS_SONG_AUDIT||status.phase==PT_PAULA_READERS_SONG_POOL_VALIDATE||status.phase==PT_PAULA_READERS_SONG_OUTPUT_SETUP;
     r=fixture_song_step(source,s);assert(++steps<10000&&r!=PT_PAULA_READERS_SONG_WAIT_ACTIVE&&r!=PT_PAULA_READERS_SONG_WAIT_PRESSURE);
     if(initial)assert(!f->backend.submissions&&!f->chip.calls);
     if(s->sequence)assert(s->sequence==fixture_transferred_sequence&&fixture_transferred_sequence);
     continue;
   }
   {
     struct pt_scheduled_batch expected=song_oracle_next_nonempty(oracle);struct model_command *m;
     unsigned before=f->backend.commands,has_trigger=0,has_control=0;uint64_t ticket;
     if(!batches){struct pt_readers_reader_receipt out,untouched;unsigned rp=fixture_reader_polls;
       memset(&out,0x5a,sizeof(out));untouched=out;
       assert(pt_paula_readers_song_service_reader(s,0,1,&out)==PT_SCHEDULED_INVALID&&!memcmp(&out,&untouched,sizeof(out))&&fixture_reader_polls==rp);
       assert(s->prospective[0]==1&&!s->closed[0]); /* Invalid reserved cancellation cannot close admission. */
     }
     assert(pt_paula_readers_song_publish_next(s,1)==PT_SCHEDULED_OK&&f->backend.commands==before+1);
     m=f->backend.command+before;ticket=m->event.scheduled.ticket;assert(m->event.scheduled.batch.frame==status.boundary_frame);
     song_batch_same(&m->event.scheduled.batch,&expected);++batches;
     for(i=0;i<m->event.scheduled.batch.count;++i){has_trigger|=m->event.scheduled.batch.action[i].kind==PT_SCHEDULED_TRIGGER;has_control|=m->event.scheduled.batch.action[i].kind==PT_SCHEDULED_CONTROL;}
     control_batches+=!has_trigger&&has_control;
     if(batches==1){struct pt_readers_key key,before_key;struct pt_paula_readers_reader *reader=s->reader[0].holder;
       assert(m->event.scheduled.batch.count==4&&has_trigger&&fixture_song_get(s).reader_mask==15);
       memset(&key,0x5a,sizeof(key));before_key=key;
       assert(pt_paula_readers_reader_key(reader,&key)==PT_SCHEDULED_STALE&&!memcmp(&key,&before_key,sizeof(key)));
       fixture_wait_active(source,s);held_first=ticket;
     }else if(has_trigger){struct pt_readers_key old_key=f->backend.slot[0],actual,untouched;
       unsigned old_index=fixture_reader_index(s,&old_key);
       memset(&actual,0x5a,sizeof(actual));untouched=actual;
       /* Future replacement closes old-origin admission even while the actual
        * backend still plays the old original; it cannot be a fallback key. */
       assert(pt_paula_readers_reader_key(s->reader[old_index].holder,&actual)==PT_SCHEDULED_STALE&&!memcmp(&actual,&untouched,sizeof(actual)));
       fixture_wait_active(source,s);assert(f->backend.active[0]&&same_key(&f->backend.slot[0],&old_key));++replacement;
     }
     fixture_fire(source,s,ticket);
     if(batches>1&&has_trigger){struct pt_readers_reader_receipt receipt;struct pt_readers_key old_key=f->backend.reader[0].key;
       unsigned old_index=fixture_reader_index(s,&old_key);
       assert(pt_paula_readers_song_service_reader(s,old_index,1,&receipt)==PT_SCHEDULED_PENDING);
       assert(s->prospective[0]!=old_index+1&&!s->closed[0]); /* Cancelling old origin cannot close replacement admission. */
     }
     for(i=0;i<m->event.scheduled.batch.count;++i){struct pt_readers_key actual;const struct pt_readers_key *key=&m->event.reader[i]->key;
       if(m->event.scheduled.batch.action[i].kind==PT_SCHEDULED_TRIGGER){unsigned index=fixture_reader_index(s,key);
         assert(pt_paula_readers_reader_key(s->reader[index].holder,&actual)==PT_SCHEDULED_OK&&same_key(&actual,key));}
       else assert(same_key(&f->backend.slot[key->slot],key));
     }
     if(batches==2){uint64_t boundary;unsigned n=0,pins_before=pins(f);
       held_second=ticket;while(fixture_song_step(source,s)!=PT_PAULA_READERS_SONG_WAIT_PRESSURE)assert(++n<4096);
       status=fixture_song_get(s);boundary=status.boundary_frame;assert(status.command_mask==3);
       for(n=0;n<4;++n){assert(fixture_song_step(source,s)==PT_PAULA_READERS_SONG_WAIT_PRESSURE);assert(fixture_song_get(s).boundary_frame==boundary&&pins(f)==pins_before);}
       fixture_detach(source,s,held_first);held_first=0;assert(pins(f)==pins_before);++pressure;
     }else if(batches!=1){
       if(held_second){fixture_detach(source,s,held_second);held_second=0;}
       fixture_detach(source,s,ticket);
     }
   }
 }
 assert(!held_first&&!held_second&&control_batches>20&&pressure==1&&replacement==1&&status.reader_mask==31&&!status.command_mask);
 assert(status.terminal_frame==SONG_START+oracle->report.frames&&status.boundary_frame==status.terminal_frame&&!status.terminal_stop_requested&&pins(f)==5);
 {struct pt_scheduled_batch empty;unsigned submissions=f->backend.submissions;
   assert(!song_oracle_next(oracle,&empty)&&oracle->ended&&empty.frame==status.terminal_frame);
   for(i=0;i<3;++i)assert(fixture_song_step(source,s)==PT_PAULA_READERS_SONG_DONE);
   assert(f->backend.submissions==submissions&&pins(f)==5&&!pt_paula_readers_song_close(&s));
 }
 assert(pt_paula_readers_song_terminal_stop(s,1)==PT_PAULA_READERS_SONG_PENDING);
 while(fixture_song_get(s).phase!=PT_PAULA_READERS_SONG_PUBLISH){assert(fixture_song_step(source,s)==PT_PAULA_READERS_SONG_PENDING&&++steps<20000);}
 {unsigned before=f->backend.commands;struct model_command *m;uint64_t ticket;struct pt_readers_key key;
   assert(pt_paula_readers_song_publish_next(s,1)==PT_SCHEDULED_OK&&f->backend.commands==before+1);m=f->backend.command+before;ticket=m->event.scheduled.ticket;
   assert(m->event.scheduled.batch.frame==status.terminal_frame&&m->event.scheduled.batch.count==4);
   for(i=0;i<4;++i)assert(m->event.scheduled.batch.action[i].kind==PT_SCHEDULED_STOP&&!m->event.scheduled.batch.action[i].period&&!m->event.scheduled.batch.action[i].volume);
   fixture_fire(source,s,ticket);assert(pins(f)==5);
   key=f->backend.reader[4].key;fixture_retire(source,s,&key);assert(pins(f)==5&&s->reader[4].holder);
   /* Old superseded original has no STOP reference; independent retirement may close it. */
   key=f->backend.reader[0].key;fixture_retire(source,s,&key);assert(pins(f)==4&&!s->reader[0].holder);
   fixture_detach(source,s,ticket);assert(pins(f)==3&&!s->reader[4].holder);
   for(i=1;i<4;++i){key=f->backend.reader[i].key;fixture_retire(source,s,&key);}
 }
 assert(!fixture_song_get(s).reader_mask&&!pins(f));song_oracle_close(oracle);fixture_finish_owner(source,&s);
}

static void fixture_cancel_drain(struct song_fixture *source,struct pt_paula_readers_song **owner)
{
 struct pt_paula_readers_song *s=*owner;struct fixture *f=&source->genuine;unsigned i,j,effects=f->backend.effects,cp=fixture_command_polls,rp=fixture_reader_polls;
 assert(pt_paula_readers_song_cancel(s)==PT_PAULA_READERS_SONG_PENDING&&fixture_command_polls==cp&&fixture_reader_polls==rp);
 assert(pt_paula_readers_song_cancel(s)==PT_PAULA_READERS_SONG_PENDING&&fixture_command_polls==cp&&fixture_reader_polls==rp);
 for(i=0;i<PT_PAULA_READERS_SONG_COMMANDS;++i)if(s->command[i].holder&&s->command[i].ticket){
   uint64_t ticket=s->command[i].ticket;for(j=0;j<f->backend.commands;++j)if(f->backend.command[j].event.scheduled.ticket==ticket)break;
   if(j<f->backend.commands)detach(&f->backend,ticket);
   {enum pt_scheduled_result r;cp=fixture_command_polls;r=pt_paula_readers_song_service_command(s,i,1,NULL);
     assert((r==PT_SCHEDULED_OK||r==PT_SCHEDULED_BACKEND)&&fixture_command_polls-cp<=1&&!s->command[i].holder);}
 }
 for(i=0;i<PT_PAULA_READERS_SONG_READERS;++i)if(s->reader[i].holder){
   struct pt_readers_key key;
   for(j=0;j<f->backend.readers;++j)if(f->backend.reader[j].key.owner==s->reader[i].holder->token&&f->backend.reader[j].key.trigger==s->reader[i].trigger)break;
   assert(j<f->backend.readers);key=f->backend.reader[j].key;retired(&f->backend,&key);
   {enum pt_scheduled_result r;rp=fixture_reader_polls;r=pt_paula_readers_song_service_reader(s,i,1,NULL);
     assert((r==PT_SCHEDULED_OK||r==PT_SCHEDULED_BACKEND)&&fixture_reader_polls-rp<=1&&!s->reader[i].holder);}
 }
 assert(f->backend.effects==effects);fixture_finish_owner(source,owner);
}
static void song_cancel_phases(void)
{
 static const enum pt_paula_readers_song_phase phases[]={PT_PAULA_READERS_SONG_INITIAL,PT_PAULA_READERS_SONG_RETAIN,PT_PAULA_READERS_SONG_AUDIT,PT_PAULA_READERS_SONG_POOL_VALIDATE,PT_PAULA_READERS_SONG_OUTPUT_SETUP,PT_PAULA_READERS_SONG_NEXT,PT_PAULA_READERS_SONG_FORECAST,PT_PAULA_READERS_SONG_KEYS,PT_PAULA_READERS_SONG_LOWER,PT_PAULA_READERS_SONG_PREPARE,PT_PAULA_READERS_SONG_ENQUEUE,PT_PAULA_READERS_SONG_PUBLISH};
 unsigned i;
 for(i=0;i<sizeof(phases)/sizeof(phases[0]);++i){struct song_fixture *f=song_fixture_new(24,1);struct pt_paula_readers_song *s=fixture_song_begin(f);unsigned n=0;
   while(fixture_song_get(s).phase!=phases[i]){assert(fixture_song_step(f,s)==PT_PAULA_READERS_SONG_PENDING&&++n<10000);}
   assert(!f->genuine.backend.submissions&&!f->genuine.backend.effects);fixture_cancel_drain(f,&s);
 }
 /* Observe and cancel all five actual inner setup phases, not invented states. */
 for(i=PT_RENDER_SETUP_VALIDATE;i<=PT_RENDER_SETUP_COMPLETE;++i){struct song_fixture *f=song_fixture_new(16,1);struct pt_paula_readers_song *s=fixture_song_begin(f);struct pt_render_setup_report report;unsigned n=0;
   for(;;){enum pt_render_setup_result r=pt_paula_preflight_setup_get(s->startup,1,f->genuine.sampler.generation,&report);
     assert(r==PT_RENDER_SETUP_PENDING||r==PT_RENDER_SETUP_READY);if((unsigned)report.phase==i)break;
     assert(pt_paula_readers_song_step(s,1,4096,NULL)==PT_PAULA_READERS_SONG_PENDING&&++n<10000);
   }
   fixture_cancel_drain(f,&s);
 }
}
static void song_submitted_cancel(unsigned adopted)
{
 struct song_fixture *source=song_fixture_new(16,1);struct fixture *f=&source->genuine;struct pt_paula_readers_song *s=fixture_song_begin(source);unsigned n=0;
 while(fixture_song_get(s).phase!=PT_PAULA_READERS_SONG_PUBLISH){assert(fixture_song_step(source,s)==PT_PAULA_READERS_SONG_PENDING&&++n<10000);}
 assert(pt_paula_readers_song_publish_next(s,1)==PT_SCHEDULED_OK&&f->backend.commands==1);
 if(adopted)fixture_fire(source,s,f->backend.command[0].event.scheduled.ticket);
 assert(!pt_paula_readers_song_close(&s));fixture_cancel_drain(source,&s);
}
static void song_eight_reader_pressure(void)
{
 struct song_fixture *source=song_fixture_new(24,1);struct fixture *f=&source->genuine;struct pt_paula_readers_song *s;unsigned i,n=0,pressure=0;
 uint64_t frame=0;struct pt_readers_key replacement[4];
 for(i=0;i<4;++i){f->document.project.events[16*4+song_track[i]]=(struct pt_event){320,0,PT_NOTE_PERIOD,2,0,0,0,0};f->document.project.events[16*6+song_track[i]]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};}
 s=fixture_song_begin(source);
 for(;;){struct pt_paula_readers_song_status status=fixture_song_get(s);enum pt_paula_readers_song_result r;
   if(status.phase==PT_PAULA_READERS_SONG_PUBLISH){unsigned before=f->backend.commands;uint64_t ticket;
     assert(pt_paula_readers_song_publish_next(s,1)==PT_SCHEDULED_OK&&f->backend.commands==before+1);
     ticket=f->backend.command[before].event.scheduled.ticket;fixture_fire(source,s,ticket);fixture_detach(source,s,ticket);continue;
   }
   r=fixture_song_step(source,s);assert(++n<10000);
   if(r==PT_PAULA_READERS_SONG_WAIT_PRESSURE){unsigned calls=f->fast.calls,chip=f->chip.calls;
     status=fixture_song_get(s);frame=status.boundary_frame;assert(status.reader_mask==255&&!status.command_mask&&pins(f)==8&&f->backend.readers==8);
     for(i=0;i<4;++i){replacement[i]=f->backend.slot[i];assert(f->backend.active[i]);}
     for(i=0;i<4;++i)assert(fixture_song_step(source,s)==PT_PAULA_READERS_SONG_WAIT_PRESSURE&&fixture_song_get(s).boundary_frame==frame&&f->fast.calls==calls&&f->chip.calls==chip);
     for(i=0;i<4;++i){struct pt_readers_key old=f->backend.reader[i].key;fixture_retire(source,s,&old);assert(same_key(&f->backend.slot[i],replacement+i)&&f->backend.active[i]);}
     assert(pins(f)==4);pressure=1;break;
   }
   assert(r==PT_PAULA_READERS_SONG_PENDING);
 }
 while(fixture_song_get(s).phase!=PT_PAULA_READERS_SONG_PUBLISH){assert(fixture_song_step(source,s)==PT_PAULA_READERS_SONG_PENDING&&fixture_song_get(s).boundary_frame==frame&&++n<20000);}
 {unsigned before=f->backend.commands;uint64_t ticket;
   assert(pt_paula_readers_song_publish_next(s,1)==PT_SCHEDULED_OK&&f->backend.commands==before+1);
   ticket=f->backend.command[before].event.scheduled.ticket;assert(f->backend.command[before].event.scheduled.batch.frame==frame&&f->backend.command[before].event.scheduled.batch.count==4);
   fixture_fire(source,s,ticket);fixture_detach(source,s,ticket);
 }
 assert(pressure&&pins(f)==8&&f->backend.readers==12);fixture_cancel_drain(source,&s);
}
static void song_clock_callback_refusals(void)
{
 unsigned action;
 for(action=1;action<=2;++action){struct song_fixture *source=song_fixture_new(24,1);struct fixture *f=&source->genuine;struct pt_paula_readers_song *s=fixture_song_begin(source);unsigned n=0,submissions,effects;enum pt_scheduled_result r;
   while(fixture_song_get(s).phase!=PT_PAULA_READERS_SONG_PUBLISH){assert(fixture_song_step(source,s)==PT_PAULA_READERS_SONG_PENDING&&++n<10000);}
   fixture_clock_owner=s;fixture_clock_source=source;fixture_clock_action=action;submissions=f->backend.submissions;effects=f->backend.effects;
   r=pt_paula_readers_song_publish_next(s,1);
   assert(r==PT_SCHEDULED_BACKEND||r==PT_SCHEDULED_STALE||r==PT_SCHEDULED_CLOCK);
   assert(!fixture_clock_action&&f->backend.submissions==submissions&&f->backend.effects==effects&&!f->backend.commands);
   assert(pt_paula_readers_song_publish_next(s,1)==PT_SCHEDULED_INVALID);
   fixture_clock_owner=NULL;fixture_clock_source=NULL;fixture_cancel_drain(source,&s);
 }
 assert(fixture_clock_reentries==1);
}
static void song_stale_and_missing(void)
{
 unsigned variant;
 {struct song_fixture *source=song_fixture_new(24,0);struct fixture *f=&source->genuine;struct pt_paula_readers_song_config c=fixture_song_config(source);struct pt_paula_readers_song *s=NULL;unsigned calls=f->fast.calls;
   assert(fixture_workspace_begin(&f->allocator,&f->sampler,&f->document.project,&c,1,&s)==PT_PAULA_READERS_SONG_INVALID&&!s&&f->fast.calls==calls&&!f->sampler.current[0]&&!f->sampler.current[1]);song_fixture_free(source);
 }
 for(variant=0;variant<3;++variant){struct song_fixture *source=song_fixture_new(16,1);struct fixture *f=&source->genuine;struct pt_paula_readers_song *s=fixture_song_begin(source);struct pt_paula_readers_song_status out,before;
   memset(&out,0x6b,sizeof(out));before=out;
   if(variant==1)++f->sampler.generation;
   if(variant==2){release(&f->fast,f->document.storage.orders);f->document.storage.orders=NULL;f->document.project.orders=NULL;}
   assert(pt_paula_readers_song_step(s,variant?1:2,256,&out)==PT_PAULA_READERS_SONG_STALE&&!memcmp(&out,&before,sizeof(out)));
   fixture_cancel_drain(source,&s);
 }
}

static void song_refresh_small_masters(struct song_fixture *source)
{
 struct fixture *f=&source->genuine;unsigned i;
 for(i=0;i<2;++i){struct pt_pcm pcm;f->document.project.samples[i].pcm.frames=2;
   assert(pt_sampler_pin(&f->sampler,&f->document.project,i,f->sampler.generation,&pcm,&source->prepared[i])==PT_EDIT_OK);
   source->sample_before[i]=f->document.project.samples[i];free(source->master_before[i]);source->master_bytes[i]=pcm.capacity*sizeof(int32_t);
   source->master_before[i]=malloc(source->master_bytes[i]);assert(source->master_before[i]);memcpy(source->master_before[i],pcm.data,source->master_bytes[i]);
 }
}
static void song_empty_with_two_held(void)
{
 struct song_fixture *source=song_fixture_new(8,0);struct fixture *f=&source->genuine;struct pt_paula_readers_song *s;unsigned i,n=0,batches=0,empty_boundaries=0;
 song_refresh_small_masters(source);memset(f->document.project.events+16*2,0,16*6*sizeof(struct pt_event));
 /* A natural one-shot end emits no renderer STOP. Real row-one NOTE_OFF
  * commands supply the second retained domain, followed by empty intervals. */
 for(i=0;i<4;++i)f->document.project.events[16+song_track[i]].kind=PT_NOTE_OFF;
 f->document.project.events[16*2+15].effect=15;f->document.project.events[16*2+15].parameter=0;s=fixture_song_begin(source);
 for(;;){struct pt_paula_readers_song_status status=fixture_song_get(s);enum pt_paula_readers_song_result r;
   if(status.done){assert(status.command_mask==3&&status.reader_mask==15&&status.intervals>2&&empty_boundaries);break;}
   if(status.phase==PT_PAULA_READERS_SONG_PUBLISH){unsigned before=f->backend.commands;uint64_t ticket;
     assert(pt_paula_readers_song_publish_next(s,1)==PT_SCHEDULED_OK&&f->backend.commands==before+1);
     ticket=f->backend.command[before].event.scheduled.ticket;fixture_fire(source,s,ticket);assert(++batches<=2);continue;
   }
   r=fixture_song_step(source,s);assert((r==PT_PAULA_READERS_SONG_PENDING||r==PT_PAULA_READERS_SONG_DONE)&&++n<10000);
   if(batches==2&&status.phase==PT_PAULA_READERS_SONG_LOWER&&s->phase==PT_PAULA_READERS_SONG_NEXT){assert(!s->plan.count&&fixture_song_get(s).command_mask==3);++empty_boundaries;}
 }
 assert(batches==2&&f->backend.effects==8&&pins(f)==4);fixture_cancel_drain(source,&s);
}
static void *fixture_alias_pointer;static unsigned fixture_alias_at,fixture_alias_returns;
static void *fixture_alias_allocate(void *context,size_t bytes)
{
 struct memory *m=context;if(fixture_alias_pointer&&m->calls+1==fixture_alias_at){++m->calls;++fixture_alias_returns;return fixture_alias_pointer;}
 return allocate(context,bytes);
}
static void song_control_and_capacity_aliases(void)
{
 struct song_fixture *source=song_fixture_new(24,1);struct fixture *f=&source->genuine;
 struct pt_paula_readers_song_config c=fixture_song_config(source),before=c;struct pt_paula_readers_song *s=NULL;
 struct pt_allocator aliased={&f->fast,fixture_alias_allocate,release};unsigned live=f->fast.live,calls=f->fast.calls;struct image owner;
 fixture_alias_pointer=&c;fixture_alias_at=calls+1;fixture_alias_returns=0;
 assert(fixture_workspace_begin(&aliased,&f->sampler,&f->document.project,&c,1,&s)==PT_PAULA_READERS_SONG_INVALID&&!s&&fixture_alias_returns==1&&f->fast.live==live&&!memcmp(&c,&before,sizeof(c)));
 fixture_alias_pointer=NULL;song_add_owned_padding(source);s=fixture_song_begin(source);owner=image(&f->fast,s);
 assert(pt_paula_readers_song_get(s,1,(struct pt_paula_readers_song_status *)((uint8_t *)s+allocated(&f->fast,s)-sizeof(struct pt_paula_readers_song_status)))==PT_PAULA_READERS_SONG_INVALID);unchanged(&owner);
 owner=image(&f->fast,s);assert(pt_paula_readers_song_step(s,1,256,(struct pt_paula_readers_song_status *)(source->padding_data+16384))==PT_PAULA_READERS_SONG_INVALID);unchanged(&owner);song_masters_unchanged(source);
 owner=image(&f->fast,s);assert(s->allocator_context.owner==s&&!pt_paula_readers_song_close(&s->allocator_context.owner)&&s->allocator_context.owner==s);unchanged(&owner);song_masters_unchanged(source);
 fixture_cancel_drain(source,&s);
 /* Returning the actual owned parent from a later child allocation acquires
  * no ownership. The ordinary release really frees legitimate allocations. */
 source=song_fixture_new(16,1);f=&source->genuine;f->allocator.allocate=fixture_alias_allocate;s=fixture_song_begin(source);
 while(fixture_song_get(s).phase!=PT_PAULA_READERS_SONG_OUTPUT_SETUP)assert(fixture_song_step(source,s)==PT_PAULA_READERS_SONG_PENDING);
 live=f->fast.live;fixture_alias_pointer=s;fixture_alias_at=f->fast.calls+1;fixture_alias_returns=0;
 assert(pt_paula_readers_song_step(s,1,256,NULL)==PT_PAULA_READERS_SONG_FAILED&&fixture_alias_returns==1&&f->fast.live==live);
 assert(allocated(&f->fast,s)==pt_paula_readers_song_control_size());fixture_alias_pointer=NULL;fixture_cancel_drain(source,&s);
}
static void song_copied_original_controls(void)
{
 struct song_fixture *source=song_fixture_new(16,1);struct fixture *f=&source->genuine;
 struct pt_paula_readers_song_config *c=malloc(sizeof(*c));struct pt_allocator *a=malloc(sizeof(*a));struct pt_paula_readers_song *s=NULL;unsigned n=0;
 assert(c&&a);*c=fixture_song_config(source);*a=f->allocator;
 assert(fixture_workspace_begin(a,&f->sampler,&f->document.project,c,1,&s)==PT_PAULA_READERS_SONG_PENDING&&s);
 memset(c,0x5a,sizeof(*c));memset(a,0x6b,sizeof(*a));free(c);free(a);
 while(fixture_song_get(s).phase!=PT_PAULA_READERS_SONG_PUBLISH){assert(fixture_song_step(source,s)==PT_PAULA_READERS_SONG_PENDING&&++n<10000);}
 fixture_cancel_drain(source,&s);
}
static void song_selection_cursor(void)
{
 struct song_fixture *source=song_fixture_new(24,1);struct fixture *f=&source->genuine;struct pt_paula_readers_song *s=fixture_song_begin(source);
 struct pt_paula_readers_song_status out,before;unsigned n=0,submissions;
 while(fixture_song_get(s).phase!=PT_PAULA_READERS_SONG_OUTPUT_SETUP){assert(fixture_song_step(source,s)==PT_PAULA_READERS_SONG_PENDING&&++n<10000);}
 f->document.project.channels.selected=15;assert(fixture_song_step(source,s)==PT_PAULA_READERS_SONG_PENDING);
 submissions=f->backend.submissions;f->document.project.channels.selected=16;memset(&out,0x5a,sizeof(out));before=out;
 assert(pt_paula_readers_song_step(s,1,256,&out)==PT_PAULA_READERS_SONG_STALE&&!memcmp(&out,&before,sizeof(out))&&f->backend.submissions==submissions);
 fixture_cancel_drain(source,&s);
}

static void fixture_first_publish_ready(struct song_fixture *source,struct pt_paula_readers_song *s)
{
 unsigned n=0;
 while(fixture_song_get(s).phase!=PT_PAULA_READERS_SONG_PUBLISH)
   assert(fixture_song_step(source,s)==PT_PAULA_READERS_SONG_PENDING&&++n<10000);
}
static struct pt_paula_readers_song_status fixture_failed_status(struct pt_paula_readers_song *s)
{
 struct pt_paula_readers_song_status out;memset(&out,0x5a,sizeof(out));
 assert(pt_paula_readers_song_get(s,1,&out)==PT_PAULA_READERS_SONG_FAILED&&out.result==PT_PAULA_READERS_SONG_FAILED);
 return out;
}
static void song_explicit_pending_acceptance(void)
{
 struct song_fixture *source=song_fixture_new(24,1);struct fixture *f=&source->genuine;
 struct pt_paula_readers_song *s=fixture_song_begin(source);struct pt_paula_readers_song_status before,after;
 struct pt_scheduled_batch batch;uint64_t ticket;unsigned index,fast,chip,cp,rp,i;
 fixture_first_publish_ready(source,s);before=fixture_song_get(s);index=s->pending_index;
 ticket=s->command[index].ticket;batch=song_oracle_batch(s->command[index].holder);
 fast=f->fast.calls;chip=f->chip.calls;cp=fixture_command_polls;rp=fixture_reader_polls;fixture_submit_mode=1;
 assert(pt_paula_readers_song_publish_next(s,1)==PT_SCHEDULED_PENDING&&!fixture_submit_mode);
 after=fixture_song_get(s);
 assert(after.phase==before.phase&&after.boundary_frame==before.boundary_frame&&after.terminal_frame==before.terminal_frame&&after.command_mask==before.command_mask&&after.reader_mask==before.reader_mask&&!after.published_mask);
 assert(s->command[index].ticket==ticket&&!s->command[index].published&&f->backend.submissions==1&&!f->backend.commands&&!f->backend.readers&&!f->backend.effects);
 assert(pt_readers_commands_held(s->queue)==1&&pt_readers_readers_held(s->queue)==4);
 for(i=0;i<4;++i){struct pt_readers_key key,untouched;memset(&key,0x5a,sizeof(key));untouched=key;
   assert(pt_paula_readers_reader_key(s->reader[i].holder,&key)==PT_SCHEDULED_STALE&&!memcmp(&key,&untouched,sizeof(key)));}
 /* Step/get cannot retry publication; only the explicit same-window call may. */
 for(i=0;i<3;++i)assert(fixture_song_step(source,s)==PT_PAULA_READERS_SONG_PENDING);
 assert(f->backend.submissions==1&&f->fast.calls==fast&&f->chip.calls==chip&&fixture_command_polls==cp&&fixture_reader_polls==rp);
 assert(pt_paula_readers_song_publish_next(s,1)==PT_SCHEDULED_OK&&f->backend.submissions==2&&f->backend.commands==1&&f->backend.readers==4);
 assert(f->backend.command[0].event.scheduled.ticket==ticket&&f->backend.command[0].event.scheduled.batch.frame==before.boundary_frame);
 song_batch_same(&f->backend.command[0].event.scheduled.batch,&batch);
 fixture_fire(source,s,ticket);fixture_detach(source,s,ticket);fixture_cancel_drain(source,&s);
}
static void song_uncertain_and_malformed_domains(unsigned variant)
{
 struct song_fixture *source=song_fixture_new(16,1);struct fixture *f=&source->genuine;
 struct pt_paula_readers_song *s=fixture_song_begin(source);struct model_command *command;
 struct pt_readers_command_receipt command_exact,command_out,command_before;
 struct pt_readers_reader_receipt reader_exact,reader_out,reader_before;
 unsigned index,ri,i,live,effects,cp,rp;uint64_t ticket,frame;
 fixture_first_publish_ready(source,s);index=s->pending_index;frame=fixture_song_get(s).boundary_frame;
 if(!variant)fixture_submit_mode=2;
 assert(pt_paula_readers_song_publish_next(s,1)==(variant?PT_SCHEDULED_OK:PT_SCHEDULED_BACKEND));
 assert(f->backend.commands==1&&f->backend.readers==4&&s->command[index].published);
 command=f->backend.command;ticket=command->event.scheduled.ticket;
 if(variant){fixture_fire(source,s,ticket);effects=f->backend.effects;}else effects=0;
 live=f->fast.live;cp=fixture_command_polls;rp=fixture_reader_polls;
 memset(&command_out,0x5a,sizeof(command_out));command_before=command_out;
 memset(&reader_out,0x6b,sizeof(reader_out));reader_before=reader_out;
 command_exact=command->receipt;reader_exact=f->backend.reader[0].receipt;ri=fixture_reader_index(s,&f->backend.reader[0].key);
 if(!variant){
   assert(command->borrowed&&f->backend.reader[0].borrowed&&f->backend.uncertain);
   assert(pt_paula_readers_song_service_command(s,index,0,&command_out)==PT_SCHEDULED_BACKEND&&!memcmp(&command_out,&command_before,sizeof(command_out)));
   assert(pt_paula_readers_song_service_reader(s,ri,0,&reader_out)==PT_SCHEDULED_BACKEND&&!memcmp(&reader_out,&reader_before,sizeof(reader_out)));
   assert(fixture_command_polls==cp+1&&fixture_reader_polls==rp+1);
 }else if(variant==1){
   /* Malformed observation envelope cannot certify command detachment. */
   command->receipt.owner^=1;
   assert(pt_paula_readers_song_service_command(s,index,0,&command_out)==PT_SCHEDULED_BACKEND&&!memcmp(&command_out,&command_before,sizeof(command_out))&&fixture_command_polls==cp+1);
 }else{
   /* Wrong full serial is an independent malformed reader proof. */
   f->backend.reader[0].receipt.key.serial^=1;
   assert(pt_paula_readers_song_service_reader(s,ri,0,&reader_out)==PT_SCHEDULED_BACKEND&&!memcmp(&reader_out,&reader_before,sizeof(reader_out))&&fixture_reader_polls==rp+1);
 }
 assert(s->command[index].holder&&s->reader[ri].holder&&pt_readers_commands_held(s->queue)==1&&pt_readers_readers_held(s->queue)==4&&pins(f)==4&&f->fast.live==live&&f->backend.effects==effects);
 assert(fixture_failed_status(s).boundary_frame==frame&&!pt_paula_readers_song_close(&s));
 cp=f->backend.submissions;
 for(i=0;i<3;++i){assert(pt_paula_readers_song_publish_next(s,1)==PT_SCHEDULED_INVALID);assert(pt_paula_readers_song_step(s,1,256,NULL)==PT_PAULA_READERS_SONG_FAILED);}
 assert(f->backend.submissions==cp&&f->backend.effects==effects);
 command->receipt=command_exact;f->backend.reader[0].receipt=reader_exact;f->backend.uncertain=0;
 /* A real retired reader may be proved first, but its command reference still
  * retains storage. Exact detached command proof then releases its own domain. */
 retired(&f->backend,&f->backend.reader[0].key);
 assert(pt_paula_readers_song_service_reader(s,ri,0,NULL)==PT_SCHEDULED_BACKEND&&s->reader[ri].holder&&pins(f)==4);
 detach(&f->backend,ticket);
 assert(pt_paula_readers_song_service_command(s,index,0,NULL)==PT_SCHEDULED_BACKEND&&!s->command[index].holder&&!s->reader[ri].holder&&pins(f)==3&&pt_readers_commands_held(s->queue)==0&&pt_readers_readers_held(s->queue)==3);
 for(i=1;i<4;++i){struct pt_readers_key key=f->backend.reader[i].key;unsigned original=fixture_reader_index(s,&key);
   retired(&f->backend,&key);assert(pt_paula_readers_song_service_reader(s,original,0,NULL)==PT_SCHEDULED_BACKEND&&!s->reader[original].holder);}
 assert(!pins(f)&&!pt_readers_readers_held(s->queue)&&f->backend.effects==effects);
 fixture_cancel_drain(source,&s);
}
static void song_original_overflow(void)
{
 unsigned variant;
 for(variant=0;variant<2;++variant){struct song_fixture *source=song_fixture_new(24,1);struct fixture *f=&source->genuine;
   struct pt_paula_readers_song_config c=fixture_song_config(source);struct pt_paula_readers_song *s=NULL;
   enum pt_paula_readers_song_result r;enum pt_paula_readers_song_phase before_phase;
   struct pt_paula_readers_song_status status;unsigned n=0;uint64_t start,frame;
   if(!variant)c.absolute_start=UINT64_MAX-1;else {c.grid.epoch=UINT64_MAX-1;f->backend.now=c.grid.epoch;}
   start=c.absolute_start;
   assert(fixture_workspace_begin(&f->allocator,&f->sampler,&f->document.project,&c,1,&s)==PT_PAULA_READERS_SONG_PENDING&&s);
   do{assert(pt_paula_readers_song_get(s,1,&status)==PT_PAULA_READERS_SONG_PENDING);before_phase=status.phase;
     r=pt_paula_readers_song_step(s,1,256,NULL);assert(++n<10000);f->pool=s->pool;f->queue=s->queue;f->backend.pool=s->pool;
     assert(r==PT_PAULA_READERS_SONG_PENDING||r==PT_PAULA_READERS_SONG_FAILED);song_masters_unchanged(source);
   }while(r!=PT_PAULA_READERS_SONG_FAILED);
   status=fixture_failed_status(s);frame=status.boundary_frame;
   assert(!f->backend.submissions&&!f->backend.effects&&!f->backend.commands&&!f->backend.readers);
   if(!variant)assert(frame==start&&!f->chip.calls&&!s->queue&&!s->pool);
   else {struct pt_elapsed_clock clock;uint64_t deadline=1234567;
     /* include_lead_in remains0: genuine first fresh interval is zero-length,
      * so the exact trigger boundary is start. Queue registration refuses its
      * original deadline, before owning any command or reader domains. */
     assert(before_phase==PT_PAULA_READERS_SONG_ENQUEUE&&frame==start&&status.scheduled_result==PT_SCHEDULED_CLOCK&&status.render_result==PT_RENDER_OK&&status.command_mask==1&&!status.reader_mask&&!status.published_mask);
     assert(!pt_readers_commands_held(s->queue)&&!pt_readers_readers_held(s->queue));
     assert(pt_elapsed_clock_init(&clock,c.grid.frequency,c.grid.rate,c.grid.epoch,0)==PT_ELAPSED_OK);
     assert(pt_elapsed_clock_deadline(&clock,frame,&deadline)==PT_ELAPSED_OVERFLOW&&deadline==1234567);
   }
   assert(pt_paula_readers_song_publish_next(s,1)==PT_SCHEDULED_INVALID&&fixture_failed_status(s).boundary_frame==frame);
   fixture_cancel_drain(source,&s);
 }
}
static uint64_t fixture_original_deadline(const struct pt_paula_readers_song *s,uint64_t frame)
{
 struct pt_elapsed_clock clock;uint64_t first=0;
 assert(pt_elapsed_clock_init(&clock,s->config.grid.frequency,s->config.grid.rate,s->config.grid.epoch,0)==PT_ELAPSED_OK);
 assert(pt_elapsed_clock_deadline(&clock,frame,&first)==PT_ELAPSED_OK);return first;
}
static void song_original_late(unsigned terminal)
{
 struct song_fixture *source=song_fixture_new(24,1);struct fixture *f=&source->genuine;
 struct pt_paula_readers_song *s=fixture_song_begin(source);struct pt_paula_readers_song_status status;
 unsigned n=0,submissions,effects;uint64_t frame;
 if(terminal){
   while(!(status=fixture_song_get(s)).done){
     if(status.phase==PT_PAULA_READERS_SONG_PUBLISH){unsigned before=f->backend.commands;uint64_t ticket;
       assert(pt_paula_readers_song_publish_next(s,1)==PT_SCHEDULED_OK&&f->backend.commands==before+1);
       ticket=f->backend.command[before].event.scheduled.ticket;fixture_fire(source,s,ticket);fixture_detach(source,s,ticket);
     }else {enum pt_paula_readers_song_result r=fixture_song_step(source,s);
       assert((r==PT_PAULA_READERS_SONG_PENDING||r==PT_PAULA_READERS_SONG_DONE)&&++n<10000);
       if(r==PT_PAULA_READERS_SONG_DONE){struct pt_paula_readers_song_status ended=fixture_song_get(s);
         assert(ended.done&&ended.phase==PT_PAULA_READERS_SONG_END&&ended.boundary_frame==status.terminal_frame&&ended.terminal_frame==status.terminal_frame);}
     }
   }
   assert(status.reader_mask==31&&!status.command_mask&&!status.terminal_stop_requested);
   assert(pt_paula_readers_song_terminal_stop(s,1)==PT_PAULA_READERS_SONG_PENDING);
 }
 fixture_first_publish_ready(source,s);status=fixture_song_get(s);frame=status.boundary_frame;
 if(terminal)assert(frame==status.terminal_frame&&status.terminal_stop_requested);
 submissions=f->backend.submissions;effects=f->backend.effects;f->backend.now=fixture_original_deadline(s,frame);
 assert(pt_paula_readers_song_publish_next(s,1)==PT_SCHEDULED_BACKEND&&s->scheduled_result==PT_SCHEDULED_LATE);
 status=fixture_failed_status(s);assert(status.boundary_frame==frame&&f->backend.submissions==submissions&&f->backend.effects==effects);
 if(terminal)assert(status.terminal_frame==frame&&status.done&&status.terminal_stop_requested&&pins(f)==5);
 assert(pt_paula_readers_song_publish_next(s,1)==PT_SCHEDULED_INVALID&&fixture_failed_status(s).boundary_frame==frame&&f->backend.submissions==submissions);
 fixture_cancel_drain(source,&s);
}
static struct pt_paula_readers_song *fixture_release_owner;
static unsigned fixture_child_release_armed,fixture_child_release_reentries;
static void fixture_song_reenter_release(void *context,void *allocation)
{
 if(fixture_child_release_armed&&fixture_release_owner&&allocation!=fixture_release_owner){
   struct pt_paula_readers_song_status out,before;struct pt_paula_readers_song *nested=fixture_release_owner;
   fixture_child_release_armed=0;memset(&out,0x5a,sizeof(out));before=out;
   /* Callback executes while both child and parent control bytes are alive. */
   assert(pt_paula_readers_song_get(nested,1,&out)==PT_PAULA_READERS_SONG_BUSY&&!memcmp(&out,&before,sizeof(out)));
   assert(!pt_paula_readers_song_close(&nested)&&nested==fixture_release_owner);
   assert(pt_paula_readers_song_cancel(nested)==PT_PAULA_READERS_SONG_BUSY);++fixture_child_release_reentries;
 }
 release(context,allocation);
}
static void song_child_release_reentry(void)
{
 struct song_fixture *source=song_fixture_new(16,1);struct fixture *f=&source->genuine;
 struct pt_paula_readers_song *s;unsigned n=0,cp=fixture_command_polls,rp=fixture_reader_polls;
 f->allocator.release=fixture_song_reenter_release;s=fixture_song_begin(source);
 while(fixture_song_get(s).phase!=PT_PAULA_READERS_SONG_NEXT)assert(fixture_song_step(source,s)==PT_PAULA_READERS_SONG_PENDING&&++n<10000);
 assert(s->sequence&&s->pool&&s->queue&&!f->backend.submissions&&!f->backend.effects);
 assert(pt_paula_readers_song_cancel(s)==PT_PAULA_READERS_SONG_PENDING);
 fixture_release_owner=s;fixture_child_release_armed=1;
 assert(!pt_paula_readers_song_close(&s)&&!s&&!fixture_child_release_armed&&fixture_child_release_reentries==1);
 fixture_release_owner=NULL;f->pool=NULL;f->queue=NULL;
 assert(fixture_command_polls==cp&&fixture_reader_polls==rp&&!f->backend.effects);song_fixture_free(source);
}


/* These constructor refusals use actual addressable scratch and real source
 * objects. No invented owner or completion state is used to reach a phase. */
static void workspace_unchanged(const struct fixture_workspace *w,const void *before)
{assert(!memcmp(w->data,before,w->capacity));}
static void *workspace_before(const struct fixture_workspace *w)
{void *p=malloc(w->capacity);assert(p);memcpy(p,w->data,w->capacity);return p;}
static void workspace_shape_and_full_capacity(void)
{
 struct song_fixture *source=song_fixture_new(24,1);struct fixture *f=&source->genuine;
 struct pt_paula_readers_song_config c=fixture_song_config(source);struct pt_paula_readers_song *s=NULL;
 struct fixture_workspace w=fixture_workspace_new(sizeof(c)+sizeof(struct backend)+64);
 size_t size=pt_paula_readers_song_begin_workspace_size(),alignment=pt_paula_readers_song_begin_workspace_alignment();
 unsigned calls=f->fast.calls,live=f->fast.live,chip=f->chip.calls;void *before=workspace_before(&w);
 assert(pt_paula_readers_song_begin_in_workspace(&f->allocator,&f->sampler,&f->document.project,&c,1,w.data,size-1,&s)==PT_PAULA_READERS_SONG_CAPACITY&&!s);
 workspace_unchanged(&w,before);
 assert(pt_paula_readers_song_begin_in_workspace(&f->allocator,&f->sampler,&f->document.project,&c,1,NULL,w.capacity,&s)==PT_PAULA_READERS_SONG_INVALID&&!s);
 workspace_unchanged(&w,before);
 if(alignment>1){assert(pt_paula_readers_song_begin_in_workspace(&f->allocator,&f->sampler,&f->document.project,&c,1,w.data+1,size,&s)==PT_PAULA_READERS_SONG_INVALID&&!s);workspace_unchanged(&w,before);}
 /* A real pointer with a wrapping declared length is refused before access. */
 assert(pt_paula_readers_song_begin_in_workspace(&f->allocator,&f->sampler,&f->document.project,&c,1,w.data,SIZE_MAX,&s)==PT_PAULA_READERS_SONG_INVALID&&!s);
 workspace_unchanged(&w,before);free(before);
 /* Used bytes are disjoint; each live borrowed control lies in spare capacity. */
 {struct pt_paula_readers_song_config *inside=(struct pt_paula_readers_song_config *)(w.data+size);
   *inside=c;before=workspace_before(&w);
   assert(pt_paula_readers_song_begin_in_workspace(&f->allocator,&f->sampler,&f->document.project,inside,1,w.data,w.capacity,&s)==PT_PAULA_READERS_SONG_INVALID&&!s);
   workspace_unchanged(&w,before);free(before);
 }
 {struct pt_allocator *inside=(struct pt_allocator *)(w.data+size);
   *inside=f->allocator;before=workspace_before(&w);
   assert(pt_paula_readers_song_begin_in_workspace(inside,&f->sampler,&f->document.project,&c,1,w.data,w.capacity,&s)==PT_PAULA_READERS_SONG_INVALID&&!s);
   workspace_unchanged(&w,before);free(before);
 }
 {struct pt_paula_readers_song **inside=(struct pt_paula_readers_song **)(w.data+size);
   *inside=NULL;before=workspace_before(&w);
   assert(pt_paula_readers_song_begin_in_workspace(&f->allocator,&f->sampler,&f->document.project,&c,1,w.data,w.capacity,inside)==PT_PAULA_READERS_SONG_INVALID&&!*inside);
   workspace_unchanged(&w,before);free(before);
 }
 {struct backend *inside=(struct backend *)(w.data+size);struct pt_paula_readers_song_config overlapping=c;
   *inside=f->backend;overlapping.backend.context=inside;before=workspace_before(&w);
   assert(pt_paula_readers_song_begin_in_workspace(&f->allocator,&f->sampler,&f->document.project,&overlapping,1,w.data,w.capacity,&s)==PT_PAULA_READERS_SONG_INVALID&&!s);
   workspace_unchanged(&w,before);free(before);
 }
 assert(f->fast.calls==calls&&f->fast.live==live&&f->chip.calls==chip&&!f->backend.submissions&&!f->backend.effects);
 song_masters_unchanged(source);fixture_workspace_discard(&w);song_fixture_free(source);
}
static void workspace_owned_pcm_and_output_alias(void)
{
 struct song_fixture *source=song_fixture_new(16,1);struct fixture *f=&source->genuine;struct pt_project *p=&f->document.project;
 struct pt_paula_readers_song_config c=fixture_song_config(source);struct pt_paula_readers_song *s=NULL;
 struct pt_pcm pcm;struct fixture_workspace w=fixture_workspace_new(0);size_t alignment=pt_paula_readers_song_begin_workspace_alignment();
 size_t values=16384+(w.capacity+alignment+sizeof(int32_t)-1)/sizeof(int32_t);unsigned i,calls,live,chip;
 assert(!source->history_active&&pt_pattern_history_init(&source->history,p,source->history_command,4,source->history_change,4)==PT_EDIT_OK);
 source->history_active=1;source->padding_slot=p->sample_count;source->padding_bytes=values*sizeof(int32_t);
 source->padding_data=allocate(&f->fast,source->padding_bytes);assert(source->padding_data);
 for(i=0;i<values;++i)source->padding_data[i]=i<16384?f->master.values[i]:0x1234;
 pcm=(struct pt_pcm){source->padding_data,values,16384,8000,1,16};
 assert(pt_sampler_append_owned(&f->sampler,p,&source->history,&pcm,&f->allocator,"workspace alias padding")==PT_EDIT_OK&&!pcm.data&&f->sampler.current[source->padding_slot]);
 source->padding_before=malloc(source->padding_bytes);assert(source->padding_before);memcpy(source->padding_before,source->padding_data,source->padding_bytes);source->table_before=p->samples;
 calls=f->fast.calls;live=f->fast.live;chip=f->chip.calls;
 {uint8_t *padding=(uint8_t *)(source->padding_data+16384);size_t skip=(alignment-(uintptr_t)padding%alignment)%alignment;
   assert(w.capacity+skip<=source->padding_bytes-16384*sizeof(int32_t));
   assert(pt_paula_readers_song_begin_in_workspace(&f->allocator,&f->sampler,p,&c,1,padding+skip,w.capacity,&s)==PT_PAULA_READERS_SONG_INVALID&&!s);
 }
 /* The output is a real live source control slot, never an invented handle. */
 {struct pt_sample_version *current=f->sampler.current[0];void *before=workspace_before(&w);
   assert(pt_paula_readers_song_begin_in_workspace(&f->allocator,&f->sampler,p,&c,1,w.data,w.capacity,(struct pt_paula_readers_song **)&f->sampler.current[0])==PT_PAULA_READERS_SONG_INVALID&&f->sampler.current[0]==current);
   workspace_unchanged(&w,before);free(before);
 }
 assert(f->fast.calls==calls&&f->fast.live==live&&f->chip.calls==chip&&!f->backend.submissions&&!f->backend.effects);
 song_masters_unchanged(source);fixture_workspace_discard(&w);song_fixture_free(source);
}
static struct fixture_workspace *workspace_fault_scratch;
static struct pt_project *workspace_fault_project;
static struct pt_paula_readers_song_config *workspace_fault_config;
static void *workspace_fault_return,*workspace_fault_image;
static size_t workspace_fault_return_available;
static unsigned workspace_fault_at,workspace_fault_action,workspace_fault_calls,workspace_fault_releases;
static void *workspace_fault_allocate(void *context,size_t bytes)
{
 struct memory *m=context;unsigned targeted=m->calls+1==workspace_fault_at;void *p;
 if(targeted&&workspace_fault_return){assert(bytes<=workspace_fault_return_available);++m->calls;++workspace_fault_calls;
   workspace_fault_image=workspace_before(workspace_fault_scratch);return workspace_fault_return;}
 p=allocate(context,bytes);
 if(targeted&&p){++workspace_fault_calls;
   if(workspace_fault_action==1)++workspace_fault_project->bpm;
   if(workspace_fault_action==2)++workspace_fault_config->control_budget;
 }
 return p;
}
static void workspace_fault_release(void *context,void *p)
{assert(p!=workspace_fault_return);++workspace_fault_releases;release(context,p);}
static void workspace_allocator_guards_and_stale(void)
{
 unsigned stage,spare,action;
 for(stage=1;stage<=3;++stage)for(spare=0;spare<=1;++spare){
   struct song_fixture *source=song_fixture_new(24,1);struct fixture *f=&source->genuine;
   struct pt_paula_readers_song_config c=fixture_song_config(source);struct pt_paula_readers_song *s=NULL;
   struct pt_allocator a={&f->fast,workspace_fault_allocate,workspace_fault_release};
   struct fixture_workspace w=fixture_workspace_new(pt_paula_readers_song_control_size()+4096);
   unsigned calls=f->fast.calls,live=f->fast.live;size_t bytes=f->fast.bytes,offset=spare?pt_paula_readers_song_begin_workspace_size():0;
   workspace_fault_scratch=&w;workspace_fault_return=w.data+offset;
   workspace_fault_return_available=w.allocated-(size_t)((uint8_t *)workspace_fault_return-w.allocation);
   workspace_fault_at=calls+stage;workspace_fault_action=0;workspace_fault_calls=workspace_fault_releases=0;workspace_fault_image=NULL;
   /* The interposed child allocator hides its refused arena as NULL. The
    * existing outer constructor therefore reports child CAPACITY, not a
    * fabricated successful allocation or a caller-selected error class. */
   assert(pt_paula_readers_song_begin_in_workspace(&a,&f->sampler,&f->document.project,&c,1,w.data,w.capacity,&s)==(stage==1?PT_PAULA_READERS_SONG_INVALID:PT_PAULA_READERS_SONG_CAPACITY)&&!s);
   assert(workspace_fault_calls==1&&workspace_fault_image&&f->fast.calls==calls+stage&&f->fast.live==live&&f->fast.bytes==bytes&&workspace_fault_releases==stage-1);
   workspace_unchanged(&w,workspace_fault_image);free(workspace_fault_image);workspace_fault_image=NULL;
   assert(!f->chip.calls&&!f->backend.submissions&&!f->backend.effects);song_masters_unchanged(source);
   workspace_fault_return=NULL;workspace_fault_scratch=NULL;fixture_workspace_discard(&w);song_fixture_free(source);
 }
 for(stage=1;stage<=3;++stage)for(action=1;action<=2;++action){
   struct song_fixture *source=song_fixture_new(16,1);struct fixture *f=&source->genuine;
   struct pt_paula_readers_song_config c=fixture_song_config(source),before=c;struct pt_paula_readers_song *s=NULL;
   struct pt_allocator a={&f->fast,workspace_fault_allocate,workspace_fault_release};struct fixture_workspace w=fixture_workspace_new(0);
   unsigned calls=f->fast.calls,live=f->fast.live,bpm=f->document.project.bpm;size_t bytes=f->fast.bytes;
   workspace_fault_project=&f->document.project;workspace_fault_config=&c;workspace_fault_return=NULL;
   workspace_fault_at=calls+stage;workspace_fault_action=action;workspace_fault_calls=workspace_fault_releases=0;
   /* Header mutation at the core-startup allocation makes the interposer
    * release the fresh arena and return NULL. Core checks the original header
    * before its NULL-capacity gate, returns STALE, and the outer constructor
    * maps that child refusal to INVALID. Original config changes likewise
    * refuse at the final constructor gate; no successful owner is published. */
   assert(pt_paula_readers_song_begin_in_workspace(&a,&f->sampler,&f->document.project,&c,1,w.data,w.capacity,&s)==(stage==1?PT_PAULA_READERS_SONG_STALE:PT_PAULA_READERS_SONG_INVALID)&&!s);
   assert(workspace_fault_calls==1&&f->fast.live==live&&f->fast.bytes==bytes&&workspace_fault_releases==f->fast.calls-calls);
   if(action==1){assert(f->document.project.bpm==bpm+1);f->document.project.bpm=(uint16_t)bpm;}else{assert(c.control_budget==before.control_budget+1);c=before;}
   assert(!f->chip.calls&&!f->backend.submissions&&!f->backend.effects);song_masters_unchanged(source);
   workspace_fault_project=NULL;workspace_fault_config=NULL;workspace_fault_action=0;fixture_workspace_discard(&w);song_fixture_free(source);
 }
 for(stage=1;stage<=3;++stage){
   struct song_fixture *source=song_fixture_new(8,1);struct fixture *f=&source->genuine;
   struct pt_paula_readers_song_config c=fixture_song_config(source);struct pt_paula_readers_song *s=NULL;
   struct fixture_workspace w=fixture_workspace_new(0);unsigned calls=f->fast.calls,live=f->fast.live;size_t bytes=f->fast.bytes;
   f->fast.fail=calls+stage;
   assert(pt_paula_readers_song_begin_in_workspace(&f->allocator,&f->sampler,&f->document.project,&c,1,w.data,w.capacity,&s)==PT_PAULA_READERS_SONG_CAPACITY&&!s&&f->fast.live==live&&f->fast.bytes==bytes);
   assert(!f->chip.calls&&!f->backend.submissions&&!f->backend.effects);f->fast.fail=0;
   fixture_workspace_discard(&w);song_fixture_free(source);
 }
}
static void workspace_exact_reuse_and_legacy(void)
{
 struct song_fixture *source=song_fixture_new(24,1);struct fixture *f=&source->genuine;
 struct pt_paula_readers_song_config c=fixture_song_config(source);struct pt_paula_readers_song *s=NULL;
 struct fixture_workspace w=fixture_workspace_new(0);unsigned n=0;
 assert(pt_paula_readers_song_begin_in_workspace(&f->allocator,&f->sampler,&f->document.project,&c,1,w.data,w.capacity,&s)==PT_PAULA_READERS_SONG_PENDING&&s);
 /* Exact capacity is immediately reused as unrelated byte storage, then freed. */
 memset(w.data,0x39,w.capacity);fixture_workspace_discard(&w);
 while(fixture_song_get(s).phase!=PT_PAULA_READERS_SONG_NEXT)assert(fixture_song_step(source,s)==PT_PAULA_READERS_SONG_PENDING&&++n<10000);
 assert(s->sequence==fixture_transferred_sequence&&s->sequence&&s->pool&&s->queue&&!f->backend.submissions);
 fixture_cancel_drain(source,&s);
 source=song_fixture_new(8,1);f=&source->genuine;c=fixture_song_config(source);n=0;fixture_transferred_sequence=NULL;
 /* This separate smoke explicitly preserves the original large-stack route. */
 assert(pt_paula_readers_song_begin(&f->allocator,&f->sampler,&f->document.project,&c,1,&s)==PT_PAULA_READERS_SONG_PENDING&&s);
 while(fixture_song_get(s).phase!=PT_PAULA_READERS_SONG_NEXT)assert(fixture_song_step(source,s)==PT_PAULA_READERS_SONG_PENDING&&++n<10000);
 assert(s->sequence==fixture_transferred_sequence&&s->sequence);fixture_cancel_drain(source,&s);
}

int main(void)
{
 workspace_shape_and_full_capacity();workspace_owned_pcm_and_output_alias();
 workspace_allocator_guards_and_stale();workspace_exact_reuse_and_legacy();
 song_whole_exact(8);song_whole_exact(16);song_whole_exact(24);
 song_cancel_phases();song_submitted_cancel(0);song_submitted_cancel(1);
 song_eight_reader_pressure();song_empty_with_two_held();song_clock_callback_refusals();
 song_stale_and_missing();song_control_and_capacity_aliases();song_copied_original_controls();song_selection_cursor();
 song_explicit_pending_acceptance();song_uncertain_and_malformed_domains(0);song_uncertain_and_malformed_domains(1);song_uncertain_and_malformed_domains(2);
 song_original_overflow();song_original_late(0);song_original_late(1);song_child_release_reentry();
 puts("PAULA READERS SONG PASS: exact whole-song schedule, independent actual domains and immutable retained masters; host software only");
 puts("PAULA READERS SONG FAULTS PASS: explicit acceptance, independent fault drain and original deadlines; host software only");
 assert(fixture_workspace_begins>20&&fixture_workspace_successes>20);
 puts("PAULA READERS SONG WORKSPACE PASS: guarded caller scratch released before genuine exact scheduling and independent domain ownership; host software only");return 0;
}
