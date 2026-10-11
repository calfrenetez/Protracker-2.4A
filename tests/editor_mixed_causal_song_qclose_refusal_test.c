/* Separate genuine Q_CLOSE negative fixture. The preserved own38 donor entry
 * is only a source dependency; it is never invoked by this main. No manufactured
 * ACTIVE/key/receipt and no copied passing count. Each case owns a real trial. */
#define PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_TEST_MAIN preserved_song_lineage38_main_not_run
#include "editor_mixed_causal_song_lineage_test.c"
#undef PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_TEST_MAIN

static unsigned sq_cases;
struct sq_before {
 struct pt_editor_mixed_causal_song_lineage_outputs genuine,altered;
 struct pt_mixed_plan_quantized_batch frozen[3];
 struct pt_editor_mixed_causal_song_lineage_history history[3];
 struct pt_editor_mixed_causal_song_lineage_command command[PT_SAMPLER_MIXED_COMMANDS];
 struct pt_editor_mixed_causal_song_lineage_reader reader[PT_SAMPLER_MIXED_READERS];
 struct pt_sample_version *current[PT_PROJECT_SAMPLES];
 struct sl_port port;
 struct fixture card;
 unsigned ordinary,chip,masters,revision,generation,pins;
};
/* The fixture changes only its genuine declared mutable U publisher, not an
 * arbitrary output address. Exact captured full capacity is still authoritative. */
