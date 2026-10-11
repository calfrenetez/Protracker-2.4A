/* Genuine original SOURCE issuance observations, never a READY/key maker. */
#include "command_registration_joint_fixture_support.inc"
static unsigned cr_cases;
static struct js_trial *cr_make(unsigned bits)
{struct js_trial *e=js_make(bits,16);++cr_cases;return e;}
static void cr_observe(struct js_trial *e,struct pt_editor_mixed_command_ref ref,
 const struct pt_editor_mixed_causal_source_borrow *borrow,
 enum pt_editor_mixed_source_registration_observation expected)
{
 struct cl_trial *c=&e->old.old;struct pt_editor_mixed_causal_prepare *s=cl_s(c);
 unsigned char *before=malloc(sizeof(*s));struct c16_counts counts;assert(before);
 memcpy(before,s,sizeof(*s));counts=c16_counts(c);
 assert(pt_editor_mixed_causal_source_command_registration(s,borrow,ref)==expected&&
    !memcmp(before,s,sizeof(*s)));c16_counts_same(c,&counts);free(before);
}
static void cr_empty(struct pt_editor_mixed_causal_prepare *s,unsigned slot)
{
 unsigned i;const struct pt_editor_mixed_command_record *r=s->command+slot;
 assert(!r->handle.address&&!r->handle.token&&!r->serial&&!r->ticket&&!r->count&&!r->transferred&&
    !s->command_registration[slot].serial&&s->command_registration[slot].kind==PT_EDITOR_MIXED_CAUSAL_COMMAND_NONE);
 for(i=0;i<PT_SAMPLER_MIXED_ACTIONS;++i)assert(!r->reader[i]);
}
static void cr_success(unsigned narrow,unsigned order)
{
 struct js_trial *e=cr_make(narrow?16:24);struct c16_trial *w=&e->old;struct cl_trial *c=&w->old;
 struct pt_editor_mixed_causal_prepare *s=cl_s(c);uint64_t root,control,third;unsigned i;
 root=js_first(e);assert(s->command_registration[c->command[0].slot].kind==PT_EDITOR_MIXED_CAUSAL_COMMAND_TRIGGER&&
    s->command_registration[c->command[0].slot].serial==c->command[0].serial&&s->command[c->command[0].slot].count==16);
 cr_observe(e,c->command[0],e->borrow,PT_EDITOR_MIXED_SOURCE_REGISTRATION_PRESENT);
 c16_input(w,narrow);c16_prepare(w,narrow);
 assert(s->command_registration[c->command[1].slot].kind==PT_EDITOR_MIXED_CAUSAL_COMMAND_CONTROL&&
    s->command_registration[c->command[1].slot].serial==c->command[1].serial&&!s->command[c->command[1].slot].count);
 cr_observe(e,c->command[1],e->borrow,PT_EDITOR_MIXED_SOURCE_REGISTRATION_PRESENT);
 control=cl_enqueue(c,1);assert(pt_editor_mixed_causal_prepare_publish(s,c->command[1])==PT_MIXED_READERS_OK);
 cr_observe(e,c->command[1],e->borrow,PT_EDITOR_MIXED_SOURCE_REGISTRATION_PRESENT);
 c->port.model.base.ticks=oracle(1920);assert(pt_mixed_causal_fire(s->causal,control)==PT_MIXED_CAUSAL_COMMITTED);
 cl_detach_root(c);cr_empty(s,c->command[0].slot);
 cr_observe(e,c->command[0],e->borrow,PT_EDITOR_MIXED_SOURCE_REGISTRATION_ABSENT);
 cl_stop_input(c,16);cl_later_prepare(c,1);
 assert(c->command[2].slot==c->command[0].slot&&c->command[2].serial!=c->command[0].serial&&
    s->command_registration[c->command[2].slot].serial==c->command[2].serial&&
    s->command_registration[c->command[2].slot].kind==PT_EDITOR_MIXED_CAUSAL_COMMAND_STOP);
 cr_observe(e,c->command[0],e->borrow,PT_EDITOR_MIXED_SOURCE_REGISTRATION_ABSENT);
 cr_observe(e,c->command[2],e->borrow,PT_EDITOR_MIXED_SOURCE_REGISTRATION_PRESENT);
 third=cl_enqueue(c,2);assert(root&&control!=root&&third!=control&&third!=root&&
    pt_editor_mixed_causal_prepare_publish(s,c->command[2])==PT_MIXED_READERS_OK);
 c->port.model.base.ticks=oracle(2880);assert(pt_mixed_causal_fire(s->causal,third)==PT_MIXED_CAUSAL_COMMITTED);
 if(!order){assert(pt_editor_mixed_causal_prepare_service_command(s,c->command[1],0,NULL)==PT_MIXED_READERS_OK);
    assert(pt_editor_mixed_causal_prepare_service_command(s,c->command[2],0,NULL)==PT_MIXED_READERS_OK);}
 c->port.model.quiet_mask=~0U;
 for(i=0;i<16;++i)assert(pt_editor_mixed_causal_prepare_service_reader(s,c->old.reader[i],0,NULL)==PT_MIXED_READERS_OK);
 if(order){assert(pt_editor_mixed_causal_prepare_service_command(s,c->command[1],0,NULL)==PT_MIXED_READERS_OK);
    assert(pt_editor_mixed_causal_prepare_service_command(s,c->command[2],0,NULL)==PT_MIXED_READERS_OK);}
 cr_empty(s,c->command[1].slot);cr_empty(s,c->command[2].slot);
 cr_observe(e,c->command[1],e->borrow,PT_EDITOR_MIXED_SOURCE_REGISTRATION_ABSENT);
 cr_observe(e,c->command[2],e->borrow,PT_EDITOR_MIXED_SOURCE_REGISTRATION_ABSENT);
 js_same(e);js_drop(e);
}
static void cr_refusal(unsigned mode)
{
 struct js_trial *e=cr_make(24);struct c16_trial *w=&e->old;struct cl_trial *c=&w->old;
 struct pt_editor_mixed_causal_prepare *s=cl_s(c);struct pt_editor_mixed_command_record saved;
 struct pt_editor_mixed_causal_command_registration issued;
 struct pt_editor_mixed_causal_source_borrow copy;
 struct pt_editor_mixed_command_ref ref;const struct pt_editor_mixed_causal_source_borrow *borrow=e->borrow;
 unsigned slot;assert(mode<16);(void)js_first(e);c16_input(w,0);c16_prepare(w,0);
 assert(cl_enqueue(c,1)&&pt_editor_mixed_causal_prepare_publish(s,c->command[1])==PT_MIXED_READERS_OK);
 ref=c->command[mode==15?0:1];slot=ref.slot;saved=s->command[slot];issued=s->command_registration[slot];
 cr_observe(e,ref,borrow,PT_EDITOR_MIXED_SOURCE_REGISTRATION_PRESENT);
 if(mode==0)s->command_registration[slot].kind=PT_EDITOR_MIXED_CAUSAL_COMMAND_NONE;
 if(mode==1)s->command_registration[slot].kind=(enum pt_editor_mixed_causal_command_kind)99;
 if(mode==2)s->command_registration[slot].kind=PT_EDITOR_MIXED_CAUSAL_COMMAND_TRIGGER;
 if(mode==3)s->command[slot].count=1;
 if(mode==4)s->command[slot].reader[PT_SAMPLER_MIXED_ACTIONS-1]=1;
 if(mode==5)s->command[slot].handle.address=NULL;
 if(mode==6)s->command[slot].handle.token=0;
 if(mode==7)s->command[slot].serial=0;
 if(mode==8)s->command[slot].transferred=2;
 if(mode==9)s->command[slot].ticket=0;
 if(mode==10){s->command_registration[slot].serial^=1;assert(s->command_registration[slot].serial!=saved.serial);}
 if(mode==11){memcpy(&copy,e->borrow,sizeof(copy));borrow=&copy;}
 if(mode==12)ref.serial=s->serial+1;
 if(mode==13)ref.slot=PT_SAMPLER_MIXED_COMMANDS;
 if(mode==14)memset(s->command+slot,0,sizeof(s->command[slot]));
 if(mode==15)s->command[slot].count=0;
 cr_observe(e,ref,borrow,PT_EDITOR_MIXED_SOURCE_REGISTRATION_INVALID);
 memcpy(s->command+slot,&saved,sizeof(saved));s->command_registration[slot]=issued;
 cr_observe(e,c->command[mode==15?0:1],e->borrow,PT_EDITOR_MIXED_SOURCE_REGISTRATION_PRESENT);
 assert(!s->first_error);js_same(e);js_drop(e);
}
static void cr_captured_only(void)
{
 struct js_trial *e=cr_make(24);struct c16_trial *w=&e->old;struct cl_trial *c=&w->old;
 struct pt_editor_mixed_causal_prepare *s=cl_s(c);
 struct pt_mixed_readers_output *queue;struct pt_sampler_mixed_pool *pool;struct pt_mixed_causal_owner *owner;
 unsigned char *before=malloc(sizeof(*s));struct c16_counts counts;assert(before);
 (void)js_first(e);c16_input(w,0);c16_prepare(w,0);queue=s->queue;pool=s->pool;owner=s->causal;counts=c16_counts(c);
 s->queue=(struct pt_mixed_readers_output *)(uintptr_t)1;s->pool=(struct pt_sampler_mixed_pool *)(uintptr_t)1;
 s->causal=(struct pt_mixed_causal_owner *)(uintptr_t)1;memcpy(before,s,sizeof(*s));
 /* No inherited count helper or lower pointer is read while poisoned. */
 assert(pt_editor_mixed_causal_source_command_registration(s,e->borrow,c->command[1])==PT_EDITOR_MIXED_SOURCE_REGISTRATION_PRESENT&&
    !memcmp(before,s,sizeof(*s)));
 s->queue=queue;s->pool=pool;s->causal=owner;c16_counts_same(c,&counts);free(before);
 assert(!s->first_error);js_same(e);js_drop(e);
}
int main(void)
{
 unsigned i;cr_success(0,0);cr_success(1,1);for(i=0;i<16;++i)cr_refusal(i);cr_captured_only();
 assert(cr_cases==19);
 puts("EDITOR MIXED CAUSAL COMMAND REGISTRATION PASS:19 genuine SOURCE instances;2 wide/old8 TRIGGER-CONTROL-STOP sequences with actual C1/C2/C3 issuance and NULL disposal/slot reuse;16 captured-metadata/identity refusals and1 captured-only nested-pointer exclusion;no callbacks/writes from registration observations;SOFTWARE_ONLY");
 return 0;
}
