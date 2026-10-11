/* SOURCE-only focused uint16 CONTROL qualification. Product C compiles
 * separately. Every first reader is genuinely issued, published, committed
 * and serviced; all donor entry points remain renamed and UNCALLED. The RAM
 * port supplies exact copied-packet evidence, not a native deadline backend. */
#include "control16_lineage_fixture_support.inc"
#include "../src/core/mixed_readers_causal_control16_internal.h"
#include "../src/editor/editor_mixed_causal_lineage_control16_prepare_internal.h"

struct c16_trial {
    struct cl_trial old;
    struct c16_trial *self;
    struct pt_editor_mixed_causal_lineage_control16_batch *wide;
    struct pt_mixed_causal_control_publication copied;
    unsigned copies,mutate_wide;
};
typedef char c16_containing_prefix[(offsetof(struct c16_trial,old)==0)?1:-1];
static unsigned c16_cases,c16_controller_cases,c16_core_cases;
static void *c16_allocate(void *context,size_t n)
{
    struct cs_memory *m=context;struct cl_trial *c=(void *)m->owner;
    struct c16_trial *w=(void *)c;assert(w->self==w);
    if(w->mutate_wide){w->mutate_wide=0;++w->wide->frame;}
    return cl_allocate(context,n);
}
static int c16_publish_control(void *context,struct pt_mixed_causal_owner *owner,
    const struct pt_mixed_causal_control_publication *d)
{
    struct cl_port *p=context;struct c16_trial *w=(void *)p->owner;
    int accepted;assert(w->self==w);accepted=cl_publish_control(context,owner,d);
    if(accepted==1){memcpy(&w->copied,d,sizeof(*d));++w->copies;}
    return accepted;
}

