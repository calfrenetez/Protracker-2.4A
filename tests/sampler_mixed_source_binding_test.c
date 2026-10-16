/* Real factory layout once, with the unchanged genuine factory constructor
 * suite and typed backend. Never manufacture a holder, pin, lease or receipt. */
#include "../src/editor/sampler_mixed_readers.c"
#define PT_MIXED_READERS_TEST_MAIN mixed_binding_donor_main
#define PT_SMF_FACTORY_COMPLETION "SAMPLER MIXED FACTORY PASS:complete original constructor assertions;bounded completion marker;SOFTWARE_ONLY"
#include "sampler_mixed_readers_test.c"
static uint64_t mixed_other_key(struct pt_sampler_mixed_reader *r,unsigned fault)
{
 struct pt_sample_cache cache;struct pt_cache_lease lease;
 struct pt_playback_format format=r->route==PT_MIXED_READERS_PAULA?
   (struct pt_playback_format){8,r->channel,0,1}:r->format;
 uint32_t identity=r->sample+1;uint64_t key;unsigned count=0;
 if(fault==1)++identity;
 else if(fault==2)format.bits=format.bits==8?16:8;
 else if(fault==3)format.word_pad^=1;
 else if(fault==4)format.little_endian^=1;
 else {assert(fault==5&&r->pcm.channels==2);format.channel^=1;}
 pt_cache_init(&cache,&count,chip_new,chip_drop,4096);
 assert(pt_playback_pcm_acquire(&cache,&r->pcm,identity,r->route==PT_MIXED_READERS_PAULA?
   r->pool->paula.version:r->pool->wave.version,&format,&lease)==PT_CACHE_LOAD);
 key=cache.entry[lease.slot].key;
 assert(pt_cache_unpin(&cache,lease)&&pt_cache_clear(&cache)&&!count);return key;
}
static void mixed_binding_case(unsigned bits,unsigned channels,unsigned card,unsigned cache_bits,
 unsigned little,unsigned fault,unsigned phase)
{
 struct smf_trial *f=smf_make(bits,cache_bits,channels==2,6,0);
 struct resources *v=f->inherited->resources;struct pt_sampler_mixed_command *c;
 struct pt_sampler_mixed_reader *r;struct pt_sample_cache *cache;
 struct pt_cache_entry entry;struct model model;uint8_t ram[4096],chip[8];
 int32_t masters[2][64];uint64_t ticket=0;unsigned done=999,steps=0,writes,calls,chipcalls;
 smf_open(f);smf_requests(f,card?5:1,little);
 if(card)f->request[0]=f->request[4];
 f->count=1;f->request[0].channel=channels-1;
 if(phase==0){
   assert(pt_sampler_mixed_begin(f->pool,f->revision,960,f->request,1,f->command)==PT_SAMPLER_MIXED_PENDING);
   c=f->command[0].address;r=c->source[0].reader;
   while(c->phase!=ACTION){
     assert(pt_sampler_mixed_advance(f->pool,f->revision,f->command[0],&done)==PT_SAMPLER_MIXED_PENDING);
     assert(++steps<100&&r->state!=READY);
   }
 }else{smf_prepare(f,0,960);c=f->command[0].address;r=c->source[0].reader;}
 smf_handles(f,0,0);
 if(phase==2){
   assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&ticket)==PT_MIXED_READERS_OK);
   assert(pt_sampler_mixed_publish(f->pool,ticket)==PT_MIXED_READERS_OK);
   fire(&f->inherited->model,ticket);
   assert(pt_sampler_mixed_service_reader(f->pool,ticket,0,0,NULL)==PT_MIXED_READERS_PENDING);
   assert(pt_sampler_mixed_reader_readiness(f->pool,f->reader[0])==PT_MIXED_READERS_OK);
 }
 cache=card?&v->card.cache.cache:&f->pool->paula.cache;
 if(fault){uint64_t key=mixed_other_key(r,fault);
   assert(key!=cache->entry[r->lease.slot].key);cache->entry[r->lease.slot].key=key;}
 entry=cache->entry[r->lease.slot];model=f->inherited->model;writes=v->card.writes;
 calls=f->ordinary.calls;chipcalls=f->chip.calls;
 memcpy(ram,v->card.ram,sizeof(ram));
 memcpy(masters[0],f->pcm[0].data,6*channels*sizeof(int32_t));
 memcpy(masters[1],f->pcm[1].data,6*channels*sizeof(int32_t));
 if(!card){assert(entry.bytes<=sizeof(chip));memcpy(chip,entry.data,entry.bytes);}
 if(phase==0){
   done=999;assert(pt_sampler_mixed_advance(f->pool,f->revision,f->command[0],&done)==
     (fault?PT_SAMPLER_MIXED_STALE:PT_SAMPLER_MIXED_OK));
   if(fault){assert(done==999&&r->released&&!r->leased&&!r->pin&&!cache->entry[r->lease.slot].pins);}
   else assert(done==1);
 }else if(phase==1){
   ticket=999;assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&ticket)==
     (fault?PT_MIXED_READERS_STALE:PT_MIXED_READERS_OK));
   if(fault)assert(ticket==999);
 }else assert(pt_sampler_mixed_reader_readiness(f->pool,f->reader[0])==
     (fault?PT_MIXED_READERS_STALE:PT_MIXED_READERS_OK));
 if(fault||phase!=1)assert(!memcmp(&model,&f->inherited->model,sizeof(model)));
 assert(writes==v->card.writes&&calls==f->ordinary.calls&&chipcalls==f->chip.calls);
 assert(!memcmp(ram,v->card.ram,sizeof(ram)));
 assert(!memcmp(masters[0],f->pcm[0].data,6*channels*sizeof(int32_t)));
 assert(!memcmp(masters[1],f->pcm[1].data,6*channels*sizeof(int32_t)));
 if(!card)assert(!memcmp(chip,entry.data,entry.bytes));
 if(phase!=0||!fault){
   assert(!memcmp(&entry,cache->entry+r->lease.slot,sizeof(entry))&&r->pin&&r->leased&&!r->released);
 }
 /* Refusal is not retirement. A live ticket drains through the genuine
  * independent command and reader proof domains with its corrupted key intact. */
 if(phase==2||(phase==1&&!fault)){
   if(phase==1){
     assert(pt_sampler_mixed_publish(f->pool,ticket)==PT_MIXED_READERS_OK);
     fire(&f->inherited->model,ticket);
     assert(pt_sampler_mixed_service_reader(f->pool,ticket,0,0,NULL)==PT_MIXED_READERS_PENDING);
   }
   assert(!pt_sampler_mixed_reader_close(f->pool,f->reader));
   assert(pt_sampler_mixed_service_command(f->pool,ticket,1,NULL)==
     (fault?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK));
   assert(r->pin&&r->leased&&!r->released);
   assert(pt_sampler_mixed_service_reader(f->pool,ticket,0,1,NULL)==
     (fault?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK));
   assert(r->released&&!r->pin&&!r->leased);
 }
 if(phase==2||(phase==1&&!fault))smf_close_handles(f,0,0,1);
 else{
   int closed;
   assert(pt_sampler_mixed_cancel(f->pool,f->command[0])==PT_SAMPLER_MIXED_OK);
   closed=pt_sampler_mixed_reader_close(f->pool,f->reader);
   assert(!f->reader[0].address&&!f->reader[0].token&&closed==!(phase==0&&fault));
   closed=pt_sampler_mixed_command_close(f->pool,f->command);
   assert(!f->command[0].address&&!f->command[0].token&&closed==!(phase==0&&fault));
 }
 smf_drop(f);
}
int main(void)
{
 unsigned bits,channels,card,cache_bits,little,fault,phase,count=0;
 for(bits=8;bits<=24;bits+=8)for(channels=1;channels<=2;++channels)for(card=0;card<2;++card)
   for(cache_bits=8;cache_bits<=(card?16U:8U);cache_bits+=8)
     for(little=0;little<(card?2U:1U);++little)
       for(fault=0;fault<(channels==2?6U:5U);++fault)for(phase=0;phase<3;++phase){
         mixed_binding_case(bits,channels,card,cache_bits,little,fault,phase);++count;}
 assert(count==495);
 puts("MIXED SOURCE BINDING PASS:495 cases;90 controls405 refusals;Paula signed8 and AmiGUS8/16 key binding before action/enqueue/active readiness;master and bus bytes preserved;independent retirement;SOFTWARE_ONLY");
 return 0;
}
