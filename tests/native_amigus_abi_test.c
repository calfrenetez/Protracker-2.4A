/* Private process-local mock vector table only. Never registers a library,
 * calls OpenLibrary/CloseLibrary, touches card registers or installs interrupts.
 * The production find/reserve/free callback bodies and GCC SFD shims are used. */
#include "native_exec_memory.h"
#include "../src/native/amigus_reservation.h"
#include <amigus/amigus.h>
#include <exec/libraries.h>
#include <stdint.h>
#include <string.h>
volatile ULONG pt_abi_card,pt_abi_base,pt_abi_owner,pt_abi_flag,pt_abi_kind;
volatile ULONG pt_abi_next,pt_abi_result;
extern void pt_abi_find(void),pt_abi_reserve(void),pt_abi_free(void),pt_abi_unexpected(void);
__asm__(
".text\n.even\n"
".globl _pt_abi_find\n_pt_abi_find:\n"
"move.l %a0,_pt_abi_card\nmove.l %a6,_pt_abi_base\nmove.l #1,_pt_abi_kind\nmove.l _pt_abi_next,%d0\nrts\n"
".globl _pt_abi_reserve\n_pt_abi_reserve:\n"
"move.l %a0,_pt_abi_card\nmove.l %a6,_pt_abi_base\nmove.l %d0,_pt_abi_flag\nmove.l %d1,_pt_abi_owner\nmove.l #2,_pt_abi_kind\nmove.l _pt_abi_result,%d0\nrts\n"
".globl _pt_abi_free\n_pt_abi_free:\n"
"move.l %a0,_pt_abi_card\nmove.l %a6,_pt_abi_base\nmove.l %d0,_pt_abi_flag\nmove.l %d1,_pt_abi_owner\nmove.l #3,_pt_abi_kind\nrts\n"
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
int main(void)
{
    unsigned char *memory;struct pt_native_amigus_library native[2];struct pt_amigus_reservation_api api[2];
    struct AmiGUS card={0};unsigned i,resource;ULONG owner_token=0x01234567;
    native_memory_start();memory=native_allocate(2*(48+sizeof(struct Library)));
    memset(memory,0,2*(48+sizeof(struct Library)));
    for(i=0;i<2;++i) {
        unsigned offset;unsigned char *base=memory+i*(48+sizeof(struct Library))+48;
        for(offset=6;offset<=48;offset+=6)vector(base,offset,pt_abi_unexpected);
        /* Independently transcribed published SFD offsets: -30,-36,-42. */
        vector(base,30,pt_abi_find);vector(base,36,pt_abi_reserve);vector(base,42,pt_abi_free);
        native[i].base=(struct Library *)base;api[i]=pt_native_amigus_reservation_api(&native[i]);
    }
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
            assert(pt_amigus_reservation_end(&r) && pt_amigus_reservation_close(&r));check(3,&native[i],&card,resource,&r);
            pt_abi_result=0x100UL|resource;
            assert(pt_amigus_reservation_open_resource(&r,&owner_api,0,which)==PT_AMIGUS_BUSY);
            check(2,&native[i],&card,resource,&r);assert(!r.opened && !r.reserved && r.driver_code==pt_abi_result);
        }
    }
    assert(fake_opens==8 && fake_closes==8);native[0].base=native[1].base=NULL;
    native_release(memory);native_memory_finish();
    puts("AMIGUS NATIVE ABI PASS: private vectors verify Find/Reserve/Free offsets and a0/d0/d1/a6, full32 return, per-context bases, PCM/wavetable owner lifetime and busy unwind; no installed library or hardware access");
    return 0;
}