static struct c16_trial *c16_make(unsigned bits,unsigned cache_bits,unsigned little)
{
    struct c16_trial *w=calloc(1,sizeof(*w));struct cl_trial *c=w?&w->old:NULL;struct cs_trial *f=(void *)c;struct pt_allocator a;struct pt_amigus_reservation_api api;
    struct pt_project *p;unsigned i,j;assert(f);++cs_cases;f->bits=bits;f->cache_bits=cache_bits;f->little=little;
    f->card=calloc(1,sizeof(*f->card));f->document=calloc(1,sizeof(*f->document));
    f->editor=calloc(1,sizeof(*f->editor));f->binding=calloc(1,sizeof(*f->binding));
    f->request=calloc(16,sizeof(*f->request));f->command=calloc(2,sizeof(*f->command));f->reader=calloc(32,sizeof(*f->reader));
    f->stop=calloc(1,sizeof(*f->stop));
    assert(f->card&&f->document&&f->editor&&f->binding&&f->request&&f->command&&f->reader&&f->stop);f->bus.owner=f;f->ordinary.owner=f;f->masters.owner=f;f->chip.owner=f;f->port.owner=f;f->port.bind_raw=1;f->port.quiet_raw=1;
    a=(struct pt_allocator){&f->masters,cs_master_new,cs_master_free};
    pt_document_init(f->document,&a);assert(pt_document_new(f->document,16,SIZE_MAX)==PT_PROJECT_OK);
    p=&f->document->project;
    for(i=0;i<16;++i)p->channels.track[i].route=i<4?PT_PAULA:PT_AMIGUS;
    for(i=0;i<2;++i){int32_t values[]={127,-128,1,-1,3,-3,64,-64,0,2,-2,126};
        for(j=0;j<64;++j)f->original[i][j]=values[j%12]*(int32_t)(1U<<(bits-8));
        p->samples[i].pcm=(struct pt_pcm){f->original[i],64,6,8000,2,(uint8_t)bits};p->samples[i].volume=64;}
    assert(pt_editor_init(f->editor,p));pt_sampler_init(&f->editor->sampler,&a,SIZE_MAX);
    /* An unused already-owned master with real spare PCM capacity exercises
     * the entire capacity ledger, independently of played sample geometry. */
    {struct pt_pcm spare;int32_t *values=cs_new(&f->masters,64*sizeof(int32_t));
        for(i=0;i<64;++i)values[i]=(int32_t)i;
        spare=(struct pt_pcm){values,64,1,8000,1,24};
        assert(pt_sampler_append_owned(&f->editor->sampler,p,&f->editor->history,&spare,&a,"unused spare")==PT_EDIT_OK);
        assert(!spare.data);}
    for(i=0;i<p->sample_count;++i)assert(pt_sampler_pin(&f->editor->sampler,p,i,f->editor->sampler.generation,
        &f->pcm[i],&f->pin[i])==PT_EDIT_OK);
    /* Pinning materializes frames*channels values from borrowed input. Retain
     * independent full declared materialized capacity and borrowed-tail images;
     * borrowed capacity is not the size of the new version allocation. */
    for(i=0;i<2;++i){assert(f->pcm[i].capacity<=64);
        memcpy(f->master_before[i],f->pcm[i].data,f->pcm[i].capacity*sizeof(int32_t));
        memcpy(f->borrowed_before[i],f->original[i],sizeof(f->original[i]));}
    assert(pt_editor_mixed_attach(f->binding,f->editor));
    f->library.available=f->library.supported=f->library.count=1;f->card->healthy=1;
    api=(struct pt_amigus_reservation_api){&f->library,open_library,close_library,find,supported,reserve,release};
    assert(pt_amigus_reservation_open_resource(&f->card->reservation,&api,0,PT_AMIGUS_WAVETABLE)==PT_AMIGUS_RESERVED);
    assert(pt_amigus_wavetable_cache_attach(&f->card->cache,&f->card->reservation,0,4096,4096,&f->bus,cl_owned,cl_write));
    f->input.original.binding=f->binding;f->input.original.backend=&f->card->cache;
    f->input.original.causal.allocator=(struct pt_allocator){&f->ordinary,cs_allocate,cs_release};
    f->input.original.causal.allocator_context=(struct pt_mixed_readers_span){&f->ordinary,sizeof(f->ordinary)};
    f->input.original.causal.grid=(struct pt_mixed_readers_grid){100,17,709379,48000};f->input.original.causal.session=31;
    f->input.original.causal.control_budget=pt_mixed_causal_control_size();
    f->input.original.causal.queue_budget=pt_mixed_readers_control_size();
    f->input.original.causal.port=(struct pt_mixed_causal_port){&f->port,sizeof(f->port),PT_MIXED_CAUSAL_PORT_VERSION,
        PT_MIXED_CAUSAL_PORT_REQUIRED,cs_clock,cs_publish,cs_commit,cs_command_quiet,cs_reader_quiet,
        cs_source_close,cs_source_quiet,cs_publish_successor};
    f->input.original.bind_original=cs_bind;
    f->port.model.base.ticks=100;f->port.model.base.frequency=709379;
    f->port.model.base.commit_raw=f->port.model.base.source_raw=1;f->port.model.publication_raw=1;
    f->input.stop=(struct pt_mixed_causal_stop_port){&f->port,sizeof(f->port),
        PT_MIXED_CAUSAL_STOP_VERSION,PT_MIXED_CAUSAL_STOP_REQUIRED,cs_publish_stop};
    f->input.original.chip_context=&f->chip;f->input.original.chip_allocate=cs_chip_new;f->input.original.chip_release=cs_chip_free;
    f->input.original.factory_budget=pt_sampler_mixed_pool_size()+2*pt_sampler_mixed_command_size()+32*pt_sampler_mixed_reader_size();
    f->input.original.chip_budget=4096;f->input.original.contexts=(struct pt_sampler_storage_span){f,sizeof(*f)};
    f->causal_capacity=pt_mixed_causal_workspace_size()+128;
    f->factory_capacity=pt_sampler_mixed_workspace_size()+128;
    f->causal_workspace=calloc(1,f->causal_capacity);f->factory_workspace=calloc(1,f->factory_capacity);
    assert(f->causal_workspace&&f->factory_workspace);
    f->input.original.causal_workspace=f->causal_workspace;f->input.original.causal_capacity=f->causal_capacity;
    f->input.original.factory_workspace=f->factory_workspace;f->input.original.factory_capacity=f->factory_capacity;
    f->saved=cs_save(f,&f->saved_bytes);
    ++cl_cases;c->port.owner=c;c->port.model.base.ticks=100;c->port.model.base.frequency=709379;
    c->port.model.base.commit_raw=c->port.model.base.source_raw=1;
    c->port.control_raw=c->port.third_raw=c->port.quiet_raw=1;
    c->change=calloc(1,sizeof(*c->change));c->stop=calloc(1,sizeof(*c->stop));
    c->command=calloc(3,sizeof(*c->command));assert(c->change&&c->stop&&c->command);
    memcpy(&c->input.original,&f->input.original,sizeof(c->input.original));
    c->input.original.causal.allocator=(struct pt_allocator){&f->ordinary,cl_allocate,cl_release};
    c->input.original.causal.port=(struct pt_mixed_causal_port){&c->port,sizeof(c->port),
        PT_MIXED_CAUSAL_PORT_VERSION,PT_MIXED_CAUSAL_PORT_REQUIRED,cl_clock,cl_publish,cl_commit,
        cl_command_quiet,cl_reader_quiet,cl_source_close,cl_source_quiet,cl_publish_successor};
    c->input.original.bind_original=cl_bind;
    c->input.original.chip_allocate=cl_chip_new;c->input.original.chip_release=cl_chip_free;
    c->input.original.contexts=(struct pt_sampler_storage_span){c,sizeof(*c)};
    c->input.lineage=(struct pt_mixed_causal_control_stop_port){&c->port,sizeof(c->port),
        PT_MIXED_CAUSAL_CONTROL_STOP_VERSION,PT_MIXED_CAUSAL_CONTROL_STOP_REQUIRED,
        cl_publish_control,cl_publish_stop};
    w->self=w;w->wide=calloc(1,sizeof(*w->wide));assert(w->wide);
    c->input.original.contexts=(struct pt_sampler_storage_span){w,sizeof(*w)};
    c->input.original.causal.allocator.allocate=c16_allocate;
    c->input.lineage.publish_control=c16_publish_control;
    ++c16_cases;++c16_controller_cases;return w;
}
static int c16_core_publish(void *context,struct pt_mixed_causal_owner *owner,
    const struct pt_mixed_causal_control_publication *d)
{
    struct cc_port *p=context;unsigned i;int raw=p->publication_raw;
    ++p->control_publications;
    /* Own geometry-free first completion and own actual registry are authority;
     * owner copied completion is compared, never substituted as port evidence. */
    if(!p->first_completed||owner!=p->base.registration.owner||!d->serial||
       !cc_identity(&p->first_identity,&d->predecessor)||
       d->first_tick!=p->first_tick||d->last_tick!=p->last_tick||d->observed!=p->observed||d->issued!=p->issued||
       d->observed<d->first_tick||d->issued<d->observed||d->issued>=d->last_tick||
       d->first_post.active_mask!=p->base.mask||d->first_post.adopted_mask!=p->base.mask||
       !ct_expected(&p->base,&d->packet))return 0;
    for(i=0;i<20;++i)if(!keys_equal(d->first_post.slot+i,p->first_post.slot+i)||
        !keys_equal(d->first_post.slot+i,p->base.slot+i))return 0;
    for(i=0;i<d->packet.count;++i)if(d->packet.action[i].kind!=PT_MIXED_READERS_CONTROL||
        d->packet.key[i].trigger!=p->first_identity.ticket||!ct_reader(&p->base,d->packet.key+i))return 0;
    if(raw!=0&&!ct_publish(&p->base,owner,&d->successor,&d->packet))return 0;
    if(p->publication_reentry){struct pt_mixed_causal_diagnostic diag;p->publication_reentry=0;
        assert(!pt_mixed_causal_diagnostic(owner,&diag));}
    if(p->publication_mutation){struct pt_mixed_causal_control_publication *bad=(void *)d;
        p->publication_mutation=0;bad->first_post.slot[19].serial=1234;}
    return raw;
}

