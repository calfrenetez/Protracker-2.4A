/* New focused HOST source fixture. All inherited entries are renamed and
 * UNCALLED. Real production owners/cache/preparation compile separately.
 * The sole input hook changes generated document-backed24-bit values BEFORE
 * genuine promotion/pinning; it never edits an immutable version or fabricates
 * a token/receipt. No native timer, IRQ ABI, DMA or card timing qualification. */
#include "../src/editor/sampler.h"
#include "../src/core/wav.h"
static unsigned fc_document_inputs;
static enum pt_edit_result fc_document_pin(struct pt_sampler *,struct pt_project *,
    unsigned,unsigned,struct pt_pcm *,struct pt_sample_version **);
#define pt_sampler_pin fc_document_pin
#define PT_PRIVATE_PAIR_TIMER_FIXTURE_ENTRY inherited_pair_timer_ten_cases_not_called
#include "native_mixed_causal_pair_timer_source_test.c"
#undef PT_PRIVATE_PAIR_TIMER_FIXTURE_ENTRY
#undef pt_sampler_pin

struct fc_master {
    struct pt_pcm pcm;struct pt_sample_version *pin;
    int32_t *capacity;uint8_t *wav;size_t wav_bytes;
};
struct fc_before {
    unsigned samples,selected;uint32_t revision,generation;
    uint8_t *project_bytes;size_t project_size;
    struct fc_master master[PT_PROJECT_SAMPLES];
};
static unsigned fc_cases,fc_matches,fc_refusals,fc_low24;
static enum pt_edit_result fc_document_pin(struct pt_sampler *sampler,
 struct pt_project *project,unsigned slot,unsigned generation,struct pt_pcm *pcm,
 struct pt_sample_version **token)
{
    if(fc_document_inputs&&slot<2&&project->samples[slot].pcm.bits==24){
        struct cp_trial *f=&ts_current->trial;unsigned j;
        assert(project==f->editor->project&&sampler==&f->editor->sampler&&!sampler->current[slot]);
        assert(project->samples[slot].pcm.data==f->original[slot]&&project->samples[slot].pcm.capacity==64);
        for(j=0;j<64;++j)f->original[slot][j]+=(int32_t)(j%7+1);
        ++fc_low24;
    }
    return pt_sampler_pin(sampler,project,slot,generation,pcm,token);
}
static uint64_t fc_tick(uint32_t frequency,uint64_t frame)
{
    assert((frequency==709379U||frequency==715909U)&&frame<=1921);
    return 100+(frame*frequency+47999)/48000;
}
static int fc_pcm_same(const struct pt_pcm *a,const struct pt_pcm *b)
{return a->data==b->data&&a->capacity==b->capacity&&a->frames==b->frames&&
 a->rate==b->rate&&a->channels==b->channels&&a->bits==b->bits;}
