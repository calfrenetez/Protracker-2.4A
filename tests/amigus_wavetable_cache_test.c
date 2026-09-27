#define main reservation_fixture_main
#include "amigus_reservation_test.c"
#undef main
#include <stdlib.h>
#include "amigus_wavetable_cache.h"
struct fixture {
    struct fake library;
    struct pt_amigus_reservation reservation;
    struct pt_amigus_wavetable_cache cache;
    unsigned healthy,writes,fail;
    uint32_t address;
    uint8_t ram[128];
};
static int bus_owned(void *ctx)
{
    struct fixture *f=ctx;
    return f->healthy && f->library.library && f->library.owner==&f->reservation &&
        f->library.acquired==PT_AMIGUS_WAVETABLE && f->reservation.reserved;
}
static int bus_write(void *ctx,unsigned reg,uint32_t value)
{
    struct fixture *f=ctx;unsigned i;
    assert(bus_owned(f) && f->reservation.access);++f->writes;
    assert(!pt_amigus_reservation_close(&f->reservation));
    if(reg==0x14)f->address=value;
    else {
        assert(reg==0x10 && !(f->address&3) && f->address<=124);
        for(i=0;i<4;++i)f->ram[f->address+i]=(uint8_t)(value>>(24-8*i));
    }
    return !f->fail || f->fail!=f->writes;
}
static void init(struct fixture *f,enum pt_amigus_resource resource)
{
    struct pt_amigus_reservation_api api={&f->library,open_library,close_library,find,supported,reserve,release};
    memset(f,0,sizeof(*f));f->healthy=1;
    f->library.available=f->library.supported=f->library.count=1;
    memset(f->ram,0xcc,sizeof(f->ram));
    assert(pt_amigus_reservation_open_resource(&f->reservation,&api,0,resource)==PT_AMIGUS_RESERVED);
}
static enum pt_cache_result load(struct fixture *f,unsigned id,unsigned version,struct pt_cache_lease *lease)
{
    int32_t data[]={8388607,1,-8388608,-1,257,258,65536,-65536,123,-123},original[10];
    uint8_t staging[300];
    struct pt_pcm p={data,10,5,48000,2,24};struct pt_playback_format format={16,0,0,0};
    enum pt_cache_result result;
    memcpy(original,data,sizeof(data));
    result=pt_amigus_wavetable_cache_acquire(&f->cache,&p,id,version,&format,staging,sizeof(staging),lease);
    assert(!memcmp(original,data,sizeof(data)));return result;
}
static void attached(struct fixture *f)
{
    assert(pt_amigus_wavetable_cache_attach(&f->cache,&f->reservation,16,32,32,f,bus_owned,bus_write));
    assert(f->reservation.access && !f->writes);
    assert(!pt_amigus_wavetable_cache_attach(&f->cache,&f->reservation,16,32,32,f,bus_owned,bus_write));
}
static void refusal(struct fixture *f)
{
    init(f,PT_AMIGUS_PCM);
    assert(!pt_amigus_wavetable_cache_attach(&f->cache,&f->reservation,16,32,32,f,bus_owned,bus_write));
    assert(!f->reservation.access && !f->writes);assert(pt_amigus_reservation_close(&f->reservation));
    init(f,PT_AMIGUS_WAVETABLE);
    assert(!pt_amigus_wavetable_cache_attach(&f->cache,&f->reservation,17,32,32,f,bus_owned,bus_write));
    assert(!pt_amigus_wavetable_cache_attach(&f->cache,&f->reservation,0xfffffffc,8,8,f,bus_owned,bus_write));
    assert(!pt_amigus_wavetable_cache_attach(&f->cache,&f->reservation,0x02000000,4,4,f,bus_owned,bus_write));
    assert(!pt_amigus_wavetable_cache_attach(&f->cache,&f->reservation,0x01fffffc,8,8,f,bus_owned,bus_write));
    assert(!f->reservation.access && !f->writes && !f->cache.reservation);
    f->healthy=0;assert(!pt_amigus_wavetable_cache_attach(&f->cache,&f->reservation,16,32,32,f,bus_owned,bus_write));
    assert(!f->reservation.access);f->healthy=1;
    assert(pt_amigus_reservation_begin(&f->reservation));
    assert(!pt_amigus_wavetable_cache_attach(&f->cache,&f->reservation,16,32,32,f,bus_owned,bus_write));
    assert(f->reservation.access);assert(pt_amigus_reservation_end(&f->reservation));
    assert(pt_amigus_reservation_close(&f->reservation));
}
static void lifetime(struct fixture *f)
{
    struct pt_cache_lease a,b,hit,out={31,999};unsigned writes;uint32_t address,bytes;
    const uint8_t expected[]={0x7f,0xff,0x80,0,0,1,1,0,0,0,0,0};
    init(f,PT_AMIGUS_WAVETABLE);attached(f);
    assert(load(f,1,1,&a)==PT_CACHE_LOAD);assert(!memcmp(f->ram+16,expected,sizeof(expected)));
    assert(pt_amigus_wavetable_cache_location(&f->cache,a,&address,&bytes) && address==16 && bytes==10);
    writes=f->writes;assert(load(f,1,1,&hit)==PT_CACHE_HIT && f->writes==writes);
    assert(pt_amigus_wavetable_cache_unpin(&f->cache,hit));
    pt_amigus_wavetable_cache_invalidate(&f->cache,1);
    assert(load(f,1,2,&b)==PT_CACHE_LOAD);
    assert(pt_amigus_wavetable_cache_location(&f->cache,b,&address,&bytes) && address==28);
    assert(!memcmp(f->ram+16,expected,sizeof(expected))); /* Old voice still pinned. */
    assert(load(f,2,1,&out)==PT_CACHE_CAPACITY && out.serial==999);
    writes=f->writes;
    assert(!pt_amigus_wavetable_cache_detach(&f->cache));
    assert(f->reservation.access && f->library.owner && !f->library.releases);
    assert(!pt_amigus_reservation_close(&f->reservation));
    assert(load(f,1,2,&out)==PT_CACHE_INVALID && out.serial==999);
    assert(!pt_amigus_wavetable_cache_location(&f->cache,b,&address,&bytes));
    assert(pt_amigus_wavetable_cache_unpin(&f->cache,a));
    assert(!pt_amigus_wavetable_cache_detach(&f->cache));
    assert(pt_amigus_wavetable_cache_unpin(&f->cache,b));
    assert(pt_amigus_wavetable_cache_detach(&f->cache));assert(f->writes==writes);
    assert(!f->reservation.access && f->reservation.reserved && !f->library.releases);
    assert(pt_amigus_wavetable_cache_detach(&f->cache));
    assert(!pt_amigus_wavetable_cache_unpin(&f->cache,b));
    assert(pt_amigus_reservation_close(&f->reservation));
    assert(f->library.releases==1 && f->library.closes==1);
}
static void failure(struct fixture *f)
{
    struct pt_cache_lease a,b={31,999};unsigned writes;uint32_t address,bytes;
    init(f,PT_AMIGUS_WAVETABLE);attached(f);f->fail=2;
    assert(load(f,1,1,&b)==PT_CACHE_TRANSFER && b.serial==999);
    assert(!f->cache.cache.bytes && f->reservation.access && !f->library.releases);
    f->fail=0;assert(load(f,1,1,&a)==PT_CACHE_LOAD);writes=f->writes;
    f->healthy=0;
    assert(load(f,1,1,&b)==PT_CACHE_INVALID && b.serial==999); /* HIT must check owner. */
    f->healthy=1;
    assert(load(f,1,1,&b)==PT_CACHE_INVALID); /* Lost ownership is latched. */
    assert(!pt_amigus_wavetable_cache_location(&f->cache,a,&address,&bytes));
    assert(f->writes==writes && !pt_amigus_reservation_close(&f->reservation));
    assert(!pt_amigus_wavetable_cache_detach(&f->cache));
    assert(pt_amigus_wavetable_cache_unpin(&f->cache,a));
    assert(pt_amigus_wavetable_cache_detach(&f->cache));
    assert(pt_amigus_reservation_close(&f->reservation));
    init(f,PT_AMIGUS_WAVETABLE);attached(f);assert(load(f,1,1,&a)==PT_CACHE_LOAD);
    assert(pt_amigus_wavetable_cache_unpin(&f->cache,a));assert(pt_amigus_wavetable_cache_detach(&f->cache));
    assert(pt_amigus_reservation_close(&f->reservation));
}
static int wavetable_fixture_main(void)
{
    struct fixture *f=malloc(sizeof(*f));assert(f);assert(reservation_fixture_main()==0);
    refusal(f);lifetime(f);failure(f);free(f);
    puts("AMIGUS WAVETABLE OWNER PASS: explicit resource, pinned cache lifetime, failure cleanup, lost ownership refusal; fake library/bus only");return 0;
}

#ifndef PT_WAVETABLE_NATIVE
int main(void) {return wavetable_fixture_main();}
#endif
