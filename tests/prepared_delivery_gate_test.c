/* Independent ordinary-C model. This is not an IRQ/SR/quiet qualification. */
#include "../src/native/prepared_delivery_gate.h"
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <proto/exec.h>
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define OK PT_NATIVE_DELIVERY_GATE_OK
#define REFUSED PT_NATIVE_DELIVERY_GATE_REFUSED
#define FAULT PT_NATIVE_DELIVERY_GATE_FAULT
#define ARENA_BYTES 16384U
#define FIXTURES 24U

union aligned_arena {
    long double alignment;
    uint64_t integer_alignment;
    void *pointer_alignment;
    unsigned char bytes[ARENA_BYTES];
};
struct fixture {
    union aligned_arena arena;
    struct pt_native_delivery_gate *gate;
    struct pt_native_delivery_gate_token task,local;
};
/* Faulted objects/anchors remain alive until process exit: no reset/disposal. */
static struct fixture fixtures[FIXTURES];
static unsigned fixture_count;
/* Each scenario gets a separate permanent fake-Exec domain. Starting a later
 * model never repairs or resets the counter/anchors retained by a faulted one. */
static struct ExecBase exec_domains[FIXTURES+8];
static unsigned exec_domain_count;
static struct ExecBase *model_exec=exec_domains;
struct ExecBase *SysBase=exec_domains;
#define exec_base (*model_exec)
static struct Task owner_task,other_task;
static unsigned disables,enables,cpu_ipl,enable_depth;
static void (*on_disable)(void),(*on_enable)(void);

void Disable(void)
{
    assert(exec_base.IDNestCnt<INT8_MAX);
    disables++;
    exec_base.IDNestCnt++;
    cpu_ipl=7;
    if(on_disable)on_disable();
}
void Enable(void)
{
    assert(exec_base.IDNestCnt>=0);
    enables++;
    exec_base.IDNestCnt--;
    if(exec_base.IDNestCnt<0)cpu_ipl=0;
    enable_depth++;
    if(on_enable)on_enable();
    enable_depth--;
}

static void model_context(int nesting)
{
    assert(nesting>=-1 && nesting<INT8_MAX);
    assert(exec_domain_count<FIXTURES+8);
    model_exec=&exec_domains[exec_domain_count++];
    memset(&exec_base,0,sizeof(exec_base));
    exec_base.IDNestCnt=(int8_t)nesting;
    exec_base.ThisTask=&owner_task;SysBase=&exec_base;
    disables=enables=enable_depth=0;
    cpu_ipl=nesting<0?0U:7U;
    on_disable=NULL;on_enable=NULL;
}
static struct fixture *fresh(int nesting)
{
    struct fixture *f;
    size_t size=pt_native_delivery_gate_size();
    size_t alignment=pt_native_delivery_gate_alignment();
    unsigned before_d,before_e;
    assert(fixture_count<FIXTURES && size>0 && size<ARENA_BYTES/2);
    assert(alignment>0 && (alignment&(alignment-1))==0);
    f=&fixtures[fixture_count++];
    memset(f,0,sizeof(*f));
    assert((uintptr_t)f->arena.bytes%alignment==0);
    f->gate=(struct pt_native_delivery_gate *)f->arena.bytes;
    memset(f->arena.bytes+size,0xA7,ARENA_BYTES-size);
    model_context(nesting);before_d=disables;before_e=enables;
    assert(pt_native_delivery_gate_init(f->gate,ARENA_BYTES,
                                      UINT64_C(0x12340000)+fixture_count)==OK);
    assert(disables==before_d && enables==before_e);
    return f;
}
static void tail_same(const struct fixture *f)
{
    size_t i;
    for(i=pt_native_delivery_gate_size();i<ARENA_BYTES;i++)
        assert(f->arena.bytes[i]==0xA7);
}
static void poison_token(struct pt_native_delivery_gate_token *t)
{ memset(t,0xB3,sizeof(*t)); }
static void token_unchanged(const struct pt_native_delivery_gate_token *a,
                            const struct pt_native_delivery_gate_token *b)
{ assert(memcmp(a,b,sizeof(*a))==0); }
static struct pt_native_delivery_gate_record record_for(uint64_t ticket)
{
    struct pt_native_delivery_gate_record r;
    memset(&r,0,sizeof(r));
    r.ticket=ticket;r.observed=UINT64_C(0x1122334455667788);
    r.issued=UINT64_C(0x8877665544332211);r.result=-1234567;
    r.detail=UINT32_C(0xA1B2C3D4);
    return r;
}
static void refusal_preserves_record(struct fixture *f,unsigned slot)
{
    struct pt_native_delivery_gate_record out,before;
    memset(&out,0xC5,sizeof(out));before=out;
    assert(pt_native_delivery_gate_observe(f->gate,&f->task,slot,&out)==REFUSED);
    assert(memcmp(&out,&before,sizeof(out))==0);
}
static void record_slot(struct fixture *f,unsigned slot,uint64_t ticket)
{
    struct pt_native_delivery_gate_record r=record_for(ticket),before=r;
    assert(pt_native_delivery_gate_local_enter(f->gate,slot,ticket,&f->local)==OK);
    assert(pt_native_delivery_gate_local_record(f->gate,&f->local,&r)==OK);
    assert(memcmp(&r,&before,sizeof(r))==0);
    assert(pt_native_delivery_gate_local_record(f->gate,&f->local,&r)==REFUSED);
    memset(&r,0xE9,sizeof(r)); /* No caller record pointer may be retained. */
    assert(pt_native_delivery_gate_local_leave(f->gate,&f->local)==OK);
}

