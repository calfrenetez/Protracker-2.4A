/* Private process-local mock vector table only. Never registers a library,
 * calls OpenLibrary/CloseLibrary, touches card registers or installs interrupts.
 * The production find/reserve/free callback bodies and GCC SFD shims are used. */
#include "native_exec_memory.h"
#include "../src/native/amigus_reservation.h"
#include "../src/native/amigus_interrupt.h"
#include "../src/native/amigus_interrupt_calls.h"
#include <amigus/amigus.h>
#include <exec/libraries.h>
#include <stdint.h>
#include <string.h>
#define PT_AMIGUS_INTERRUPT_OWNER_MAIN interrupt_lifetime_fixture_main
#include "amigus_interrupt_owner_test.c"
volatile ULONG pt_abi_card,pt_abi_base,pt_abi_owner,pt_abi_flag,pt_abi_kind;
volatile ULONG pt_abi_next,pt_abi_result,pt_abi_callback_context;
volatile ULONG pt_abi_handler,pt_abi_data;
volatile ULONG pt_abi_after_d1,pt_abi_after_a0,pt_abi_after_a1;
extern LONG pt_abi_invoke(void (*)(void),APTR);
static LONG private_callback(void *data)
{
    pt_abi_callback_context=(ULONG)data;
    __asm__ volatile("moveq #23,%%d1\nmove.l #0x11223344,%%a0\nmove.l #0x55667788,%%a1" : : : "d1","a0","a1");
    return (LONG)pt_abi_result;
}
extern void pt_abi_find(void),pt_abi_reserve(void),pt_abi_free(void),pt_abi_unexpected(void);
extern void pt_abi_install(void),pt_abi_remove(void);
__asm__(
".text\n.even\n"
".globl _pt_abi_invoke\n_pt_abi_invoke:\n"
"move.l 4(%sp),%a1\nmove.l 8(%sp),%a0\nmove.l #0xa5a5a5a5,%d1\njsr (%a1)\nmove.l %d1,_pt_abi_after_d1\nmove.l %a0,_pt_abi_after_a0\nmove.l %a1,_pt_abi_after_a1\nrts\n"
".globl _pt_abi_find\n_pt_abi_find:\n"
"move.l %a0,_pt_abi_card\nmove.l %a6,_pt_abi_base\nmove.l #1,_pt_abi_kind\nmove.l _pt_abi_next,%d0\nrts\n"
".globl _pt_abi_reserve\n_pt_abi_reserve:\n"
"move.l %a0,_pt_abi_card\nmove.l %a6,_pt_abi_base\nmove.l %d0,_pt_abi_flag\nmove.l %d1,_pt_abi_owner\nmove.l #2,_pt_abi_kind\nmove.l _pt_abi_result,%d0\nrts\n"
".globl _pt_abi_free\n_pt_abi_free:\n"
"move.l %a0,_pt_abi_card\nmove.l %a6,_pt_abi_base\nmove.l %d0,_pt_abi_flag\nmove.l %d1,_pt_abi_owner\nmove.l #3,_pt_abi_kind\nrts\n"
".globl _pt_abi_install\n_pt_abi_install:\n"
"move.l %a0,_pt_abi_card\nmove.l %a6,_pt_abi_base\nmove.l %d0,_pt_abi_flag\nmove.l %d1,_pt_abi_owner\nmove.l %d2,_pt_abi_handler\nmove.l %d3,_pt_abi_data\nmove.l #4,_pt_abi_kind\nmove.l _pt_abi_result,%d0\nrts\n"
".globl _pt_abi_remove\n_pt_abi_remove:\n"
"move.l %a0,_pt_abi_card\nmove.l %a6,_pt_abi_base\nmove.l %d0,_pt_abi_flag\nmove.l %d1,_pt_abi_owner\nmove.l #5,_pt_abi_kind\nmove.l #0xdeadbeef,%d0\nrts\n"
".globl _pt_abi_unexpected\n_pt_abi_unexpected:\nmove.l #99,_pt_abi_kind\nmoveq #0,%d0\nrts\n");
static void vector(unsigned char *base,unsigned offset,void (*function)(void))
{
    uintptr_t address=(uintptr_t)function;unsigned char *p=base-offset;
    p[0]=0x4e;p[1]=0xf9;p[2]=(unsigned char)(address>>24);p[3]=(unsigned char)(address>>16);p[4]=(unsigned char)(address>>8);p[5]=(unsigned char)address;
}
static unsigned fake_opens,fake_closes;
static int private_open(void *context) {assert(((struct pt_native_amigus_library *)context)->base);++fake_opens;return 1;}
static void private_close(void *context) {assert(((struct pt_native_amigus_library *)context)->base);++fake_closes;}
static void check(unsigned kind,struct pt_native_amigus_library *n,void *card,ULONG flag,void *owner)
{
    assert(pt_abi_kind==kind && pt_abi_base==(ULONG)n->base && pt_abi_card==(ULONG)card);
    if(kind!=1)assert(pt_abi_flag==flag && pt_abi_owner==(ULONG)owner);
}
static int irq_quiescence;
static int private_quiesce(void *context,struct pt_amigus_reservation *r,void *binding)
{
    assert(context==r->api.context && binding && r->interrupt && r->access);
    return irq_quiescence;
}
static void interrupt_calls(struct pt_native_amigus_library *n,struct pt_amigus_reservation *r)
{
    unsigned failure;ULONG token=0x87654321;
    struct pt_native_amigus_interrupt binding={private_callback,&token,1};
    struct pt_amigus_interrupt_api api={n,pt_native_amigus_interrupt_install,pt_native_amigus_interrupt_remove,private_quiesce};
    pt_abi_kind=0;
    assert(pt_native_amigus_interrupt_install(n,r,&binding)==AmiGUS_InterruptInstallFailed);
    pt_native_amigus_interrupt_remove(n,r);assert(!pt_abi_kind); /* No active guard. */
    for(failure=0;failure<=1;++failure) {
        struct pt_amigus_interrupt_owner owner={0};
        pt_abi_result=failure?0xf1234567UL:0;
        assert(pt_amigus_interrupt_owner_begin(&owner,r,&api,&binding)==
               (failure?PT_AMIGUS_INTERRUPT_FAILED:PT_AMIGUS_INTERRUPT_READY));
        check(4,n,r->card,(ULONG)r->resource,r);
        assert(owner.install_code==(failure?0xf1234567UL:0));
        assert(pt_abi_handler==(ULONG)pt_native_amigus_interrupt_entry && pt_abi_data==(ULONG)&binding);
        /* A failed install may still leave a callback. Exercise its exact saved
         * entry/data as ordinary task code; never create a real interrupt. */
        pt_abi_result=1;pt_abi_callback_context=0;
        assert(pt_abi_invoke((void (*)(void))pt_abi_handler,(APTR)pt_abi_data)==1);
        assert(pt_abi_callback_context==(ULONG)&token);
        irq_quiescence=0;assert(!pt_amigus_interrupt_owner_stop(&owner));
        check(5,n,r->card,(ULONG)r->resource,r);
        assert(!pt_amigus_reservation_end(r) && !pt_amigus_reservation_close(r));
        irq_quiescence=1;assert(pt_amigus_interrupt_owner_stop(&owner));
        assert(r->access && !r->interrupt && pt_amigus_interrupt_owner_detach(&owner));
    }
}
int main(void)
{
    unsigned char *memory;struct pt_native_amigus_library native[2];struct pt_amigus_reservation_api api[2];
    struct AmiGUS card={0};unsigned i,resource;ULONG owner_token=0x01234567;
    struct pt_native_amigus_interrupt interrupt={private_callback,&owner_token,0};
    native_memory_start();assert(interrupt_lifetime_fixture_main()==0);
    memory=native_allocate(2*(60+sizeof(struct Library)));
    memset(memory,0,2*(60+sizeof(struct Library)));
    for(i=0;i<2;++i) {
        unsigned offset;unsigned char *base=memory+i*(60+sizeof(struct Library))+60;
        for(offset=6;offset<=60;offset+=6)vector(base,offset,pt_abi_unexpected);
        /* Independently transcribed published SFD offsets: -30,-36,-42. */
        vector(base,30,pt_abi_find);vector(base,36,pt_abi_reserve);vector(base,42,pt_abi_free);
        vector(base,48,pt_abi_install);vector(base,54,pt_abi_remove);
        native[i].base=(struct Library *)base;api[i]=pt_native_amigus_reservation_api(&native[i]);
    }
    /* Independent a0-context assembly caller poisons d1. The explicit native
     * shim bridges to stack-based C dispatch; no actual interrupt is installed. */
    pt_abi_result=1;pt_abi_callback_context=0;
    assert(pt_abi_invoke(pt_native_amigus_interrupt_entry,&interrupt)==0 && !pt_abi_callback_context);
    interrupt.armed=1;
    assert(pt_abi_invoke(pt_native_amigus_interrupt_entry,&interrupt)==1);
    assert(pt_abi_callback_context==(ULONG)&owner_token);
    assert(pt_abi_after_d1==0xa5a5a5a5UL && pt_abi_after_a0==(ULONG)&interrupt && pt_abi_after_a1==(ULONG)pt_native_amigus_interrupt_entry);
    interrupt.context=&card;pt_abi_result=0;
    assert(pt_abi_invoke(pt_native_amigus_interrupt_entry,&interrupt)==0);
    assert(pt_abi_callback_context==(ULONG)&card);
    pt_abi_result=0xffffffffUL;assert(pt_abi_invoke(pt_native_amigus_interrupt_entry,&interrupt)==0);
    pt_abi_result=2;assert(pt_abi_invoke(pt_native_amigus_interrupt_entry,&interrupt)==0);
    interrupt.handler=NULL;pt_abi_callback_context=0;
    assert(pt_abi_invoke(pt_native_amigus_interrupt_entry,&interrupt)==0 && !pt_abi_callback_context);
    assert(pt_abi_invoke(pt_native_amigus_interrupt_entry,NULL)==0);
    CacheClearU(); /* Make freshly written private jump vectors executable. */
    card.agus_TypeId=AmiGUS_mini;card.agus_PcmBase=&owner_token;card.agus_WavetableBase=&owner_token;
    for(i=0;i<2;++i) {
        struct pt_amigus_reservation_api owner_api=api[i];
        /* Production open refuses an existing reference before any Exec call. */
        assert(!api[i].open(api[i].context));
        pt_abi_next=(ULONG)&card;assert(api[i].find(api[i].context,NULL)==&card);check(1,&native[i],NULL,0,NULL);
        pt_abi_next=0;assert(!api[i].find(api[i].context,&card));check(1,&native[i],&card,0,NULL);
        assert(api[i].supported(api[i].context,&card,PT_AMIGUS_PCM));
        assert(api[i].supported(api[i].context,&card,PT_AMIGUS_WAVETABLE));
        card.agus_TypeId=0;assert(!api[i].supported(api[i].context,&card,PT_AMIGUS_PCM));card.agus_TypeId=AmiGUS_Zorro2;
        for(resource=1;resource<=2;++resource) {
            struct pt_amigus_reservation r={0};enum pt_amigus_resource which=(enum pt_amigus_resource)resource;
            pt_abi_result=0xf1234567UL;
            assert(api[i].reserve(api[i].context,&card,which,&owner_token)==pt_abi_result);check(2,&native[i],&card,resource,&owner_token);
            api[i].release(api[i].context,&card,which,&owner_token);check(3,&native[i],&card,resource,&owner_token);
            owner_api.open=private_open;owner_api.close=private_close;pt_abi_next=(ULONG)&card;pt_abi_result=0;
            assert(pt_amigus_reservation_open_resource(&r,&owner_api,0,which)==PT_AMIGUS_RESERVED);check(2,&native[i],&card,resource,&r);
            assert(pt_amigus_reservation_begin(&r) && !pt_amigus_reservation_close(&r));
            interrupt_calls(&native[i],&r);
            assert(pt_amigus_reservation_end(&r) && pt_amigus_reservation_close(&r));check(3,&native[i],&card,resource,&r);
            pt_abi_result=0x100UL|resource;
            assert(pt_amigus_reservation_open_resource(&r,&owner_api,0,which)==PT_AMIGUS_BUSY);
            check(2,&native[i],&card,resource,&r);assert(!r.opened && !r.reserved && r.driver_code==pt_abi_result);
        }
    }
    assert(fake_opens==8 && fake_closes==8);native[0].base=native[1].base=NULL;
    native_release(memory);native_memory_finish();
    puts("AMIGUS NATIVE ABI PASS: private Find/Reserve/Free/Install/Remove vectors, a0/d0/d1/d2/d3/a6, full32 return, explicit callback bridge, partial-install retention and separate quiescence, per-context PCM/wavetable owners; no installed library or hardware access");
    return 0;
}
