#include "amigus_reservation.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
struct fake {
    int available, supported, count, cycle, opens, closes, finds, reserves, releases;
    int cards[16], library;
    unsigned long code;
    void *owner, *card;
};
static int open_library(void *c)
{
    struct fake *f=c; ++f->opens;
    if (!f->available) return 0;
    assert(!f->library); f->library=1; return 1;
}
static void close_library(void *c)
{
    struct fake *f=c; assert(f->library && !f->owner);
    ++f->closes; f->library=0;
}
static void *find(void *c,void *previous)
{
    struct fake *f=c; int i;
    assert(f->library); ++f->finds;
    if (previous && f->cycle) return previous;
    if (!previous) return f->count ? &f->cards[0] : 0;
    for (i=0;i<f->count;++i) if (previous==&f->cards[i])
        return i+1<f->count ? &f->cards[i+1] : 0;
    assert(0); return 0;
}
static int supported(void *c,void *card)
{ struct fake *f=c; assert(card && f->library); return f->supported; }
static unsigned long reserve(void *c,void *card,void *owner)
{
    struct fake *f=c; assert(f->library && !f->owner); ++f->reserves;
    if (!f->code) {f->card=card;f->owner=owner;} return f->code;
}
static void release(void *c,void *card,void *owner)
{
    struct fake *f=c; assert(f->library && card==f->card && owner==f->owner);
    ++f->releases; f->owner=0; f->card=0;
}
int main(void)
{
    struct fake f;
    struct pt_amigus_reservation r={0};
    struct pt_amigus_reservation_api api={&f,open_library,close_library,find,supported,reserve,release};
    memset(&f,0,sizeof(f));
    assert(pt_amigus_reservation_open(&r,&api,16)==PT_AMIGUS_INVALID);
    assert(!f.opens);
    assert(pt_amigus_reservation_open(&r,&api,0)==PT_AMIGUS_NO_LIBRARY);
    assert(!f.closes && !f.finds);
    f.available=1;
    assert(pt_amigus_reservation_open(&r,&api,0)==PT_AMIGUS_NO_CARD);
    assert(f.closes==1 && !f.reserves);
    f.count=2;
    assert(pt_amigus_reservation_open(&r,&api,0)==PT_AMIGUS_UNSUPPORTED);
    assert(f.closes==2 && !f.reserves);
    f.supported=1; f.cycle=1;
    assert(pt_amigus_reservation_open(&r,&api,1)==PT_AMIGUS_BAD_ENUMERATION);
    assert(f.closes==3 && !f.reserves);
    f.cycle=0; f.code=0x101;
    assert(pt_amigus_reservation_open(&r,&api,1)==PT_AMIGUS_BUSY);
    assert(r.driver_code==0x101 && f.closes==4 && !f.releases);
    f.code=0x404;
    assert(pt_amigus_reservation_open(&r,&api,0)==PT_AMIGUS_DRIVER_ERROR);
    assert(r.driver_code==0x404 && f.closes==5 && !f.releases);
    f.code=0;
    assert(pt_amigus_reservation_open(&r,&api,1)==PT_AMIGUS_RESERVED);
    assert(f.card==&f.cards[1] && f.owner==&r && !r.driver_code);
    assert(pt_amigus_reservation_open(&r,&api,0)==PT_AMIGUS_INVALID);
    assert(pt_amigus_reservation_begin(&r));
    assert(!pt_amigus_reservation_begin(&r));
    assert(!pt_amigus_reservation_close(&r));
    assert(f.library && f.owner==&r && f.closes==5);
    assert(pt_amigus_reservation_end(&r));
    assert(!pt_amigus_reservation_end(&r));
    assert(pt_amigus_reservation_close(&r));
    assert(f.releases==1 && f.closes==6);
    assert(pt_amigus_reservation_close(&r));
    assert(f.releases==1 && f.closes==6);
    assert(!pt_amigus_reservation_begin(&r));
    f.count=16;
    assert(pt_amigus_reservation_open(&r,&api,15)==PT_AMIGUS_RESERVED);
    assert(f.card==&f.cards[15]);
    assert(pt_amigus_reservation_close(&r));
    api.release=0;
    assert(pt_amigus_reservation_open(&r,&api,0)==PT_AMIGUS_INVALID);
    /* Discovery must work with reservation callbacks absent. */
    api.reserve=0;api.release=0;
    {
        struct pt_amigus_discovery d;
        int closes=f.closes,reserves=f.reserves,releases=f.releases;
        f.available=0;
        assert(pt_amigus_discover(&api,&d)==1 && !d.available && !d.cards);
        assert(f.closes==closes);
        f.available=1;f.count=0;
        assert(pt_amigus_discover(&api,&d)==1 && d.available && !d.cards);
        f.count=16;
        assert(pt_amigus_discover(&api,&d)==1 && d.cards==16 && d.pcm_cards==16);
        f.supported=0;
        assert(pt_amigus_discover(&api,&d)==1 && d.cards==16 && !d.pcm_cards);
        f.cycle=1;
        assert(pt_amigus_discover(&api,&d)==-1 && d.cards==1);
        assert(!f.library && f.closes==closes+4);
        assert(f.reserves==reserves && f.releases==releases);
        assert(pt_amigus_discover(0,&d)==0 && !d.cards);
    }
    puts("amigus reservation lifecycle: PASS (fake library, no hardware)");
    return 0;
}
