#include "driver_window.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

struct fixture {
    struct pt_driver_snapshot s;
    int locked, removes, restores, refuse_remove, fail_restore;
};
static void enter(void *v) { struct fixture *f=v; assert(!f->locked); f->locked=1; }
static void leave(void *v) { struct fixture *f=v; assert(f->locked); f->locked=0; }
static struct pt_driver_snapshot snapshot(void *v)
{ struct fixture *f=v; assert(f->locked); return f->s; }
static void remove_idle(void *v)
{
    struct fixture *f=v;
    assert(f->locked && f->s.present && f->s.supported);
    assert(!f->s.driver_users && !f->s.ahi_users && !f->s.delayed_expunge);
    ++f->removes;
    if (!f->refuse_remove) f->s.present=0;
    else f->s.delayed_expunge=1;
}
static int restore(void *v)
{
    struct fixture *f=v; assert(!f->locked); ++f->restores;
    if (f->fail_restore) return 0;
    f->s.present=1; f->s.delayed_expunge=0;
    return 1;
}
int main(void)
{
    unsigned int i;
    for (i=0; i<8; ++i) {
        struct fixture f={{1,1,0,0,0},0,0,0,0,0};
        struct pt_driver_api api={&f,enter,leave,snapshot,remove_idle,restore};
        struct pt_driver_window r;
        if (i==0) f.s.present=0;
        if (i==1) f.s.supported=0;
        if (i==2) f.s.driver_users=1;
        if (i==3) f.s.ahi_users=1;
        if (i==4) f.s.delayed_expunge=1;
        if (i==6) f.refuse_remove=1;
        if (i==7) f.fail_restore=1;
        r=pt_driver_begin(&api);
        assert(!f.locked);
        if (i<5) {
            assert(r.result==PT_SKIP && !r.unloaded && !r.restore_needed);
            assert(pt_driver_end(&api,&r) && !f.removes && !f.restores);
        } else {
            assert(f.removes==1 && r.restore_needed);
            assert(r.result==(i==6 ? PT_FAIL : PT_PASS));
            assert(r.unloaded==(i!=6));
            assert(pt_driver_end(&api,&r)==(i!=7));
            assert(f.restores==1 && r.restored==(i!=7));
            assert(r.restore_needed==(i==7));
            if (i!=7) {
                assert(pt_driver_end(&api,&r) && f.restores==1);
                assert(f.s.present && !f.s.delayed_expunge);
            } else assert(!strcmp(r.stage,"driver-restoration-unconfirmed"));
        }
    }
    puts("driver window: 8 scenarios passed");
    return 0;
}
