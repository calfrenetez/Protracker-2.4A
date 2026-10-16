/* Reuse the genuine reader/core/backend fixture, but execute only this bounded
 * source-binding regression. The original full workload remains independent. */
#define main readers_binding_donor_main
#include "sampler_paula_readers_test.c"
#undef main

static void binding_init(struct fixture *f,unsigned bits,unsigned channels)
{
 struct pt_paula_readers_config c;struct pt_scheduled_grid grid={100,7,1000,100};
 struct pt_readers_backend b;unsigned i;memset(f,0,sizeof(*f));
#ifdef PT_READERS_NATIVE_MEMORY
 f->chip.chip=1;
#endif
 f->allocator=(struct pt_allocator){&f->fast,allocate,release};
 pt_document_init(&f->document,&f->allocator);assert(pt_document_new(&f->document,4,SIZE_MAX)==PT_PROJECT_OK);
 for(i=0;i<VALUES;++i)f->master.values[i]=((int32_t)(i%200)-100)*(1L<<(bits-8));
 f->document.project.samples[0].pcm=(struct pt_pcm){f->master.values,VALUES,32,48000,(uint8_t)channels,(uint8_t)bits};
 f->document.project.samples[1]=f->document.project.samples[0];
 pt_sampler_init(&f->sampler,&f->allocator,1024*1024);
 c=(struct pt_paula_readers_config){2,2,1024*1024,64,7,&f->chip,chip_allocate,chip_release};
 assert(pt_paula_readers_open(&f->allocator,&f->sampler,&f->document.project,&c,&f->pool)==PT_PAULA_READERS_OK);
 f->backend.pool=f->pool;f->backend.now=100;
 b=(struct pt_readers_backend){&f->backend,sizeof(f->backend),{7,8,4},PT_READERS_VERSION,3,2,clock_read,submit,poll_command,cancel_command,poll_reader,cancel_reader};
 assert(pt_readers_open(&f->allocator,&grid,1001,&b,2,2,&f->queue)==PT_SCHEDULED_OK);
}
/* Obtain an actual different representation key through the production packer.
 * Never duplicate its encoding or alter a pin/serial/version/resource counter.
 * Deliberate key corruption violates cache immutability; cleanup must still
 * retain the original live resource until independent reader retirement. */
static uint64_t binding_other_key(struct fixture *f,struct pt_paula_readers_reader *r,unsigned fault)
{
 struct pt_sample_cache cache;struct pt_cache_lease lease;
 struct pt_playback_format format={8,r->channel,0,1};uint32_t identity=r->sample+1;uint64_t key;
 if(fault==1)++identity;
 else if(fault==2)format.bits=16;
 else if(fault==3)format.word_pad=0;
 else if(fault==4)format.little_endian=1;
 else {assert(fault==5&&r->pcm.channels==2);format.channel^=1;}
 pt_cache_init(&cache,&f->chip,chip_allocate,chip_release,128);
 assert(pt_playback_pcm_acquire(&cache,&r->pcm,identity,f->pool->bridge.version,&format,&lease)==PT_CACHE_LOAD);
 key=cache.entry[lease.slot].key;
 assert(key!=f->pool->bridge.cache.entry[r->lease.slot].key);
 assert(pt_cache_unpin(&cache,lease)&&pt_cache_clear(&cache));return key;
}
static void binding_case(unsigned bits,unsigned channels,unsigned fault,unsigned active_reader)
{
 struct fixture f;struct pt_paula_readers_command *c,*refused=(void *)1;
 struct pt_paula_readers_reader *r;struct pt_paula_readers_view view,saved;
 struct pt_readers_key key;struct pt_scheduled_batch b;struct backend backend;
 struct pt_cache_entry entry;struct pt_paula_readers_request request;
 int32_t master[VALUES],held[64];uint8_t chip[32];uint64_t ticket=777;unsigned calls;
 binding_init(&f,bits,channels);c=begin(&f,10,PT_SCHEDULED_TRIGGER,0,0,channels-1,NULL);
 prepare(&f,c);b=batch(c);r=c->source[0].reader;assert(r&&r->bytes==sizeof(chip));
 if(active_reader){uint64_t t=enqueue(c);r=reader_handle(c,0);observe_fire(&f,t);key=active_key(r);
   detach_collect(&f,t);assert(pt_paula_readers_command_close(c));c=NULL;}
 if(fault)f.pool->bridge.cache.entry[r->lease.slot].key=binding_other_key(&f,r,fault);
 entry=f.pool->bridge.cache.entry[r->lease.slot];backend=f.backend;calls=f.fast.calls;
 memcpy(master,f.master.values,sizeof(master));memcpy(held,r->pcm.data,32*channels*sizeof(*held));
 memcpy(chip,r->data,sizeof(chip));memset(&view,0x6d,sizeof(view));saved=view;
 if(active_reader){
   assert(pt_paula_readers_reader_view(r,&view)==(fault?PT_PAULA_READERS_STALE:PT_PAULA_READERS_OK));
   if(fault){memset(&request,0,sizeof(request));request.kind=PT_SCHEDULED_CONTROL;
     request.key=key;request.channel=channels-1;
     assert(pt_paula_readers_begin(f.pool,f.queue,20,&request,1,&refused)==PT_PAULA_READERS_INVALID&&refused==(void *)1);}
 }else{
   assert(pt_paula_readers_view(c,0,&view)==(fault?PT_PAULA_READERS_STALE:PT_PAULA_READERS_OK));
   if(fault)assert(pt_paula_readers_enqueue(c,&b,&ticket)==PT_SCHEDULED_STALE&&ticket==777);
 }
 if(fault)assert(!memcmp(&view,&saved,sizeof(view)));
 else assert(view.data==r->data&&view.bytes==r->bytes);
 assert(!memcmp(&backend,&f.backend,sizeof(backend))&&f.fast.calls==calls);
 assert(!memcmp(&entry,f.pool->bridge.cache.entry+r->lease.slot,sizeof(entry)));
 assert(!memcmp(master,f.master.values,sizeof(master))&&!memcmp(held,r->pcm.data,32*channels*sizeof(*held)));
 assert(!memcmp(chip,r->data,sizeof(chip))&&pins(&f)==1&&r->pin&&!r->released);
 if(active_reader){assert(!pt_paula_readers_reader_close(r));remove_active(&f,r,key);}
 else assert(pt_paula_readers_command_close(c));
 finish(&f);
}
static int readers_binding_fixture(void)
{
 unsigned bits,channels,fault,active_reader,count=0;
 for(bits=8;bits<=24;bits+=8)for(channels=1;channels<=2;++channels)
   for(fault=0;fault<(channels==2?6U:5U);++fault)for(active_reader=0;active_reader<2;++active_reader){
     binding_case(bits,channels,fault,active_reader);++count;}
 assert(count==66);
 puts("SAMPLER READERS SOURCE BINDING PASS:66 cases;12 genuine controls,54 sample/side/representation refusals;no dispatch,master preserved,independent retirement,zero owned bytes");
 return 0;
}
#ifndef PT_READERS_NATIVE_MEMORY
int main(void){return readers_binding_fixture();}
#endif
