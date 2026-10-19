#ifndef FH_LEDGER_H
#define FH_LEDGER_H
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "document.h"
#ifndef FH_MAX_ALLOCATIONS
#define FH_MAX_ALLOCATIONS 64
#endif
#define FH_SLOTS 64
struct fh_entry {void *p;size_t bytes;unsigned char *snapshot;};
struct fh_ledger {struct fh_entry e[FH_SLOTS];size_t calls,fail,triggered,live,peak,releases,limit;};
static inline void *fh_alloc(void *ctx,size_t n)
{
    struct fh_ledger *l=ctx;size_t i;void *p;
    assert(n && n<=1048576);++l->calls;
    if(l->calls==l->fail || (l->limit && n>l->limit-l->live)) {++l->triggered;return NULL;}
    p=malloc(n);assert(p);memset(p,0xa7,n);
    for(i=0;i<FH_SLOTS && l->e[i].p;++i) {}
    assert(i<FH_SLOTS);l->e[i].p=p;l->e[i].bytes=n;l->live+=n;
    if(l->live>l->peak)l->peak=l->live;
    return p;
}
static inline void fh_free(void *ctx,void *p)
{
    struct fh_ledger *l=ctx;size_t i;
    assert(p);for(i=0;i<FH_SLOTS && l->e[i].p!=p;++i) {}
    assert(i<FH_SLOTS && !l->e[i].snapshot && l->live>=l->e[i].bytes);
    l->live-=l->e[i].bytes;free(p);memset(l->e+i,0,sizeof(l->e[i]));++l->releases;
}
static inline void fh_reset(struct fh_ledger *l,size_t fail)
{l->calls=l->triggered=l->releases=0;l->peak=l->live;l->fail=fail;}
static inline void fh_snapshot(struct fh_ledger *l)
{
    size_t i;for(i=0;i<FH_SLOTS;++i)if(l->e[i].p) {
        assert(!l->e[i].snapshot);l->e[i].snapshot=malloc(l->e[i].bytes);assert(l->e[i].snapshot);
        memcpy(l->e[i].snapshot,l->e[i].p,l->e[i].bytes);
    }
}
static inline void fh_check_snapshot(struct fh_ledger *l)
{size_t i;for(i=0;i<FH_SLOTS;++i)if(l->e[i].snapshot)assert(!memcmp(l->e[i].p,l->e[i].snapshot,l->e[i].bytes));}
static inline void fh_drop_snapshot(struct fh_ledger *l)
{size_t i;for(i=0;i<FH_SLOTS;++i) {free(l->e[i].snapshot);l->e[i].snapshot=NULL;}}
#endif
