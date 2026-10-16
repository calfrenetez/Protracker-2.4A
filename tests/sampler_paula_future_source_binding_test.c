/* One genuine production inclusion; substitute the standalone future unit in
 * the driver. Reuse original setup/backend/cleanup without running its main. */
#include "../src/editor/sampler_paula_future.c"
#define main future_binding_donor_main
#include "sampler_paula_future_test.c"
#undef main
static uint64_t future_other_key(struct fixture *f,struct source *s,unsigned fault)
{
 struct pt_sample_cache cache;struct pt_cache_lease lease;
 struct pt_playback_format format={8,s->request.channel,0,1};uint32_t identity=s->request.sample+1;uint64_t key;
 if(fault==1)++identity;
 else if(fault==2)format.bits=16;
 else if(fault==3)format.word_pad=0;
 else if(fault==4)format.little_endian=1;
 else {assert(fault==5&&s->pcm.channels==2);format.channel^=1;}
 pt_cache_init(&cache,&f->chip,chip_allocate,chip_release,32768);
 assert(pt_playback_pcm_acquire(&cache,&s->pcm,identity,f->pool->bridge.version,&format,&lease)==PT_CACHE_LOAD);
 key=cache.entry[lease.slot].key;assert(key!=f->pool->bridge.cache.entry[s->lease.slot].key);
 assert(pt_cache_unpin(&cache,lease)&&pt_cache_clear(&cache));return key;
}
static void future_binding_case(unsigned bits,unsigned channels,unsigned fault,unsigned live)
{
 struct fixture f;struct pt_paula_future_owner *o;struct pt_paula_future_view view,saved;
 struct pt_scheduled_batch b;struct source *s;struct backend backend;struct pt_cache_entry entry;
 int32_t original[VALUES];uint8_t chip[8194];uint64_t ticket=777;unsigned calls;
 init(&f,bits,channels,2,16388);o=begin(&f,10,0,0,channels-1);prepare(&f,o);
 assert(pt_paula_future_view(o,0,&view)==PT_FUTURE_OK);b=batch(view,10);s=o->source;
 if(live){assert(pt_paula_future_enqueue(o,&b,&ticket)==PT_SCHEDULED_OK);
   assert(pt_scheduled_output_publish(f.queue,ticket)==PT_SCHEDULED_OK);}
 if(fault)f.pool->bridge.cache.entry[s->lease.slot].key=future_other_key(&f,s,fault);
 entry=f.pool->bridge.cache.entry[s->lease.slot];backend=f.backend;calls=f.fast.calls;
 memcpy(original,f.master.values,sizeof(original));assert(s->bytes==sizeof(chip));memcpy(chip,s->data,sizeof(chip));
 memset(&view,0x6d,sizeof(view));saved=view;
 assert(pt_paula_future_view(o,0,&view)==(fault?PT_FUTURE_STALE:PT_FUTURE_OK));
 if(fault){assert(!memcmp(&view,&saved,sizeof(view)));
   if(!live){uint64_t untouched=999;assert(pt_paula_future_enqueue(o,&b,&untouched)==PT_SCHEDULED_STALE&&untouched==999);}}
 else assert(view.data==s->data&&view.bytes==s->bytes);
 assert(!memcmp(&backend,&f.backend,sizeof(backend))&&calls==f.fast.calls);
 assert(!memcmp(&entry,f.pool->bridge.cache.entry+s->lease.slot,sizeof(entry)));
 assert(!memcmp(original,f.master.values,sizeof(original))&&!memcmp(chip,s->data,sizeof(chip)));
 assert(s->pin&&s->leased&&!o->released&&f.chip.live==1);
 if(live){assert(!pt_paula_future_owner_close(o)&&f.chip.live==1);f.backend.poll=1;
   assert(pt_scheduled_output_poll(f.queue,ticket)==PT_SCHEDULED_OK);
   assert(o->released&&!s->pin&&!s->leased&&!f.pool->bridge.cache.entry[s->lease.slot].pins);}
 assert(pt_paula_future_owner_close(o));finish(&f);
}
int main(void)
{
 unsigned bits,channels,fault,live,count=0;
 for(bits=8;bits<=24;bits+=8)for(channels=1;channels<=2;++channels)
   for(fault=0;fault<(channels==2?6U:5U);++fault)for(live=0;live<2;++live){
     future_binding_case(bits,channels,fault,live);++count;}
 assert(count==66);
 puts("FUTURE SOURCE BINDING PASS:66 cases;12 controls54 refusals;ready/live outputs preserved,no dispatch,pins retained until independent retirement,zero owned bytes");
 return 0;
}