static void constructors(void)
{
    union aligned_arena a,before;
    struct ExecBase exec_before;
    struct Task task_before;
    size_t size=pt_native_delivery_gate_size(),alignment=pt_native_delivery_gate_alignment();
    struct pt_native_delivery_gate *g=(struct pt_native_delivery_gate *)a.bytes;
    model_context(-1);memset(&a,0,sizeof(a));before=a;
    assert(size>0 && size<ARENA_BYTES && alignment>1);
    assert(pt_native_delivery_gate_init(NULL,ARENA_BYTES,1)==REFUSED);
    assert(pt_native_delivery_gate_init(g,size-1,1)==REFUSED);
    assert(pt_native_delivery_gate_init(g,ARENA_BYTES,0)==REFUSED);
    assert(pt_native_delivery_gate_init((struct pt_native_delivery_gate *)(a.bytes+1),
                                      ARENA_BYTES-1,1)==REFUSED);
    assert(pt_native_delivery_gate_init(g,SIZE_MAX,1)==REFUSED);
    assert(memcmp(&a,&before,sizeof(a))==0);
    exec_before=exec_base;task_before=owner_task;
    assert(pt_native_delivery_gate_init((struct pt_native_delivery_gate *)&exec_base,
                                      ARENA_BYTES,1)==REFUSED);
    assert(pt_native_delivery_gate_init((struct pt_native_delivery_gate *)&owner_task,
                                      ARENA_BYTES,1)==REFUSED);
    assert(memcmp(&exec_base,&exec_before,sizeof(exec_base))==0);
    assert(memcmp(&owner_task,&task_before,sizeof(owner_task))==0);
    SysBase=NULL;
    assert(pt_native_delivery_gate_init(g,ARENA_BYTES,1)==REFUSED);
    SysBase=&exec_base;exec_base.ThisTask=NULL;
    assert(pt_native_delivery_gate_init(g,ARENA_BYTES,1)==REFUSED);
    exec_base.ThisTask=&owner_task;
    assert(memcmp(&a,&before,sizeof(a))==0);
    a.bytes[size-1]=1;before=a;
    assert(pt_native_delivery_gate_init(g,ARENA_BYTES,1)==REFUSED);
    assert(memcmp(&a,&before,sizeof(a))==0);
    assert(disables==0 && enables==0);
    /* Reinitialization never disposes a live gate or clears its slots. */
    {
        struct fixture *f=fresh(-1);
        memcpy(&before,&f->arena,sizeof(before));
        assert(pt_native_delivery_gate_init(f->gate,ARENA_BYTES,77)==REFUSED);
        assert(memcmp(&before,&f->arena,sizeof(before))==0);
        assert(disables==0 && enables==0);tail_same(f);
    }
}
static void own_nesting_pairs(void)
{
    static const int prior_nesting[]={-1,0,1,2,126};
    unsigned index;
    for(index=0;index<sizeof(prior_nesting)/sizeof(prior_nesting[0]);index++) {
        int n=prior_nesting[index];
        struct fixture *f=fresh(n);
        struct pt_native_delivery_gate_token rejected,before,copy,stale;
        poison_token(&rejected);before=rejected;
        assert(pt_native_delivery_gate_task_enter(f->gate,&f->task)==OK);
        assert(exec_base.IDNestCnt==n+1 && cpu_ipl==7 && disables==1 && enables==0);
        assert(pt_native_delivery_gate_task_enter(f->gate,&rejected)==REFUSED);
        token_unchanged(&rejected,&before);
        assert(exec_base.IDNestCnt==n+1 && enables==0);
        copy=f->task;
        assert(pt_native_delivery_gate_stage(f->gate,&copy,0,1)==REFUSED);
        assert(pt_native_delivery_gate_task_leave(f->gate,&copy)==REFUSED);
        assert(enables==0 && exec_base.IDNestCnt==n+1);
        stale=f->task;
        assert(pt_native_delivery_gate_task_leave(f->gate,&f->task)==OK);
        assert(exec_base.IDNestCnt==n && disables==1 && enables==1);
        assert(cpu_ipl==(n<0?0U:7U));
        assert(pt_native_delivery_gate_task_leave(f->gate,&f->task)==REFUSED);
        assert(pt_native_delivery_gate_task_enter(f->gate,&f->local)==OK);
        assert(pt_native_delivery_gate_stage(f->gate,&stale,0,1)==REFUSED);
        assert(pt_native_delivery_gate_task_leave(f->gate,&stale)==REFUSED);
        assert(pt_native_delivery_gate_stage(f->gate,&f->task,0,1)==REFUSED);
        assert(pt_native_delivery_gate_task_leave(f->gate,&f->task)==REFUSED);
        assert(exec_base.IDNestCnt==n+1 && enables==1);
        assert(pt_native_delivery_gate_task_leave(f->gate,&f->local)==OK);
        assert(exec_base.IDNestCnt==n && disables==2 && enables==2);tail_same(f);
    }
    /* Direct fake-Exec demonstration only, outside the supported gate path:
     * equal counters do not establish arbitrary saved CPU IPL restoration. */
    model_context(-1);cpu_ipl=5;Disable();Enable();
    assert(exec_base.IDNestCnt==-1 && cpu_ipl==0 && cpu_ipl!=5);
}
static void original_token_field_refusals(struct fixture *f,unsigned local)
{
    struct pt_native_delivery_gate_token *token=local?&f->local:&f->task;
    struct pt_native_delivery_gate_token original;
    struct pt_native_delivery_gate_record input=record_for(71),before_input=input;
    union aligned_arena before_gate=f->arena;
    unsigned i,d=disables,e=enables;
    int prior=exec_base.IDNestCnt;
    memcpy(&original,token,sizeof(original));
    /* Adverse controls use the ORIGINAL active object, so address rejection
     * cannot mask a missing gate/generation/serial/kind scalar check. Restore
     * only this caller token's complete beforeimage, never the gate or pair. */
    for(i=0;i<4;i++) {
        switch(i) {
        case 0:token->gate=NULL;break;
        case 1:token->generation^=UINT64_C(1);break;
        case 2:token->serial^=UINT64_C(1);break;
        default:token->kind^=UINT32_C(1);break;
        }
        if(local) {
            assert(pt_native_delivery_gate_local_record(f->gate,token,&input)==REFUSED);
            assert(pt_native_delivery_gate_local_leave(f->gate,token)==REFUSED);
        }else {
            assert(pt_native_delivery_gate_stage(f->gate,token,0,71)==REFUSED);
            assert(pt_native_delivery_gate_task_leave(f->gate,token)==REFUSED);
        }
        assert(disables==d && enables==e && exec_base.IDNestCnt==prior);
        assert(memcmp(&f->arena,&before_gate,sizeof(before_gate))==0);
        assert(memcmp(&input,&before_input,sizeof(input))==0);
        memcpy(token,&original,sizeof(original));
        assert(memcmp(token,&original,sizeof(original))==0);
    }
}
static void original_token_fields_preserve_borrowed_pairs(void)
{
    struct fixture *f=fresh(-1);
    struct pt_native_delivery_gate_record input=record_for(71),out;
    assert(pt_native_delivery_gate_task_enter(f->gate,&f->task)==OK);
    original_token_field_refusals(f,0);
    assert(pt_native_delivery_gate_stage(f->gate,&f->task,0,71)==OK);
    assert(pt_native_delivery_gate_task_leave(f->gate,&f->task)==OK);
    assert(disables==1 && enables==1 && exec_base.IDNestCnt==-1);
    assert(pt_native_delivery_gate_local_enter(f->gate,0,71,&f->local)==OK);
    original_token_field_refusals(f,1);
    assert(pt_native_delivery_gate_local_record(f->gate,&f->local,&input)==OK);
    assert(pt_native_delivery_gate_local_leave(f->gate,&f->local)==OK);
    assert(disables==2 && enables==2 && exec_base.IDNestCnt==-1);
    assert(pt_native_delivery_gate_task_enter(f->gate,&f->task)==OK);
    assert(pt_native_delivery_gate_observe(f->gate,&f->task,0,&out)==OK);
    assert(memcmp(&out,&input,sizeof(out))==0);
    assert(pt_native_delivery_gate_task_leave(f->gate,&f->task)==OK);
    assert(disables==3 && enables==3 && exec_base.IDNestCnt==-1);tail_same(f);
}
static void staged_slots_and_observations(void)
{
    struct fixture *f=fresh(-1);
    struct pt_native_delivery_gate_token refused,before,copy;
    struct pt_native_delivery_gate_record out,want,input,before_input;
    unsigned i;
    assert(pt_native_delivery_gate_task_enter(f->gate,&f->task)==OK);
    for(i=0;i<PT_NATIVE_DELIVERY_GATE_SLOTS;i++) {
        assert(pt_native_delivery_gate_stage(f->gate,&f->task,i,100+i)==OK);
        refusal_preserves_record(f,i);
    }
    assert(pt_native_delivery_gate_stage(f->gate,&f->task,4,104)==REFUSED);
    assert(pt_native_delivery_gate_stage(f->gate,&f->task,0,105)==REFUSED);
    assert(pt_native_delivery_gate_stage(f->gate,&f->task,0,0)==REFUSED);
    poison_token(&refused);before=refused;
    assert(pt_native_delivery_gate_local_enter(f->gate,0,100,&refused)==REFUSED);
    token_unchanged(&refused,&before);assert(enables==0);
    assert(pt_native_delivery_gate_task_leave(f->gate,&f->task)==OK);
    assert(pt_native_delivery_gate_local_enter(f->gate,4,104,&refused)==REFUSED);
    assert(pt_native_delivery_gate_local_enter(f->gate,0,999,&refused)==REFUSED);
    token_unchanged(&refused,&before);
    assert(pt_native_delivery_gate_local_enter(f->gate,0,100,&f->local)==OK);
    assert(pt_native_delivery_gate_local_enter(f->gate,1,101,&refused)==REFUSED);
    assert(pt_native_delivery_gate_task_enter(f->gate,&refused)==REFUSED);
    token_unchanged(&refused,&before);
    copy=f->local;input=record_for(100);before_input=input;
    assert(pt_native_delivery_gate_local_record(f->gate,&copy,&input)==REFUSED);
    assert(pt_native_delivery_gate_local_leave(f->gate,&copy)==REFUSED);
    input.ticket=101;
    assert(pt_native_delivery_gate_local_record(f->gate,&f->local,&input)==REFUSED);
    assert(input.ticket==101);
    input=before_input;
    assert(pt_native_delivery_gate_local_record(f->gate,&f->local,&input)==OK);
    assert(memcmp(&input,&before_input,sizeof(input))==0);
    assert(pt_native_delivery_gate_local_record(f->gate,&f->local,&input)==REFUSED);
    memset(&input,0xE9,sizeof(input));
    assert(pt_native_delivery_gate_local_leave(f->gate,&f->local)==OK);
    assert(pt_native_delivery_gate_local_leave(f->gate,&f->local)==REFUSED);
    assert(pt_native_delivery_gate_local_enter(f->gate,0,100,&refused)==REFUSED);
    for(i=1;i<PT_NATIVE_DELIVERY_GATE_SLOTS;i++)record_slot(f,i,100+i);
    assert(pt_native_delivery_gate_task_enter(f->gate,&f->task)==OK);
    for(i=0;i<PT_NATIVE_DELIVERY_GATE_SLOTS;i++) {
        want=record_for(100+i);memset(&out,0xC5,sizeof(out));
        assert(pt_native_delivery_gate_observe(f->gate,&f->task,i,&out)==OK);
        assert(memcmp(&out,&want,sizeof(out))==0);
        refusal_preserves_record(f,i);
        assert(pt_native_delivery_gate_stage(f->gate,&f->task,i,200+i)==REFUSED);
    }
    assert(pt_native_delivery_gate_task_leave(f->gate,&f->task)==OK);
    assert(disables==enables && exec_base.IDNestCnt==-1);tail_same(f);
}

