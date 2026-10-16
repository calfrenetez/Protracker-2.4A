/* Frozen original numeric-helper source; only the six bound function names change. */
static struct pt_editor_mixed_causal_rolling_state *pt_desc_ref_rolling_joint_scope(
 struct pt_editor_mixed_causal_prepare *s)
{
 const struct pt_editor_mixed_causal_rolling_source_inputs *v=(void *)s->source_inputs;
 const struct pt_editor_mixed_causal_rolling_inputs *in=(void *)s->source_preparation;
 struct pt_editor_mixed_causal_rolling_state *r;unsigned i,control=0,input=0,preparation=0;
 for(i=0;i<s->guard_count&&(!control||!input||!preparation);++i){
    if(!control&&inside(s->guards[i],s,sizeof(struct pt_editor_mixed_causal_rolling_source_prepare)))control=1;
    if(!input&&inside(s->guards[i],v,sizeof(*v)))input=1;
    if(!preparation&&inside(s->guards[i],in,sizeof(*in)))preparation=1;
 }
 if(!control||!input||!preparation)return NULL;
 r=&((struct pt_editor_mixed_causal_rolling_source_prepare *)(void *)s)->rolling;
 if(s->rolling_owner!=r)return NULL;
 if(r->self!=r||r->preparation!=(void *)s||!r->inputs||r->inputs!=in||
    r->joint!=1||r->wrapper.data!=(void *)s||
    r->wrapper.bytes!=sizeof(struct pt_editor_mixed_causal_rolling_source_prepare))return NULL;
 if(v->preparation!=in||v->original.preparation!=&in->original)return NULL;
 return r;
}
static struct pt_editor_mixed_causal_rolling_state *pt_desc_ref_rolling_scope(struct pt_editor_mixed_causal_prepare *s)
{
 struct pt_editor_mixed_causal_rolling_state *r;unsigned i,covered=0,input=0,wrapper=0;
 if(!s->rolling_owner||s->stop_owner||!s->lineage_owner||s->lineage_owner!=(void *)s||
    s->guard_count>PT_EDITOR_MIXED_READERS_GUARDS)return NULL;
 if(s->source_mode==3)return pt_desc_ref_rolling_joint_scope(s);
 /* Coverage is existential: pure numeric probes cannot revoke a found role.
  * Complete extents, constructor backlinks and lifetime checks remain below. */
 for(i=0;i<s->guard_count&&!covered;++i)if(inside(s->guards[i],s->rolling_owner,sizeof(*r)))covered=1;
 if(!covered)return NULL;
 r=s->rolling_owner;
 if(r->self!=r||r->preparation!=(void *)s||!r->inputs||r->joint>1||r->wrapper.data!=(void *)s||
    r->wrapper.bytes!=(r->joint?sizeof(struct pt_editor_mixed_causal_rolling_source_prepare):
       sizeof(struct pt_editor_mixed_causal_rolling_prepare))||
    (void *)r!=(r->joint?(void *)&((struct pt_editor_mixed_causal_rolling_source_prepare *)(void *)s)->rolling:
       (void *)&((struct pt_editor_mixed_causal_rolling_prepare *)(void *)s)->rolling)||
    (r->joint?(s->source_mode!=3):(s->source_mode!=0)))return NULL;
 for(i=0;i<s->guard_count&&(!input||!wrapper);++i){
    if(!input&&inside(s->guards[i],r->inputs,sizeof(*r->inputs)))input=1;
    if(!wrapper&&inside(s->guards[i],r->wrapper.data,r->wrapper.bytes))wrapper=1;
 }
 if(!input||!wrapper||&r->inputs->original!=(void *)(s->source_mode?s->source_preparation:s->inputs))return NULL;
 if(r->joint){const struct pt_editor_mixed_causal_rolling_source_inputs *v=(void *)s->source_inputs;
    covered=0;for(i=0;i<s->guard_count&&!covered;++i)if(inside(s->guards[i],v,sizeof(*v)))covered=1;
    if(!covered||v->preparation!=r->inputs||v->original.preparation!=&r->inputs->original)return NULL;
 }
 return r;
}
static int pt_desc_ref_rolling_current(struct pt_editor_mixed_causal_prepare *s)
{
 struct pt_editor_mixed_causal_rolling_state *r=pt_desc_ref_rolling_scope(s);
 if(!r||!zero(&r->inputs->original.lineage,sizeof(r->inputs->original.lineage))||
    memcmp(&r->inputs->rolling,&r->saved.rolling,sizeof(r->saved.rolling)))return 0;
 return s->source_mode&&!s->source_activated?1:!memcmp(r->inputs,&r->saved,sizeof(r->saved));
}
static struct pt_editor_mixed_causal_lineage_source_prepare *pt_desc_ref_lineage_source_scope(
 struct pt_editor_mixed_causal_prepare *s)
{
 struct pt_editor_mixed_causal_lineage_source_prepare *w;unsigned i,control=0,input=0,preparation=0;
 if((s->source_mode!=2&&s->source_mode!=3)||!s->lineage_owner||s->lineage_owner!=(void *)s||s->stop_owner||
    !s->source_inputs||!s->source_preparation||s->guard_count>PT_EDITOR_MIXED_READERS_GUARDS)return NULL;
 if(s->source_mode==3){
    /* rolling_scope proves the full joint wrapper, extended source descriptor,
     * and rolling preparation plus exact embedded lineage address. Those same
     * valid complete parents also cover the three original child roles. This
     * repeats the actual proof each call; no cached flag authorizes a read. */
    if(!pt_desc_ref_rolling_scope(s))return NULL;
 }else{
    for(i=0;i<s->guard_count&&(!control||!input||!preparation);++i){
       if(!control&&inside(s->guards[i],s,sizeof(*w)))control=1;
       if(!input&&inside(s->guards[i],s->source_inputs,sizeof(struct pt_editor_mixed_causal_lineage_source_inputs)))input=1;
       if(!preparation&&inside(s->guards[i],s->source_preparation,sizeof(struct pt_editor_mixed_causal_lineage_inputs)))preparation=1;
    }
    if(!control||!input||!preparation)return NULL;
 }
 w=(void *)s;
 if(w->self!=w||w->inputs!=(void *)s->source_inputs||
    w->saved.preparation!=(void *)s->source_preparation||
    w->original.self!=&w->original||w->original.inputs!=w->saved.preparation||
    (s->source_activated?s->inputs!=(void *)w->saved.preparation:s->inputs!=NULL))return NULL;
 return w;
}
static int pt_desc_ref_source_identity(struct pt_editor_mixed_causal_prepare *s)
{
 const struct pt_editor_mixed_causal_source_borrow *b=s->source_publisher;
 if((s->source_mode!=1&&(s->source_mode!=2&&s->source_mode!=3))||!s->source_serial||!b||s->saved.binding!=s->source_binding||
    (s->source_mode==1&&(s->lineage_owner||s->stop_owner))||
    ((s->source_mode==2||s->source_mode==3)&&!pt_desc_ref_lineage_source_scope(s)))return 0;
 if(s->source_held)return !s->source_released&&b->address==s&&b->serial==s->source_serial;
 return s->source_released&&!b->address&&!b->serial;
}
static int pt_desc_ref_source_descriptor(struct pt_editor_mixed_causal_prepare *s)
{
 const struct pt_editor_mixed_causal_source_inputs *v=s->source_inputs;
 if(s->rolling_owner&&!pt_desc_ref_rolling_current(s))return 0;
 if((s->source_mode==2||s->source_mode==3)){struct pt_editor_mixed_causal_lineage_source_prepare *joint=pt_desc_ref_lineage_source_scope(s);
    if(!joint||memcmp(joint->inputs,&joint->saved,sizeof(joint->saved)))return 0;}
 if(!pt_desc_ref_source_identity(s)||!v||v->binding!=s->source_binding||
    v->preparation!=s->source_preparation||v->contexts.data!=s->source_contexts.data||
    v->contexts.bytes!=s->source_contexts.bytes||
    v->causal_workspace.data!=s->source_causal_workspace.data||
    v->causal_workspace.bytes!=s->source_causal_workspace.bytes||
    v->factory_workspace.data!=s->source_factory_workspace.data||
    v->factory_workspace.bytes!=s->source_factory_workspace.bytes||
    v->backend_parent.data!=s->source_backend_parent.data||
    v->backend_parent.bytes!=s->source_backend_parent.bytes||
    v->immutable_count!=s->source_immutable_count||
    v->mutable_count!=s->source_mutable_count||
    memcmp(v->immutable,s->source_immutable,sizeof(s->source_immutable))||
    memcmp(v->mutable,s->source_mutable,sizeof(s->source_mutable)))return 0;
 return 1;
}
