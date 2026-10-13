#include "editor_document_replacement.h"
#include "editor_mixed_internal.h"
#include "mixed_owner_established_internal.h"
#include "sampler_lifetime_internal.h"
#include "../core/document_retirement_internal.h"
#include "../core/render_storage_internal.h"
#include <string.h>
static struct pt_editor_document_replacement *active;
static int span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int apart(const void *a,size_t an,const void *b,size_t bn)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return span(a,an)&&span(b,bn)&&(!an||!bn||x>=y+bn||y>=x+an);}
static int zero(const void *p,size_t n)
{const unsigned char *v=p;size_t i;for(i=0;i<n;++i)if(v[i])return 0;return 1;}
static int contains(struct pt_sampler_storage_span s,const void *p,size_t n)
{return span(s.data,s.bytes)&&span(p,n)&&(uintptr_t)p>=(uintptr_t)s.data&&
    n<=s.bytes&&(uintptr_t)p-(uintptr_t)s.data<=s.bytes-n;}
static int headers(struct pt_editor_document_replacement *c)
{return !memcmp(c->document,&c->old_header,sizeof c->old_header)&&
    !memcmp(c->candidate,&c->next_header,sizeof c->next_header);}
static enum pt_document_replace_result held(struct pt_editor_document_replacement *c)
{c->result=PT_DOCUMENT_REPLACE_HOLD;return c->result;}
static void before_call(struct pt_editor_document_replacement *c,struct pt_editor_document_replacement *saved)
{c->busy=1;memcpy(saved,c,sizeof(*saved));}
static int after_control(struct pt_editor_document_replacement *c,struct pt_editor_document_replacement *saved)
{c->busy=0;saved->busy=0;return !memcmp(c,saved,sizeof(*c))&&!c->fault;}
static int after(struct pt_editor_document_replacement *c,struct pt_editor_document_replacement *saved)
{return after_control(c,saved)&&headers(c);}
static enum pt_document_replace_result no_owner(struct pt_editor_document_replacement *c)
{
    int quiet;struct pt_editor_document_replacement saved;
    before_call(c,&saved);quiet=pt_editor_mixed_stop(c->binding);
    if(!after(c,&saved)||!quiet)return held(c);
    c->result=PT_DOCUMENT_REPLACE_COMMITTED_NO_OWNER;return c->result;
}
enum pt_document_replace_result pt_editor_document_replacement_begin(
    struct pt_editor_document_replacement *c,struct pt_document *d,struct pt_document *next,
    struct pt_editor_mixed *o,struct pt_mixed_established *old,
    struct pt_paula_voices *p,struct pt_wavetable_voices *w,
    struct pt_editor_mixed_establish *establish,struct pt_mixed_established *fresh,
    struct pt_editor_mixed_bridges *bridges,const struct pt_editor_mixed_bridge_inputs *in,
    const struct pt_render_options *options,const struct pt_paula_render_caps *caps,
    const struct pt_playback_format *format,struct pt_sampler_storage_span context,unsigned work)
{
    struct pt_editor *e;const void *controls[17];size_t sizes[17],bytes[7];void *owned[7];unsigned i,j;
    if(active||!c||!d||!next||d==next||!o||!old||!p||!w||!establish||!fresh||old==fresh||
       !bridges||!in||!options||!caps||!format||!work||work>256||
       !pt_editor_mixed_attached(o)||!(e=o->editor)||e->project!=&d->project||
       !pt_editor_mixed_checked_binding_current(o,old,p,w)||
       o->preparation_context||o->preparation_close||
       !p->bridge||!w->bridge||p->song_owner!=o->owner||w->song_owner!=o->owner||
       p->bridge->sampler!=&e->sampler||w->bridge->sampler!=&e->sampler||
       p->bridge->project!=&d->project||w->bridge->project!=&d->project||
       !contains(context,c,sizeof(*c))||
       !next->loaded||pt_project_validate(&next->project,NULL)!=PT_PROJECT_OK||
       !pt_document_replacement_allowed(next)||!in->backend||!in->backend->reservation||
       in->backend==w->bridge->backend)return PT_DOCUMENT_REPLACE_INVALID;
    controls[0]=c;sizes[0]=sizeof(*c);controls[1]=establish;sizes[1]=sizeof(*establish);
    controls[2]=fresh;sizes[2]=sizeof(*fresh);controls[3]=bridges;sizes[3]=sizeof(*bridges);
    controls[4]=d;sizes[4]=sizeof(*d);controls[5]=next;sizes[5]=sizeof(*next);
    controls[6]=e;sizes[6]=sizeof(*e);controls[7]=o;sizes[7]=sizeof(*o);
    controls[8]=old;sizes[8]=sizeof(*old);controls[9]=in->backend;sizes[9]=sizeof(*in->backend);
    controls[10]=in->backend->reservation;sizes[10]=sizeof(*in->backend->reservation);
    controls[11]=p;sizes[11]=sizeof(*p);controls[12]=w;sizes[12]=sizeof(*w);
    controls[13]=p->bridge;sizes[13]=sizeof(*p->bridge);
    controls[14]=w->bridge;sizes[14]=sizeof(*w->bridge);
    controls[15]=w->bridge->backend;sizes[15]=sizeof(*w->bridge->backend);
    controls[16]=w->bridge->backend->reservation;sizes[16]=sizeof(*w->bridge->backend->reservation);
    if(!apart(c,sizeof(*c),in,sizeof(*in))||!apart(c,sizeof(*c),options,sizeof(*options))||
       !apart(c,sizeof(*c),caps,sizeof(*caps))||!apart(c,sizeof(*c),format,sizeof(*format)))
        return PT_DOCUMENT_REPLACE_INVALID;
    for(i=0;i<17;++i){
        for(j=0;j<i;++j)if(!apart(controls[i],sizes[i],controls[j],sizes[j]))return PT_DOCUMENT_REPLACE_INVALID;
        if(i!=5&&!pt_document_storage_output_disjoint(next,controls[i],sizes[i]))return PT_DOCUMENT_REPLACE_INVALID;
        if(i!=4&&!pt_document_storage_output_disjoint(d,controls[i],sizes[i]))return PT_DOCUMENT_REPLACE_INVALID;
        if(i<4&&(!pt_sampler_output_disjoint(&e->sampler,controls[i],sizes[i])||
           !pt_render_project_storage_output_disjoint(&d->project,controls[i],sizes[i])))return PT_DOCUMENT_REPLACE_INVALID;
    }
    if(!pt_document_owned_storage_spans(next,owned,bytes))return PT_DOCUMENT_REPLACE_INVALID;
    for(i=0;i<7;++i)if(bytes[i]&&
       (!pt_document_storage_output_disjoint(d,owned[i],bytes[i])||
        !pt_sampler_output_disjoint(&e->sampler,owned[i],bytes[i])||
        !pt_render_project_storage_output_disjoint(&d->project,owned[i],bytes[i])))
        return PT_DOCUMENT_REPLACE_INVALID;
    if(!zero(c,sizeof(*c))||!zero(establish,sizeof(*establish))||
       !zero(fresh,sizeof(*fresh))||!zero(bridges,sizeof(*bridges)))return PT_DOCUMENT_REPLACE_INVALID;
    memset(c,0,sizeof(*c));c->self=c;c->document=d;c->candidate=next;c->editor=e;c->binding=o;
    c->old_control=old;c->old_paula=p;c->old_amigus=w;c->establish=establish;c->new_control=fresh;c->new_bridges=bridges;
    memcpy(&c->old_header,d,sizeof(*d));memcpy(&c->next_header,next,sizeof(*next));
    memcpy(&c->inputs,in,sizeof(*in));memcpy(&c->options,options,sizeof(*options));
    memcpy(&c->caps,caps,sizeof(*caps));memcpy(&c->format,format,sizeof(*format));
    c->sampler_allocator=e->sampler.allocator;c->sampler_budget=e->sampler.budget;c->context=context;
    c->parent=(struct pt_sampler_storage_span){c,sizeof(*c)};c->work=work;c->result=PT_DOCUMENT_REPLACE_PREPARED;
    if(!pt_document_owned_storage_spans(d,c->protected_storage,c->protected_bytes)||
       !pt_document_owned_storage_spans(next,c->protected_storage+7,c->protected_bytes+7)){
        memset(c,0,sizeof(*c));return PT_DOCUMENT_REPLACE_INVALID;}
    active=c;
    return c->result;
}
enum pt_document_replace_result pt_editor_document_replacement_step(struct pt_editor_document_replacement *c)
{
    int a,b;enum pt_establish_result r;enum pt_mixed_owner_result owner;
    struct pt_editor_document_replacement saved;
    if(!c||active!=c||c->self!=c)return PT_DOCUMENT_REPLACE_INVALID;
    if(c->busy){c->fault=1;return PT_DOCUMENT_REPLACE_HOLD;}
    if(c->result==PT_DOCUMENT_REPLACE_HOLD||c->result==PT_DOCUMENT_REPLACE_COMMITTED||
       c->result==PT_DOCUMENT_REPLACE_COMMITTED_NO_OWNER)return c->result;
    if(c->fault||!headers(c))return held(c);
    if(c->phase==0)c->phase=1;
    if(c->phase==1){
        before_call(c,&saved);a=pt_editor_mixed_stop(c->binding);
        if(!after(c,&saved))return held(c);
        if(!a){c->result=PT_DOCUMENT_REPLACE_WAITING;return c->result;}
        before_call(c,&saved);a=pt_paula_voices_close(c->old_paula);b=pt_wavetable_voices_close(c->old_amigus);
        if(!after(c,&saved))return held(c);
        if(!a||!b){c->result=PT_DOCUMENT_REPLACE_WAITING;return c->result;}
        before_call(c,&saved);a=pt_editor_dispose(c->editor);
        if(!after(c,&saved)||!a)return held(c);
        /* No descriptor traversal after dispose: the original sample table can
         * be retired. Its captured numeric ownership/header must remain exact. */
        c->phase=2;
    }
    if(c->phase==2){
        if(!pt_sampler_lifetime_storage_retired(&c->editor->sampler)){
            c->result=PT_DOCUMENT_REPLACE_RETIRING;return c->result;
        }
        if(!pt_editor_mixed_detach(c->binding))return held(c);
        if(c->release_attempted)return held(c);
        c->release_attempted=1;before_call(c,&saved);
        a=pt_document_release_checked(c->document)==PT_DOCUMENT_RELEASED;
        if(!after_control(c,&saved)||!a||memcmp(c->candidate,&c->next_header,sizeof c->next_header))return held(c);
        /* Transfer once, after every old allocation was positively retired.
         * No rollback or old-descriptor access exists beyond this boundary. */
        memcpy(c->document,&c->next_header,sizeof(*c->document));
        memcpy(c->protected_storage,c->protected_storage+7,7*sizeof(*c->protected_storage));
        memcpy(c->protected_bytes,c->protected_bytes+7,7*sizeof(*c->protected_bytes));
        memset(c->protected_storage+7,0,7*sizeof(*c->protected_storage));
        memset(c->protected_bytes+7,0,7*sizeof(*c->protected_bytes));
        {struct pt_allocator allocator=c->candidate->allocator;
         memset(c->candidate,0,sizeof(*c->candidate));c->candidate->allocator=allocator;}
        memcpy(&c->old_header,c->document,sizeof c->old_header);
        memcpy(&c->next_header,c->candidate,sizeof c->next_header);
        c->phase=3;c->result=PT_DOCUMENT_REPLACE_PUBLISHED;
        if(!pt_editor_init(c->editor,&c->document->project))return no_owner(c);
        c->editor->sampler.allocator=c->sampler_allocator;c->editor->sampler.budget=c->sampler_budget;
        if(!pt_editor_mixed_attach(c->binding,c->editor))return held(c);
        before_call(c,&saved);r=pt_editor_mixed_establish_begin(c->establish,c->binding,&c->parent,1);
        if(!after(c,&saved))return held(c);
        if(r!=PT_ESTABLISH_PENDING&&r!=PT_ESTABLISH_READY)return no_owner(c);
    }
    if(c->phase==3){
        before_call(c,&saved);r=pt_editor_mixed_establish_step(c->establish,c->work);
        if(!after(c,&saved))return held(c);
        if(r==PT_ESTABLISH_PENDING)return c->result;
        if(r!=PT_ESTABLISH_READY)return no_owner(c);
        before_call(c,&saved);a=pt_editor_mixed_stop(c->binding);
        if(!after(c,&saved)||!a)return held(c);
        c->phase=4;
    }
    if(c->phase==4){
        if(!pt_editor_mixed_bridges_bind(c->new_bridges,c->binding,&c->inputs,&c->context,1))return no_owner(c);
        before_call(c,&saved);owner=pt_editor_mixed_checked_begin(c->binding,c->new_control,
            &c->new_bridges->paula,&c->new_bridges->amigus,&c->options,&c->caps,&c->format,c->context,c->work);
        if(!after(c,&saved))return held(c);
        if(owner!=PT_MIXED_OWNER_PREPARING)return no_owner(c);
        if(!pt_editor_mixed_checked_binding_current(c->binding,c->new_control,
            &c->new_bridges->paula,&c->new_bridges->amigus))return held(c);
        c->phase=5;c->result=PT_DOCUMENT_REPLACE_COMMITTED;return c->result;
    }
    return c->result;
}
int pt_editor_document_replacement_finish(struct pt_editor_document_replacement *c)
{
    if(!c)return 1;
    if(!c->self)return active!=c;
    if(c->self!=c||active!=c)return 0;
    if(c->busy){c->fault=1;return 0;}
    if(c->result==PT_DOCUMENT_REPLACE_PREPARED&&!c->phase){active=NULL;memset(c,0,sizeof(*c));return 1;}
    if((c->result!=PT_DOCUMENT_REPLACE_COMMITTED&&c->result!=PT_DOCUMENT_REPLACE_COMMITTED_NO_OWNER)||
       c->binding->owner||c->binding->transport||c->binding->preparation_context||c->binding->preparation_close||
       c->binding->owner_finish||c->binding->owner_finish_context||c->new_control->storage.memory.active||
       c->establish->binding||c->establish->job.active||c->establish->job.busy||
       !pt_sampler_lifetime_storage_retired(&c->editor->sampler))return 0;
    active=NULL;memset(c,0,sizeof(*c));return 1;
}

