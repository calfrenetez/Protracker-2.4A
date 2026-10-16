/* Frozen637c1897 rolling and lineage predicates; symbol renames only. */
static struct pt_editor_mixed_causal_rolling_state *pt_rolling_scope_reference(struct pt_editor_mixed_causal_prepare *s)
{
 struct pt_editor_mixed_causal_rolling_state *r;unsigned i,covered=0,input=0,wrapper=0;
 if(!s->rolling_owner||s->stop_owner||!s->lineage_owner||s->lineage_owner!=(void *)s||
    s->guard_count>PT_EDITOR_MIXED_READERS_GUARDS)return NULL;
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
static struct pt_editor_mixed_causal_lineage_source_prepare *pt_scope_reference(
 struct pt_editor_mixed_causal_prepare *s)
{
 struct pt_editor_mixed_causal_lineage_source_prepare *w;unsigned i,control=0,input=0,preparation=0;
 if((s->source_mode!=2&&s->source_mode!=3)||!s->lineage_owner||s->lineage_owner!=(void *)s||s->stop_owner||
    !s->source_inputs||!s->source_preparation||s->guard_count>PT_EDITOR_MIXED_READERS_GUARDS)return NULL;
 for(i=0;i<s->guard_count&&(!control||!input||!preparation);++i){
    if(!control&&inside(s->guards[i],s,s->source_mode==3?sizeof(struct pt_editor_mixed_causal_rolling_source_prepare):sizeof(*w)))control=1;
    if(!input&&inside(s->guards[i],s->source_inputs,sizeof(struct pt_editor_mixed_causal_lineage_source_inputs)))input=1;
    if(!preparation&&inside(s->guards[i],s->source_preparation,sizeof(struct pt_editor_mixed_causal_lineage_inputs)))preparation=1;
 }
 if(!control||!input||!preparation||(s->source_mode==3&&!pt_rolling_scope_reference(s)))return NULL;
 w=(void *)s;
 if(w->self!=w||w->inputs!=(void *)s->source_inputs||
    w->saved.preparation!=(void *)s->source_preparation||
    w->original.self!=&w->original||w->original.inputs!=w->saved.preparation||
    (s->source_activated?s->inputs!=(void *)w->saved.preparation:s->inputs!=NULL))return NULL;
 return w;
}