static uint8_t *fc_wav(const struct pt_pcm *pcm,size_t *bytes)
{
    uint8_t *out;size_t written;struct pt_wav_info info;struct pt_pcm decoded;
    int32_t *values=NULL;size_t count=(size_t)pcm->frames*pcm->channels;
    assert(pt_wav_size(pcm,bytes)==PT_WAV_OK);out=malloc(*bytes);assert(out);
    assert(pt_wav_encode(pcm,out,*bytes,&written)==PT_WAV_OK&&written==*bytes);
    assert(pt_wav_inspect(out,*bytes,&info)==PT_WAV_OK&&info.frames==pcm->frames&&
        info.rate==pcm->rate&&info.channels==pcm->channels&&info.bits==pcm->bits);
    if(count){values=malloc(count*sizeof(*values));assert(values);}
    decoded=*pcm;decoded.data=values;decoded.capacity=count;
    assert(pt_wav_decode(out,*bytes,&decoded)==PT_WAV_OK);
    if(count)assert(!memcmp(values,pcm->data,count*sizeof(*values)));
    free(values);return out;
}
static struct fc_before *fc_capture(struct cp_trial *f)
{
    struct fc_before *b=calloc(1,sizeof(*b));unsigned i;assert(b);
    b->samples=f->editor->project->sample_count;b->revision=f->editor->history.revision;
    b->generation=f->editor->sampler.generation;
    b->selected=f->editor->project->channels.selected;
    b->project_bytes=cp_save(f,&b->project_size);
    assert(b->samples==32&&b->samples<=PT_PROJECT_SAMPLES);
    for(i=0;i<b->samples;++i){struct fc_master *m=b->master+i;
        m->pcm=f->editor->project->samples[i].pcm;m->pin=f->pin[i];assert(m->pin);
        assert(fc_pcm_same(&m->pcm,f->pcm+i));
        if(m->pcm.capacity){assert(m->pcm.data&&m->pcm.capacity<=SIZE_MAX/sizeof(int32_t));
            m->capacity=malloc(m->pcm.capacity*sizeof(int32_t));assert(m->capacity);
            memcpy(m->capacity,m->pcm.data,m->pcm.capacity*sizeof(int32_t));}
        m->wav=fc_wav(&m->pcm,&m->wav_bytes);
    }
    assert(b->master[31].pcm.capacity==64&&b->master[31].pcm.frames==1&&b->master[31].pcm.bits==24);
    if(f->bits==24)for(i=0;i<12;++i)assert((uint32_t)b->master[0].capacity[i]&255U);
    return b;
}
static void fc_same(struct cp_trial *f,const struct fc_before *b)
{
    unsigned i;size_t project_size;uint8_t *project_bytes;
    /* Inspect raw selection/export BEFORE the inherited helper can normalize
     * selection. Exact whole-project preservation is an actual assertion. */
    assert(b->selected==f->editor->project->channels.selected);
    project_bytes=cp_save(f,&project_size);
    assert(project_size==b->project_size&&!memcmp(project_bytes,b->project_bytes,project_size));
    free(project_bytes);cp_same(f);
    assert(b->samples==f->editor->project->sample_count&&b->revision==f->editor->history.revision&&
        b->generation==f->editor->sampler.generation);
    for(i=0;i<b->samples;++i){const struct fc_master *m=b->master+i;
        const struct pt_pcm *pcm=&f->editor->project->samples[i].pcm;size_t n;uint8_t *wav;
        assert(fc_pcm_same(pcm,&m->pcm)&&fc_pcm_same(f->pcm+i,&m->pcm)&&f->pin[i]==m->pin&&
            f->editor->sampler.current[i]==m->pin);
        if(pcm->capacity)assert(!memcmp(pcm->data,m->capacity,pcm->capacity*sizeof(int32_t)));
        wav=fc_wav(pcm,&n);assert(n==m->wav_bytes&&!memcmp(wav,m->wav,n));free(wav);
    }
}
static void fc_before_drop(struct fc_before *b)
{unsigned i;for(i=0;i<b->samples;++i){free(b->master[i].capacity);free(b->master[i].wav);}free(b->project_bytes);free(b);}
static struct cp_trial *fc_make(unsigned bits,unsigned cache_bits,unsigned little,uint32_t frequency)
{
    struct cp_trial *f;struct ts_hardware *h;struct pt_private_pair_timer_ops ops;
    struct pt_private_mixed_causal_ram_adapter adapter;unsigned i;
    assert(!fc_document_inputs&&!ts_current);fc_document_inputs=ts_expand=1;
    f=cp_make(bits,cache_bits,little);fc_document_inputs=0;
    assert(!ts_expand&&f==&ts_current->trial);++fc_cases;
    h=&ts_current->hardware;h->port=&ts_current->port;h->now=100;h->frequency=frequency;
    h->state.available=1;for(i=0;i<8;++i)h->state.before_image[i]=0xcafe0000U+i;
    memcpy(&h->original,&h->state,sizeof(h->original));
    ops=(struct pt_private_pair_timer_ops){h,sizeof(*h),PT_PRIVATE_PAIR_TIMER_OPS_VERSION,PT_PRIVATE_PAIR_TIMER_REQUIRED,
        ts_exclude,ts_restore,ts_clock,ts_state,ts_acquire,ts_arm,ts_rearm,ts_ack,ts_cancel,ts_release};
    assert(pt_private_pair_timer_init(&ts_current->source,&ops));
    adapter=pt_private_pair_timer_adapter(&ts_current->source);
    assert(pt_private_mixed_causal_ram_init(&ts_current->port,31,17,frequency,&adapter,64,8));
    f->input.causal.grid.frequency=frequency;
    f->input.contexts.bytes=sizeof(*ts_current);f->input.causal.port=pt_private_mixed_causal_ram_api(&ts_current->port);
    f->input.bind_original=ts_bind;f->input.causal.allocator.release=ts_release_command;
    return f;
}
static void fc_refuse(struct cp_trial *f,unsigned mode,const struct fc_before *before)
{
    struct pt_editor_mixed_command_ref out={99,999},saved=out;
    struct pt_private_mixed_causal_ram_port p;
    struct ts_hardware h;struct pt_pcm bad=f->pcm[0];
    uint8_t bytes[256],original[256];size_t n=12345,written=54321;
    unsigned calls=f->ordinary.calls,chips=f->chip.calls,writes=f->card->writes;
    memcpy(&p,&ts_current->port,sizeof(p));memcpy(&h,&ts_current->hardware,sizeof(h));
    cp_requests(f,6);
    if(mode==0)f->request[4].geometry.amigus.bits=24;
    else{assert(mode==1);f->request[4].geometry.amigus.little_endian=2;}
    assert(pt_editor_mixed_causal_prepare_batch_begin(&f->control,960,f->request,6,&out)==PT_EDITOR_MIXED_READERS_INVALID);
    assert(out.slot==saved.slot&&out.serial==saved.serial&&!f->control.prepared_batches&&!f->control.first_error);
    assert(calls==f->ordinary.calls&&chips==f->chip.calls&&writes==f->card->writes);
    assert(!memcmp(&p,&ts_current->port,sizeof(p))&&!memcmp(&h,&ts_current->hardware,sizeof(h)));
    memset(bytes,0xa6,sizeof(bytes));memcpy(original,bytes,sizeof(bytes));bad.bits=12;
    assert(pt_wav_size(&bad,&n)==PT_WAV_INVALID&&n==12345);
    assert(pt_wav_encode(&bad,bytes,sizeof(bytes),&written)==PT_WAV_INVALID&&written==54321);
    assert(!memcmp(bytes,original,sizeof(bytes)));fc_same(f,before);++fc_refusals;
}
static void fc_cache_bytes(struct cp_trial *f)
{
    static const int8_t paula[6]={127,1,3,64,0,-2};
    static const int8_t card8[6]={-128,-1,-3,-64,2,126};
    static const int16_t card16[6]={-32768,-256,-768,-16384,512,32256};
    struct pt_private_mixed_causal_ram_port *p=&ts_current->port;unsigned ci,i,j;
    for(ci=0;ci<2;++ci)for(i=0;i<6;++i){const struct pt_mixed_readers_action *a=p->command[ci].packet.action+i;
        if(i<4){assert(a->route==PT_MIXED_READERS_PAULA&&a->geometry.paula.words==3&&
            a->geometry.paula.period==428&&a->geometry.paula.volume==64);
            assert(a->geometry.paula.data!=(const uint8_t *)f->pcm[i%2].data);
            for(j=0;j<6;++j)assert(a->geometry.paula.data[j]==(uint8_t)paula[j]);
        }else{const struct pt_mixed_readers_card *c=p->command[ci].packet.card+i;
            const struct pt_cache_entry *entry;const uint8_t *data;
            assert(a->route==PT_MIXED_READERS_AMIGUS&&c->bits==f->cache_bits&&
                c->little_endian==f->little&&c->source_channel==1&&c->cache_slot<PT_CACHE_SLOTS);
            assert(c->logical_bytes==6U*(f->cache_bits/8U)&&c->full_capacity>=c->logical_bytes&&
                !(c->full_capacity&3U)&&c->full_capacity<=sizeof(f->card->ram)&&
                c->address<=sizeof(f->card->ram)-c->full_capacity);
            entry=f->card->cache.cache.entry+c->cache_slot;
            assert(entry->valid&&entry->pins>=2&&entry->serial==c->serial&&entry->bytes==c->logical_bytes);
            data=f->card->ram+c->address;
            for(j=0;j<6;++j)if(f->cache_bits==8)assert(data[j]==(uint8_t)card8[j]);
            else{uint16_t value=(uint16_t)card16[j];
                assert(data[2*j+f->little]==(uint8_t)(value>>8)&&data[2*j+1-f->little]==(uint8_t)value);}
            assert(a->geometry.amigus.start==c->address&&a->geometry.amigus.end_exclusive==c->address+c->logical_bytes);
        }
    }
}
static struct pt_mixed_causal_first_completion_match fc_prediction(void)
{
    struct pt_private_mixed_causal_ram_port *p=&ts_current->port;struct pt_mixed_causal_first_completion_match r;
    memset(&r,0,sizeof(r));r.predecessor=p->command[0].identity;r.successor=p->command[1].identity;
    r.serial=p->command[1].serial;r.first_tick=p->command[0].packet.first;r.last_tick=p->command[0].packet.last;
    r.observed=r.issued=r.first_tick;r.post.active_mask=r.post.adopted_mask=p->command[1].packet.expected_mask;
    memcpy(r.post.slot,p->command[1].packet.expected,sizeof(r.post.slot));return r;
}
static void fc_actual(struct cp_trial *f,struct pt_mixed_causal_first_completion_match *r)
{
    struct pt_mixed_causal_diagnostic d;struct pt_private_mixed_causal_ram_port *p=&ts_current->port;
    memset(&d,0,sizeof(d));assert(pt_mixed_causal_diagnostic(f->control.causal,&d));
    assert(d.completed&&d.admitted&&d.published&&!d.suppressed&&d.commit_called&&d.commit_outcome==1);
    assert(d.first==r->predecessor.ticket&&d.successor==r->successor.ticket&&d.serial==r->serial);
    r->observed=d.observed;r->issued=d.issued;r->post.active_mask=r->post.adopted_mask=p->mask;
    memcpy(r->post.slot,p->slot,sizeof(r->post.slot));
    assert(p->command[1].predecessor_completed&&r->observed==p->command[1].predecessor_observed&&
        r->issued==p->command[1].predecessor_issued);
}
static void fc_match(struct cp_trial *f,const struct pt_mixed_causal_first_completion_match *r,int expected)
{
    size_t n=pt_mixed_causal_control_size();void *owner=malloc(n);struct pt_private_mixed_causal_ram_port p;
    struct pt_private_mixed_causal_pair_timer_source s;struct ts_hardware h;
    struct pt_mixed_causal_first_completion_match original;assert(owner&&f->control.causal);
    memcpy(&p,&ts_current->port,sizeof(p));memcpy(&s,&ts_current->source,sizeof(s));
    memcpy(&h,&ts_current->hardware,sizeof(h));memcpy(&original,r,sizeof(original));
    memcpy(owner,f->control.causal,n);assert(pt_mixed_causal_first_completion_matches(f->control.causal,*r)==expected);
    assert(!memcmp(owner,f->control.causal,n)&&!memcmp(&p,&ts_current->port,sizeof(p))&&
        !memcmp(&s,&ts_current->source,sizeof(s))&&!memcmp(&h,&ts_current->hardware,sizeof(h)));
    assert(!memcmp(&original,r,sizeof(original)));free(owner);++fc_matches;
}
static void fc_finish(struct cp_trial *f,struct fc_before *before)
{
    struct ts_hardware *h=&ts_current->hardware;unsigned i;
    ts_begin();if(!pt_editor_mixed_causal_prepare_close(&f->control))assert(pt_editor_mixed_causal_prepare_close(&f->control));
    assert(!f->control.causal&&!f->control.queue&&!f->control.pool&&!f->binding->preparation_context);
    assert(ts_current->source.source_closed&&ts_current->port.source_closed&&h->releases==1&&!h->entry);
    assert(!memcmp(&h->state,&h->original,sizeof(h->state))&&!ts_current->port.mask);
    for(i=0;i<2;++i)assert(!ts_current->port.command[i].live&&!ts_current->port.command[i].owner&&ts_absent(h->state.event+i));
    for(i=0;i<32;++i)assert(!f->control.reader[i].handle.address&&!ts_current->port.reader[i].live);
    assert(!cp_live(&f->ordinary)&&!cp_live(&f->chip));fc_same(f,before);assert(ts_end()==1);
    fc_before_drop(before);cp_drop(f);ts_current=NULL;
}
static void fc_cell(unsigned bits,unsigned cache_bits,unsigned little,uint32_t frequency)
{
    struct cp_trial *f=fc_make(bits,cache_bits,little,frequency);struct fc_before *before=fc_capture(f);
    struct ts_hardware *h=&ts_current->hardware;struct pt_private_mixed_causal_ram_port *p=&ts_current->port;
    struct pt_mixed_causal_packet packets[2];struct pt_private_pair_timer_hw_event second;
    struct pt_mixed_causal_first_completion_match match;void *first_reader;uint64_t first,successor;
    unsigned writes,chips,chip_releases,live;
    ts_begin();cp_open(f);fc_refuse(f,0,before);fc_refuse(f,1,before);cp_requests(f,6);
    cp_prepare(f,0,960);cp_refs(f,0,0);first=cp_admit(f,0);
    cp_prepare(f,1,1920);cp_refs(f,1,16);successor=cp_admit(f,1);
    assert(first!=successor&&h->arms==2&&p->publishes==2&&!p->effects&&p->pair_used==2);
    assert(pt_mixed_readers_commands_held(f->control.queue)==2&&pt_mixed_readers_readers_held(f->control.queue)==12);
    assert(p->command[0].packet.frame==960&&p->command[1].packet.frame==1920);
    assert(p->command[0].packet.first==fc_tick(frequency,960)&&p->command[0].packet.last==fc_tick(frequency,961));
    assert(p->command[1].packet.first==fc_tick(frequency,1920)&&p->command[1].packet.last==fc_tick(frequency,1921));
    assert(h->state.event[0].first==p->command[0].packet.first&&h->state.event[1].first==p->command[1].packet.first&&
        h->state.event[0].frequency==frequency&&h->state.event[1].frequency==frequency);
    fc_cache_bytes(f);fc_same(f,before);match=fc_prediction();fc_match(f,&match,0);
    memcpy(packets,&p->command[0].packet,sizeof(packets[0]));memcpy(packets+1,&p->command[1].packet,sizeof(packets[1]));
    memcpy(&second,h->state.event+1,sizeof(second));writes=f->card->writes;chips=f->chip.calls;chip_releases=f->chip.releases;live=cp_live(&f->ordinary);
    assert(ts_end()==1);ts_advance(h,fc_tick(frequency,960)-1);ts_early_wake(h,0);
    assert(h->vectors==1&&h->vector_result==1&&h->rearms==1&&!h->acks&&!p->effects&&
        ts_current->source.event[0].fire_outcome==PT_MIXED_CAUSAL_EARLY);
    assert(h->state.event[0].first==fc_tick(frequency,960)&&h->state.event[0].armed&&
        !memcmp(&second,h->state.event+1,sizeof(second)));
    assert(!memcmp(packets,&p->command[0].packet,sizeof(packets[0]))&&!memcmp(packets+1,&p->command[1].packet,sizeof(packets[1])));
    ts_begin();fc_match(f,&match,0);assert(ts_end()==1);
    ts_advance(h,fc_tick(frequency,960));assert(h->vectors==2&&h->vector_result==1&&p->commits==1&&p->effects==6&&h->acks==1);
    ts_begin();fc_actual(f,&match);assert(match.observed==fc_tick(frequency,960)&&match.issued==match.observed);
    fc_match(f,&match,1);ts_observe(f,0,match.observed);
    first_reader=f->control.reader[f->reader[0].slot].handle.address;
    ts_current->poison=f->control.command[f->command[0].slot].handle.address;assert(ts_current->poison);
    cp_drain_command(f,0,0);assert(ts_current->poisoned==1&&!ts_current->poison&&!p->command[0].live&&!p->command[0].owner);
    fc_match(f,&match,1);
    assert(pt_mixed_readers_commands_held(f->control.queue)==1&&pt_mixed_readers_readers_held(f->control.queue)==12);
    assert(pt_editor_mixed_causal_prepare_service_reader(&f->control,f->reader[0],0,NULL)==PT_MIXED_READERS_PENDING&&
        f->control.reader[f->reader[0].slot].handle.address==first_reader);
    assert(p->command[1].owner==f->control.causal&&p->command[1].live&&p->command[1].armed&&p->command[1].predecessor_completed);
    assert(!memcmp(packets+1,&p->command[1].packet,sizeof(packets[1]))&&!memcmp(&second,h->state.event+1,sizeof(second)));
    assert(h->entry&&h->state.source==&ts_current->source&&h->state.vector_live&&!ts_current->source.source_attempted&&
        !ts_current->port.source_attempted&&f->binding->preparation_context==&f->control);
    assert(f->card->writes==writes&&f->chip.calls==chips&&f->chip.releases==chip_releases&&cp_live(&f->ordinary)==live-1);
    fc_same(f,before);assert(ts_end()==1);
    ts_advance(h,fc_tick(frequency,1920)-1);ts_early_wake(h,1);
    assert(h->vectors==3&&h->vector_result==1&&h->rearms==2&&h->acks==1&&p->commits==1&&p->effects==6&&
        ts_current->source.event[1].fire_outcome==PT_MIXED_CAUSAL_EARLY);
    assert(h->state.event[1].first==fc_tick(frequency,1920)&&h->state.event[1].armed&&
        !memcmp(packets+1,&p->command[1].packet,sizeof(packets[1])));
    ts_begin();fc_match(f,&match,1);assert(ts_end()==1);
    ts_advance(h,fc_tick(frequency,1920));assert(h->vectors==4&&h->vector_result==1&&h->acks==2&&h->rearms==2&&p->commits==2&&p->effects==12);
    ts_begin();fc_match(f,&match,0);ts_observe(f,16,fc_tick(frequency,1920));
    cp_drain_command(f,1,0);cp_drain_readers(f,0,6,0);cp_drain_readers(f,16,6,0);
    assert(!pt_mixed_readers_commands_held(f->control.queue)&&!pt_mixed_readers_readers_held(f->control.queue));
    assert(h->entry&&h->releases==0&&!p->source_closed&&!ts_current->source.source_closed);
    fc_same(f,before);assert(ts_end()==1);fc_finish(f,before);
}
int main(void)
{
    static const unsigned bits[3]={8,16,24},cache[3]={8,16,16},little[3]={0,0,1};
    static const uint32_t frequency[2]={709379,715909};unsigned i,j,k;
    assert(fc_tick(709379,960)==14288&&fc_tick(709379,1920)==28476);
    assert(fc_tick(715909,960)==14419&&fc_tick(715909,1920)==28737);
    for(i=0;i<3;++i)for(j=0;j<3;++j)for(k=0;k<2;++k)fc_cell(bits[i],cache[j],little[j],frequency[k]);
    assert(fc_cases==18&&cp_cases==18&&fc_matches==108&&fc_refusals==36&&fc_low24==12);
    assert(!ts_cases&&!ct_cases&&!fc_document_inputs&&!ts_current);
    puts("HOST PAIR FORMAT CLOCK PASS:18 new matrix cases;masters8/16/24 with played24 low bits;Paula8 and card8BE/16BE/16LE;PAL709379/NTSC715909;108 immutable genuine completion matches;36 unchanged-output cache-format refusals;full master capacities and exact project/WAV exports;original960/1920 EARLY nonrebase;disposed first C retains R/successor/SOURCE;HOST_MODEL_ONLY");
    return 0;
}
