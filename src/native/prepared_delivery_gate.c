#include "prepared_delivery_gate.h"
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#define ALIGNMENT(T) offsetof(struct {char byte;T value;},value)
#define ENTRY_TASK 1U
#define ENTRY_LOCAL 2U

struct gate_entry {
    struct pt_native_delivery_gate_token *token;
    uint64_t serial;
    int prior_nesting;
    unsigned acquiring,active,restoring,uncertain,slot;
};
struct gate_slot {
    uint64_t ticket;
    struct pt_native_delivery_gate_record record;
    unsigned staged,local_attempted,recorded,observed;
};
struct pt_native_delivery_gate {
    struct pt_native_delivery_gate *self;
    struct ExecBase *exec;
    struct Task *task;
    size_t capacity;
    uint64_t generation,serial;
    unsigned stopped,failed;
    struct gate_entry task_entry,local_entry;
    struct gate_slot slot[PT_NATIVE_DELIVERY_GATE_SLOTS];
};

static int object(const void *p,size_t n,size_t alignment)
{
    uintptr_t a=(uintptr_t)p;
    return p && n && alignment && a%alignment==0 && n<=UINTPTR_MAX-a;
}
static int apart(const void *a,size_t n,const void *b,size_t m)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return object(a,n,1) && object(b,m,1) && (x+n<=y || y+m<=x);
}
static int environment(struct ExecBase **exec,struct Task **task)
{
    struct ExecBase *base=SysBase;
    struct Task *current;
    if(!object(base,sizeof(*base),ALIGNMENT(struct ExecBase)))return 0;
    current=base->ThisTask;
    if(!object(current,sizeof(*current),ALIGNMENT(struct Task)))return 0;
    *exec=base;*task=current;return 1;
}
static int nesting(const struct pt_native_delivery_gate *g)
{return (int)(signed char)g->exec->IDNestCnt;}
static int legal_prior(int n)
{return n>=-1 && n<127;}
static int current_environment(const struct pt_native_delivery_gate *g)
{
    struct ExecBase *exec;struct Task *task;
    return environment(&exec,&task) && exec==g->exec && task==g->task;
}
static int valid(const struct pt_native_delivery_gate *g)
{
    return object(g,sizeof(*g),ALIGNMENT(struct pt_native_delivery_gate)) &&
        g->self==g && g->generation && g->capacity>=sizeof(*g) &&
        object(g,g->capacity,ALIGNMENT(struct pt_native_delivery_gate)) &&
        current_environment(g) &&
        apart(g,g->capacity,g->exec,sizeof(*g->exec)) &&
        apart(g,g->capacity,g->task,sizeof(*g->task));
}
/* Real Exec vectors have a memory clobber too. Keep publication ordering
 * explicit for the same source compiled with the separate fake Exec headers. */
static void publication_barrier(void)
{__asm__ volatile ("" ::: "memory");}
static enum pt_native_delivery_gate_result fault(
    struct pt_native_delivery_gate *g,struct gate_entry *entry)
{
    g->failed=1;
    if(entry)entry->uncertain=1;
    return PT_NATIVE_DELIVERY_GATE_FAULT;
}
static int token_span_clear(const struct pt_native_delivery_gate *g,
    const void *p,size_t n)
{
    return apart(p,n,g,g->capacity) && apart(p,n,g->exec,sizeof(*g->exec)) &&
        apart(p,n,g->task,sizeof(*g->task));
}
static int external_clear(const struct pt_native_delivery_gate *g,
    const void *p,size_t n,size_t alignment)
{
    const struct gate_entry *a=&g->task_entry,*b=&g->local_entry;
    return object(p,n,alignment) && token_span_clear(g,p,n) &&
        (!a->token || apart(p,n,a->token,sizeof(*a->token))) &&
        (!b->token || apart(p,n,b->token,sizeof(*b->token)));
}
static int token_matches(const struct pt_native_delivery_gate *g,
    const struct gate_entry *entry,
    const struct pt_native_delivery_gate_token *token,unsigned kind)
{
    return entry->active && !entry->acquiring && !entry->restoring &&
        token==entry->token &&
        object(token,sizeof(*token),ALIGNMENT(struct pt_native_delivery_gate_token)) &&
        token_span_clear(g,token,sizeof(*token)) &&
        token->gate==g && token->generation==g->generation &&
        token->serial==entry->serial && token->kind==kind;
}
static enum pt_native_delivery_gate_result active(
    struct pt_native_delivery_gate *g,
    const struct pt_native_delivery_gate_token *token,unsigned kind)
{
    struct gate_entry *entry;
    /* A caller arriving through a foreign Task/Exec is not permitted to mark
     * this owner's gate failed or restore its outstanding pair. */
    if(!valid(g))return PT_NATIVE_DELIVERY_GATE_REFUSED;
    entry=kind==ENTRY_TASK?&g->task_entry:&g->local_entry;
    if(!token_matches(g,entry,token,kind))return PT_NATIVE_DELIVERY_GATE_REFUSED;
    if(g->failed)return PT_NATIVE_DELIVERY_GATE_FAULT;
    if(!current_environment(g) || nesting(g)!=entry->prior_nesting+1)
        return fault(g,entry);
    return PT_NATIVE_DELIVERY_GATE_OK;
}

