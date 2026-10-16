/* Exercise genuine direct/public/incremental starts and old pinned copies. */
#define PT_TEST_DISPATCH_EXEC
#include "paula_dispatch_test.c"
#include "../src/editor/paula_internal.h"
#include "../src/core/playback_internal.h"
static uint64_t active_other_key(const struct pt_pcm *pcm,uint64_t version,unsigned channel,unsigned fault)
{
 struct pt_sample_cache donor;struct pt_cache_lease lease;struct driver d={0};
 struct pt_playback_format format={8,channel,0,1};uint32_t identity=1;uint64_t key;
 if(fault==1)++identity;
 else if(fault==2)format.bits=16;
 else if(fault==3)format.word_pad=0;
 else if(fault==4)format.little_endian=1;
 else {assert(fault==5&&pcm->channels==2);format.channel^=1;}
 pt_cache_init(&donor,&d,chip_alloc,chip_free,64);
 assert(pt_playback_pcm_acquire(&donor,pcm,identity,version,&format,&lease)==PT_CACHE_LOAD);
 key=donor.entry[lease.slot].key;
 assert(pt_cache_unpin(&donor,lease)&&pt_cache_clear(&donor)&&!d.live);return key;
}
static void active_binding_case(unsigned bits,unsigned channels,unsigned side,unsigned activation,unsigned retired,unsigned fault)
{
 struct pt_allocator alloc={NULL,fast_alloc,fast_free};struct pt_document doc;struct pt_sampler sampler;
 struct pt_sampler_paula cache={0};struct pt_paula_voices voices={0},before;
 struct driver d={0},driver;struct pt_paula_voice_api api={&d,start,stop,control};
 struct pt_paula_render_caps caps={3546895,124,65535};struct pt_paula_voice_request request={0,4,428,64};
 struct pt_render_plan plan={0};struct pt_paula_prepared *p=calloc(1,sizeof(*p));
 struct pt_pcm pcm;struct pt_sample_version *pin;struct pt_cache_entry entry;struct pt_paula_voice voice;
 struct pt_pattern_history history;struct pt_pattern_command commands[4];struct pt_event_change changes[4];
 int32_t values[8]={127,-128,3,-3,1,2,4,5},master[8];uint8_t bytes[4];
 unsigned i,token=17,steps=0;enum pt_cache_result loaded;int result;size_t master_bytes;
 assert(p&&(!activation||(channels==1&&!side))&&(!retired||activation!=2));
 d.start_result=d.control_result=d.quiesce_result=1;for(i=0;i<4;++i)d.stop_result[i]=1;
 pt_document_init(&doc,&alloc);assert(pt_document_new(&doc,16,SIZE_MAX)==PT_PROJECT_OK);
 for(i=0;i<16;++i)doc.project.channels.track[i].route=PT_AMIGUS;
 doc.project.channels.track[4].route=PT_PAULA;
 doc.project.samples[0].pcm=(struct pt_pcm){values,8,4,8000,(uint8_t)channels,(uint8_t)bits};
 pt_sampler_init(&sampler,&alloc,1024*1024);
 assert(pt_sampler_pin(&sampler,&doc.project,0,sampler.generation,&pcm,&pin)==PT_EDIT_OK);
 assert(pt_pattern_history_init(&history,&doc.project,commands,4,changes,4)==PT_EDIT_OK);
 assert(pt_sampler_paula_bind(&cache,&sampler,&doc.project,&d,chip_alloc,chip_free,64));
 assert(pt_paula_voices_bind(&voices,&cache,&api));
 if(!activation)assert(pt_paula_voices_trigger(&voices,4,0,side,&request)==PT_PAULA_VOICE_ACTIVE);
 else {
  plan.count=1;action(plan.action,&doc.project.samples[0].pcm,4,0);
  if(activation==1){assert(pt_paula_prepare(p,&voices,cache.version,48000,&plan,&caps));assert(pt_paula_apply(p)==1);}
  else {
   voices.song_owner=&token;
   assert(pt_paula_prepare_begin_owned(p,&voices,cache.version,48000,&plan,&caps,sampler.current,&token));
   do{assert(++steps<20);loaded=pt_paula_prepare_step_owned(p);}while(loaded==PT_CACHE_PENDING);
   assert(loaded==PT_CACHE_LOAD&&pt_paula_prepared_ready_owned(p));
   assert(pt_paula_prepared_action_owned(p,0)==1);pt_paula_cancel(p);
  }
 }
 assert(voices.voice[0].held&&!voices.voice[0].uncertain&&d.reading[0]&&d.starts==1);
 if(retired){
  assert(pt_sampler_edit(&sampler,&doc.project,&history,0,PT_PCM_REVERSE,0,4,0)==PT_EDIT_OK);
  assert(pt_paula_voices_sync(&voices)==1);
  assert(cache.cache.entry[voices.voice[0].lease.slot].valid==2);
 }
 if(activation==2){
  plan.action[0].kind=PT_RENDER_CONTROL;plan.action[0].gain[0]=65536;
  assert(pt_paula_prepare_owned(p,&voices,cache.version,48000,&plan,&caps,&token));
 }
 if(fault){struct pt_cache_entry *e=&cache.cache.entry[voices.voice[0].lease.slot];
  uint64_t key=active_other_key(&pcm,e->version,side,fault);assert(key!=e->key);e->key=key;}
 entry=cache.cache.entry[voices.voice[0].lease.slot];before=voices;driver=d;
 master_bytes=doc.project.samples[0].pcm.capacity*sizeof(int32_t);assert(master_bytes<=sizeof(master));
 memcpy(bytes,entry.data,sizeof(bytes));memcpy(master,doc.project.samples[0].pcm.data,master_bytes);
 if(activation==2)result=pt_paula_apply(p);
 else result=pt_paula_voices_control(&voices,4,214,32);
 if(result!=(fault?(activation==2?0:PT_PAULA_VOICE_REFUSED):1))
  fprintf(stderr,"active binding mismatch bits=%u channels=%u side=%u activation=%u retired=%u fault=%u result=%d\n",bits,channels,side,activation,retired,fault,result);
 assert(result==(fault?(activation==2?0:PT_PAULA_VOICE_REFUSED):1));
 assert(!memcmp(&before,&voices,sizeof(voices)));
 if(fault)assert(!memcmp(&driver,&d,sizeof(d)));
 else assert(d.controls==driver.controls+1&&d.starts==driver.starts&&d.stops==driver.stops&&d.calls==driver.calls);
 assert(!memcmp(&entry,&cache.cache.entry[voices.voice[0].lease.slot],sizeof(entry)));
 assert(!memcmp(bytes,entry.data,sizeof(bytes))&&!memcmp(master,doc.project.samples[0].pcm.data,master_bytes));
 assert(doc.project.samples[0].pcm.bits==bits);
 /* Corruption is not a stop receipt. Positive driver confirmation alone ends
  * the old cache lifetime, including a genuinely invalidated published copy. */
 voice=voices.voice[0];d.stop_result[0]=0;
 assert(!pt_paula_close_owned(&voices,activation==2?&token:NULL)&&d.reading[0]);
 assert(!memcmp(&voice,&voices.voice[0],sizeof(voice))&&pt_cache_data(&cache.cache,voice.lease)==entry.data);
 d.stop_result[0]=-1;assert(!pt_paula_close_owned(&voices,activation==2?&token:NULL)&&d.reading[0]);
 assert(!memcmp(&voice,&voices.voice[0],sizeof(voice))&&pt_cache_data(&cache.cache,voice.lease)==entry.data);
 d.stop_result[0]=1;assert(pt_paula_close_owned(&voices,activation==2?&token:NULL)&&!d.live&&!d.reading[0]);
 pt_sampler_unpin(pin);pt_pattern_history_release(&history);pt_sampler_release(&sampler);pt_document_release(&doc);free(p);
}
int main(void)
{
 unsigned bits,channels,side,activation,retired,fault,count=0;
 for(bits=8;bits<=24;bits+=8)for(activation=0;activation<3;++activation)
  for(channels=1;channels<=(activation?1U:2U);++channels)for(side=0;side<channels;++side)
   for(retired=0;retired<(activation==2?1U:2U);++retired)
    for(fault=0;fault<(channels==2?6U:5U);++fault){
     active_binding_case(bits,channels,side,activation,retired,fault);++count;}
 assert(count==147);
 puts("PAULA ACTIVE SOURCE BINDING PASS:147 cases;27 controls120 refusals;direct/public/incremental starts,original8/16/24 mono/stereo sources,invalidated pinned copies,confirmed stop-only release");
 return 0;
}
