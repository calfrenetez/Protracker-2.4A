#ifndef PT_NATIVE_TASK_PRIORITY_H
#define PT_NATIVE_TASK_PRIORITY_H
#include <stddef.h>
#include <exec/tasks.h>
#include <proto/exec.h>
/* Zero-init, noncopyable, serialized scope for the calling task only. Never
 * change another task. Failed restoration retains the obligation/context; the
 * owner must restore before disposal, including diagnostic error/HOLD paths.
 * This is scheduling context, not a clock, deadline or playback qualification. */
struct pt_native_task_priority {struct Task *task;BYTE saved;unsigned active;};
static inline int pt_native_task_priority_restore(struct pt_native_task_priority *p)
{
    if(!p)return 0;
    if(!p->active)return 1;
    if(FindTask(NULL)!=p->task)return 0;
    if(p->task->tc_Node.ln_Pri!=p->saved)SetTaskPri(p->task,p->saved);
    if(FindTask(NULL)!=p->task || p->task->tc_Node.ln_Pri!=p->saved)return 0;
    p->active=0;return 1;
}
static inline int pt_native_task_priority_acquire(struct pt_native_task_priority *p,LONG priority)
{
    struct Task *task;BYTE before;
    if(!p || p->active || priority<-128 || priority>127)return 0;
    task=FindTask(NULL);if(!task)return 0;
    p->task=task;p->saved=task->tc_Node.ln_Pri;p->active=1;
    before=SetTaskPri(task,priority);
    if(FindTask(NULL)==task && before==p->saved && task->tc_Node.ln_Pri==priority)return 1;
    pt_native_task_priority_restore(p);return 0;
}
#endif
