/* Private shared fixture hooks; include AFTER the genuine34 fixture types.
 * Platform leaves are defined before any allocation macro. HOST labels memory
 * classes as models; Exec checks actual memory. No source/queue/owner action is
 * performed here beyond the original fixture's typed Chip callback binding.
 * Static tracking is caller-owned, excluded across this complete single entry,
 * never copied/reset, and survives each trial's genuine final source closure.
 */
#ifndef PT_NATIVE_MIXED_CAUSAL_RAM_HOOKS_INTERNAL_H
#define PT_NATIVE_MIXED_CAUSAL_RAM_HOOKS_INTERNAL_H
#define NCH_FAST 1U
#define NCH_CHIP 2U
#define NCH_CASES 34U
union nch_alignment {long double real;void *pointer;uint64_t integer;};
struct nch_fast_record {
    struct nch_fast_record *next;
    size_t bytes,total;
    union nch_alignment alignment;
};
struct nch_chip_record {
    struct nch_chip_record *next;
    struct cp_memory *context;
    void *data;size_t bytes;
};
static struct nch_fast_record *nch_fast_records;
static struct nch_chip_record *nch_chip_records;
static struct cp_memory *nch_chip_context;
static struct cp_trial *nch_chip_owner;
static size_t nch_fast_bytes,nch_chip_bytes;
static unsigned nch_fast_calls,nch_fast_releases,nch_fast_live;
static unsigned nch_fast_checks,nch_fast_release_checks;
static unsigned nch_chip_calls,nch_chip_releases,nch_chip_live;
static unsigned nch_chip_checks,nch_chip_release_checks,nch_chip_bindings;
static unsigned nch_case_started,nch_case_ended,nch_case_active,nch_case_chip_base;
/* Source-derived selective allocation census of the unchanged34 entry:
 * mixed successes12, all-card2, arm6, expected2, timing3, source4, packet2,
 * empty1, init1, authority1. Each used mixed trial has two actual Paula caches;
 * the second published packet is a HIT and must not allocate another copy.
 */