static void sq_mutable_current(struct sl_trial *e)
{
 struct pt_editor_mixed_causal_prepare *s=sl_s(e);
 struct pt_editor_mixed_source_observation o=pt_editor_mixed_causal_source_observe(s,e->borrow);
 assert(o.scope==PT_EDITOR_MIXED_SOURCE_SCOPE_HELD&&!o.first_error&&!o.cancel_requested&&o.fixed_tags_current);
 assert(e->producer.outputs==e->outputs&&s->source_mutable_count==2&&
    s->source_mutable[1].data==e->outputs&&s->source_mutable[1].bytes==e->output_capacity&&
    e->source->original.mutable_count==2&&e->source->original.mutable[1].data==e->outputs&&
    e->source->original.mutable[1].bytes==e->output_capacity&&
    e->output_capacity==sizeof(*e->outputs)+CS_TAIL);
}
static void sq_to_control_close(struct sl_trial *e)
{
 unsigned n=0;sl_begin(e);
 while(e->producer.phase!=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_Q_CLOSE||e->producer.candidate_count!=1){
    assert(++n<100000&&sl_step(e,17)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PENDING);
 }
 assert(e->producer.activated&&e->producer.target==1920&&e->producer.frames==0&&e->outputs->interval.frames==960&&
    e->producer.frozen[0].normalized.frame==960&&e->producer.frozen[0].normalized.count==16&&
    e->outputs->normalized.normalized.frame==1920&&e->outputs->normalized.normalized.count==16&&
    e->outputs->normalizer&&!e->producer.candidate_pending&&!e->producer.attempted_mask&&
    !e->port.binds&&!e->port.model.base.publications&&!sl_s(e)->pool&&!sl_s(e)->queue&&!sl_s(e)->causal);
}
static void sq_refusal(unsigned bits,unsigned card,unsigned mode)
{
 struct sl_trial *e=sl_make(bits,16,0,card?CS_CARD16:CS_MIXED,0);
 struct sq_before *before=malloc(sizeof(*before));
 const struct pt_mixed_plan_record *first;
 struct pt_mixed_plan_record *record;
 unsigned i,slot=0;
 enum pt_editor_mixed_causal_song_lineage_result actual;
 assert(before&&mode<3);sq_to_control_close(e);
 /* Complete scratch address/capacity admission precedes its first write. */
 sl_local_numeric(e,(uintptr_t)before,sizeof(*before));sq_mutable_current(e);
 before->genuine=*e->outputs;before->altered=before->genuine;
 memcpy(before->frozen,e->producer.frozen,sizeof(before->frozen));
 memcpy(before->history,e->producer.history,sizeof(before->history));
 memcpy(before->command,e->producer.command,sizeof(before->command));
 memcpy(before->reader,e->producer.reader,sizeof(before->reader));
 memcpy(before->current,e->base.editor->sampler.current,sizeof(before->current));
 before->port=e->port;before->card=*e->base.card;
 before->ordinary=e->base.ordinary.calls;before->chip=e->base.chip.calls;before->masters=e->base.masters.calls;
 before->revision=e->base.editor->history.revision;before->generation=e->base.editor->sampler.generation;
 before->pins=e->producer.pin_count;
 first=before->frozen[0].normalized.record+slot;
 record=before->altered.normalized.normalized.record+slot;
 assert(first->kind==PT_MIXED_PLAN_TRIGGER&&record->kind==PT_MIXED_PLAN_CONTROL&&
    first->track==slot&&record->track==slot&&first->route==record->route&&
    first->sample==record->sample&&first->channel==record->channel&&first->slot==record->slot&&
    record->route==(card?PT_AMIGUS:PT_PAULA)&&record->first_action<before->genuine.plan.count&&
    before->genuine.plan.count<=PT_RENDER_ACTIONS&&
    before->genuine.plan.action[record->first_action].kind==PT_RENDER_CONTROL&&
    before->genuine.plan.action[record->first_action].channel==slot);
 if(mode==0){
    if(card){
       assert(before->frozen[0].levels[slot].mode==PT_MIXED_PLAN_LEVEL_QUANTIZED&&
          (record->geometry.amigus.rate!=first->image.rate||
           record->geometry.amigus.left!=before->frozen[0].levels[slot].left||
           record->geometry.amigus.right!=before->frozen[0].levels[slot].right));
       record->geometry.amigus.rate=first->image.rate;
       record->geometry.amigus.left=before->frozen[0].levels[slot].left;
       record->geometry.amigus.right=before->frozen[0].levels[slot].right;
    }else{
       assert(record->geometry.paula.period!=first->geometry.paula.period||
          record->geometry.paula.volume!=first->geometry.paula.volume);
       record->geometry.paula.period=first->geometry.paula.period;
       record->geometry.paula.volume=first->geometry.paula.volume;
    }
    assert(memcmp(&before->genuine.normalized,&before->altered.normalized,sizeof(before->genuine.normalized))&&
       !memcmp(&before->genuine.plan,&before->altered.plan,sizeof(before->genuine.plan)));
 }else if(mode==1){
    /* Remove an actual changed CONTROL, keeping complete origins/masks/frame
     * and raw plan exact. Tail compaction is well-formed but lacks coverage. */
    memmove(before->altered.normalized.normalized.record+slot,
       before->altered.normalized.normalized.record+slot+1,
       (15-slot)*sizeof(before->altered.normalized.normalized.record[0]));
    memmove(before->altered.normalized.levels+slot,before->altered.normalized.levels+slot+1,
       (15-slot)*sizeof(before->altered.normalized.levels[0]));
    before->altered.normalized.normalized.count=15;
    memset(before->altered.normalized.normalized.record+15,0,sizeof(before->altered.normalized.normalized.record[15]));
    memset(before->altered.normalized.levels+15,0,sizeof(before->altered.normalized.levels[15]));
    assert(!memcmp(&before->genuine.plan,&before->altered.plan,sizeof(before->genuine.plan))&&
       !memcmp(before->genuine.normalized.normalized.next,before->altered.normalized.normalized.next,
          sizeof(before->genuine.normalized.normalized.next))&&
       !memcmp(before->genuine.normalized.normalized.samples,before->altered.normalized.normalized.samples,
          sizeof(before->genuine.normalized.normalized.samples))&&
       before->genuine.normalized.normalized.frame==before->altered.normalized.normalized.frame);
 }else{
    /* A yielded post-GET count mutation must refuse before action indexing. */
    before->altered.plan.count=PT_RENDER_ACTIONS+1;
    assert(!memcmp(&before->genuine.normalized,&before->altered.normalized,sizeof(before->genuine.normalized)));
 }
 sq_mutable_current(e);*e->outputs=before->altered;
 assert(memcmp(&before->genuine,e->outputs,sizeof(before->genuine))&&
    !memcmp(&before->altered,e->outputs,sizeof(before->altered)));
 actual=sl_step(e,17); /* One real Q_CLOSE; no separate result-producing call. */
 assert(actual==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED&&
    e->producer.first_error==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED&&
    e->producer.phase==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_DRAIN&&
    !memcmp(&before->altered,e->outputs,sizeof(before->altered))&&
    e->outputs->normalizer==before->genuine.normalizer&&e->outputs->normalizer&&
    e->producer.candidate_count==1&&!e->producer.candidate_pending&&!e->producer.attempted_mask&&
    !memcmp(before->frozen,e->producer.frozen,sizeof(before->frozen))&&
    !memcmp(before->history,e->producer.history,sizeof(before->history))&&
    !memcmp(before->command,e->producer.command,sizeof(before->command))&&
    !memcmp(before->reader,e->producer.reader,sizeof(before->reader)));
 assert(before->ordinary==e->base.ordinary.calls&&before->chip==e->base.chip.calls&&
    before->masters==e->base.masters.calls&&!memcmp(&before->port,&e->port,sizeof(before->port))&&
    !memcmp(&before->card,e->base.card,sizeof(before->card))&&
    before->revision==e->base.editor->history.revision&&before->generation==e->base.editor->sampler.generation&&
    before->pins==e->producer.pin_count&&
    !memcmp(before->current,e->base.editor->sampler.current,sizeof(before->current))&&
    !sl_s(e)->pool&&!sl_s(e)->queue&&!sl_s(e)->causal&&!e->port.binds&&!e->port.model.base.publications);
 for(i=0;i<e->producer.pin_count;++i)assert(e->producer.pin[i]==before->current[i]&&before->current[i]);
 sl_same(e);sl_close(e);sl_same(e);
 assert(!memcmp(before->current,e->base.editor->sampler.current,sizeof(before->current))&&
    before->revision==e->base.editor->history.revision&&before->generation==e->base.editor->sampler.generation);
 free(before);sl_drop(e);++sq_cases;
}
#ifndef PT_EDITOR_MIXED_CAUSAL_SONG_QCLOSE_REFUSAL_TEST_MAIN
#define PT_EDITOR_MIXED_CAUSAL_SONG_QCLOSE_REFUSAL_TEST_MAIN main
#endif
int PT_EDITOR_MIXED_CAUSAL_SONG_QCLOSE_REFUSAL_TEST_MAIN(void)
{
 unsigned bits,card,mode;
 for(bits=16;bits<=24;bits+=8)for(card=0;card<2;++card)for(mode=0;mode<3;++mode)sq_refusal(bits,card,mode);
 assert(sq_cases==12&&sl_cases==12&&cs_cases==0&&cp_cases==0&&ct_cases==0);
 puts("EDITOR MIXED CAUSAL SONG Q_CLOSE REFUSAL PASS:12 genuine cases;geometry-to-prior,omitted CONTROL and post-GET raw count over64 acrossPaula/card and16/24-bit masters;actual Cxx32 renderer+normalizer,unchanged tampered whole output,zero owner/publication/cache writes,original master/save/full-capacity tails and genuine close; SOFTWARE_ONLY");
 return 0;
}
