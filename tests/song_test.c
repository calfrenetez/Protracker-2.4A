#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/editor.h"
#include "mod_project.h"
static unsigned calls,fail,live;
static void *allocate(void *c,size_t n) {(void)c;void *p;if(++calls==fail)return NULL;p=malloc(n);if(p)++live;return p;}
static void release(void *c,void *p) {(void)c;if(p) {assert(live);--live;free(p);}}
static struct pt_pattern_command commands[128];
static struct pt_event_change changes[2048];
static void reset(struct pt_document *d,struct pt_song *s,struct pt_pattern_history *h,unsigned channels)
{
    pt_pattern_history_release(h);pt_song_release(s);assert(!s->bytes);
    assert(pt_document_new(d,channels,SIZE_MAX)==PT_PROJECT_OK);
    assert(pt_pattern_history_init(h,&d->project,commands,128,changes,2048)==PT_EDIT_OK);
}
int main(int argc,char **argv)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d,reopened;struct pt_song s;
    struct pt_pattern_history h;struct pt_project *p;struct pt_event_update u;struct pt_event *original,*second,*third;
    unsigned i,j,allocated,revision,cursor,limit=argc>1?16:256,kept;
    (void)argv;size_t budget,bytes,n,w;uint8_t *encoded,*again;struct pt_mod_export_report report;
    struct pt_editor *e;
    pt_document_init(&d,&a);pt_document_init(&reopened,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);p=&d.project;
    pt_song_init(&s,&a,8UL*1024*1024);assert(pt_pattern_history_init(&h,p,commands,128,changes,2048)==PT_EDIT_OK);
    original=p->events;allocated=live;
    for(i=1;i<=3;++i) {
        fail=calls+i;assert(pt_song_append(&s,p,&h,0,1)==PT_EDIT_CAPACITY);
        assert(live==allocated && !s.bytes && !s.current && !h.count && p->events==original && p->order_count==1 && p->pattern_count==1);
    }
    fail=0;
    assert(pt_song_append(&s,p,&h,0,1)==PT_EDIT_OK);second=p->events;
    assert(second!=original && p->pattern_count==2 && p->order_count==2 && p->orders[1]==1);
    u=(struct pt_event_update){256,{428,0,PT_NOTE_PERIOD,1,0,0,0,0}};
    assert(pt_pattern_apply(p,&h,&u,1)==PT_EDIT_OK);
    assert(pt_song_append(&s,p,&h,0,1)==PT_EDIT_OK);third=p->events;
    assert(third!=second && third[256].pitch==428 && p->pattern_count==3);
    assert(pt_song_append(&s,p,&h,0,1)==PT_EDIT_OK && p->events==third && p->pattern_count==4);
    assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_OK && p->pattern_count==3 && p->events==third);
    assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_OK && p->pattern_count==2 && p->events==second && p->events[256].pitch==428);
    assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_OK && !p->events[256].pitch);
    p->events[256]=u.event;cursor=h.cursor;revision=h.revision;
    assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_CONFLICT && h.cursor==cursor && h.revision==revision);
    memset(&p->events[256],0,sizeof(*p->events));p->orders[0]=1;
    assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_CONFLICT);p->orders[0]=0;
    assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_OK && p->events==original && !pt_pattern_dirty(&h));
    for(i=0;i<4;++i)assert(pt_pattern_undo(p,&h,1)==PT_EDIT_OK);
    assert(p->pattern_count==4 && p->events[256].pitch==428);
    assert(pt_song_assign(&s,p,&h,0,3)==PT_EDIT_OK && p->orders[0]==3);
    assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_OK && !p->orders[0]);cursor=h.cursor;revision=h.revision;bytes=s.bytes;
    assert(pt_song_assign(&s,p,&h,0,0)==PT_EDIT_OK && h.cursor==cursor && h.revision==revision && s.bytes==bytes);
    budget=s.budget;s.budget=s.bytes;assert(pt_song_append(&s,p,&h,0,1)==PT_EDIT_CAPACITY);s.budget=budget;
    for(i=1;i<=2;++i) {fail=calls+i;allocated=live;assert(pt_song_append(&s,p,&h,0,1)==PT_EDIT_CAPACITY && live==allocated && h.cursor==cursor);}
    fail=0;assert(pt_pattern_undo(p,&h,1)==PT_EDIT_OK && p->orders[0]==3);
    p->orders[0]=2;assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_CONFLICT);p->orders[0]=3;
    assert(pt_song_append(&s,p,&h,2,0)==PT_EDIT_OK && p->order_count==5 && p->pattern_count==4 && p->orders[4]==2);
    assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_OK && p->order_count==4);
    assert(pt_song_append(&s,p,&h,1,0)==PT_EDIT_OK && p->orders[4]==1 && h.cursor==h.count);
    assert(pt_pattern_undo(p,&h,1)==PT_EDIT_END);
    assert(pt_project_size(p,&n)==PT_PROJECT_OK);encoded=malloc(n);again=malloc(n);assert(encoded && again);
    assert(pt_project_encode(p,encoded,n,&w)==PT_PROJECT_OK && w==n);
    assert(pt_document_load(&reopened,encoded,n,SIZE_MAX)==PT_PROJECT_OK);
    assert(pt_project_encode(&reopened.project,again,n,&w)==PT_PROJECT_OK && !memcmp(encoded,again,n));free(encoded);free(again);
    assert(pt_mod_export_analyse(p,&report)==PT_PROJECT_OK && !report.issues);encoded=malloc(report.bytes);assert(encoded);
    assert(pt_mod_export_direct(p,encoded,report.bytes,&w)==PT_PROJECT_OK && w==report.bytes);
    assert(pt_document_load(&reopened,encoded,w,SIZE_MAX)==PT_PROJECT_OK && reopened.project.order_count==5 && reopened.project.pattern_count==4 && reopened.project.events[256].pitch==428);free(encoded);
    /* Position insertion/removal/move keeps every pattern and all note data. */
    {uint16_t initial[5]={3,1,2,3,1},arranged[5]={1,3,2,3,2};unsigned count;
        assert(!memcmp(p->orders,initial,sizeof(initial)));second=p->events;
        fail=calls+1;allocated=live;revision=h.revision;
        assert(pt_song_insert(&s,p,&h,0,2)==PT_EDIT_CAPACITY && live==allocated && h.revision==revision && p->order_count==5);
        fail=0;
        assert(pt_song_insert(&s,p,&h,0,2)==PT_EDIT_OK && p->order_count==6 && p->orders[0]==2 && p->orders[1]==3);
        assert(pt_song_remove(&s,p,&h,2)==PT_EDIT_OK && p->order_count==5);
        assert(pt_song_move(&s,p,&h,4,1)==PT_EDIT_OK);
        assert(pt_song_move(&s,p,&h,0,4)==PT_EDIT_OK && !memcmp(p->orders,arranged,sizeof(arranged)));
        assert(p->events==second && p->pattern_count==4 && p->events[256].pitch==428);
        for(i=0;i<4;++i)assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_OK);
        assert(!memcmp(p->orders,initial,sizeof(initial)));cursor=h.cursor;revision=h.revision;
        assert(pt_song_move(&s,p,&h,0,0)==PT_EDIT_OK && h.cursor==cursor && h.revision==revision);
        p->orders[3]=0;assert(pt_pattern_undo(p,&h,1)==PT_EDIT_CONFLICT);p->orders[3]=3;
        for(i=0;i<4;++i)assert(pt_pattern_undo(p,&h,1)==PT_EDIT_OK);
        assert(!memcmp(p->orders,arranged,sizeof(arranged)));
        assert(pt_song_insert(&s,p,&h,6,0)==PT_EDIT_INVALID && pt_song_insert(&s,p,&h,0,4)==PT_EDIT_INVALID);
        assert(pt_song_remove(&s,p,&h,5)==PT_EDIT_INVALID && pt_song_move(&s,p,&h,5,0)==PT_EDIT_INVALID && pt_song_move(&s,p,&h,0,5)==PT_EDIT_INVALID);
        while(p->order_count>1)assert(pt_song_remove(&s,p,&h,p->order_count-1)==PT_EDIT_OK);
        revision=h.revision;count=(unsigned)h.count;
        assert(pt_song_remove(&s,p,&h,0)==PT_EDIT_UNSUPPORTED && h.revision==revision && h.count==count && p->pattern_count==4);
        assert(pt_song_insert(&s,p,&h,1,p->orders[0])==PT_EDIT_OK);revision=h.revision;
        assert(pt_song_move(&s,p,&h,0,1)==PT_EDIT_OK && h.revision==revision);
        assert(p->events==second && p->events[256].pitch==428);
    }
    /* Eviction and capacity stay bounded even at the 16-channel maximum. */
    reset(&d,&s,&h,16);
    for(i=1;i<limit;++i) {
        assert(pt_song_append(&s,p,&h,0,1)==PT_EDIT_OK && p->order_count==i+1 && p->pattern_count==i+1);
        assert(s.bytes<6UL*1024*1024 && h.count<=128);
    }
    if(limit==256)assert(pt_song_append(&s,p,&h,0,1)==PT_EDIT_CAPACITY && pt_song_append(&s,p,&h,0,0)==PT_EDIT_CAPACITY && pt_song_insert(&s,p,&h,128,0)==PT_EDIT_CAPACITY);
    kept=limit>128?128:limit-1;
    for(i=0;i<kept;++i)assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_OK);
    assert(p->pattern_count==limit-kept && pt_pattern_undo(p,&h,-1)==PT_EDIT_END);
    for(i=0;i<kept;++i)assert(pt_pattern_undo(p,&h,1)==PT_EDIT_OK);
    assert(p->pattern_count==limit && pt_project_validate(p,NULL)==PT_PROJECT_OK);
    /* Replacing the document before disposing editor history is safe. */
    assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    pt_pattern_history_release(&h);pt_song_release(&s);assert(!s.bytes);
    assert(pt_pattern_history_init(&h,p,commands,128,changes,2048)==PT_EDIT_OK);
    /* Revision exhaustion and corrupt bindings never publish staged arrays. */
    original=p->events;h.next_revision=UINT32_MAX;allocated=live;
    assert(pt_song_append(&s,p,&h,0,1)==PT_EDIT_CAPACITY && p->events==original && !s.bytes && live==allocated);
    h.next_revision=1;h.bound_patterns=7;
    assert(pt_song_append(&s,p,&h,0,1)==PT_EDIT_INVALID && p->events==original && !s.bytes);
    h.bound_patterns=1;pt_pattern_history_release(&h);
    e=calloc(1,sizeof(*e));assert(e && pt_editor_init(e,p));
    assert(pt_editor_click(e,500,50)==PT_UI_NONE && e->song_details && e->panel==1);
    pt_editor_key(e,0x36,0);assert(p->pattern_count==2 && e->position==1 && e->pattern==1);
    pt_editor_key(e,0x20,0);assert(p->order_count==3 && p->orders[2]==1);
    pt_editor_key(e,0x4f,0);assert(p->orders[2]==0 && e->pattern==0);
    pt_editor_key(e,0x31,8);assert(p->orders[2]==1 && e->pattern==1);
    pt_editor_key(e,0x31,8);assert(p->order_count==2 && e->position==1);
    pt_editor_key(e,0x31,8);assert(p->pattern_count==1 && e->position==0 && e->pattern==0);
    for(j=0;j<3;++j)pt_editor_key(e,0x31,9);
    assert(p->pattern_count==2 && p->order_count==3 && p->orders[2]==0);
    pt_editor_key(e,0x45,0);assert(e->panel==0);
    pt_editor_key(e,0x19,8);assert(e->song_details && e->panel==1);
    e->position=1;e->pattern=1;
    pt_editor_key(e,0x17,0);assert(p->order_count==4 && p->orders[1]==1 && e->position==1);
    pt_editor_key(e,0x4d,1);assert(e->position==2 && p->order_count==4); /* equal adjacent patterns: no-op history, navigation follows */
    pt_editor_key(e,0x4d,1);assert(e->position==3 && p->orders[2]==0 && p->orders[3]==1);
    pt_editor_key(e,0x22,0);assert(p->order_count==3 && e->position==2 && e->pattern==0 && p->pattern_count==2);
    pt_editor_key(e,0x31,8);assert(p->order_count==4 && p->orders[3]==1);
    pt_editor_key(e,0x37,0);assert(e->song_tools);
    pt_editor_click(e,500,50);assert(e->position==3); /* MOVE DN */
    pt_editor_click(e,500,31);assert(p->order_count==3 && e->position==2); /* REMOVE */
    pt_editor_dispose(e);free(e);pt_document_release(&d);pt_document_release(&reopened);assert(!live);
    puts("SONG PASS: bounded growth, insert/remove/move, assignments, allocation rollback, mixed note undo, conflicts, eviction, limits, round trips, controller and complete release");return 0;
}