struct c16_counts {
    unsigned calls,releases,live,chip,masters,writes,pins,owned;
    unsigned clocks,publications,commands,readers,command_proofs,reader_proofs;
};
static struct c16_counts c16_counts(struct cl_trial *c)
{
    struct cs_trial *f=&c->old;struct pt_editor_mixed_causal_prepare *s=cl_s(c);
    struct c16_counts n={f->ordinary.calls,f->ordinary.releases,cs_live(&f->ordinary),
        f->chip.calls,f->masters.calls,f->card->writes,cs_cache_pins(f),f->owned_calls,
        c->port.model.base.clocks,c->port.model.base.publications,
        pt_mixed_readers_commands_held(s->queue),pt_mixed_readers_readers_held(s->queue),
        c->port.model.base.command_proofs,c->port.model.base.reader_proofs};return n;
}
static void c16_counts_same(struct cl_trial *c,const struct c16_counts *before)
{struct c16_counts now=c16_counts(c);assert(!memcmp(&now,before,sizeof(now)));}
static void c16_drop(struct c16_trial *w)
{
    struct pt_editor_mixed_causal_lineage_control16_batch *wide=w->wide;
    assert(w->self==w);cs_same(&w->old.old);cl_drop(&w->old);
    /* The complete containing callback context is gone only after genuine
     * pool/queue/SOURCE cleanup. The separately guarded caller input follows. */
    free(wide);
}
static void c16_input(struct c16_trial *w,unsigned narrow)
{
    static const uint16_t boundary[4]={0,255,256,65535};
    struct cl_trial *c=&w->old;unsigned i;
    memset(w->wide,0,sizeof(*w->wide));memset(c->change,0,sizeof(*c->change));
    w->wide->frame=c->change->frame=1920;w->wide->count=c->change->count=16;
    for(i=0;i<16;++i){struct pt_editor_mixed_causal_lineage_control16_action *a=w->wide->action+i;
        struct pt_editor_mixed_causal_lineage_control_action *old=c->change->action+i;
        a->reader=old->reader=c->old.reader[i];
        if(c->old.editor->project->channels.track[i].route==PT_PAULA){
            a->period=old->period=(uint16_t)(214+i);a->volume=old->volume=(uint8_t)(32+i);
        }else{
            a->rate=old->rate=0x10000U+i;
            if(narrow){old->left=(uint8_t)((i&1)?255:0);old->right=(uint8_t)((i&1)?0:255);
                a->left=old->left;a->right=old->right;}
            else{a->left=boundary[i%4];a->right=boundary[(i+1)%4];}
        }
    }
}
static void c16_card_subset(struct c16_trial *w)
{
    struct pt_editor_mixed_causal_lineage_control16_action selected=w->wide->action[4];
    memset(w->wide->action,0,sizeof(w->wide->action));w->wide->action[0]=selected;w->wide->count=1;
}
static void c16_prepare(struct c16_trial *w,unsigned narrow)
{
    struct cl_trial *c=&w->old;struct c16_counts n=c16_counts(c);enum pt_editor_mixed_readers_result r;
    r=narrow?pt_editor_mixed_causal_lineage_control_prepare_batch_begin(&c->control,c->change,c->command+1):
        pt_editor_mixed_causal_lineage_control16_prepare_batch_begin(&c->control,w->wide,c->command+1);
    assert(r==PT_EDITOR_MIXED_READERS_PENDING&&c->old.ordinary.calls==n.calls+1&&
        !cl_s(c)->command[c->command[1].slot].count&&c->control.stage==2);
    cl_advance(c,1);
    assert(c->old.ordinary.calls==n.calls+1&&c->old.chip.calls==n.chip&&c->old.masters.calls==n.masters&&
        c->old.card->writes==n.writes&&cs_cache_pins(&c->old)==n.pins&&
        pt_mixed_readers_readers_held(cl_s(c)->queue)==16);
}
static void c16_packet(struct c16_trial *w,uint64_t root,uint64_t ticket)
{
    struct cl_trial *c=&w->old;struct ct_command *copy=ct_command(&c->port.model.base,ticket);
    const struct pt_mixed_causal_packet *p=&w->copied.packet;unsigned i,seen_left=0,seen_right=0;
    assert(w->copies==1&&copy&&p->ticket==ticket&&p->frame==1920&&p->count==16&&
        p->first==oracle(1920)&&p->last==oracle(1921)&&!memcmp(p,&copy->packet,sizeof(*p))&&
        w->copied.predecessor.ticket==root&&w->copied.successor.ticket==ticket&&
        w->copied.first_tick==oracle(960)&&w->copied.last_tick==oracle(961)&&
        w->copied.observed==oracle(960)&&w->copied.issued==oracle(960));
    for(i=0;i<16;++i){const struct pt_mixed_readers_action *a=p->action+i;
        const struct pt_editor_mixed_causal_lineage_control16_action *wanted=w->wide->action+i;
        struct pt_mixed_readers_key original;
        assert(pt_mixed_causal_reader_key(cl_s(c)->causal,root,i,&original)==PT_MIXED_READERS_OK);
        assert(a->kind==PT_MIXED_READERS_CONTROL&&keys_equal(p->key+i,&original)&&
            a->route==original.route&&a->slot==original.slot&&p->key[i].trigger==root);
        if(a->route==PT_MIXED_READERS_PAULA){
            assert(!a->geometry.paula.data&&!a->geometry.paula.words&&
                a->geometry.paula.period==wanted->period&&a->geometry.paula.volume==wanted->volume);
        }else{uint16_t left=a->geometry.amigus.left,right=a->geometry.amigus.right;
            assert(a->route==PT_MIXED_READERS_AMIGUS&&!a->geometry.amigus.start&&!a->geometry.amigus.loop&&
                !a->geometry.amigus.end_exclusive&&!a->geometry.amigus.control&&
                a->geometry.amigus.rate==wanted->rate&&left==wanted->left&&right==wanted->right);
            seen_left|=left==0?1U:left==255?2U:left==256?4U:left==65535?8U:0U;
            seen_right|=right==0?1U:right==255?2U:right==256?4U:right==65535?8U:0U;
        }
    }
    if(w->wide->action[4].left<=255&&w->wide->action[5].left<=255&&
        w->wide->action[6].left<=255&&w->wide->action[7].left<=255)
        assert(seen_left==3&&seen_right==3);
    else assert(seen_left==15&&seen_right==15);
    /* p is the production queue-derived packet passed to the port; copy is
     * that port's separately retained value. Opaque event pointers are never
     * dereferenced or presented as an independent private queue inspection. */
}
static void c16_success(unsigned bits,unsigned cache,unsigned all_card,unsigned narrow)
{
    struct c16_trial *w=c16_make(bits,cache,0);struct cl_trial *c=&w->old;struct cs_trial *f=&c->old;
    struct pt_editor_mixed_reader_record reader_rows[32];struct ct_reader expected[32];
    struct pt_editor_mixed_causal_lineage_control16_batch input_before;
    struct pt_editor_mixed_causal_lineage_control_batch old_before;
    struct pt_mixed_causal_control_publication publication_before;
    struct pt_editor_mixed_command_ref refused={99,999};struct c16_counts first;
    uint64_t root,control,stop;unsigned i;
    if(all_card)cs_all_card(f);root=cl_first(c,1);c16_input(w,narrow);
    memcpy(reader_rows,cl_s(c)->reader,sizeof(reader_rows));memcpy(expected,c->port.model.base.reader,sizeof(expected));
    first=c16_counts(c);c16_prepare(w,narrow);control=cl_enqueue(c,1);
    assert(control!=root&&pt_mixed_readers_commands_held(cl_s(c)->queue)==2);
    assert(pt_editor_mixed_causal_prepare_publish(cl_s(c),c->command[1])==PT_MIXED_READERS_OK);c16_packet(w,root,control);
    memcpy(&input_before,w->wide,sizeof(input_before));memcpy(&old_before,c->change,sizeof(old_before));
    memcpy(&publication_before,&w->copied,sizeof(publication_before));
    ++w->wide->frame;w->wide->action[4].left^=1U;++c->change->frame;c->change->action[4].left^=1U;
    assert(!memcmp(&publication_before,&w->copied,sizeof(publication_before))&&
        !memcmp(&publication_before.packet,&ct_command(&c->port.model.base,control)->packet,sizeof(publication_before.packet)));
    for(i=0;i<16;++i){const struct pt_mixed_causal_packet *p=&w->copied.packet;
        struct ct_reader *actual=ct_reader(&c->port.model.base,p->key+i);struct ct_reader *wanted;
        assert(actual);wanted=expected+(actual-c->port.model.base.reader);
        if(p->action[i].route==PT_MIXED_READERS_PAULA){wanted->action.geometry.paula.period=p->action[i].geometry.paula.period;
            wanted->action.geometry.paula.volume=p->action[i].geometry.paula.volume;}
        else{wanted->action.geometry.amigus.rate=p->action[i].geometry.amigus.rate;
            wanted->action.geometry.amigus.left=p->action[i].geometry.amigus.left;
            wanted->action.geometry.amigus.right=p->action[i].geometry.amigus.right;}
    }
    c->port.model.base.ticks=oracle(1920)-1;
    assert(pt_mixed_causal_fire(cl_s(c)->causal,control)==PT_MIXED_CAUSAL_EARLY&&!c->port.model.controls);
    c->port.model.base.ticks++;
    assert(pt_mixed_causal_fire(cl_s(c)->causal,control)==PT_MIXED_CAUSAL_COMMITTED&&
        c->port.model.controls==16&&c->port.control_completed&&c->port.control.frame==1920&&
        c->port.control.observed==oracle(1920)&&c->port.control.issued==oracle(1920)&&
        !memcmp(expected,c->port.model.base.reader,sizeof(expected))&&!memcmp(reader_rows,cl_s(c)->reader,sizeof(reader_rows)));
    assert(!memcmp(&publication_before,&w->copied,sizeof(publication_before)));
    memcpy(w->wide,&input_before,sizeof(input_before));memcpy(c->change,&old_before,sizeof(old_before));
    cl_stop_input(c,16);
    assert(pt_editor_mixed_causal_lineage_stop_prepare_batch_begin(&c->control,c->stop,&refused)==PT_EDITOR_MIXED_READERS_INVALID&&
        refused.slot==99&&refused.serial==999&&f->ordinary.calls==first.calls+1&&!c->control.root_closed);
    cl_detach_root(c);assert(pt_mixed_readers_commands_held(cl_s(c)->queue)==1);
    cl_later_prepare(c,1);stop=cl_enqueue(c,2);
    assert(stop!=root&&stop!=control&&c->command[2].slot==c->command[0].slot&&
        c->command[2].serial!=c->command[0].serial&&c->control.root_closed&&c->control.root_detached);
    assert(pt_editor_mixed_causal_prepare_publish(cl_s(c),c->command[0])==PT_MIXED_READERS_INVALID);
    assert(pt_editor_mixed_causal_prepare_publish(cl_s(c),c->command[2])==PT_MIXED_READERS_OK);
    {struct ct_command *p=ct_command(&c->port.model.base,stop);assert(p&&p->packet.frame==2880&&
        p->packet.first==oracle(2880)&&p->packet.last==oracle(2881)&&p->packet.count==16);}
    c->port.model.base.ticks=oracle(2880)-1;
    assert(pt_mixed_causal_fire(cl_s(c)->causal,stop)==PT_MIXED_CAUSAL_EARLY&&!c->port.model.stops);
    c->port.model.base.ticks++;
    assert(pt_mixed_causal_fire(cl_s(c)->causal,stop)==PT_MIXED_CAUSAL_COMMITTED&&c->port.model.stops==16&&
        !c->port.model.base.mask&&pt_mixed_readers_readers_held(cl_s(c)->queue)==16&&
        !memcmp(reader_rows,cl_s(c)->reader,sizeof(reader_rows))&&f->ordinary.calls==first.calls+2&&
        f->chip.calls==first.chip&&f->masters.calls==first.masters&&f->card->writes==first.writes&&
        cs_cache_pins(f)==first.pins&&c->high_water<=3+2+16);
    assert(pt_editor_mixed_causal_lineage_control16_prepare_batch_begin(&c->control,w->wide,&refused)==PT_EDITOR_MIXED_READERS_CAPACITY);
    assert(pt_editor_mixed_causal_lineage_stop_prepare_batch_begin(&c->control,c->stop,&refused)==PT_EDITOR_MIXED_READERS_CAPACITY);
    for(i=0;i<16;++i){struct pt_mixed_readers_reader_receipt out,before;struct pt_mixed_readers_key original;
        memset(&out,0xa5,sizeof(out));memcpy(&before,&out,sizeof(before));
        assert(pt_editor_mixed_causal_prepare_service_reader(cl_s(c),f->reader[i],0,&out)==PT_MIXED_READERS_PENDING&&
            !memcmp(&out,&before,sizeof(out))&&
            pt_mixed_causal_reader_key(cl_s(c)->causal,root,i,&original)==PT_MIXED_READERS_STALE);}
    assert(pt_editor_mixed_causal_prepare_service_command(cl_s(c),c->command[1],0,NULL)==PT_MIXED_READERS_OK);
    assert(pt_editor_mixed_causal_prepare_service_command(cl_s(c),c->command[2],0,NULL)==PT_MIXED_READERS_OK);
    assert(pt_mixed_readers_readers_held(cl_s(c)->queue)==16&&cs_cache_pins(f)==first.pins);
    c->port.model.quiet_mask=~0U;
    for(i=0;i<16;++i)assert(pt_editor_mixed_causal_prepare_service_reader(cl_s(c),f->reader[i],0,NULL)==PT_MIXED_READERS_OK);
    assert(!pt_mixed_readers_commands_held(cl_s(c)->queue)&&!pt_mixed_readers_readers_held(cl_s(c)->queue));
    cs_same(f);c16_drop(w);
}
static void c16_guard_refusal(unsigned mode)
{
    struct c16_trial *w=c16_make(24,16,0);struct cl_trial *c=&w->old;
    struct pt_editor_mixed_causal_lineage_prepare saved;
    struct pt_editor_mixed_causal_lineage_control16_batch input;
    struct c16_counts before;unsigned char target_before[sizeof(struct pt_sampler_mixed_command_handle)];void *alias;
    assert(mode<6);(void)cl_first(c,1);c16_input(w,0);
    if(mode<4){
        size_t pool_bytes=pt_sampler_mixed_pool_size(),protected_bytes;
        unsigned char *pool_before=malloc(pool_bytes),*protected_before;void *protected;
        if(mode==0){c16_card_subset(w);alias=(char *)w->wide+sizeof(*w->wide)-sizeof(struct pt_editor_mixed_command_ref);}
        else if(mode==1)alias=(char *)c->old.causal_workspace+c->old.causal_capacity-sizeof(struct pt_editor_mixed_command_ref);
        else if(mode==2)alias=(char *)c->old.factory_workspace+c->old.factory_capacity-sizeof(struct pt_editor_mixed_command_ref);
        else alias=cl_s(c)->reader[c->old.reader[0].slot].handle.address;
        if(mode==0){protected=w->wide;protected_bytes=sizeof(*w->wide);}
        else if(mode==1){protected=c->old.causal_workspace;protected_bytes=c->old.causal_capacity;}
        else if(mode==2){protected=c->old.factory_workspace;protected_bytes=c->old.factory_capacity;}
        else{protected=alias;protected_bytes=pt_sampler_mixed_reader_size();}
        protected_before=malloc(protected_bytes);assert(pool_before&&protected_before);
        memcpy(pool_before,cl_s(c)->pool,pool_bytes);memcpy(protected_before,protected,protected_bytes);
        assert((uintptr_t)alias%offsetof(struct {char c;struct pt_editor_mixed_command_ref r;},r)==0);
        memcpy(target_before,alias,sizeof(struct pt_editor_mixed_command_ref));
        memcpy(&saved,&c->control,sizeof(saved));memcpy(&input,w->wide,sizeof(input));before=c16_counts(c);
        assert(pt_editor_mixed_causal_lineage_control16_prepare_batch_begin(&c->control,w->wide,alias)==PT_EDITOR_MIXED_READERS_INVALID);
        assert(!memcmp(target_before,alias,sizeof(struct pt_editor_mixed_command_ref))&&
            !memcmp(&saved,&c->control,sizeof(saved))&&!memcmp(&input,w->wide,sizeof(input))&&
            !memcmp(pool_before,cl_s(c)->pool,pool_bytes)&&!memcmp(protected_before,protected,protected_bytes));
        c16_counts_same(c,&before);free(protected_before);free(pool_before);
    }else{
        struct pt_sampler_mixed_causal_lineage_control16_batch *b=calloc(1,sizeof(*b)),original;
        size_t pool_bytes=pt_sampler_mixed_pool_size();unsigned char *pool_before=malloc(pool_bytes);
        struct pt_sampler_mixed_causal_lineage_control16_action *a;
        assert(b&&pool_before);b->frame=1920;b->count=1;a=b->action;
        a->reader=cl_s(c)->reader[c->old.reader[4].slot].handle;a->rate=0x10004U;a->left=65535;a->right=256;
        if(mode==4)alias=(char *)b+sizeof(*b)-sizeof(struct pt_sampler_mixed_command_handle);
        else{const struct pt_pcm *pcm=c->old.pcm;assert(pcm->data&&pcm->capacity*sizeof(int32_t)>=sizeof(target_before));
            alias=(char *)pcm->data+pcm->capacity*sizeof(int32_t)-sizeof(struct pt_sampler_mixed_command_handle);}
        assert((uintptr_t)alias%offsetof(struct {char c;struct pt_sampler_mixed_command_handle h;},h)==0);
        memcpy(target_before,alias,sizeof(target_before));memcpy(&original,b,sizeof(original));
        memcpy(pool_before,cl_s(c)->pool,pool_bytes);memcpy(&saved,&c->control,sizeof(saved));before=c16_counts(c);
        assert(pt_sampler_mixed_causal_lineage_control16_begin(cl_s(c)->pool,cl_s(c)->revision,b,alias)==PT_SAMPLER_MIXED_INVALID);
        assert(!memcmp(target_before,alias,sizeof(target_before))&&!memcmp(b,&original,sizeof(original))&&
            !memcmp(pool_before,cl_s(c)->pool,pool_bytes)&&!memcmp(&saved,&c->control,sizeof(saved)));
        c16_counts_same(c,&before);free(pool_before);free(b);
    }
    assert(c->control.stage==1&&!cl_s(c)->first_error);cs_same(&c->old);c16_drop(w);
}
static void c16_authority_refusal(unsigned mode)
{
    struct c16_trial *w=c16_make(16,8,0);struct cl_trial *c=&w->old;
    struct pt_editor_mixed_command_ref out={99,999};struct pt_editor_mixed_reader_record rows[32];
    struct pt_editor_mixed_causal_lineage_control16_batch original;struct c16_counts before;
    assert(mode<3);(void)cl_first(c,mode!=0);c16_input(w,0);
    if(mode==1)++w->wide->action[4].reader.serial;
    if(mode==2)w->wide->action[5].reader=w->wide->action[4].reader;
    memcpy(&original,w->wide,sizeof(original));memcpy(rows,cl_s(c)->reader,sizeof(rows));before=c16_counts(c);
    assert(pt_editor_mixed_causal_lineage_control16_prepare_batch_begin(&c->control,w->wide,&out)==PT_EDITOR_MIXED_READERS_INVALID&&
        out.slot==99&&out.serial==999&&c->control.stage==1&&!cl_s(c)->first_error&&
        !memcmp(&original,w->wide,sizeof(original))&&!memcmp(rows,cl_s(c)->reader,sizeof(rows))&&
        c->old.ordinary.calls==before.calls&&c->old.chip.calls==before.chip&&
        c->old.masters.calls==before.masters&&c->old.card->writes==before.writes&&cs_cache_pins(&c->old)==before.pins);
    if(mode)assert(c->old.owned_calls==before.owned);
    cs_same(&c->old);c16_drop(w);
}
static void c16_allocator_outcome(unsigned mode)
{
    struct c16_trial *w=c16_make(24,16,0);struct cl_trial *c=&w->old;struct cs_trial *f=&c->old;
    struct pt_editor_mixed_command_ref *out=c->command+1;
    struct pt_editor_mixed_reader_record rows[32];struct pt_sampler_storage_span guards[PT_EDITOR_MIXED_READERS_GUARDS];
    struct cs_ledger ledger[80];struct c16_counts before;unsigned gcount,refusals;
    enum pt_editor_mixed_readers_result r;assert(mode<5);(void)cl_first(c,1);c16_input(w,0);
    *out=(struct pt_editor_mixed_command_ref){99,999};before=c16_counts(c);gcount=cl_s(c)->guard_count;refusals=f->ordinary.refusals;
    memcpy(rows,cl_s(c)->reader,sizeof(rows));memcpy(guards,cl_s(c)->guards,sizeof(guards));memcpy(ledger,f->ordinary.live,sizeof(ledger));
    if(mode==0)f->ordinary.fail=before.calls+1;
    if(mode==1)f->ordinary.alias=(char *)w->wide+sizeof(*w->wide)-sizeof(w->wide->action[15]);
    if(mode==2)c->allocation_reentry=1;
    if(mode==3)w->mutate_wide=1;
    if(mode==4)c->scratch_alias=1;
    r=pt_editor_mixed_causal_lineage_control16_prepare_batch_begin(&c->control,w->wide,out);
    assert(r!=PT_EDITOR_MIXED_READERS_PENDING&&out->slot==99&&out->serial==999&&f->ordinary.calls==before.calls+1&&
        c->control.stage==2&&!c->control.constructing&&!memcmp(rows,cl_s(c)->reader,sizeof(rows))&&
        cl_s(c)->guard_count==gcount&&!memcmp(guards,cl_s(c)->guards,sizeof(guards))&&f->chip.calls==before.chip&&
        f->masters.calls==before.masters&&f->card->writes==before.writes&&cs_cache_pins(f)==before.pins&&
        pt_mixed_readers_readers_held(cl_s(c)->queue)==16);
    if(mode!=3)assert(!cl_s(c)->command[1].handle.address&&cs_live(&f->ordinary)==before.live&&
        !memcmp(ledger,f->ordinary.live,sizeof(ledger)));
    else{const struct pt_editor_mixed_command_record *record=cl_s(c)->command+1;
        assert(record->handle.address&&record->handle.token&&record->serial&&!record->transferred&&!record->ticket&&!record->count&&
            c->control.control.slot==1&&c->control.control.serial==record->serial&&cs_live(&f->ordinary)==before.live+1&&
            f->ordinary.releases==before.releases);--w->wide->frame;}
    if(mode==0)assert(r==PT_EDITOR_MIXED_READERS_CAPACITY&&!cl_s(c)->first_error&&
        f->ordinary.refusals==refusals+1&&f->ordinary.releases==before.releases);
    else assert(cl_s(c)->first_error);
    if(mode==1||mode==4)assert(f->ordinary.refusals==refusals+1&&f->ordinary.releases==before.releases&&!f->ordinary.alias_releases);
    if(mode==2)assert(f->ordinary.releases==before.releases+1);
    if(mode==4)assert(c->scratch_hits==1&&!c->scratch_alias);
    /* The constructor has consumed its lifetime stage, including NULL and
     * numeric alias refusal. The full transformed scratch alias is observed
     * inside the allocator; no ownership write admits that returned pointer. */
    assert(pt_editor_mixed_causal_lineage_control16_prepare_batch_begin(&c->control,w->wide,out)!=PT_EDITOR_MIXED_READERS_PENDING&&
        f->ordinary.calls==before.calls+1);cs_same(f);c16_drop(w);
}
static void c16_positive_enqueue_fault(void)
{
    struct c16_trial *w=c16_make(24,16,0);struct cl_trial *c=&w->old;
    struct pt_editor_mixed_reader_record rows[32];struct pt_editor_mixed_command_record *record;
    uint64_t ticket=777;struct c16_counts before;
    (void)cl_first(c,1);c16_input(w,0);c16_prepare(w,0);before=c16_counts(c);
    memcpy(rows,cl_s(c)->reader,sizeof(rows));c->old.owned_hook=1;c->old.owned_after=w->wide->count+1;
    /* Every selected original ACTIVE getter precedes the actual lower holder
     * callback. Reentry there must preserve positively returned queue custody. */
    assert(pt_editor_mixed_causal_prepare_enqueue(cl_s(c),c->command[1],&ticket)==PT_MIXED_READERS_OK&&ticket!=777);
    record=cl_s(c)->command+c->command[1].slot;
    assert(!c->old.owned_hook&&cl_s(c)->first_error&&record->transferred&&record->ticket==ticket&&
        record->handle.address&&record->handle.token&&c->control.control_ticket==ticket&&
        !memcmp(rows,cl_s(c)->reader,sizeof(rows))&&pt_mixed_readers_commands_held(cl_s(c)->queue)==2&&
        c->old.ordinary.calls==before.calls&&c->old.chip.calls==before.chip&&
        c->old.masters.calls==before.masters&&c->old.card->writes==before.writes&&cs_cache_pins(&c->old)==before.pins);
    assert(pt_editor_mixed_causal_prepare_publish(cl_s(c),c->command[1])==PT_MIXED_READERS_INVALID);
    cs_same(&c->old);c16_drop(w);
}

