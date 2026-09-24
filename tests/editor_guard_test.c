#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "../src/editor/editor.h"
static void *allocate(void *c,size_t n) {(void)c;return malloc(n);}
static void release(void *c,void *p) {(void)c;free(p);}
struct guard {struct pt_editor *editor;unsigned calls,expected;};
static void stop(void *context)
{
    struct guard *g=context;
    assert(g->editor->project->events[0].kind==g->expected);++g->calls;
}
int main(void)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d;struct pt_editor *e=calloc(1,sizeof(*e));struct guard g;
    assert(e);pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    assert(pt_editor_init(e,&d.project));g=(struct guard){e,0,PT_NOTE_NONE};pt_editor_change_guard(e,stop,&g);
    /* Navigation must not stop playback; real note edit must stop before write. */
    pt_editor_key(e,0x4d,0);assert(!g.calls);e->row=0;e->editing=1;
    pt_editor_key(e,0x31,0);assert(g.calls==1 && d.project.events[0].kind==PT_NOTE_PERIOD);
    g.expected=PT_NOTE_PERIOD;pt_editor_key(e,0x31,8);assert(g.calls==2 && d.project.events[0].kind==PT_NOTE_NONE);
    g.expected=PT_NOTE_NONE;pt_editor_prepare_change(e);assert(g.calls==3);
    pt_editor_dispose(e);assert(g.calls==4);pt_document_release(&d);free(e);
    puts("EDITOR GUARD PASS: note edit and undo stop before mutation, navigation preserved, disposal guarded");return 0;
}