static const unsigned nch_case_chip_calls[NCH_CASES]={
    2,2,2,2,2,2,2,2,2,2,2,2,0,0,
    2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,2,2,0,0,2
};
static void *nch_allocate(size_t bytes)
{
    struct nch_fast_record *record;void *p;size_t total;uintptr_t address;
    if(!bytes || bytes>SIZE_MAX-sizeof(*record))return NULL;
    total=sizeof(*record)+bytes;
    assert(nch_fast_calls<UINT_MAX && nch_fast_live<UINT_MAX &&
        nch_fast_checks<UINT_MAX && bytes<=SIZE_MAX-nch_fast_bytes);
    record=nch_platform_allocate(total,NCH_FAST);
    assert(record && nch_platform_kind(record,total,NCH_FAST));
    p=record+1;address=(uintptr_t)p;
    assert(address && bytes-1U<=UINTPTR_MAX-address &&
        address%offsetof(struct cr_carrier_alignment,value)==0);
    assert(nch_platform_kind(p,bytes,NCH_FAST));
    record->bytes=bytes;record->total=total;record->next=nch_fast_records;
    nch_fast_records=record;++nch_fast_calls;++nch_fast_live;
    ++nch_fast_checks;nch_fast_bytes+=bytes;return p;
}
static void *nch_callocate(size_t count,size_t bytes)
{
    void *p;size_t total;
    if(!count || !bytes || count>SIZE_MAX/bytes)return NULL;
    total=count*bytes;p=nch_allocate(total);if(p)memset(p,0,total);return p;
}
static void nch_release(void *p)
{
    struct nch_fast_record **link=&nch_fast_records,*record;size_t total,bytes;
    if(!p)return;
    while(*link && (void *)(*link+1)!=p)link=&(*link)->next;
    assert(*link && nch_fast_live && nch_fast_releases<UINT_MAX &&
        nch_fast_release_checks<UINT_MAX);record=*link;
    bytes=record->bytes;total=record->total;
    assert(bytes && bytes<=nch_fast_bytes && total==sizeof(*record)+bytes &&
        nch_platform_kind(record,total,NCH_FAST) && nch_platform_kind(p,bytes,NCH_FAST));
    *link=record->next;--nch_fast_live;nch_fast_bytes-=bytes;
    ++nch_fast_releases;++nch_fast_release_checks;
    nch_platform_release(record,total,NCH_FAST);
}
static void nch_chip_identity(struct cp_memory *memory)
{
    assert(nch_case_active && memory && memory==nch_chip_context &&
        memory->owner==nch_chip_owner && cr_current &&
        nch_chip_owner==&cr_current->trial && memory==&cr_current->trial.chip);
}
static void *nch_chip_allocate(void *context,size_t bytes)
{
    struct cp_memory *memory=context;struct nch_chip_record *record;
    void *p;unsigned index;uintptr_t address;
    nch_chip_identity(memory);
    /* Preserve the genuine diagnostic callback outcomes, not an always-success
     * shortcut. Additional Chip fail/alias/reentry modes are not exercised by
     * the34 entry and gain no new qualification from this wrapper.
     */
    if(memory->hook){memory->hook=0;cp_reenter(memory->owner);}
    assert(memory->calls<UINT_MAX);++memory->calls;
    if(memory->fail==memory->calls)return NULL;
    if(memory->alias)return memory->alias;
    if(!bytes || bytes>UINT32_MAX || bytes>nch_platform_chip_available())return NULL;
    for(index=0;index<80 && memory->live[index].p;++index){}
    assert(index<80 && nch_chip_calls<UINT_MAX && nch_chip_live<UINT_MAX &&
        nch_chip_checks<UINT_MAX && bytes<=SIZE_MAX-nch_chip_bytes);
    record=nch_allocate(sizeof(*record));assert(record);
    p=nch_platform_allocate(bytes,NCH_CHIP);
    if(!p){nch_release(record);return NULL;}
    address=(uintptr_t)p;
    assert(address && bytes-1U<=UINTPTR_MAX-address && !(address&1U) &&
        nch_platform_kind(p,bytes,NCH_CHIP));
    memory->live[index]=(struct cp_ledger){p,bytes};
    record->context=memory;record->data=p;record->bytes=bytes;
    record->next=nch_chip_records;nch_chip_records=record;
    ++nch_chip_calls;++nch_chip_live;++nch_chip_checks;nch_chip_bytes+=bytes;return p;
}
static void nch_chip_release(void *context,void *p,size_t bytes)
{
    struct cp_memory *memory=context;struct nch_chip_record **link=&nch_chip_records,*record;
    unsigned index;
    nch_chip_identity(memory);
    for(index=0;index<PT_EDITOR_MIXED_READERS_CHIP;++index)
        assert(memory->owner->control.chip[index].data!=p);
    if(memory->hook){memory->hook=0;cp_reenter(memory->owner);}
    if(p==memory->alias){assert(memory->alias_releases<UINT_MAX);++memory->alias_releases;return;}
    assert(p && bytes && memory->releases<UINT_MAX && nch_chip_live &&
        nch_chip_releases<UINT_MAX && nch_chip_release_checks<UINT_MAX && bytes<=nch_chip_bytes);
    for(index=0;index<80 && memory->live[index].p!=p;++index){}
    assert(index<80 && memory->live[index].n==bytes);
    while(*link && (*link)->data!=p)link=&(*link)->next;
    assert(*link);record=*link;
    assert(record->context==memory && record->bytes==bytes &&
        nch_platform_kind(p,bytes,NCH_CHIP));
    memory->live[index]=(struct cp_ledger){NULL,0};++memory->releases;
    *link=record->next;--nch_chip_live;nch_chip_bytes-=bytes;
    ++nch_chip_releases;++nch_chip_release_checks;
    nch_platform_release(p,bytes,NCH_CHIP);nch_release(record);
}
static void nch_configure_chip(struct cp_trial *f)
{
    struct pt_editor_mixed_causal_prepare_inputs *input;
    assert(nch_case_active && f && cr_current && f==&cr_current->trial &&
        !nch_chip_context && !nch_chip_owner && !nch_chip_records && !nch_chip_live && !nch_chip_bytes);
    input=&f->input;
    assert(input->contexts.data==f && input->contexts.bytes==sizeof(*cr_current) &&
        input->chip_context==&f->chip && input->chip_allocate==cp_chip_new &&
        input->chip_release==cp_chip_free && input->chip_budget==4096 &&
        f->chip.owner==f && !f->chip.calls && !f->chip.releases && !cp_live(&f->chip));
    assert(cr_zero(&f->control,sizeof(f->control)) && !f->binding->preparation_context &&
        !f->ordinary.calls && !cp_live(&f->ordinary) && !f->card->writes &&
        cr_current->port.initialized && !cr_current->port.bound &&
        !cr_current->state.arms && !cr_current->state.clocks &&
        input->causal.port.context==&cr_current->port);
    assert(nch_chip_bindings<UINT_MAX);nch_chip_context=&f->chip;nch_chip_owner=f;
    input->chip_allocate=nch_chip_allocate;input->chip_release=nch_chip_release;
    ++nch_chip_bindings;
}
static void nch_case_begin(unsigned number,unsigned bits,unsigned cache,unsigned little,unsigned residency)
{
    assert(!nch_case_active && number==nch_case_started+1U && number<=NCH_CASES &&
        (bits==8 || bits==16 || bits==24) && (cache==8 || cache==16) && little<=1 &&
        (residency==1 || residency==64));
    assert(!nch_fast_records && !nch_fast_live && !nch_fast_bytes &&
        !nch_chip_records && !nch_chip_live && !nch_chip_bytes &&
        !nch_chip_context && !nch_chip_owner && nch_platform_empty());
    nch_case_active=number;++nch_case_started;nch_case_chip_base=nch_chip_calls;
    printf(NCH_TRACE_PREFIX " CASE BEGIN: case=%u\n",number);assert(!fflush(stdout));
}
static void nch_case_end(unsigned number)
{
    /* cp_drop actually released the complete carrier before this callback.
     * Stale pointer identities are cleared without reading their former bytes.
     */
    assert(number==nch_case_active && number==nch_case_ended+1U && number<=NCH_CASES &&
        !cr_current && nch_chip_context && nch_chip_owner &&
        nch_chip_bindings==number && nch_chip_calls-nch_case_chip_base==nch_case_chip_calls[number-1U]);
    assert(!nch_fast_records && !nch_fast_live && !nch_fast_bytes &&
        !nch_chip_records && !nch_chip_live && !nch_chip_bytes && nch_platform_empty());
    nch_chip_context=NULL;nch_chip_owner=NULL;
    printf(NCH_TRACE_PREFIX " CASE END: case=%u\n",number);assert(!fflush(stdout));
    ++nch_case_ended;nch_case_active=0;
}
static void nch_final(void)
{
    assert(nch_case_started==NCH_CASES && nch_case_ended==NCH_CASES && !nch_case_active &&
        nch_chip_bindings==NCH_CASES && nch_chip_calls==52U &&
        !nch_chip_context && !nch_chip_owner && !nch_fast_records && !nch_chip_records &&
        !nch_fast_live && !nch_fast_bytes && !nch_chip_live && !nch_chip_bytes &&
        nch_fast_calls && nch_fast_releases==nch_fast_calls && nch_fast_checks==nch_fast_calls &&
        nch_fast_release_checks==nch_fast_calls && nch_chip_releases==nch_chip_calls &&
        nch_chip_checks==nch_chip_calls && nch_chip_release_checks==nch_chip_calls && nch_platform_empty());
}
#endif
