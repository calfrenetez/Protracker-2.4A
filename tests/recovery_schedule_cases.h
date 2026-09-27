#include "../src/core/recovery.h"
struct recovery_clock_fixture {struct pt_recovery_schedule *schedule;unsigned calls;int result;};
static int scheduled_snapshot(void *context)
{
    struct recovery_clock_fixture *f=context;struct pt_recovery_policy p={1,0,0};
    ++f->calls;
    assert(pt_recovery_tick(f->schedule,1000,2,1,1,0,1,scheduled_snapshot,f)==PT_RECOVERY_INVALID);
    assert(!pt_recovery_configure(f->schedule,&p));
    return f->result;
}
static void recovery_schedule_cases(void)
{
    struct pt_recovery_schedule s={0};struct pt_recovery_policy p={10,1,0},bad={0,1,0};
    struct recovery_clock_fixture f={&s,0,1};
    assert(pt_recovery_configure(&s,&p));
    assert(!pt_recovery_configure(&s,&bad));
#define T(now,rev,saved,persist,removable,safe) pt_recovery_tick(&s,now,rev,saved,persist,removable,safe,scheduled_snapshot,&f)
    assert(T(0,1,0,1,0,1)==PT_RECOVERY_SKIPPED);
    assert(T(9,2,0,1,0,1)==PT_RECOVERY_SKIPPED);
    assert(T(10,3,0,1,0,0)==PT_RECOVERY_SKIPPED); /* Busy playback defers. */
    assert(T(11,4,0,1,0,1)==PT_RECOVERY_SAVED && f.calls==1); /* Edits never starve due work. */
    assert(T(100,4,0,1,0,1)==PT_RECOVERY_SKIPPED && f.calls==1);
    f.result=0;assert(T(101,5,0,1,0,1)==PT_RECOVERY_FAILED && f.calls==2);
    assert(T(110,6,0,1,0,1)==PT_RECOVERY_SKIPPED && f.calls==2);
    f.result=2;assert(T(111,6,0,1,0,1)==PT_RECOVERY_FAILED && f.calls==3);
    assert(s.snapshot_revision==4); /* Unknown positive result is not publication. */
    f.result=-1;assert(T(121,6,0,1,0,1)==PT_RECOVERY_FAILED && f.calls==4);
    f.result=1;assert(T(131,6,0,1,0,1)==PT_RECOVERY_SAVED && f.calls==5);
    assert(T(132,6,6,1,0,1)==PT_RECOVERY_SKIPPED); /* Manual save. */
    assert(T(140,0,6,1,0,1)==PT_RECOVERY_SKIPPED); /* Undo to initial is dirty. */
    assert(T(150,0,6,1,0,1)==PT_RECOVERY_SAVED && f.calls==6);
    assert(T(151,1,6,0,0,1)==PT_RECOVERY_SKIPPED); /* Unknown or volatile destination. */
    assert(T(200,1,6,1,1,1)==PT_RECOVERY_SKIPPED && f.calls==6); /* Floppy disabled. */
    assert(T(201,1,6,1,0,1)==PT_RECOVERY_SKIPPED);
    assert(T(211,1,6,1,0,1)==PT_RECOVERY_SAVED && f.calls==7);
    assert(T(212,2,6,1,0,1)==PT_RECOVERY_SKIPPED);
    assert(T(205,2,6,1,0,1)==PT_RECOVERY_SKIPPED); /* Clock rolls back: rebase. */
    assert(T(214,2,6,1,0,1)==PT_RECOVERY_SKIPPED);
    assert(T(215,2,6,1,0,1)==PT_RECOVERY_SAVED && f.calls==8);
    p.enabled=0;assert(pt_recovery_configure(&s,&p));
    assert(T(9999,3,6,1,0,1)==PT_RECOVERY_SKIPPED && f.calls==8);
    p.enabled=1;p.allow_removable=1;assert(pt_recovery_configure(&s,&p));
    assert(T(UINT64_MAX-10,3,6,1,1,1)==PT_RECOVERY_SKIPPED);
    assert(T(UINT64_MAX,3,6,1,1,1)==PT_RECOVERY_SAVED && f.calls==9);
    assert(T(0,4,6,1,1,1)==PT_RECOVERY_SKIPPED); /* Clock overflow handled like rollback. */
    assert(T(10,4,6,1,1,1)==PT_RECOVERY_SAVED && f.calls==10);
#undef T
}