size_t pt_native_delivery_gate_size(void)
{return sizeof(struct pt_native_delivery_gate);}
size_t pt_native_delivery_gate_alignment(void)
{return ALIGNMENT(struct pt_native_delivery_gate);}

enum pt_native_delivery_gate_result pt_native_delivery_gate_init(
    struct pt_native_delivery_gate *g,size_t capacity,uint64_t generation)
{
    struct ExecBase *exec;struct Task *task;
    const unsigned char *bytes=(const unsigned char *)g;size_t i;
    if(!generation || capacity<sizeof(*g) ||
       !object(g,capacity,ALIGNMENT(struct pt_native_delivery_gate)) ||
       !environment(&exec,&task) ||
       !apart(g,capacity,exec,sizeof(*exec)) ||
       !apart(g,capacity,task,sizeof(*task)))return PT_NATIVE_DELIVERY_GATE_REFUSED;
    /* Bounded owned-prefix check only: the caller's protected capacity tail
     * can contain arbitrary bytes and is never read or written here. */
    for(i=0;i<sizeof(*g);++i)if(bytes[i])return PT_NATIVE_DELIVERY_GATE_REFUSED;
    g->self=g;g->exec=exec;g->task=task;g->capacity=capacity;
    g->generation=generation;return PT_NATIVE_DELIVERY_GATE_OK;
}

static enum pt_native_delivery_gate_result enter(
    struct pt_native_delivery_gate *g,unsigned kind,unsigned slot,
    struct pt_native_delivery_gate_token *token)
{
    struct gate_entry *entry=kind==ENTRY_TASK?&g->task_entry:&g->local_entry;
    int prior=nesting(g);
    if(!legal_prior(prior))return fault(g,NULL);
    /* Normal-task, no-callback admission marker. It prevents a recursively
     * entered fake/vector call from acquiring the same entry obligation. */
    entry->token=token;entry->serial=g->serial+1;
    entry->prior_nesting=prior;entry->slot=slot;entry->acquiring=1;
    publication_barrier();
    Disable();
    publication_barrier();
    /* A missing/changed nesting effect is uncertain, not permission to issue
     * an Enable on speculation. Keep the actual acquisition anchor alive. */
    if(!current_environment(g) || nesting(g)!=prior+1 ||
       entry->acquiring!=1 || entry->active || entry->restoring)
        return fault(g,entry);
    g->serial=entry->serial;entry->acquiring=0;entry->active=1;
    if(kind==ENTRY_LOCAL)g->slot[slot].local_attempted=1;
    token->gate=g;token->generation=g->generation;
    token->serial=entry->serial;token->kind=kind;
    return PT_NATIVE_DELIVERY_GATE_OK;
}

enum pt_native_delivery_gate_result pt_native_delivery_gate_task_enter(
    struct pt_native_delivery_gate *g,struct pt_native_delivery_gate_token *token)
{
    if(!valid(g))return PT_NATIVE_DELIVERY_GATE_REFUSED;
    if(g->failed)return PT_NATIVE_DELIVERY_GATE_FAULT;
    if(g->serial==UINT64_MAX || g->task_entry.acquiring || g->task_entry.active ||
       g->task_entry.restoring || g->local_entry.acquiring ||
       g->local_entry.active || g->local_entry.restoring ||
       !external_clear(g,token,sizeof(*token),ALIGNMENT(struct pt_native_delivery_gate_token)))
        return PT_NATIVE_DELIVERY_GATE_REFUSED;
    return enter(g,ENTRY_TASK,0,token);
}

enum pt_native_delivery_gate_result pt_native_delivery_gate_stage(
    struct pt_native_delivery_gate *g,const struct pt_native_delivery_gate_token *token,
    unsigned slot,uint64_t ticket)
{
    enum pt_native_delivery_gate_result r;unsigned i;
    if(slot>=PT_NATIVE_DELIVERY_GATE_SLOTS || !ticket)return PT_NATIVE_DELIVERY_GATE_REFUSED;
    r=active(g,token,ENTRY_TASK);if(r!=PT_NATIVE_DELIVERY_GATE_OK)return r;
    if(g->stopped || g->slot[slot].staged)return PT_NATIVE_DELIVERY_GATE_REFUSED;
    for(i=0;i<PT_NATIVE_DELIVERY_GATE_SLOTS;++i)
        if(g->slot[i].staged && g->slot[i].ticket==ticket)
            return PT_NATIVE_DELIVERY_GATE_REFUSED;
    g->slot[slot].ticket=ticket;publication_barrier();g->slot[slot].staged=1;
    return PT_NATIVE_DELIVERY_GATE_OK;
}