static void aliases_and_wrong_owner(void)
{
    struct fixture *f=fresh(-1);
    struct pt_native_delivery_gate_token t,before_token;
    struct ExecBase before_exec,foreign_exec;
    struct Task before_owner;
    union aligned_arena before_gate;
    struct pt_native_delivery_gate_record in;
    unsigned d,e;
    poison_token(&t);before_token=t;
    before_gate=f->arena;before_exec=exec_base;before_owner=owner_task;
    assert(pt_native_delivery_gate_task_enter(f->gate,NULL)==REFUSED);
    assert(pt_native_delivery_gate_task_enter(f->gate,
           (struct pt_native_delivery_gate_token *)(f->arena.bytes+1))==REFUSED);
    assert(pt_native_delivery_gate_task_enter(f->gate,
           (struct pt_native_delivery_gate_token *)(UINTPTR_MAX-7U))==REFUSED);
    assert(pt_native_delivery_gate_task_enter(f->gate,
           (struct pt_native_delivery_gate_token *)(f->arena.bytes+ARENA_BYTES-sizeof(t)))==REFUSED);
    assert(pt_native_delivery_gate_task_enter(f->gate,
           (struct pt_native_delivery_gate_token *)&exec_base)==REFUSED);
    assert(pt_native_delivery_gate_task_enter(f->gate,
           (struct pt_native_delivery_gate_token *)&owner_task)==REFUSED);
    assert(memcmp(&f->arena,&before_gate,sizeof(before_gate))==0);
    assert(memcmp(&exec_base,&before_exec,sizeof(exec_base))==0);
    assert(memcmp(&owner_task,&before_owner,sizeof(owner_task))==0);
    assert(disables==0 && enables==0);
    exec_base.ThisTask=&other_task;
    assert(pt_native_delivery_gate_task_enter(f->gate,&t)==REFUSED);
    token_unchanged(&t,&before_token);
    exec_base.ThisTask=&owner_task;
    assert(pt_native_delivery_gate_task_enter(f->gate,&f->task)==OK);
    assert(pt_native_delivery_gate_stage(f->gate,&f->task,0,41)==OK);
    before_gate=f->arena;before_exec=exec_base;before_owner=owner_task;
    assert(pt_native_delivery_gate_observe(f->gate,&f->task,0,
           (struct pt_native_delivery_gate_record *)&f->task)==REFUSED);
    assert(pt_native_delivery_gate_observe(f->gate,&f->task,0,
           (struct pt_native_delivery_gate_record *)(f->arena.bytes+ARENA_BYTES-sizeof(in)))==REFUSED);
    assert(pt_native_delivery_gate_observe(f->gate,&f->task,0,
           (struct pt_native_delivery_gate_record *)&exec_base)==REFUSED);
    assert(pt_native_delivery_gate_observe(f->gate,&f->task,0,
           (struct pt_native_delivery_gate_record *)&owner_task)==REFUSED);
    assert(memcmp(&f->arena,&before_gate,sizeof(before_gate))==0);
    assert(memcmp(&exec_base,&before_exec,sizeof(exec_base))==0);
    assert(memcmp(&owner_task,&before_owner,sizeof(owner_task))==0);
    d=disables;e=enables;before_gate=f->arena;
    exec_base.ThisTask=&other_task;
    assert(pt_native_delivery_gate_task_leave(f->gate,&f->task)==REFUSED);
    assert(memcmp(&f->arena,&before_gate,sizeof(before_gate))==0);
    assert(disables==d && enables==e);
    exec_base.ThisTask=&owner_task;
    foreign_exec=exec_base;SysBase=&foreign_exec;
    assert(pt_native_delivery_gate_task_leave(f->gate,&f->task)==REFUSED);
    assert(memcmp(&f->arena,&before_gate,sizeof(before_gate))==0);
    assert(disables==d && enables==e);SysBase=&exec_base;
    assert(pt_native_delivery_gate_task_leave(f->gate,&f->task)==OK);
    assert(pt_native_delivery_gate_local_enter(f->gate,0,41,&f->local)==OK);
    in=record_for(41);before_gate=f->arena;
    assert(pt_native_delivery_gate_local_record(f->gate,&f->local,
           (const struct pt_native_delivery_gate_record *)&f->local)==REFUSED);
    assert(pt_native_delivery_gate_local_record(f->gate,&f->local,
           (const struct pt_native_delivery_gate_record *)f->arena.bytes)==REFUSED);
    assert(memcmp(&f->arena,&before_gate,sizeof(before_gate))==0);
    assert(pt_native_delivery_gate_local_record(f->gate,&f->local,&in)==OK);
    assert(pt_native_delivery_gate_local_leave(f->gate,&f->local)==OK);
    assert(disables==enables && exec_base.IDNestCnt==-1);tail_same(f);
}

