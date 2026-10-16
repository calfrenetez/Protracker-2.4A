/* Include the production owner exactly once, then reuse the genuine original
 * fixture and typed backend without executing its separate main. */
#include "../src/editor/sampler_paula_lineage.c"
#define main lineage_binding_donor_main
#include "sampler_paula_lineage_test.c"
#undef main
static uint64_t lineage_other_key(struct fixture *f,struct source *s,unsigned fault)
{
 struct pt_sample_cache cache;struct pt_cache_lease lease;
 struct pt_playback_format format={8,s->request.channel,0,1};
 uint32_t identity=s->request.sample+1;uint64_t key;
 if(fault==1)++identity;
 else if(fault==2)format.bits=16;
 else if(fault==3)format.word_pad=0;
 else if(fault==4)format.little_endian=1;
 else {assert(fault==5&&s->pcm.channels==2);format.channel^=1;}
 pt_cache_init(&cache,&f->chip,chip_allocate,chip_release,32768);
 assert(pt_playback_pcm_acquire(&cache,&s->pcm,identity,f->pool->bridge.version,&format,&lease)==PT_CACHE_LOAD);
 key=cache.entry[lease.slot].key;
 assert(key!=f->pool->bridge.cache.entry[s->lease.slot].key);
 assert(pt_cache_unpin(&cache,lease)&&pt_cache_clear(&cache));return key;
}
static void lineage_binding_case(unsigned bits,unsigned channels,unsigned fault,unsigned kind,unsigned live)
{
 struct fixture f;struct pt_paula_lineage_owner *origin,*o;
 struct pt_paula_lineage_view view,saved;struct pt_lineage_key key;
 struct pt_scheduled_batch b;struct pt_lineage_receipt receipt;
 struct source *s;struct backend backend;struct pt_cache_entry entry;
 int32_t original[VALUES],current[VALUES];uint8_t chip[8194];uint64_t ticket=0,origin_ticket=0;
 unsigned fastcalls,chipcalls;uint64_t frame=10;
 enum pt_scheduled_kind action=kind==0?PT_SCHEDULED_TRIGGER:kind==1?PT_SCHEDULED_CONTROL:PT_SCHEDULED_STOP;
 init(&f,bits,channels,2,16388);origin=begin(&f,10,0,0,channels-1);
 prepare(&f,origin);assert(pt_paula_lineage_view(origin,0,&view)==PT_PAULA_LINEAGE_OK);o=origin;
 if(kind||live){
   origin_ticket=enqueue(&f,origin,view,10,PT_SCHEDULED_TRIGGER);
   f.backend.now=record(&f.backend,origin_ticket)->event.scheduled.first;
   assert(fire(&f.backend,origin_ticket));
   assert(pt_lineage_poll(f.queue,origin_ticket,&receipt)==PT_SCHEDULED_PENDING);
 }
 if(kind){
   assert(pt_paula_lineage_reader_key(origin,0,&key)==PT_SCHEDULED_OK);
   frame=20;o=command(&f,origin,view,key,frame,action);prepare(&f,o);
   assert(pt_paula_lineage_view(o,0,&view)==PT_PAULA_LINEAGE_OK);
   if(live){ticket=enqueue(&f,o,view,frame,action);
     f.backend.now=record(&f.backend,ticket)->event.scheduled.first;
     assert(fire(&f.backend,ticket));
     assert(pt_lineage_poll(f.queue,ticket,&receipt)==PT_SCHEDULED_PENDING);}
 }else if(live)ticket=origin_ticket;
 b=batch(view,frame,action);s=o->source;
 if(fault)f.pool->bridge.cache.entry[s->lease.slot].key=lineage_other_key(&f,s,fault);
 entry=f.pool->bridge.cache.entry[s->lease.slot];backend=f.backend;
 fastcalls=f.fast.calls;chipcalls=f.chip.calls;
 memcpy(original,f.master.values,sizeof(original));
 memcpy(current,s->pcm.data,(size_t)s->pcm.frames*s->pcm.channels*sizeof(*current));
 assert(s->bytes==sizeof(chip));memcpy(chip,s->data,sizeof(chip));
 memset(&view,0x6d,sizeof(view));saved=view;
 assert(pt_paula_lineage_view(o,0,&view)==(fault?PT_PAULA_LINEAGE_STALE:PT_PAULA_LINEAGE_OK));
 if(fault){
   assert(!memcmp(&view,&saved,sizeof(view)));
   if(!live){uint64_t untouched=999;
     assert(pt_paula_lineage_enqueue(o,&b,&untouched)==PT_SCHEDULED_STALE&&untouched==999);}
 }else assert(view.data==s->data&&view.bytes==s->bytes);
 assert(!memcmp(&backend,&f.backend,sizeof(backend))&&fastcalls==f.fast.calls&&chipcalls==f.chip.calls);
 assert(!memcmp(&entry,f.pool->bridge.cache.entry+s->lease.slot,sizeof(entry)));
 assert(!memcmp(original,f.master.values,sizeof(original))&&!memcmp(chip,s->data,sizeof(chip)));
 assert(!memcmp(current,s->pcm.data,(size_t)s->pcm.frames*s->pcm.channels*sizeof(*current)));
 assert(s->pin&&s->leased&&!o->released&&f.chip.live==1);
 /* Cache rejection never asserts reader stop. Only the genuine independent
  * backend retirement releases each exact ticket; no key/pin/serial repair. */
 if(live){
   assert(!pt_paula_lineage_owner_close(o)&&f.chip.live==1);
   drain(&f.backend);retire(&f.backend,ticket);
   assert(pt_lineage_poll(f.queue,ticket,&receipt)==PT_SCHEDULED_OK);
   assert(o->released&&!s->pin&&!s->leased);
 }
 assert(pt_paula_lineage_owner_close(o));
 if(kind){
   assert(origin->source[0].pin&&origin->source[0].leased&&!origin->released);
   assert(f.pool->bridge.cache.entry[origin->source[0].lease.slot].pins==1);
   drain(&f.backend);collected(&f,origin,origin_ticket);
 }
 finish(&f);
}
int main(void)
{
 unsigned bits,channels,fault,kind,live,count=0;
 for(bits=8;bits<=24;bits+=8)for(channels=1;channels<=2;++channels)
   for(fault=0;fault<(channels==2?6U:5U);++fault)for(kind=0;kind<3;++kind)for(live=0;live<2;++live){
     lineage_binding_case(bits,channels,fault,kind,live);++count;}
 assert(count==198);
 puts("LINEAGE SOURCE BINDING PASS:198 cases;36 controls162 refusals;trigger/control/stop ready/active outputs preserved,no new dispatch,independent ticket retirement,zero owned bytes");
 return 0;
}
