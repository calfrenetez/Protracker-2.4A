#define main inherited_readers_fixture_main
#include "sampler_paula_readers_test.c"
#undef main
#include "../src/editor/paula_preflight.h"
#include "../src/core/render_lookahead.h"

/* Reuse the existing genuine sampler, cache, allocation accounting and bounded
 * backend model. The model remains serialized host work, never native activation.
 * The independent fixture drives actual preflight/sequence/lookahead plans. */
#define BOUNDARY_RATE 48000U
#define BOUNDARY_FREQUENCY 1000000U
#define BOUNDARY_START 10000U
static const unsigned boundary_track[4]={0,2,4,6};
static const struct pt_paula_render_caps boundary_caps={3546895,124,65535};
static void *boundary_borrow;
static unsigned boundary_borrow_at,boundary_borrow_calls,boundary_borrow_releases;
static void *boundary_allocate(void *context,size_t bytes)
{
 if(boundary_borrow&&++boundary_borrow_calls==boundary_borrow_at) {
   ++((struct memory *)context)->calls;return boundary_borrow;
 }
 return allocate(context,bytes);
}
static void boundary_release(void *context,void *data)
{
 if(data==boundary_borrow&&data){++boundary_borrow_releases;return;}
 release(context,data);
}
static int boundary_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{struct backend *b=context;*ticks=b->now;*frequency=BOUNDARY_FREQUENCY;return 1;}
static void boundary_init(struct fixture *f,unsigned bits)
{
 struct pt_project *p;struct pt_paula_readers_config config;struct pt_readers_backend b;
 struct pt_scheduled_grid grid={100,7,BOUNDARY_FREQUENCY,BOUNDARY_RATE};unsigned i;
 memset(f,0,sizeof(*f));f->allocator=(struct pt_allocator){&f->fast,boundary_allocate,boundary_release};
 pt_document_init(&f->document,&f->allocator);assert(pt_document_new(&f->document,16,SIZE_MAX)==PT_PROJECT_OK);
 p=&f->document.project;
 for(i=0;i<VALUES;++i)f->master.values[i]=((int32_t)(i%200)-100)*(1L<<(bits-8))+(bits==8?0:(int32_t)(i&63));
 f->master.values[0]=-(1L<<(bits-1));f->master.values[1]=(1L<<(bits-1))-1;
 f->master.values[2]=bits==8?0x12:bits==16?0x1234:0x123456;f->master.values[3]=-f->master.values[2];
 for(i=0;i<16;++i)p->channels.track[i].route=PT_AMIGUS;
 for(i=0;i<4;++i){p->channels.track[boundary_track[i]].route=PT_PAULA;p->channels.track[boundary_track[i]].pan=i==0||i==3?0:255;}
 p->samples[0].pcm=(struct pt_pcm){f->master.values,VALUES,16384,8000,1,(uint8_t)bits};
 p->samples[0].volume=64;p->samples[1]=p->samples[0];
 for(i=0;i<4;++i)p->events[boundary_track[i]]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
 p->events[0].effect=14;p->events[0].parameter=0x11; /* Final CONTROL changes tick-zero pitch. */
 p->events[2].effect=12;p->events[2].parameter=32; /* Final gain must replace zero trigger gains. */
 p->events[16+15].effect=15;p->events[16+15].parameter=150; /* Unselected global-flow track. */
 p->events[16*4]=(struct pt_event){320,0,PT_NOTE_PERIOD,2,0,0,0,0};
 p->events[16*5+15].effect=15;p->events[16*5+15].parameter=0;
 pt_sampler_init(&f->sampler,&f->allocator,1024*1024);
 config=(struct pt_paula_readers_config){2,8,1024*1024,131072,7,&f->chip,chip_allocate,chip_release};
 assert(pt_paula_readers_open(&f->allocator,&f->sampler,p,&config,&f->pool)==PT_PAULA_READERS_OK);
 f->backend.pool=f->pool;f->backend.now=100;
 b=(struct pt_readers_backend){&f->backend,sizeof(f->backend),{7,8,4},PT_READERS_VERSION,3,8,boundary_clock,submit,poll_command,cancel_command,poll_reader,cancel_reader};
 assert(pt_readers_open(&f->allocator,&grid,1001,&b,2,8,&f->queue)==PT_SCHEDULED_OK);
}
static struct pt_render_sequence *boundary_sequence(struct fixture *f,struct pt_paula_preflight_report *report)
{
 struct pt_render_options options;struct pt_render_sequence *sequence=NULL;
 memset(&options,0,sizeof(options));options.rate=BOUNDARY_RATE;options.bits=24;
 options.tracks=(1U<<0)|(1U<<2)|(1U<<4)|(1U<<6);options.gain_q16=65536;
 options.tick_limit=100;options.frame_limit=100000;
 assert(pt_paula_preflight_take(&f->document.project,&options,NULL,&boundary_caps,1,
   &f->allocator,report,&sequence)==PT_PAULA_COMPATIBLE&&sequence);
 assert(report->map[0]==0&&report->map[2]==1&&report->map[4]==2&&report->map[6]==3&&report->map[15]==-1);
 return sequence;
}
static void boundary_forecast(struct pt_render_sequence *sequence,struct pt_render_interval *span,
 struct pt_render_lookahead *ahead,struct pt_render_plan *plan)
{
 unsigned ready=0,steps=0;memset(ahead,0,sizeof(*ahead));
 assert(pt_render_sequence_next(sequence,span)==PT_RENDER_OK&&span->emit);
 assert(pt_render_lookahead_begin(ahead,sequence)==PT_RENDER_OK);
 do{assert(pt_render_lookahead_step(ahead,256,plan,&ready)==PT_RENDER_OK&&++steps<100);}while(!ready);
}
static void boundary_consume(struct pt_render_sequence *sequence,uint32_t frames)
{while(frames){unsigned n=frames>256?256:frames;assert(pt_render_sequence_consume(sequence,n)==PT_RENDER_OK);frames-=n;}}
static void boundary_first_plan(struct fixture *f,struct pt_render_plan *plan)
{
 struct pt_paula_preflight_report report;struct pt_render_sequence *sequence=boundary_sequence(f,&report);
 struct pt_render_interval span;struct pt_render_lookahead ahead;unsigned n=0;
 do{boundary_forecast(sequence,&span,&ahead,plan);assert(++n<20);boundary_consume(sequence,span.frames);
 assert(pt_render_lookahead_commit(&ahead)==PT_RENDER_OK);}while(!plan->count);
 assert(plan->count==8);pt_render_sequence_close(sequence);
}
static void boundary_ready(struct fixture *f,struct pt_paula_readers_command *c)
{
 unsigned ready=0,steps=0,effects=f->backend.effects,submissions=f->backend.submissions;
 enum pt_paula_readers_result result;
 do{result=pt_paula_readers_step(c,&ready);assert(++steps<4096);
 assert(result==PT_PAULA_READERS_PENDING||result==PT_PAULA_READERS_OK);
 assert(ready==(result==PT_PAULA_READERS_OK));assert(f->backend.effects==effects&&f->backend.submissions==submissions);
 }while(!ready);
}
static struct pt_scheduled_batch boundary_batch(struct pt_paula_readers_command *c)
{
 struct pt_scheduled_batch b;unsigned i;memset(&b,0,sizeof(b));b.frame=c->frame;b.generation=7;b.count=c->count;
 for(i=0;i<c->count;++i){struct command_source *s=c->source+i;struct pt_scheduled_action *a=b.action+i;
 a->kind=s->request.kind;a->slot=(unsigned)c->pool->map[s->request.track];a->period=c->lower[i].period;a->volume=c->lower[i].volume;
 if(a->kind==PT_SCHEDULED_TRIGGER){struct pt_paula_readers_view view;assert(pt_paula_readers_view(c,i,&view)==PT_PAULA_READERS_OK);
 a->data=view.data+c->lower[i].offset;a->words=(uint16_t)(c->lower[i].length/2);}}
 return b;
}
struct boundary_saved_span {const void *data;void *saved;size_t bytes;};
static unsigned boundary_save_version(struct pt_sample_version *version,struct boundary_saved_span saved[PT_SAMPLER_VERSION_SPANS])
{
 struct pt_sampler_storage_span spans[PT_SAMPLER_VERSION_SPANS];unsigned i,n=0;
 assert(pt_sampler_version_spans(version,spans,PT_SAMPLER_VERSION_SPANS,&n)&&n);
 for(i=0;i<n;++i){saved[i].data=spans[i].data;saved[i].bytes=spans[i].bytes;saved[i].saved=malloc(spans[i].bytes);
 assert(saved[i].saved);memcpy(saved[i].saved,spans[i].data,spans[i].bytes);}return n;
}
static void boundary_check_version(struct boundary_saved_span *saved,unsigned n)
{unsigned i;for(i=0;i<n;++i)assert(!memcmp(saved[i].data,saved[i].saved,saved[i].bytes));}
static void boundary_free_version(struct boundary_saved_span *saved,unsigned n)
{unsigned i;boundary_check_version(saved,n);for(i=0;i<n;++i)free(saved[i].saved);}
static void boundary_full_song(unsigned bits)
{
 struct fixture f;struct pt_paula_preflight_report report;struct pt_render_sequence *sequence;
 struct pt_render_interval span;struct pt_render_lookahead ahead;struct pt_render_plan plan;
 struct pt_paula_readers_command *c,*delayed=NULL;struct pt_paula_readers_reader *held[8]={0};
 struct pt_readers_key keys[PT_CHANNEL_LIMIT],held_keys[8];struct boundary_saved_span saved[PT_SAMPLER_VERSION_SPANS];
 unsigned saved_count=0,held_count=0,boundaries=0,controls=0,seen960=0,seen800=0,i;
 uint64_t frame=BOUNDARY_START,ticket=0,delayed_ticket=0;int32_t before[VALUES];
 boundary_init(&f,bits);memcpy(before,f.master.values,sizeof(before));memset(keys,0,sizeof(keys));
 sequence=boundary_sequence(&f,&report);
 do{
   unsigned before_allocs=f.fast.calls,chip_calls=f.chip.calls;uint32_t frames;
   boundary_forecast(sequence,&span,&ahead,&plan);assert(++boundaries<100);frames=span.frames;
   seen960|=frames==960;seen800|=frames==800;assert(frame<=UINT64_MAX-frames);frame+=frames;
   c=(void *)(uintptr_t)1;
   assert(pt_paula_readers_lower_begin(f.pool,f.queue,frame,BOUNDARY_RATE,&plan,&boundary_caps,keys,&c)==PT_PAULA_READERS_OK);
   if(!plan.count){assert(!c&&f.fast.calls==before_allocs);}
   else {
     assert(c&&c->count==4&&c->frame==frame);boundary_ready(&f,c);
     if(!held_count){
       unsigned change;struct image ci;
       for(change=0;change<3;++change){struct pt_scheduled_batch bad=boundary_batch(c);uint64_t out=777;ci=image(&f.fast,c);
         if(change==0)++bad.action[0].period;
         if(change==1)--bad.action[0].words;
         if(change==2){bad.action[0].data+=2;--bad.action[0].words;}
         assert(pt_paula_readers_enqueue(c,&bad,&out)==PT_SCHEDULED_INVALID&&out==777);unchanged(&ci);
       }
       ci=image(&f.fast,c);assert(pt_paula_readers_lower_enqueue(c,&f.master.member.ticket)==PT_SCHEDULED_INVALID);unchanged(&ci);
     }
     assert(pt_paula_readers_lower_enqueue(c,&ticket)==PT_SCHEDULED_OK);
     /* Enqueue alone is neither backend acceptance nor actual activation. */
     for(i=0;i<c->count;++i)if(c->source[i].request.kind==PT_SCHEDULED_TRIGGER){
       struct pt_paula_readers_reader *r=reader_handle(c,i);struct pt_readers_key out;uint8_t expected[16384];
       struct pt_playback_format format={8,0,0,1};memset(&out,0x5a,sizeof(out));
       {struct pt_readers_key untouched=out;assert(pt_paula_readers_reader_key(r,&out)!=PT_SCHEDULED_OK&&!memcmp(&out,&untouched,sizeof(out)));}
       assert(r->bytes==sizeof(expected)&&pt_playback_pcm_pack(&r->pcm,&format,expected,sizeof(expected))==PT_PCM_OK);
       assert(!memcmp(expected,r->data,sizeof(expected)));assert(!memcmp(r->pcm.data,before,16384*sizeof(int32_t)));
       assert(held_count<8);held[held_count++]=r;
     }
     if(held_count==4&&!controls){
       struct pt_render_plan waiting;struct pt_paula_readers_command *refused=(void *)(uintptr_t)1;
       unsigned allocs=f.fast.calls;memset(&waiting,0,sizeof(waiting));waiting.count=1;waiting.action[0]=plan.action[4];
       assert(pt_paula_readers_lower_begin(f.pool,f.queue,frame+1,BOUNDARY_RATE,&waiting,&boundary_caps,keys,&refused)==PT_PAULA_READERS_INVALID);
       assert(refused==(void *)(uintptr_t)1&&f.fast.calls==allocs); /* Reserved is not ACTIVE admission. */
     }
     if(!delayed&&held_count==4&&!controls){delayed=c;delayed_ticket=ticket;}
     observe_fire(&f,ticket);
     {
       struct model_command *m=mc(&f.backend,ticket);unsigned triggers=0;
       assert(m->event.scheduled.batch.frame==frame);
       for(i=0;i<4;++i){const struct pt_render_action *final=NULL;unsigned j;uint16_t period;uint8_t volume;
         for(j=0;j<plan.count;++j)if(plan.action[j].channel==boundary_track[i]&&plan.action[j].kind==PT_RENDER_CONTROL)final=plan.action+j;
         assert(final&&pt_paula_render_control(final->voice.step,BOUNDARY_RATE,final->gain,i,&boundary_caps,&period,&volume));
         assert(m->event.scheduled.batch.action[i].slot==i&&m->event.scheduled.batch.action[i].period==period&&m->event.scheduled.batch.action[i].volume==volume);
         if(m->event.scheduled.batch.action[i].kind==PT_SCHEDULED_TRIGGER){struct pt_paula_readers_reader *r=reader_handle(c,i);
           keys[boundary_track[i]]=active_key(r);++triggers;
           /* Populate original keys by handle, including four simultaneous originals. */
           for(j=0;j<held_count;++j)if(held[j]==r)held_keys[j]=keys[boundary_track[i]];
           assert(m->event.scheduled.batch.action[i].words==8192&&m->event.scheduled.batch.action[i].data==r->data);
         }else{assert(!c->source[i].reader&&!c->source[i].reader_token);++controls;}
       }
       if(held_count==4&&!saved_count){
         assert(triggers==4&&m->event.scheduled.batch.action[0].period==442&&m->event.scheduled.batch.action[0].volume==64);
         assert(m->event.scheduled.batch.action[1].period==443&&m->event.scheduled.batch.action[1].volume==32);
         assert(plan.action[0].voice.step!=plan.action[4].voice.step);
         saved_count=boundary_save_version(held[0]->pin,saved);
       }
       if(!triggers)assert(f.chip.calls==chip_calls);
       if(!triggers&&controls==4){
         struct pt_readers_key forged[PT_CHANNEL_LIMIT];struct pt_paula_readers_command *refused=(void *)(uintptr_t)1;
         unsigned allocs=f.fast.calls;memcpy(forged,keys,sizeof(forged));++forged[0].serial;
         assert(pt_paula_readers_lower_begin(f.pool,f.queue,frame+1,BOUNDARY_RATE,&plan,&boundary_caps,forged,&refused)==PT_PAULA_READERS_INVALID);
         assert(refused==(void *)(uintptr_t)1&&f.fast.calls==allocs);
       }
     }
     if(c!=delayed){
       if(delayed){struct pt_paula_readers_command *pressure=(void *)(uintptr_t)1;unsigned allocs=f.fast.calls;
         assert(pt_paula_readers_lower_begin(f.pool,f.queue,frame+1,BOUNDARY_RATE,&plan,&boundary_caps,keys,&pressure)==PT_PAULA_READERS_CAPACITY);
         assert(pressure==(void *)(uintptr_t)1&&f.fast.calls==allocs);detach_collect(&f,delayed_ticket);
         assert(pt_paula_readers_command_close(delayed));delayed=NULL;
       }
       detach_collect(&f,ticket);assert(pt_paula_readers_command_close(c));
     }
   }
   boundary_consume(sequence,frames);assert(pt_render_lookahead_commit(&ahead)==PT_RENDER_OK);
   if(saved_count)boundary_check_version(saved,saved_count);
   assert(!memcmp(before,f.master.values,sizeof(before)));
 }while(!span.end);
 assert(!delayed&&controls>=80&&held_count==5&&seen960&&seen800&&frame-BOUNDARY_START==report.frames);
 assert(!plan.count&&pt_readers_readers_held(f.queue)==5&&pins(&f)==5); /* DONE did not STOP. */
 boundary_free_version(saved,saved_count);pt_render_sequence_close(sequence);
 memset(&plan,0,sizeof(plan));plan.count=4;for(i=0;i<4;++i){plan.action[i].kind=PT_RENDER_STOP;plan.action[i].channel=boundary_track[i];}
 assert(pt_paula_readers_lower_begin(f.pool,f.queue,frame,BOUNDARY_RATE,&plan,&boundary_caps,keys,&c)==PT_PAULA_READERS_OK);
 boundary_ready(&f,c);assert(pt_paula_readers_lower_enqueue(c,&ticket)==PT_SCHEDULED_OK);observe_fire(&f,ticket);
 retire_collect(&f,held_keys[4]);assert(!pt_paula_readers_reader_close(held[4])&&pins(&f)==5);
 retire_collect(&f,held_keys[0]);assert(pt_paula_readers_reader_close(held[0]));
 detach_collect(&f,ticket);assert(pt_paula_readers_command_close(c)&&pt_paula_readers_reader_close(held[4]));
 for(i=1;i<4;++i){retire_collect(&f,held_keys[i]);assert(pt_paula_readers_reader_close(held[i]));}
 assert(!memcmp(before,f.master.values,sizeof(before)));finish(&f);
}
static void boundary_refusals(void)
{
 struct fixture f;struct pt_render_plan original,plan;struct pt_paula_readers_command *out;
 struct pt_readers_key keys[PT_CHANNEL_LIMIT];struct pt_paula_render_caps caps=boundary_caps;int32_t before[VALUES];unsigned kind,i;
 boundary_init(&f,24);boundary_first_plan(&f,&original);memcpy(before,f.master.values,sizeof(before));memset(keys,0,sizeof(keys));
 for(kind=0;kind<8;++kind){struct image pi=image(&f.fast,f.pool);unsigned allocs=f.fast.calls;
   plan=original;out=(void *)(uintptr_t)1;
   if(kind==0)plan.count=PT_RENDER_ACTIONS+1;
   if(kind==1)plan.action[plan.count++]=(struct pt_render_action){PT_RENDER_SEGMENT,0,{0},{0,0}};
   if(kind==2)plan.action[plan.count++]=(struct pt_render_action){(enum pt_render_action_kind)99,15,{0},{0,0}};
   if(kind==3)plan.action[plan.count++]=(struct pt_render_action){PT_RENDER_CONTROL,16,{0},{0,0}};
   if(kind==4)plan.action[4].voice.phase++;
   if(kind==5)plan.action[4].voice.pcm=&f.document.project.samples[1].pcm;
   if(kind==6)plan.action[plan.count++]=plan.action[0];
   if(kind==7)plan.action[0].voice.start++;
   assert(pt_paula_readers_lower_begin(f.pool,f.queue,BOUNDARY_START,BOUNDARY_RATE,&plan,&caps,keys,&out)==PT_PAULA_READERS_INVALID);
   assert(out==(void *)(uintptr_t)1&&f.fast.calls==allocs);unchanged(&pi);assert(!memcmp(before,f.master.values,sizeof(before)));
 }
 plan=original;
 {struct image pi=image(&f.fast,f.pool);struct pt_render_plan saved;struct pt_paula_render_caps savedcaps;unsigned allocs=f.fast.calls;
 memcpy(&saved,&plan,sizeof(saved));memcpy(&savedcaps,&caps,sizeof(savedcaps));
 assert(pt_paula_readers_lower_begin(f.pool,f.queue,BOUNDARY_START,BOUNDARY_RATE,&plan,&caps,keys,(struct pt_paula_readers_command **)&plan.count)==PT_PAULA_READERS_INVALID);
 assert(pt_paula_readers_lower_begin(f.pool,f.queue,BOUNDARY_START,BOUNDARY_RATE,&plan,&caps,keys,(struct pt_paula_readers_command **)&caps)==PT_PAULA_READERS_INVALID);
 assert(pt_paula_readers_lower_begin(f.pool,f.queue,BOUNDARY_START,BOUNDARY_RATE,&plan,&caps,keys,(struct pt_paula_readers_command **)keys)==PT_PAULA_READERS_INVALID);
 assert(pt_paula_readers_lower_begin(f.pool,f.queue,BOUNDARY_START,BOUNDARY_RATE,&plan,&caps,keys,(struct pt_paula_readers_command **)&f.master.values[VALUES-2])==PT_PAULA_READERS_INVALID);
 assert(!memcmp(&plan,&saved,sizeof(plan))&&!memcmp(&caps,&savedcaps,sizeof(caps))&&!memcmp(before,f.master.values,sizeof(before))&&f.fast.calls==allocs);unchanged(&pi);}
 /* Known operations of other routes are outside this Paula lowering contract. */
 plan=original;plan.action[plan.count++]=(struct pt_render_action){PT_RENDER_REPEAT,15,{0},{0,0}};
 assert(pt_paula_readers_lower_begin(f.pool,f.queue,BOUNDARY_START,BOUNDARY_RATE,&plan,&caps,keys,&out)==PT_PAULA_READERS_OK&&out->count==4);
 memset(&plan,0xa5,sizeof(plan));caps.clock_hz=0; /* Input declarations are no longer borrowed after begin. */
 boundary_ready(&f,out);caps=boundary_caps;
 assert(pt_paula_readers_cancel(out)==PT_PAULA_READERS_OK&&pt_paula_readers_command_close(out));
 for(i=0;i<6;++i){unsigned steps=0,ready=0,limit=i==0?0:i==1?1:i==2?2:i==3?10:i==4?25:70;
   assert(pt_paula_readers_lower_begin(f.pool,f.queue,BOUNDARY_START,BOUNDARY_RATE,&original,&caps,keys,&out)==PT_PAULA_READERS_OK);
   while(steps++<limit&&!ready){enum pt_paula_readers_result r=pt_paula_readers_step(out,&ready);assert(r==PT_PAULA_READERS_OK||r==PT_PAULA_READERS_PENDING);}
   assert(pt_paula_readers_cancel(out)==PT_PAULA_READERS_OK&&pt_paula_readers_command_close(out));
   assert(!memcmp(before,f.master.values,sizeof(before))&&!pt_readers_commands_held(f.queue)&&!pt_readers_readers_held(f.queue));
 }
 finish(&f);
}
static void boundary_allocation_input_aliases(void)
{
 unsigned input,which;
 for(input=0;input<3;++input)for(which=1;which<=2;++which){
   struct fixture f;struct pt_render_plan original,*plan;struct pt_paula_render_caps *caps;
   struct pt_readers_key *keys;struct pt_paula_readers_command *out=(void *)(uintptr_t)1;
   unsigned live;size_t bytes=sizeof(struct pt_render_plan);void *arena,*saved;int32_t before[VALUES];
   if(bytes<sizeof(struct pt_paula_readers_command))bytes=sizeof(struct pt_paula_readers_command);
   if(bytes<sizeof(struct pt_paula_readers_reader))bytes=sizeof(struct pt_paula_readers_reader);
   if(bytes<PT_CHANNEL_LIMIT*sizeof(struct pt_readers_key))bytes=PT_CHANNEL_LIMIT*sizeof(struct pt_readers_key);
   boundary_init(&f,24);boundary_first_plan(&f,&original);memcpy(before,f.master.values,sizeof(before));
   arena=malloc(bytes);saved=malloc(bytes);assert(arena&&saved);memset(arena,0,bytes);
   plan=input==0?arena:&original;caps=input==1?arena:(struct pt_paula_render_caps *)&boundary_caps;
   keys=input==2?arena:NULL;if(input==0)memcpy(arena,&original,sizeof(original));if(input==1)memcpy(arena,&boundary_caps,sizeof(boundary_caps));
   memcpy(saved,arena,bytes);live=f.fast.live;boundary_borrow=arena;boundary_borrow_at=which;boundary_borrow_calls=boundary_borrow_releases=0;
   assert(pt_paula_readers_lower_begin(f.pool,f.queue,BOUNDARY_START,BOUNDARY_RATE,plan,caps,keys,&out)==PT_PAULA_READERS_INVALID);
   assert(out==(void *)(uintptr_t)1&&boundary_borrow_releases==1&&f.fast.live==live&&!memcmp(saved,arena,bytes));
   assert(!memcmp(before,f.master.values,sizeof(before))&&!pt_readers_commands_held(f.queue)&&!pt_readers_readers_held(f.queue));
   boundary_borrow=NULL;free(arena);free(saved);finish(&f);
 }
}
int main(void)
{
 boundary_full_song(8);boundary_full_song(16);boundary_full_song(24);
 boundary_refusals();boundary_allocation_input_aliases();
 puts("PAULA READERS BOUNDARY PASS: exact renderer plans, folded triggers and independently retained readers; software ownership only");return 0;
}
