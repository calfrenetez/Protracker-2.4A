/* Distinct SOURCE-only candidate. All 39 cases and its oracle are prospective.
 * Production bodies are unchanged. The genuine original74 main is renamed in
 * the included derived helper and remains defined, but UNCALLED here. */
#include "sampler_mixed_causal_lineage_quantized_first_helpers.inc"

struct sq_version_image {
 struct pt_sampler_storage_span span[PT_SAMPLER_VERSION_SPANS];
 unsigned char *body[PT_SAMPLER_VERSION_SPANS];unsigned count;
};
struct sq_image {
 struct pt_amigus_ram_block block[PT_CACHE_SLOTS];
 struct pt_cache_entry entry[PT_CACHE_SLOTS];
 struct sl_ledger chip[40];unsigned char *chip_body[40];
 unsigned char ram[4096];struct sq_version_image *version;unsigned count;
 unsigned chip_calls,writes;
};
static unsigned sq_cases;

static struct sq_image *sq_capture(struct sl_case *f)
{
 struct resources *r=f->base->trial->resources;struct sq_image *s=calloc(1,sizeof(*s));
 unsigned i,j;assert(s&&sizeof(s->ram)==sizeof(r->card.ram));
 memcpy(s->block,r->card.cache.arena.block,sizeof(s->block));
 memcpy(s->entry,r->card.cache.cache.entry,sizeof(s->entry));
 memcpy(s->chip,f->chip.live,sizeof(s->chip));memcpy(s->ram,r->card.ram,sizeof(s->ram));
 s->chip_calls=f->chip.calls;s->writes=r->card.writes;s->count=r->document.project.sample_count;
 assert(s->count&&s->count<=PT_PROJECT_SAMPLES);
 s->version=calloc(s->count,sizeof(*s->version));assert(s->version);
 for(i=0;i<40;++i)if(s->chip[i].data){
  s->chip_body[i]=malloc(s->chip[i].bytes);assert(s->chip_body[i]);
  memcpy(s->chip_body[i],s->chip[i].data,s->chip[i].bytes);
 }
 /* Queried genuine version spans include the complete version controls,
  * full actual PCM capacity and markers, including their live pin counts. */
 for(i=0;i<s->count;++i){struct sq_version_image *v=s->version+i;
  assert(pt_sampler_version_spans(f->pin[i],v->span,PT_SAMPLER_VERSION_SPANS,&v->count));
  for(j=0;j<v->count;++j){v->body[j]=malloc(v->span[j].bytes);assert(v->body[j]);
   memcpy(v->body[j],v->span[j].data,v->span[j].bytes);}
 }
 sl_same(f);return s;
}
static void sq_same(struct sl_case *f,const struct sq_image *s)
{
 struct resources *r=f->base->trial->resources;unsigned i,j;
 assert(s->count==r->document.project.sample_count&&s->chip_calls==f->chip.calls&&s->writes==r->card.writes);
 assert(!memcmp(s->block,r->card.cache.arena.block,sizeof(s->block))&&
    !memcmp(s->entry,r->card.cache.cache.entry,sizeof(s->entry))&&
    !memcmp(s->chip,f->chip.live,sizeof(s->chip))&&!memcmp(s->ram,r->card.ram,sizeof(s->ram)));
 for(i=0;i<40;++i)if(s->chip[i].data)
  assert(!memcmp(s->chip_body[i],s->chip[i].data,s->chip[i].bytes));
 for(i=0;i<s->count;++i){struct pt_sampler_storage_span current[PT_SAMPLER_VERSION_SPANS];unsigned count=0;
  assert(pt_sampler_version_spans(f->pin[i],current,PT_SAMPLER_VERSION_SPANS,&count)&&count==s->version[i].count);
  for(j=0;j<count;++j)assert(current[j].data==s->version[i].span[j].data&&
    current[j].bytes==s->version[i].span[j].bytes&&
    !memcmp(s->version[i].body[j],current[j].data,current[j].bytes));
 }
 sl_same(f);
}
static void sq_image_drop(struct sq_image *s)
{
 unsigned i,j;for(i=0;i<40;++i)free(s->chip_body[i]);
 for(i=0;i<s->count;++i)for(j=0;j<s->version[i].count;++j)free(s->version[i].body[j]);
 free(s->version);free(s);
}
static void sq_batch(struct sl_case *f,struct pt_sampler_mixed_quantized_batch *b)
{
 static const uint16_t pair[7][2]={{32639,32895},{0,0},{65535,65535},
    {0,65535},{65535,0},{1,65534},{7,19}};
 struct resources *r=f->base->trial->resources;unsigned i;
 memset(b,0,sizeof(*b));b->count=16;
 for(i=0;i<16;++i){struct pt_sampler_mixed_request *a=b->action+i;
  a->kind=PT_MIXED_READERS_TRIGGER;a->track=i;a->sample=i%2;a->expected=f->pin[i%2];
  if(r->document.project.channels.track[i].route==PT_PAULA){
   a->geometry.paula.period=428;a->geometry.paula.volume=64;
  }else{
   a->geometry.amigus.bits=r->cache_bits;
   a->geometry.amigus.trigger=(struct pt_amigus_voice_request){8000,1,0,0,0};
   if(i==15){a->geometry.amigus.trigger.volume=64;a->geometry.amigus.trigger.pan=128;}
   else{b->levels[i].mode=PT_SAMPLER_MIXED_TRIGGER_QUANTIZED;
    b->levels[i].left=pair[i%7][0];b->levels[i].right=pair[i%7][1];}
  }
 }
}
static uint32_t sq_convert(int32_t value,unsigned from,unsigned to)
{
 uint32_t v=(uint32_t)value;assert(value>0&&value<=61);
 if(to>=from)return v<<(to-from);
 return (v+(1U<<(from-to-1)))>>(from-to);
}
static void sq_geometry(struct sl_case *f,uint64_t ticket,
 const struct pt_sampler_mixed_quantized_batch *original)
{
 struct resources *r=f->base->trial->resources;
 struct ct_command *c=ct_command(&f->base->port.original.base,ticket);unsigned i,j;
 assert(c&&c->packet.count==16&&c->packet.frame==960&&
    c->packet.first==oracle(960)&&c->packet.last==oracle(961));
 for(i=0;i<16;++i){const struct pt_sampler_mixed_request *x=original->action+i;
  const struct pt_mixed_readers_action *a=c->packet.action+i;
  const struct pt_pcm *pcm=&r->document.project.samples[x->sample].pcm;
  assert(a->kind==PT_MIXED_READERS_TRIGGER&&pcm->frames==64&&pcm->channels==1);
  if(a->route==PT_MIXED_READERS_PAULA){
   assert(a->geometry.paula.period==428&&a->geometry.paula.volume==64&&a->geometry.paula.words==32);
   for(j=0;j<64;++j)assert(a->geometry.paula.data[j]==(uint8_t)sq_convert(pcm->data[j],r->bits,8));
  }else{
   const struct pt_mixed_readers_card *card=c->packet.card+i;
   struct pt_playback_format format={(uint8_t)r->cache_bits,0,0,0};
   struct pt_amigus_voice_request request=x->geometry.amigus.trigger;
   struct pt_amigus_voice_plan expected;struct pt_cache_lease lease={card->cache_slot,card->serial};
   void *resource=pt_cache_data(&r->card.cache.cache,lease);unsigned arena;
   for(arena=0;arena<PT_CACHE_SLOTS&&resource!=&r->card.cache.arena.block[arena];++arena){}
   assert(arena<PT_CACHE_SLOTS&&card->cache==&r->card.cache.cache&&card->reservation==&r->card.reservation&&
     card->version==r->card.cache.cache.entry[card->cache_slot].version&&
     card->bits==r->cache_bits&&!card->little_endian&&!card->source_channel&&
     card->logical_bytes==64*r->cache_bits/8&&card->full_capacity==r->card.cache.arena.block[arena].reserved);
   if(original->levels[i].mode==PT_SAMPLER_MIXED_TRIGGER_QUANTIZED){request.volume=64;request.pan=128;}
   assert(pt_amigus_voice_plan_prepare(r->document.project.samples+x->sample,&format,&request,
     card->address,card->logical_bytes,&expected));
   if(original->levels[i].mode==PT_SAMPLER_MIXED_TRIGGER_QUANTIZED){
    expected.left=original->levels[i].left;expected.right=original->levels[i].right;
   }
   assert(a->geometry.amigus.start==expected.start&&a->geometry.amigus.loop==expected.loop&&
     a->geometry.amigus.end_exclusive==expected.end_exclusive&&a->geometry.amigus.rate==expected.rate&&
     a->geometry.amigus.control==expected.control&&a->geometry.amigus.left==expected.left&&
     a->geometry.amigus.right==expected.right);
   for(j=0;j<64;++j){uint32_t value=sq_convert(pcm->data[j],r->bits,r->cache_bits);
    uint32_t address=card->address+j*r->cache_bits/8;
    if(r->cache_bits==8)assert(r->card.ram[address]==(uint8_t)value);
    else assert(r->card.ram[address]==(uint8_t)(value>>8)&&r->card.ram[address+1]==(uint8_t)value);
   }
   for(j=card->logical_bytes;j<card->full_capacity;++j)assert(!r->card.ram[card->address+j]);
  }
 }
}
static uint64_t sq_first(struct sl_case *f,struct pt_sampler_mixed_quantized_batch *original)
{
 struct sl_client *v=f->client;struct pt_sampler_mixed_quantized_batch *b=malloc(sizeof(*b));
 uint64_t first=0;unsigned i;assert(b);sq_batch(f,b);memcpy(original,b,sizeof(*b));
 memcpy(v->trigger,b->action,sizeof(v->trigger));
 assert(pt_sampler_mixed_begin_quantized(v->pool,f->revision,960,b,v->command)==PT_SAMPLER_MIXED_PENDING);
 assert(!memcmp(original,b,sizeof(*b)));
 /* Original batch expires on begin return, before pins/cache preparation. */
 memset(b,0xa5,sizeof(*b));CT_POISON(b,sizeof(*b));sl_advance(f,0);CT_UNPOISON(b,sizeof(*b));free(b);
 for(i=0;i<16;++i)assert(pt_sampler_mixed_reader(v->pool,v->command[0],i,v->reader+i)==PT_SAMPLER_MIXED_OK);
 assert(pt_sampler_mixed_enqueue(v->pool,f->revision,v->command[0],&first)==PT_MIXED_READERS_OK);
 assert(pt_mixed_readers_commands_held(v->config.queue)==1&&pt_mixed_readers_readers_held(v->config.queue)==16);
 assert(pt_sampler_mixed_publish(v->pool,first)==PT_MIXED_READERS_OK);sq_geometry(f,first,original);
 f->base->port.original.base.ticks=oracle(960)-1;
 assert(pt_mixed_causal_fire(f->base->owner,first)==PT_MIXED_CAUSAL_EARLY&&!f->base->port.original.base.effects);
 ++f->base->port.original.base.ticks;
 assert(pt_mixed_causal_fire(f->base->owner,first)==PT_MIXED_CAUSAL_COMMITTED);
 f->base->port.original.hold_first=1;
 assert(pt_sampler_mixed_service_command(v->pool,first,0,NULL)==PT_MIXED_READERS_PENDING);sl_same(f);return first;
}
static void sq_again(struct sl_case *f,const struct pt_sampler_mixed_quantized_batch *b)
{
 struct pt_sampler_mixed_command_handle out={NULL,777};unsigned calls=f->ordinary.calls;
 unsigned held_c=pt_mixed_readers_commands_held(f->client->config.queue);
 unsigned held_r=pt_mixed_readers_readers_held(f->client->config.queue);
 assert(pt_sampler_mixed_begin_quantized(f->client->pool,f->revision,3840,b,&out)==PT_SAMPLER_MIXED_CAPACITY);
 assert(!out.address&&out.token==777&&calls==f->ordinary.calls&&
   held_c==pt_mixed_readers_commands_held(f->client->config.queue)&&
   held_r==pt_mixed_readers_readers_held(f->client->config.queue));sl_same(f);
}
static void sq_success(unsigned bits,unsigned cache,unsigned all_card,unsigned reader_first)
{
 struct sl_case *f=sl_make(bits,cache,all_card,1);struct sl_client *v=f->client;
 struct pt_sampler_mixed_quantized_batch *original=malloc(sizeof(*original));struct sq_image *image;
 struct pt_sampler_mixed_command_handle expired,out={NULL,777};struct pt_mixed_readers_key key[16];
 uint64_t first,control,stop;unsigned i,calls,count=reader_first?6:16;
 assert(original);++sq_cases;sl_open(f);first=sq_first(f,original);expired=v->command[0];
 for(i=0;i<16;++i)assert(pt_sampler_mixed_reader_key(v->pool,v->reader[i],key+i)==PT_MIXED_READERS_OK);
 image=sq_capture(f);calls=f->ordinary.calls;sq_again(f,original);sq_same(f,image);
 control=sl_control(f,count,1);assert(f->ordinary.calls==calls+1&&
    pt_mixed_readers_commands_held(v->config.queue)==2&&pt_mixed_readers_readers_held(v->config.queue)==16);
 sq_same(f,image);sq_again(f,original);sq_same(f,image);sl_batch(f,count);
 assert(pt_sampler_mixed_causal_lineage_stop_begin(v->pool,f->revision,&v->stop,&out)==PT_SAMPLER_MIXED_INVALID);
 assert(!out.address&&out.token==777&&f->ordinary.calls==calls+1);sq_same(f,image);
 sl_root_close(f,first);assert(pt_sampler_mixed_cancel(v->pool,expired)==PT_SAMPLER_MIXED_INVALID);
 stop=sl_stop(f,count);assert(v->command[0].token!=expired.token&&f->ordinary.calls==calls+2&&
   pt_mixed_readers_commands_held(v->config.queue)==2&&pt_mixed_readers_readers_held(v->config.queue)==16);
 assert(pt_sampler_mixed_publish(v->pool,stop)==PT_MIXED_READERS_OK);
 assert(f->base->port.root.identity.event==f->base->port.third_identity.event&&
   ct_command(&f->base->port.original.base,stop)->packet.frame==2880&&
   ct_command(&f->base->port.original.base,stop)->packet.first==oracle(2880)&&
   ct_command(&f->base->port.original.base,stop)->packet.last==oracle(2881));
 f->base->port.original.base.ticks=oracle(2880)-1;
 assert(pt_mixed_causal_fire(f->base->owner,stop)==PT_MIXED_CAUSAL_EARLY&&!f->base->port.original.stops);
 ++f->base->port.original.base.ticks;
 assert(pt_mixed_causal_fire(f->base->owner,stop)==PT_MIXED_CAUSAL_COMMITTED&&f->base->port.original.stops==count);
 sq_same(f,image);sq_again(f,original);sq_same(f,image);
 assert(pt_sampler_mixed_cancel(v->pool,expired)==PT_SAMPLER_MIXED_INVALID);
 for(i=0;i<16;++i){unsigned index=ct_index(key[i].route,key[i].slot);
  if(i<count)assert(!(f->base->port.original.base.mask&(1U<<index)));
  else assert(keys_equal(key+i,f->base->port.original.base.slot+index));}
 sq_image_drop(image);free(original);
 if(reader_first){struct pt_mixed_readers_reader_receipt actual,saved;
  memset(&actual,0xa5,sizeof(actual));memcpy(&saved,&actual,sizeof(saved));
  assert(pt_sampler_mixed_service_reader(v->pool,first,0,0,&actual)==PT_MIXED_READERS_PENDING&&
    !memcmp(&actual,&saved,sizeof(saved)));
  f->base->port.original.quiet_mask=1U<<ct_index(key[0].route,key[0].slot);
  assert(pt_sampler_mixed_service_reader(v->pool,first,0,0,NULL)==PT_MIXED_READERS_OK);
  assert(!pt_sampler_mixed_reader_close(v->pool,v->reader));
  assert(pt_sampler_mixed_service_command(v->pool,control,0,NULL)==PT_MIXED_READERS_OK);
  assert(!pt_sampler_mixed_reader_close(v->pool,v->reader));
  assert(pt_sampler_mixed_service_command(v->pool,stop,0,NULL)==PT_MIXED_READERS_OK);
  assert(pt_sampler_mixed_reader_close(v->pool,v->reader)&&!v->reader[0].address);
  sl_drain(f,first,control,stop,1,1,1,0);
 }else{
  assert(pt_sampler_mixed_service_command(v->pool,stop,0,NULL)==PT_MIXED_READERS_OK);
  assert(pt_sampler_mixed_service_reader(v->pool,first,0,0,NULL)==PT_MIXED_READERS_PENDING);
  sl_drain(f,first,control,stop,1,0,1,0);
 }
 f->unknown_source=reader_first;sl_drop(f);
}
static void sq_refusal(unsigned mode)
{
 struct sl_case *f=sl_make(24,16,0,1);struct sl_client *v=f->client;
 struct pt_sampler_mixed_quantized_batch *b=malloc(sizeof(*b)),*saved=malloc(sizeof(*saved));
 const struct pt_sampler_mixed_quantized_batch *input=b;
 struct pt_sampler_mixed_command_handle out={NULL,777},*output=&out;struct sq_image *image;
 void *pool_before;uint32_t revision=f->revision;unsigned calls;
 assert(b&&saved);++sq_cases;sl_open(f);sq_batch(f,b);
 switch(mode){
 case 0:b->levels[4].mode=(enum pt_sampler_mixed_trigger_level_mode)2;break;
 case 1:b->action[4].geometry.amigus.trigger.volume=1;break;
 case 2:b->action[4].geometry.amigus.trigger.pan=1;break;
 case 3:b->levels[0].mode=PT_SAMPLER_MIXED_TRIGGER_QUANTIZED;break;
 case 4:b->levels[15].left=1;break;
 case 5:b->count=17;break;
 case 6:b->count=0;break;
 case 7:b->count=15;memset(b->action+15,0,sizeof(b->action[15]));
  memset(b->levels+15,0,sizeof(b->levels[15]));b->levels[15].mode=PT_SAMPLER_MIXED_TRIGGER_QUANTIZED;break;
 case 8:b->count=15;memset(b->action+15,0,sizeof(b->action[15]));
  memset(b->levels+15,0,sizeof(b->levels[15]));b->action[15].track=1;break;
 case 9:output=(void *)(b->levels+15);break;
 case 10:input=(void *)v->pool;break;
 default:assert(mode==11);++revision;break;
 }
 memcpy(saved,b,sizeof(*saved));calls=f->ordinary.calls;image=sq_capture(f);
 pool_before=malloc(pt_sampler_mixed_pool_size());assert(pool_before);
 memcpy(pool_before,v->pool,pt_sampler_mixed_pool_size());
 assert(pt_sampler_mixed_begin_quantized(v->pool,revision,960,input,output)==PT_SAMPLER_MIXED_INVALID);
 assert(!out.address&&out.token==777&&f->ordinary.calls==calls&&!memcmp(saved,b,sizeof(*saved))&&
   !memcmp(pool_before,v->pool,pt_sampler_mixed_pool_size())&&
   !pt_mixed_readers_commands_held(v->config.queue)&&!pt_mixed_readers_readers_held(v->config.queue));
 sq_same(f,image);sq_image_drop(image);free(pool_before);
 /* A refusal must not consume the private first stage. A genuine corrected
  * first batch can still admit; cancelling that admitted attempt does consume it. */
 sq_batch(f,b);
 assert(pt_sampler_mixed_begin_quantized(v->pool,f->revision,960,b,v->command)==PT_SAMPLER_MIXED_PENDING);
 assert(pt_sampler_mixed_cancel(v->pool,v->command[0])==PT_SAMPLER_MIXED_OK);
 assert(pt_sampler_mixed_command_close(v->pool,v->command)&&!v->command[0].address);
 image=sq_capture(f);sq_again(f,b);sq_same(f,image);sq_image_drop(image);
 free(saved);free(b);sl_drop(f);
}
static void sq_consumed(unsigned mode)
{
 struct sl_case *f=sl_make(24,16,0,1);struct sl_client *v=f->client;
 struct pt_sampler_mixed_quantized_batch *b=malloc(sizeof(*b));struct sq_image *image;
 struct pt_sampler_mixed_command_handle out={NULL,777};unsigned calls;
 assert(b);++sq_cases;sl_open(f);sq_batch(f,b);calls=f->ordinary.calls;
 if(!mode){f->ordinary.fail=1;
  assert(pt_sampler_mixed_begin_quantized(v->pool,f->revision,960,b,&out)==PT_SAMPLER_MIXED_CAPACITY&&
    !out.address&&out.token==777&&f->ordinary.calls==calls+1);f->ordinary.fail=0;
 }else{
  assert(mode<=2&&pt_sampler_mixed_begin_quantized(v->pool,f->revision,960,b,v->command)==PT_SAMPLER_MIXED_PENDING);
  if(mode==2)sl_advance(f,0);
  assert(pt_sampler_mixed_cancel(v->pool,v->command[0])==PT_SAMPLER_MIXED_OK);
  assert(pt_sampler_mixed_command_close(v->pool,v->command)&&!v->command[0].address);
 }
 assert(!pt_mixed_readers_commands_held(v->config.queue)&&!pt_mixed_readers_readers_held(v->config.queue)&&
   !f->base->port.original.base.publications&&!f->base->port.original.base.effects);
 image=sq_capture(f);sq_again(f,b);sq_same(f,image);sq_image_drop(image);free(b);sl_drop(f);
}
int main(void)
{
 unsigned bits,cache,card,order,mode;
 for(bits=8;bits<=24;bits+=8)for(cache=8;cache<=16;cache+=8)
  for(card=0;card<2;++card)for(order=0;order<2;++order)sq_success(bits,cache,card,order);
 for(mode=0;mode<12;++mode)sq_refusal(mode);
 for(mode=0;mode<3;++mode)sq_consumed(mode);
 assert(sq_cases==39&&sl_cases==39);
 puts("SAMPLER CAUSAL LINEAGE QUANTIZED FIRST HOST COMPLETE cases=39;24 genuine mixed4/12 or16card 8/16/24-master cache8/16 lifetimes;copied expired first batch and seven-field final-level geometry;exact960/1920/2880;two live C and reachable16R within32R bound;12 before-admission refusals;3 consumed first attempts;typed CONTROL/STOP zero new R/pin/cache/upload;actual C1 NULL close/token reuse;independent C/R/SOURCE;full version/master/cache/Chip/RAM/save beforeimages;NOT_NATIVE_CONTROLLER_DEVICE_AUDIO");
 return 0;
}