static void empty_local_attempt_never_reopens(void)
{
    struct fixture *f=fresh(-1);
    struct pt_native_delivery_gate_token out,before;
    assert(pt_native_delivery_gate_task_enter(f->gate,&f->task)==OK);
    assert(pt_native_delivery_gate_stage(f->gate,&f->task,0,51)==OK);
    assert(pt_native_delivery_gate_task_leave(f->gate,&f->task)==OK);
    poison_token(&out);before=out;
    /* A refused bad ticket consumes no attempt. */
    assert(pt_native_delivery_gate_local_enter(f->gate,0,52,&out)==REFUSED);
    token_unchanged(&out,&before);
    assert(pt_native_delivery_gate_local_enter(f->gate,0,51,&f->local)==OK);
    assert(pt_native_delivery_gate_local_leave(f->gate,&f->local)==OK);
    assert(pt_native_delivery_gate_local_enter(f->gate,0,51,&out)==REFUSED);
    token_unchanged(&out,&before);
    assert(pt_native_delivery_gate_task_enter(f->gate,&f->task)==OK);
    refusal_preserves_record(f,0);
    assert(pt_native_delivery_gate_stage(f->gate,&f->task,0,53)==REFUSED);
    assert(pt_native_delivery_gate_stage(f->gate,&f->task,1,51)==REFUSED);
    assert(pt_native_delivery_gate_task_leave(f->gate,&f->task)==OK);
    assert(disables==enables && exec_base.IDNestCnt==-1);tail_same(f);
}