struct c16_core_case {
    struct trial *trial;struct cc_port port;struct ct_memory memory;
    struct pt_mixed_causal_config config;struct pt_mixed_causal_owner *owner;
    void *workspace;uint8_t *before;size_t before_bytes;
};
static struct c16_core_case *c16_core_make(void)
{
    struct c16_core_case *c=calloc(1,sizeof(*c));struct pt_mixed_causal_control_port port;
    assert(c);++c16_cases;++c16_core_cases;c->trial=trial_make(24,16);
    c->before=save(c->trial->resources,&c->before_bytes);
    c->port.base.ticks=100;c->port.base.frequency=709379;c->port.base.commit_raw=c->port.base.source_raw=1;c->port.publication_raw=1;
    c->config.allocator=(struct pt_allocator){&c->memory,ct_allocate,ct_release};
    c->config.allocator_context=(struct pt_mixed_readers_span){&c->memory,sizeof(c->memory)};
    c->config.grid=(struct pt_mixed_readers_grid){100,17,709379,48000};c->config.session=19;
    c->config.control_budget=pt_mixed_causal_control_size();c->config.queue_budget=pt_mixed_readers_control_size();
    c->config.port=(struct pt_mixed_causal_port){&c->port,sizeof(c->port),1,31,ct_clock,ct_publish,cc_commit,
        cc_command_quiet,ct_reader_quiet,ct_source_close,ct_source_quiet,ct_publish_successor};
    c->workspace=calloc(1,pt_mixed_causal_workspace_size());assert(c->workspace);
    assert(pt_mixed_causal_open(&c->config,c->workspace,pt_mixed_causal_workspace_size(),&c->owner)==PT_MIXED_READERS_OK);
    assert(pt_mixed_causal_borrow_queue(c->owner,&c->trial->queue)==PT_MIXED_READERS_OK&&c->memory.live==2);
    c->port.base.registration=(struct pt_mixed_causal_registration){c->owner,c->trial->queue,19,17};
    port=(struct pt_mixed_causal_control_port){&c->port,sizeof(c->port),1,3,c16_core_publish};
    assert(pt_mixed_causal_control_bind(c->owner,&port)==PT_MIXED_READERS_OK);return c;
}
static void c16_core_guard(unsigned mode)
{
    struct c16_core_case *c=c16_core_make();struct trial *t=c->trial;
    struct pt_mixed_causal_control16_request *input=calloc(1,sizeof(*input)),before;
    struct holder holder_before;struct cc_port port_before;struct ct_memory memory_before;
    size_t owner_bytes=pt_mixed_causal_control_size();unsigned char *owner_before=malloc(owner_bytes);
    uint64_t first=0;unsigned i;void *alias;
    assert(mode<2&&input&&owner_before);trigger_input(t,16,0,960);
    assert(pt_mixed_causal_enqueue(c->owner,t->input,&first)==PT_MIXED_READERS_OK&&first);
    assert(pt_mixed_causal_publish(c->owner,first)==PT_MIXED_READERS_OK);
    c->port.base.ticks=oracle(960);
    assert(pt_mixed_causal_fire(c->owner,first)==PT_MIXED_CAUSAL_COMMITTED&&c->port.first_completed);
    c->port.hold_first=1;
    assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_PENDING);
    holder_init(t->command+1,101);t->command[1].queue=t->queue;
    input->predecessor=first;input->frame=1920;input->count=mode?16:1;input->command=control_of(t->command+1);
    for(i=0;i<input->count;++i){struct pt_mixed_readers_key actual;struct pt_mixed_causal_control16_action *a=input->action+i;
        unsigned index=mode?i:4;
        assert(pt_mixed_causal_reader_key(c->owner,first,index,&actual)==PT_MIXED_READERS_OK);
        /* Numeric route/slot are derived from an actual issued ACTIVE getter;
         * enqueue must independently obtain the authoritative original key. */
        a->route=actual.route;a->slot=actual.slot;
        if(a->route==PT_MIXED_READERS_PAULA){a->period=(uint16_t)(214+index);a->volume=(uint8_t)(32+index);}
        else{static const uint16_t boundary[4]={0,255,256,65535};
            a->rate=0x10000U+index;a->left=boundary[index%4];a->right=boundary[(index+1)%4];}
    }
    alias=mode?(char *)(t->command+1)+sizeof(t->command[1])-sizeof(uint64_t):
        (char *)input+sizeof(*input)-sizeof(uint64_t);
    assert((uintptr_t)alias%offsetof(struct {char c;uint64_t v;},v)==0);
    memcpy(&before,input,sizeof(before));memcpy(&holder_before,t->command+1,sizeof(holder_before));
    memcpy(&port_before,&c->port,sizeof(port_before));memcpy(&memory_before,&c->memory,sizeof(memory_before));
    memcpy(owner_before,c->owner,owner_bytes);
    assert(pt_mixed_causal_control16_enqueue(c->owner,input,alias)==PT_MIXED_READERS_INVALID&&
        !memcmp(input,&before,sizeof(before))&&!memcmp(t->command+1,&holder_before,sizeof(holder_before))&&
        !memcmp(&c->port,&port_before,sizeof(port_before))&&!memcmp(&c->memory,&memory_before,sizeof(memory_before))&&
        !memcmp(c->owner,owner_before,owner_bytes)&&pt_mixed_readers_commands_held(t->queue)==1&&
        pt_mixed_readers_readers_held(t->queue)==16);
    held_terminal(t->command+1,t->command[1].token,1);held_release(t->command+1,t->command[1].token);
    assert(pt_mixed_causal_service_command(c->owner,first,1,NULL)==PT_MIXED_READERS_OK);
    for(i=0;i<16;++i)assert(pt_mixed_causal_service_reader(c->owner,first,i,1,NULL)==PT_MIXED_READERS_OK);
    assert(!pt_mixed_readers_commands_held(t->queue)&&!pt_mixed_readers_readers_held(t->queue));
    same_save(t->resources,c->before,c->before_bytes);
    assert(pt_mixed_causal_close(&c->owner)&&!c->owner&&!c->memory.live);t->queue=NULL;
    same_save(t->resources,c->before,c->before_bytes);trial_drop(t);
    free(owner_before);free(input);free(c->before);free(c->workspace);free(c);
}
int main(void)
{
    unsigned i;
    c16_success(8,8,0,0);c16_success(16,16,0,0);c16_success(24,16,0,0);
    c16_success(8,16,1,0);c16_success(16,8,1,0);c16_success(24,16,1,0);
    c16_success(8,8,0,1);c16_success(24,16,1,1);
    for(i=0;i<6;++i)c16_guard_refusal(i);
    for(i=0;i<3;++i)c16_authority_refusal(i);
    for(i=0;i<5;++i)c16_allocator_outcome(i);
    c16_positive_enqueue_fault();c16_core_guard(0);c16_core_guard(1);
    assert(c16_cases==25&&c16_controller_cases==23&&c16_core_cases==2&&cl_cases==23&&cs_cases==23);
    puts("EDITOR MIXED CAUSAL CONTROL16 PREPARE PASS:25 genuine instances;6 mixed/card8/16/24master8/16cache schedules exact0/255/256/65535 copied levels;2 unchanged uint8 schedules;6 full-input/workspace/reader/master output aliases;3 actual-first/stale/duplicate authority refusals;5 consumed allocator/reentry/input/transformed-scratch outcomes;1 lower enqueue OK amid outer fault;2 direct core whole-input/holder aliases;original960/1920/2880 windows,genuine C1 service/NULL close,C3 old STOP,zero later readers/pins/cache/upload,independent quiet,complete master/save custody;SOFTWARE_ONLY");
    return 0;
}