static int retirement_frame(const struct pt_editor_document_replacement *c)
{
    return c&&active==c&&c->self==c&&!c->fault&&c->busy&&c->phase==2&&c->release_attempted&&
        !c->binding->editor&&!c->binding->owner&&!c->binding->transport&&
        !c->binding->owner_finish&&!c->binding->owner_finish_context&&
        !c->binding->preparation_close&&!c->binding->preparation_context&&
        !c->old_control->storage.memory.active&&!c->old_control->startup&&!c->old_control->starting&&
        pt_sampler_lifetime_storage_retired(&c->editor->sampler);
}
/* Core negative veto, opt-in to this private typed composition. Registration is
 * issued only by the genuine final factory/publisher checks in begin. There is
 * no caller permit/status parameter. The sole positive old release occurs
 * synchronously after actual closes/disposal and storage retirement. */
int pt_document_typed_replacement_project_allowed(const struct pt_project *p)
{
    struct pt_editor_document_replacement *c=active;
    if(!c)return 1;
    if(c->self!=c||c->fault)return 0;
    if(p==&c->candidate->project)return 0;
    if(p!=&c->document->project)return 1;
    return retirement_frame(c);
}
int pt_document_typed_replacement_output_disjoint(const void *p,size_t bytes)
{
    struct pt_editor_document_replacement *c=active;unsigned i;
    if(!c)return span(p,bytes);
    if(c->self!=c||c->fault||!apart(p,bytes,c,sizeof(*c))||
       !apart(p,bytes,c->candidate,sizeof(*c->candidate)))return 0;
    /* Only the exact captured old header is a synchronous release INPUT.
     * Header subspans/overlapping outputs cannot consume this exception. */
    if(!apart(p,bytes,c->document,sizeof(*c->document)))
        return retirement_frame(c)&&p==c->document&&bytes==sizeof(*c->document);
    /* During the one checked old release, these are inputs, not new output
     * allocations. Core release reentry and factory callback admission refuse
     * before any unrelated operation can consume this synchronous allowance. */
    if(retirement_frame(c))return 1;
    for(i=0;i<14;++i)if(!apart(p,bytes,c->protected_storage[i],c->protected_bytes[i]))return 0;
    return 1;
}
