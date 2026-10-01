#include <stddef.h>
#include <assert.h>
#include <stdio.h>
#include "../src/native/task_priority.h"
static struct Task own={{-3}},other={{7}};static struct Task *current=&own;
static unsigned sets,fail_set,wrong_return;
struct Task *FindTask(const char *name){assert(!name);return current;}
BYTE SetTaskPri(struct Task *t,LONG priority)
{BYTE old=t->tc_Node.ln_Pri;assert(t==&own && current==&own);++sets;if(!fail_set)t->tc_Node.ln_Pri=(BYTE)priority;return wrong_return?old+1:old;}
int main(void)
{
    struct pt_native_task_priority p={0};unsigned n;
    assert(pt_native_task_priority_restore(&p));
    assert(!pt_native_task_priority_acquire(&p,128) && !sets && !p.active);
    current=NULL;assert(!pt_native_task_priority_acquire(&p,5) && !p.active && !sets);current=&own;
    assert(pt_native_task_priority_acquire(&p,5) && p.saved==-3 && p.active && own.tc_Node.ln_Pri==5);
    n=sets;assert(!pt_native_task_priority_acquire(&p,4) && sets==n);
    current=&other;assert(!pt_native_task_priority_restore(&p) && p.active && sets==n && other.tc_Node.ln_Pri==7);
    current=&own;fail_set=1;assert(!pt_native_task_priority_restore(&p) && p.active && own.tc_Node.ln_Pri==5);
    fail_set=0;assert(pt_native_task_priority_restore(&p) && !p.active && own.tc_Node.ln_Pri==-3);
    n=sets;assert(pt_native_task_priority_restore(&p) && sets==n);
    fail_set=1;assert(!pt_native_task_priority_acquire(&p,5) && !p.active && own.tc_Node.ln_Pri==-3);fail_set=0;
    wrong_return=1;assert(!pt_native_task_priority_acquire(&p,5) && !p.active && own.tc_Node.ln_Pri==-3);wrong_return=0;
    assert(pt_native_task_priority_acquire(&p,5));assert(pt_native_task_priority_restore(&p));
    assert(own.tc_Node.ln_Pri==-3 && !p.active);
    puts("NATIVE TASK PRIORITY HOST PASS: own identity only, invalid/missing/failed acquire, retained failed restore, exact saved restoration and inert repeat");return 0;
}