static void guarded_tail_and_numeric_spans(void)
{
    long page=sysconf(_SC_PAGESIZE);
    unsigned char *mapped;
    struct pt_native_delivery_gate *g;
    size_t size=pt_native_delivery_gate_size();
    struct pt_native_delivery_gate_token t;
    struct pt_native_delivery_gate_record out,before;
    assert(page>0 && size<(size_t)page);
    mapped=mmap(NULL,(size_t)page*2,PROT_READ|PROT_WRITE,
                MAP_PRIVATE|MAP_ANON,-1,0);
    assert(mapped!=MAP_FAILED);
    assert(mprotect(mapped+page,(size_t)page,PROT_NONE)==0);
    g=(struct pt_native_delivery_gate *)mapped;
    model_context(-1);
    /* The declared capacity includes inaccessible tail pages. Only the owned
     * zero prefix may be inspected; numeric full-capacity guards precede output. */
    assert(pt_native_delivery_gate_init(g,(size_t)page*2,9)==OK);
    assert(pt_native_delivery_gate_task_enter(g,
           (struct pt_native_delivery_gate_token *)(mapped+page))==REFUSED);
    assert(disables==0 && enables==0);
    assert(pt_native_delivery_gate_task_enter(g,&t)==OK);
    assert(pt_native_delivery_gate_stage(g,&t,0,1)==OK);
    memset(&out,0xD8,sizeof(out));before=out;
    assert(pt_native_delivery_gate_observe(g,&t,0,&out)==REFUSED);
    assert(memcmp(&out,&before,sizeof(out))==0);
    assert(pt_native_delivery_gate_observe(g,&t,0,
           (struct pt_native_delivery_gate_record *)(mapped+page))==REFUSED);
    assert(pt_native_delivery_gate_task_leave(g,&t)==OK);
    assert(munmap(mapped,(size_t)page*2)==0);
    /* Wrapped complete extents are rejected numerically, before object reads. */
    assert(pt_native_delivery_gate_init((struct pt_native_delivery_gate *)
           (UINTPTR_MAX-(pt_native_delivery_gate_alignment()-1)),ARENA_BYTES,1)==REFUSED);
    assert(disables==1 && enables==1);
}

