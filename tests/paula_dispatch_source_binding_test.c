/* Genuine prepared trigger, cache and driver ownership; no native output. */
#define PT_TEST_DISPATCH_EXEC
#include "paula_dispatch_test.c"
#include "../src/editor/paula_internal.h"
#include "../src/core/playback_internal.h"
static uint64_t dispatch_other_key(struct pt_sampler_paula *bridge,unsigned sample,unsigned fault)
{
 struct pt_sample_cache donor;struct pt_cache_lease lease;struct driver d={0};
 struct pt_playback_format format={8,0,0,1};uint32_t identity=sample+1;uint64_t key;
 if(fault==1)++identity;
 else if(fault==2)format.bits=16;
 else if(fault==3)format.word_pad=0;
 else {assert(fault==4);format.little_endian=1;}
 pt_cache_init(&donor,&d,chip_alloc,chip_free,64);
 assert(pt_playback_pcm_acquire(&donor,&bridge->project->samples[sample].pcm,identity,
   bridge->version,&format,&lease)==PT_CACHE_LOAD);
 key=donor.entry[lease.slot].key;
 assert(pt_cache_unpin(&donor,lease)&&pt_cache_clear(&donor)&&!d.live);return key;
}
static void dispatch_binding_case(unsigned bits,unsigned fault,unsigned boundary)
{
 struct pt_allocator alloc={NULL,fast_alloc,fast_free};struct pt_document doc;
 struct pt_sampler sampler;struct pt_sampler_paula cache={0};struct pt_paula_voices owner={0};
 struct driver d={0},before;struct pt_paula_voice_api api={&d,start,stop,control};
 struct pt_paula_render_caps caps={3546895,124,65535};struct pt_render_plan plan={0};
 struct pt_paula_prepared *p=calloc(1,sizeof(*p));struct pt_pcm held;struct pt_sample_version *pin;
 struct pt_cache_entry entry;enum pt_cache_result result;unsigned i,steps=0,token=17;
 int32_t values[4]={1,2,3,4},master[4];uint8_t bytes[4];
 assert(p);d.start_result=d.control_result=d.quiesce_result=1;
 for(i=0;i<4;++i)d.stop_result[i]=1;
 pt_document_init(&doc,&alloc);assert(pt_document_new(&doc,16,SIZE_MAX)==PT_PROJECT_OK);
 for(i=0;i<16;++i)doc.project.channels.track[i].route=PT_AMIGUS;
 doc.project.channels.track[4].route=PT_PAULA;
 doc.project.samples[0].pcm=(struct pt_pcm){values,4,4,8000,1,(uint8_t)bits};
 pt_sampler_init(&sampler,&alloc,1024*1024);
 assert(pt_sampler_pin(&sampler,&doc.project,0,sampler.generation,&held,&pin)==PT_EDIT_OK);
 assert(pt_sampler_paula_bind(&cache,&sampler,&doc.project,&d,chip_alloc,chip_free,64));
 assert(pt_paula_voices_bind(&owner,&cache,&api));
 plan.count=1;action(&plan.action[0],&doc.project.samples[0].pcm,4,0);
 if(boundary==3)assert(pt_paula_prepare(p,&owner,cache.version,48000,&plan,&caps));
 else {
  owner.song_owner=&token;
  assert(pt_paula_prepare_begin_owned(p,&owner,cache.version,48000,&plan,&caps,sampler.current,&token));
  while(p->index!=plan.count){assert(++steps<20&&pt_paula_prepare_step_owned(p)==PT_CACHE_PENDING);}
  if(boundary){assert(pt_paula_prepare_step_owned(p)==PT_CACHE_LOAD);assert(pt_paula_prepared_ready_owned(p));}
 }
 assert(p->batch.entry[0].held&&p->batch.entry[0].sample==0);
 if(fault){uint64_t key=dispatch_other_key(&cache,0,fault);
  assert(key!=cache.cache.entry[p->batch.entry[0].lease.slot].key);
  cache.cache.entry[p->batch.entry[0].lease.slot].key=key;}
 entry=cache.cache.entry[p->batch.entry[0].lease.slot];before=d;
 memcpy(master,doc.project.samples[0].pcm.data,sizeof(master));memcpy(bytes,entry.data,sizeof(bytes));
 if(boundary==0){result=pt_paula_prepare_step_owned(p);
  if(result!=(fault?PT_CACHE_INVALID:PT_CACHE_LOAD))fprintf(stderr,"dispatch binding mismatch bits=%u fault=%u boundary=%u result=%u\n",bits,fault,boundary,(unsigned)result);
  assert(result==(fault?PT_CACHE_INVALID:PT_CACHE_LOAD));}
 else if(boundary==1)assert(pt_paula_prepared_ready_owned(p)==!fault);
 else assert(pt_paula_apply(p)==!fault);
 if(boundary<2||fault)assert(!memcmp(&before,&d,sizeof(d)));
 assert(!memcmp(master,doc.project.samples[0].pcm.data,sizeof(master)));
 assert(!memcmp(bytes,entry.data,sizeof(bytes))&&doc.project.samples[0].pcm.bits==bits);
 assert(cache.cache.entry[p->batch.entry[0].lease.slot].key==entry.key&&cache.cache.entry[p->batch.entry[0].lease.slot].serial==entry.serial);
 if(boundary==1)assert(!memcmp(&entry,&cache.cache.entry[p->batch.entry[0].lease.slot],sizeof(entry)));
 /* Readiness refusal is pure. Cancel only unstarted leases; a real successful
  * start is stopped through its driver before any cache release. No key repair. */
 pt_paula_cancel(p);
 if(boundary!=3)assert(owner.song_owner==&token);
 else assert(!owner.song_owner);
 owner.song_owner=NULL;
 assert(pt_paula_voices_close(&owner)&&!d.live);
 pt_sampler_unpin(pin);pt_sampler_release(&sampler);pt_document_release(&doc);free(p);
}
int main(void)
{
 unsigned bits,fault,boundary,count=0;
 for(bits=8;bits<=24;bits+=8)for(fault=0;fault<5;++fault)for(boundary=0;boundary<4;++boundary){
  dispatch_binding_case(bits,fault,boundary);++count;}
 assert(count==60);
 puts("PAULA DISPATCH SOURCE BINDING PASS:60 cases;12 controls48 source/representation refusals;final preparation,ready/private/public apply;masters preserved,no refused output,genuine cleanup");
 return 0;
}
