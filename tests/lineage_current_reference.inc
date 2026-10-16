/* Frozen b1e929ce lineage predicate closure; six function-name bindings only. */
static struct pt_editor_mixed_causal_rolling_state *pt_lc_ref_rolling_joint_scope(
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
static struct pt_editor_mixed_causal_rolling_state *pt_lc_ref_rolling_scope(struct pt_editor_mixed_causal_prepare *s)
{
 struct pt_editor_mixed_causal_rolling_state *r;unsigned i,covered=0,input=0,wrapper=0;
 if(!s->rolling_owner||s->stop_owner||!s->lineage_owner||s->lineage_owner!=(void *)s||
    s->guard_count>PT_EDITOR_MIXED_READERS_GUARDS)return NULL;
 if(s->source_mode==3)return pt_lc_ref_rolling_joint_scope(s);
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
static int pt_lc_ref_rolling_current(struct pt_editor_mixed_causal_prepare *s)
{
 struct pt_editor_mixed_causal_rolling_state *r=pt_lc_ref_rolling_scope(s);
 if(!r||!zero(&r->inputs->original.lineage,sizeof(r->inputs->original.lineage))||
    memcmp(&r->inputs->rolling,&r->saved.rolling,sizeof(r->saved.rolling)))return 0;
 return s->source_mode&&!s->source_activated?1:!memcmp(r->inputs,&r->saved,sizeof(r->saved));
}
static struct pt_editor_mixed_causal_lineage_source_prepare *pt_lc_ref_lineage_source_scope(
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
    if(!pt_lc_ref_rolling_scope(s))return NULL;
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
static struct pt_editor_mixed_causal_lineage_prepare *pt_lc_ref_lineage_scope(struct pt_editor_mixed_causal_prepare *s)
{
 unsigned i,control=0,input=0;struct pt_editor_mixed_causal_lineage_prepare *w;
 if(s->source_mode){struct pt_editor_mixed_causal_lineage_source_prepare *joint=pt_lc_ref_lineage_source_scope(s);
    return joint&&s->source_activated?&joint->original:NULL;}
 if(!s->lineage_owner||s->lineage_owner!=(void *)s||s->stop_owner||
    s->guard_count>PT_EDITOR_MIXED_READERS_GUARDS)return NULL;
 for(i=0;i<s->guard_count;++i){
    if(inside(s->guards[i],s->lineage_owner,sizeof(*w)))control=1;
    if(inside(s->guards[i],s->inputs,sizeof(struct pt_editor_mixed_causal_lineage_inputs)))input=1;
 }
 if(!control||!input)return NULL;
 w=s->lineage_owner;
 return w->self==w&&w->inputs==(void *)s->inputs?w:NULL;
}
static int pt_lc_ref_lineage_current(struct pt_editor_mixed_causal_prepare *s)
{
 struct pt_editor_mixed_causal_lineage_prepare *w=pt_lc_ref_lineage_scope(s);
 return w&&w->stage<=(s->rolling_owner?4U:3U)&&w->constructing<=(s->rolling_owner?4U:3U)&&
    (!s->rolling_owner||pt_lc_ref_rolling_current(s))&&w->root_detached<=1&&w->root_closed<=1&&
    !memcmp(w->inputs,&w->saved,sizeof(w->saved))&&
    !memcmp(&w->saved.original,&s->saved,sizeof(s->saved));
}