enum pt_native_delivery_gate_result pt_native_delivery_gate_observe(
    struct pt_native_delivery_gate *g,const struct pt_native_delivery_gate_token *token,
    unsigned slot,struct pt_native_delivery_gate_record *out)
{
    enum pt_native_delivery_gate_result r;
    if(slot>=PT_NATIVE_DELIVERY_GATE_SLOTS || !valid(g) ||
       !external_clear(g,out,sizeof(*out),ALIGNMENT(struct pt_native_delivery_gate_record)))
        return PT_NATIVE_DELIVERY_GATE_REFUSED;
    r=active(g,token,ENTRY_TASK);if(r!=PT_NATIVE_DELIVERY_GATE_OK)return r;
    if(!g->slot[slot].recorded || g->slot[slot].observed)
        return PT_NATIVE_DELIVERY_GATE_REFUSED;
    *out=g->slot[slot].record;g->slot[slot].observed=1;
    return PT_NATIVE_DELIVERY_GATE_OK;
}

enum pt_native_delivery_gate_result pt_native_delivery_gate_stop_admission(
    struct pt_native_delivery_gate *g,const struct pt_native_delivery_gate_token *token)
{
    enum pt_native_delivery_gate_result r=active(g,token,ENTRY_TASK);
    if(r!=PT_NATIVE_DELIVERY_GATE_OK)return r;
    g->stopped=1;return PT_NATIVE_DELIVERY_GATE_OK;
}

enum pt_native_delivery_gate_result pt_native_delivery_gate_local_enter(
    struct pt_native_delivery_gate *g,unsigned slot,uint64_t ticket,
    struct pt_native_delivery_gate_token *token)
{
    if(slot>=PT_NATIVE_DELIVERY_GATE_SLOTS || !ticket || !valid(g))
        return PT_NATIVE_DELIVERY_GATE_REFUSED;
    if(g->failed)return PT_NATIVE_DELIVERY_GATE_FAULT;
    if(g->serial==UINT64_MAX || g->task_entry.acquiring || g->task_entry.active ||
       g->local_entry.acquiring || g->local_entry.active || g->local_entry.restoring ||
       !g->slot[slot].staged || g->slot[slot].local_attempted ||
       g->slot[slot].ticket!=ticket ||
       !external_clear(g,token,sizeof(*token),ALIGNMENT(struct pt_native_delivery_gate_token)))
        return PT_NATIVE_DELIVERY_GATE_REFUSED;
    return enter(g,ENTRY_LOCAL,slot,token);
}

enum pt_native_delivery_gate_result pt_native_delivery_gate_local_record(
    struct pt_native_delivery_gate *g,const struct pt_native_delivery_gate_token *token,
    const struct pt_native_delivery_gate_record *record)
{
    enum pt_native_delivery_gate_result r;struct gate_slot *slot;
    if(!valid(g) || !external_clear(g,record,sizeof(*record),
                                  ALIGNMENT(struct pt_native_delivery_gate_record)))
        return PT_NATIVE_DELIVERY_GATE_REFUSED;
    r=active(g,token,ENTRY_LOCAL);if(r!=PT_NATIVE_DELIVERY_GATE_OK)return r;
    if(g->local_entry.slot>=PT_NATIVE_DELIVERY_GATE_SLOTS)
        return fault(g,&g->local_entry);
    slot=g->slot+g->local_entry.slot;
    if(!slot->staged || slot->recorded || record->ticket!=slot->ticket)
        return PT_NATIVE_DELIVERY_GATE_REFUSED;
    slot->record=*record;publication_barrier();slot->recorded=1;
    return PT_NATIVE_DELIVERY_GATE_OK;
}

static enum pt_native_delivery_gate_result leave(
    struct pt_native_delivery_gate *g,const struct pt_native_delivery_gate_token *token,
    unsigned kind)
{
    struct gate_entry *entry;enum pt_native_delivery_gate_result r;
    r=active(g,token,kind);if(r!=PT_NATIVE_DELIVERY_GATE_OK)return r;
    entry=kind==ENTRY_TASK?&g->task_entry:&g->local_entry;
    /* Consume this one obligation before restoration; it cannot be replayed.
     * Keep its separate anchor while Enable may interleave local model work. */
    entry->active=0;entry->restoring=1;publication_barrier();
    Enable();
    publication_barrier();
    if(!current_environment(g) || nesting(g)!=entry->prior_nesting || g->failed)
        return fault(g,entry);
    /* Only this entry's anchor ends. Never restore a pre-Enable gate/slot image:
     * deferred local delivery may have legitimately changed the mailboxes. */
    entry->restoring=0;entry->token=NULL;entry->serial=0;
    return PT_NATIVE_DELIVERY_GATE_OK;
}

enum pt_native_delivery_gate_result pt_native_delivery_gate_task_leave(
    struct pt_native_delivery_gate *g,const struct pt_native_delivery_gate_token *token)
{return leave(g,token,ENTRY_TASK);}
enum pt_native_delivery_gate_result pt_native_delivery_gate_local_leave(
    struct pt_native_delivery_gate *g,const struct pt_native_delivery_gate_token *token)
{return leave(g,token,ENTRY_LOCAL);}