static void increment_nesting(void){exec_base.IDNestCnt++;}
static void change_owner(void){exec_base.ThisTask=&other_task;}
static void faults_retain_owned_obligations(void)
{
    struct fixture *f=fresh(-1);
    struct pt_native_delivery_gate_token out,before;
    unsigned d,e;
    poison_token(&f->task);before=f->task;
    on_disable=increment_nesting;
    assert(pt_native_delivery_gate_task_enter(f->gate,&f->task)==FAULT);
    on_disable=NULL;token_unchanged(&f->task,&before);
    assert(disables==1 && enables==0 && exec_base.IDNestCnt==1);
    assert(pt_native_delivery_gate_task_enter(f->gate,&out)==FAULT);
    assert(pt_native_delivery_gate_init(f->gate,ARENA_BYTES,2)==REFUSED);
    assert(enables==0);tail_same(f);

    f=fresh(-1);on_disable=change_owner;poison_token(&f->task);before=f->task;
    assert(pt_native_delivery_gate_task_enter(f->gate,&f->task)==FAULT);
    on_disable=NULL;token_unchanged(&f->task,&before);
    assert(disables==1 && enables==0 && exec_base.IDNestCnt==0);
    exec_base.ThisTask=&owner_task;
    assert(pt_native_delivery_gate_task_enter(f->gate,&out)==FAULT);
    assert(enables==0);tail_same(f);

    f=fresh(-1);
    assert(pt_native_delivery_gate_task_enter(f->gate,&f->task)==OK);
    exec_base.IDNestCnt++;d=disables;e=enables;
    assert(pt_native_delivery_gate_stage(f->gate,&f->task,0,1)==FAULT);
    assert(pt_native_delivery_gate_task_leave(f->gate,&f->task)==FAULT);
    assert(disables==d && enables==e);tail_same(f);

    f=fresh(-1);
    assert(pt_native_delivery_gate_task_enter(f->gate,&f->task)==OK);
    on_enable=increment_nesting;
    assert(pt_native_delivery_gate_task_leave(f->gate,&f->task)==FAULT);
    on_enable=NULL;d=disables;e=enables;
    assert(e==1 && exec_base.IDNestCnt==0);
    assert(pt_native_delivery_gate_task_leave(f->gate,&f->task)==REFUSED);
    poison_token(&out);before=out;
    assert(pt_native_delivery_gate_task_enter(f->gate,&out)==FAULT);
    token_unchanged(&out,&before);assert(disables==d && enables==e);tail_same(f);

    f=fresh(-1);
    assert(pt_native_delivery_gate_task_enter(f->gate,&f->task)==OK);
    on_enable=change_owner;
    assert(pt_native_delivery_gate_task_leave(f->gate,&f->task)==FAULT);
    on_enable=NULL;d=disables;e=enables;
    exec_base.ThisTask=&owner_task;
    assert(pt_native_delivery_gate_task_enter(f->gate,&out)==FAULT);
    assert(disables==d && enables==e && e==1);tail_same(f);

    f=fresh(-1);exec_base.IDNestCnt=-2;poison_token(&out);before=out;
    assert(pt_native_delivery_gate_task_enter(f->gate,&out)==FAULT);
    token_unchanged(&out,&before);assert(disables==0 && enables==0);tail_same(f);
    f=fresh(2);exec_base.IDNestCnt=INT8_MAX;poison_token(&out);before=out;
    assert(pt_native_delivery_gate_task_enter(f->gate,&out)==FAULT);
    token_unchanged(&out,&before);assert(disables==0 && enables==0);tail_same(f);
}

static struct fixture *interleaved;
static unsigned deliveries,deferred_before_return,local_restore_probes;
static void deferred_delivery(void)
{
    struct pt_native_delivery_gate_token refused,before,restoring_before;
    struct pt_native_delivery_gate_record r=record_for(900);
    if(enable_depth==2) {
        struct pt_native_delivery_gate_token task_before,local_before;
        unsigned d=disables,e=enables;
        int prior=exec_base.IDNestCnt;
        assert(deliveries==1);
        local_restore_probes++;
        poison_token(&refused);before=refused;
        memcpy(&task_before,&interleaved->task,sizeof(task_before));
        memcpy(&local_before,&interleaved->local,sizeof(local_before));
        /* LOCAL active is already cleared, while its restoring anchor still
         * owns the return. A different pending slot avoids recorded/attempted
         * rejection masking the LOCAL-restoring admission guard. */
        assert(pt_native_delivery_gate_task_enter(interleaved->gate,&refused)==REFUSED);
        token_unchanged(&refused,&before);
        assert(pt_native_delivery_gate_local_enter(interleaved->gate,1,901,
                                                  &refused)==REFUSED);
        token_unchanged(&refused,&before);
        token_unchanged(&interleaved->task,&task_before);
        token_unchanged(&interleaved->local,&local_before);
        assert(disables==d && enables==e && exec_base.IDNestCnt==prior);
        return;
    }
    if(enable_depth!=1 || deliveries)return;
    deliveries++;
    assert(exec_base.IDNestCnt==-1);
    poison_token(&refused);before=refused;
    assert(pt_native_delivery_gate_task_enter(interleaved->gate,&refused)==REFUSED);
    token_unchanged(&refused,&before);
    restoring_before=interleaved->task;
    assert(pt_native_delivery_gate_local_enter(interleaved->gate,0,900,
                                              &interleaved->task)==REFUSED);
    token_unchanged(&interleaved->task,&restoring_before);
    assert(pt_native_delivery_gate_local_enter(interleaved->gate,0,900,
                                              &interleaved->local)==OK);
    assert(pt_native_delivery_gate_local_enter(interleaved->gate,0,900,&refused)==REFUSED);
    token_unchanged(&refused,&before);
    assert(pt_native_delivery_gate_local_record(interleaved->gate,&interleaved->local,&r)==OK);
    memset(&r,0xE9,sizeof(r));
    assert(pt_native_delivery_gate_local_leave(interleaved->gate,&interleaved->local)==OK);
    deferred_before_return=1;
}
static void deferred_restore_and_stop(void)
{
    struct fixture *f=fresh(-1);
    struct pt_native_delivery_gate_record out,want=record_for(900);
    unsigned before;
    interleaved=f;deliveries=deferred_before_return=local_restore_probes=0;
    assert(pt_native_delivery_gate_task_enter(f->gate,&f->task)==OK);
    assert(pt_native_delivery_gate_stage(f->gate,&f->task,0,900)==OK);
    assert(pt_native_delivery_gate_stage(f->gate,&f->task,1,901)==OK);
    /* Ticket identity is globally unique across the four retained slots. */
    assert(pt_native_delivery_gate_stage(f->gate,&f->task,2,900)==REFUSED);
    assert(pt_native_delivery_gate_stop_admission(f->gate,&f->task)==OK);
    assert(pt_native_delivery_gate_stop_admission(f->gate,&f->task)==OK);
    assert(pt_native_delivery_gate_stage(f->gate,&f->task,2,902)==REFUSED);
    on_enable=deferred_delivery;
    assert(pt_native_delivery_gate_task_leave(f->gate,&f->task)==OK);
    on_enable=NULL;
    assert(deliveries==1 && deferred_before_return && local_restore_probes==1 &&
           exec_base.IDNestCnt==-1);
    assert(disables==2 && enables==2);
    before=disables;
    record_slot(f,1,901); /* Stop did not erase pending delivery or retire a source. */
    assert(disables==before+1);
    assert(pt_native_delivery_gate_task_enter(f->gate,&f->task)==OK);
    assert(pt_native_delivery_gate_observe(f->gate,&f->task,0,&out)==OK);
    assert(memcmp(&out,&want,sizeof(out))==0);
    want=record_for(901);
    assert(pt_native_delivery_gate_observe(f->gate,&f->task,1,&out)==OK);
    assert(memcmp(&out,&want,sizeof(out))==0);
    refusal_preserves_record(f,0);
    assert(pt_native_delivery_gate_stage(f->gate,&f->task,2,902)==REFUSED);
    assert(pt_native_delivery_gate_task_leave(f->gate,&f->task)==OK);
    assert(disables==enables && exec_base.IDNestCnt==-1);tail_same(f);
}

int main(void)
{
    constructors();own_nesting_pairs();staged_slots_and_observations();
    original_token_fields_preserve_borrowed_pairs();
    deferred_restore_and_stop();aliases_and_wrong_owner();
    empty_local_attempt_never_reopens();
    guarded_tail_and_numeric_spans();faults_retain_owned_obligations();
    puts("PREPARED DELIVERY GATE HOST PASS:constructor/task/local/deferred/fault;MODELED ORDINARY C ONLY");
    return 0;
}
