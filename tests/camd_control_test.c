#include "src/platform/camd_control.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

struct receiver {struct receiver *next;};
struct sender {struct sender *next;const void *owner;};
struct cluster {struct cluster *next;char name[64];struct receiver *receivers;struct sender *senders;};
struct model_node;
struct model_link {struct model_link *next;struct model_node *owner;struct cluster *cluster;const char *name;struct sender sender;};
struct model_node {struct model_node *next;struct model_link *links;const char *name;};
struct model_library {struct model_node *nodes;};
enum fault {NONE,OPEN_NULL,NODE_NULL,ADD_NULL,LOCK_NULL,NEXT_FAIL,REMOVE_NO_EFFECT,UNLOCK_UNKNOWN,REMOVE_UNKNOWN,DELETE_UNKNOWN,CLOSE_UNKNOWN};
struct model {
    struct pt_camd_control *control;struct model_library library,*opened;
    struct model_node node;struct model_link links[16];
    struct cluster clusters[17],*first;struct receiver receivers[17];
    struct sender external_senders[17];int lock_token;void *locked;
    enum fault fault;unsigned calls,next_calls,reenter,reentry_checks;
    char events[192];unsigned event_count;
};
struct rig {struct pt_camd_control control;struct model model;};
/* Retained UNKNOWN models and contexts remain reachable for the whole fixture. */
static struct rig rigs[16];
static void event(struct model *m,char code)
{
    assert(m->event_count<sizeof(m->events));m->events[m->event_count++]=code;++m->calls;
    if(m->reenter){
        struct pt_camd_control *c=m->control;unsigned char before[sizeof(*c)];
        memcpy(before,c,sizeof(before));
        assert(pt_camd_control_activate(c)==PT_CAMD_BUSY);
        assert(pt_camd_control_refresh(c)==PT_CAMD_BUSY);
        assert(pt_camd_control_link(c,0)==PT_CAMD_BUSY);
        assert(pt_camd_control_retire(c,0)==PT_CAMD_BUSY);
        assert(pt_camd_control_close_step(c)==PT_CAMD_BUSY);
        assert(pt_camd_control_state(c)==-1&&pt_camd_control_registry(c)==NULL);
        assert(!memcmp(before,c,sizeof(before)));++m->reentry_checks;
    }
}
static void *open_library(void *context)
{
    struct model *m=context;event(m,'O');assert(!m->opened);
    if(m->fault==OPEN_NULL)return NULL;
    m->opened=&m->library;return m->opened;
}
static void *create_node(void *context,void *library,const char *name)
{
    struct model *m=context;event(m,'N');assert(library==m->opened&&!m->library.nodes&&name);
    if(m->fault==NODE_NULL)return NULL;
    m->node.name=name;m->node.next=NULL;m->library.nodes=&m->node;return &m->node;
}
static struct cluster *cluster_named(struct model *m,const char *name)
{
    struct cluster *c;for(c=m->first;c;c=c->next)if(!strcmp(c->name,name))return c;
    return NULL;
}
static void *add_sender(void *context,void *library,void *node,const char *name)
{
    struct model *m=context;struct model_link *link=NULL;struct cluster *cluster;unsigned i;
    event(m,'A');assert(library==m->opened&&node==m->library.nodes&&!m->locked&&name);
    if(m->fault==ADD_NULL)return NULL;
    cluster=cluster_named(m,name);assert(cluster);
    for(i=0;i<16;++i)if(!m->links[i].owner){link=&m->links[i];break;}
    assert(link);link->owner=node;link->cluster=cluster;link->name=name;
    link->next=m->node.links;m->node.links=link;link->sender.owner=node;
    link->sender.next=cluster->senders;cluster->senders=&link->sender;return link;
}
static void *lock_lists(void *context,void *library)
{
    struct model *m=context;event(m,'K');assert(library==m->opened&&!m->locked);
    if(m->fault==LOCK_NULL)return NULL;
    m->locked=&m->lock_token;return m->locked;
}
static enum pt_camd_next next_copy(void *context,void *library,const void *last,struct pt_camd_view *v)
{
    struct model *m=context;const struct cluster *c=last;event(m,'X');++m->next_calls;
    assert(library==m->opened&&m->locked==&m->lock_token&&v);
    if(m->fault==NEXT_FAIL)return PT_CAMD_READ_FAILED;
    c=c?c->next:m->first;
    if(!c)return PT_CAMD_END;
    v->cursor=c;memcpy(v->name,c->name,sizeof(v->name));v->receivers=c->receivers!=NULL;
    return PT_CAMD_ITEM;
}
static enum pt_camd_done unlock_lists(void *context,void *library,void *lock)
{
    struct model *m=context;event(m,'U');assert(library==m->opened&&lock==m->locked&&lock==&m->lock_token);
    m->locked=NULL;return m->fault==UNLOCK_UNKNOWN?PT_CAMD_UNKNOWN:PT_CAMD_DONE;
}
static enum pt_camd_done remove_link(void *context,void *library,void *handle)
{
    struct model *m=context;struct model_link *link=handle,**lp;struct sender **sp;
    event(m,'R');assert(library==m->opened&&!m->locked&&link&&link->owner==m->library.nodes);
    if(m->fault==REMOVE_NO_EFFECT)return PT_CAMD_NO_EFFECT;
    for(lp=&m->node.links;*lp&&*lp!=link;lp=&(*lp)->next){}
    assert(*lp==link);*lp=link->next;
    for(sp=&link->cluster->senders;*sp&&*sp!=&link->sender;sp=&(*sp)->next){}
    assert(*sp==&link->sender);*sp=link->sender.next;
    link->owner=NULL;link->cluster=NULL;link->name=NULL;link->next=NULL;
    link->sender.next=NULL;link->sender.owner=NULL;
    return m->fault==REMOVE_UNKNOWN?PT_CAMD_UNKNOWN:PT_CAMD_DONE;
}
static enum pt_camd_done delete_node(void *context,void *library,void *node)
{
    struct model *m=context;event(m,'D');assert(library==m->opened&&node==m->library.nodes&&!m->locked&&!m->node.links);
    m->library.nodes=NULL;m->node.name=NULL;
    return m->fault==DELETE_UNKNOWN?PT_CAMD_UNKNOWN:PT_CAMD_DONE;
}
static enum pt_camd_done close_library(void *context,void *library)
{
    struct model *m=context;event(m,'C');assert(library==m->opened&&!m->library.nodes&&!m->locked);
    m->opened=NULL;return m->fault==CLOSE_UNKNOWN?PT_CAMD_UNKNOWN:PT_CAMD_DONE;
}
static const struct pt_camd_ops OPS={open_library,create_node,add_sender,lock_lists,next_copy,unlock_lists,remove_link,delete_node,close_library};
static void setup(struct rig *f)
{
    memset(f,0,sizeof(*f));f->model.control=&f->control;
    assert(pt_camd_control_init(&f->control,&OPS,&f->model,sizeof(f->model)));
}
static void clusters(struct rig *f,unsigned count,unsigned receiver_mask)
{
    unsigned i;struct model *m=&f->model;assert(count<=17&&!m->node.links&&!m->locked);
    memset(m->clusters,0,sizeof(m->clusters));memset(m->receivers,0,sizeof(m->receivers));
    memset(m->external_senders,0,sizeof(m->external_senders));
    for(i=0;i<count;++i){
        snprintf(m->clusters[i].name,64,"Port %u",i);
        m->clusters[i].next=i+1<count?&m->clusters[i+1]:NULL;
        if(receiver_mask&(1U<<i))m->clusters[i].receivers=&m->receivers[i];
    }
    m->first=count?m->clusters:NULL;
}
static const struct pt_midi_endpoints *registry(struct rig *f)
{const struct pt_midi_endpoints *r=pt_camd_control_registry(&f->control);assert(r);return r;}
static unsigned index_of(struct rig *f,const char *name)
{int p=pt_midi_endpoints_find(registry(f),name,strlen(name)+1);assert(p>=0);return (unsigned)p;}
static void quiet_registry(struct rig *f)
{
    const struct pt_midi_endpoints *r=registry(f);unsigned p,t,b;
    assert(!r->link_active&&!r->busy);
    for(t=0;t<PT_MIDI_TRACKS;++t)assert(!r->midi.voices[t].active&&!r->midi.voices[t].bound);
    for(p=0;p<PT_MIDI_PORTS;++p){
        assert(!r->midi.ports[p].connected&&!r->midi.ports[p].needs_flush&&!r->midi.ports[p].sustain);
        for(t=0;t<16;++t)for(b=0;b<16;++b)assert(!r->midi.ports[p].pending_off[t][b]);
    }
}
static void close_all(struct rig *f)
{
    unsigned i,p,before;enum pt_camd_result r=PT_CAMD_MORE;
    for(i=0;i<20&&r!=PT_CAMD_FINISHED;++i){
        before=f->model.calls;r=pt_camd_control_close_step(&f->control);
        assert(r==PT_CAMD_MORE||r==PT_CAMD_FINISHED);assert(f->model.calls-before<=1);
    }
    assert(r==PT_CAMD_FINISHED&&pt_camd_control_state(&f->control)==PT_CAMD_CLOSED);
    assert(!f->model.opened&&!f->model.library.nodes&&!f->model.node.links&&!f->model.locked);
    for(p=0;p<16;++p)assert(!f->model.links[p].owner&&!f->control.links[p]);
    assert(!f->control.library&&!f->control.node&&!f->control.lock&&!f->control.context&&!f->control.context_bytes);
    quiet_registry(f);
}
static void activation_and_guards(void)
{
    struct rig *f=&rigs[0];struct pt_camd_ops bad=OPS;unsigned char before[sizeof(f->control)];unsigned calls;
    setup(f);assert(pt_camd_control_refresh(&f->control)==PT_CAMD_REFUSED&&!f->model.calls);
    memcpy(before,&f->control,sizeof(before));
    assert(!pt_camd_control_init(&f->control,&OPS,&f->model,sizeof(f->model))&&!memcmp(before,&f->control,sizeof(before)));
    assert(pt_camd_control_activate(&f->control)==PT_CAMD_OK);
    assert(f->control.library==&f->model.library&&f->control.node==&f->model.node);
    assert(f->model.node.name==f->control.node_name&&!strcmp(f->model.node.name,"ProTracker 2.4G"));
    calls=f->model.calls;assert(pt_camd_control_activate(&f->control)==PT_CAMD_REFUSED&&calls==f->model.calls);close_all(f);
    f=&rigs[1];memset(f,0,sizeof(*f));memcpy(before,&f->control,sizeof(before));
    bad.unlock=NULL;assert(!pt_camd_control_init(&f->control,&bad,&f->model,sizeof(f->model)));
    assert(!pt_camd_control_init(&f->control,&OPS,&f->control,sizeof(f->control)));
    assert(!pt_camd_control_init(&f->control,&OPS,(void *)(uintptr_t)(UINTPTR_MAX-1),4));
    assert(!memcmp(before,&f->control,sizeof(before))&&!f->model.calls);
}
static void receiver_snapshot(void)
{
    struct rig *f=&rigs[2];unsigned a,b;setup(f);clusters(f,3,5);
    f->model.clusters[1].senders=&f->model.external_senders[1];f->model.external_senders[1].owner=&f->model;
    assert(pt_camd_control_activate(&f->control)==PT_CAMD_OK&&pt_camd_control_refresh(&f->control)==PT_CAMD_OK);
    a=index_of(f,"Port 0");b=index_of(f,"Port 2");assert(a!=b);
    assert(pt_midi_endpoints_find(registry(f),"Port 1",7)==-1);
    assert(pt_camd_control_link(&f->control,a)==PT_CAMD_OK&&f->model.clusters[0].senders);
    f->model.clusters[0].receivers=NULL;
    assert(pt_camd_control_refresh(&f->control)==PT_CAMD_OK);
    assert(pt_midi_endpoints_available(registry(f),a)==0&&pt_midi_endpoints_available(registry(f),b)==1);
    assert(f->model.clusters[0].senders&&!f->model.clusters[0].receivers);
    close_all(f);assert(f->model.clusters[1].senders==&f->model.external_senders[1]);
}
static void snapshot_atomicity(void)
{
    struct rig *f=&rigs[3];unsigned char before[sizeof(f->control.endpoints)];unsigned calls;
    setup(f);clusters(f,1,1);assert(pt_camd_control_activate(&f->control)==PT_CAMD_OK&&pt_camd_control_refresh(&f->control)==PT_CAMD_OK);
    memcpy(before,registry(f),sizeof(before));
    f->model.clusters[0].name[0]=0;assert(pt_camd_control_refresh(&f->control)==PT_CAMD_INVALID);assert(!memcmp(before,registry(f),sizeof(before)));
    memset(f->model.clusters[0].name,'x',64);assert(pt_camd_control_refresh(&f->control)==PT_CAMD_INVALID);assert(!memcmp(before,registry(f),sizeof(before)));
    clusters(f,2,3);memcpy(f->model.clusters[1].name,f->model.clusters[0].name,64);
    assert(pt_camd_control_refresh(&f->control)==PT_CAMD_INVALID&&!memcmp(before,registry(f),sizeof(before)));
    clusters(f,1,1);f->model.clusters[0].next=&f->model.clusters[0];
    assert(pt_camd_control_refresh(&f->control)==PT_CAMD_INVALID&&!memcmp(before,registry(f),sizeof(before)));
    clusters(f,17,0x1ffff);calls=f->model.next_calls;
    assert(pt_camd_control_refresh(&f->control)==PT_CAMD_CAPACITY&&f->model.next_calls-calls==17&&!memcmp(before,registry(f),sizeof(before)));
    clusters(f,16,0xffff);assert(pt_camd_control_refresh(&f->control)==PT_CAMD_OK);memcpy(before,registry(f),sizeof(before));
    memcpy(f->model.clusters[0].name,"New port",9);
    assert(pt_camd_control_refresh(&f->control)==PT_CAMD_CAPACITY&&!memcmp(before,registry(f),sizeof(before)));
    f->model.fault=LOCK_NULL;assert(pt_camd_control_refresh(&f->control)==PT_CAMD_REFUSED&&!memcmp(before,registry(f),sizeof(before)));
    f->model.fault=NEXT_FAIL;assert(pt_camd_control_refresh(&f->control)==PT_CAMD_REFUSED&&!memcmp(before,registry(f),sizeof(before))&&!f->model.locked);
    f->model.fault=NONE;close_all(f);
}
static void acquisition_failures(void)
{
    struct rig *f=&rigs[4];unsigned a;setup(f);f->model.fault=OPEN_NULL;
    assert(pt_camd_control_activate(&f->control)==PT_CAMD_REFUSED&&pt_camd_control_state(&f->control)==PT_CAMD_EMPTY&&!f->model.opened);close_all(f);
    f=&rigs[5];setup(f);f->model.fault=NODE_NULL;
    assert(pt_camd_control_activate(&f->control)==PT_CAMD_REFUSED&&pt_camd_control_state(&f->control)==PT_CAMD_PARTIAL&&f->model.opened&&!f->model.library.nodes);
    assert(pt_camd_control_refresh(&f->control)==PT_CAMD_REFUSED&&pt_camd_control_link(&f->control,0)==PT_CAMD_REFUSED);close_all(f);
    f=&rigs[6];setup(f);clusters(f,1,1);assert(pt_camd_control_activate(&f->control)==PT_CAMD_OK&&pt_camd_control_refresh(&f->control)==PT_CAMD_OK);a=index_of(f,"Port 0");
    f->model.fault=ADD_NULL;assert(pt_camd_control_link(&f->control,a)==PT_CAMD_REFUSED&&!f->model.node.links&&f->model.library.nodes);
    assert(!strcmp(pt_midi_endpoints_name(registry(f),a)->identity,"Port 0"));close_all(f);
}
static void retirement_and_reuse(void)
{
    struct rig *f=&rigs[7];unsigned a,b,c;unsigned calls;setup(f);clusters(f,2,3);
    assert(pt_camd_control_activate(&f->control)==PT_CAMD_OK&&pt_camd_control_refresh(&f->control)==PT_CAMD_OK);a=index_of(f,"Port 0");b=index_of(f,"Port 1");
    assert(pt_camd_control_link(&f->control,a)==PT_CAMD_OK&&pt_camd_control_link(&f->control,b)==PT_CAMD_OK);calls=f->model.calls;
    assert(pt_camd_control_link(&f->control,a)==PT_CAMD_IN_USE&&f->model.calls==calls);
    assert(pt_camd_control_retire(&f->control,a)==PT_CAMD_OK&&pt_midi_endpoints_find(registry(f),"Port 0",7)==-1);
    memcpy(f->model.clusters[0].name,"Port C",7);assert(pt_camd_control_refresh(&f->control)==PT_CAMD_OK);c=index_of(f,"Port C");assert(c==a);
    assert(pt_camd_control_link(&f->control,c)==PT_CAMD_OK&&!strcmp(f->model.node.links->name,"Port C"));
    assert(pt_camd_control_link(&f->control,16)==PT_CAMD_INVALID&&pt_camd_control_retire(&f->control,16)==PT_CAMD_INVALID);close_all(f);
}
static void known_nonconsumption(void)
{
    struct rig *f=&rigs[8];unsigned a;unsigned char before[sizeof(f->control)];struct model_link *original;
    setup(f);clusters(f,1,1);assert(pt_camd_control_activate(&f->control)==PT_CAMD_OK&&pt_camd_control_refresh(&f->control)==PT_CAMD_OK);a=index_of(f,"Port 0");
    assert(pt_camd_control_link(&f->control,a)==PT_CAMD_OK);original=f->model.node.links;memcpy(before,&f->control,sizeof(before));
    f->model.fault=REMOVE_NO_EFFECT;assert(pt_camd_control_retire(&f->control,a)==PT_CAMD_REFUSED);
    assert(f->model.node.links==original&&original->owner==&f->model.node&&!memcmp(before,&f->control,sizeof(before)));
    f->model.fault=NONE;assert(pt_camd_control_retire(&f->control,a)==PT_CAMD_OK&&!original->owner);close_all(f);
}
static void held(struct rig *f)
{
    unsigned calls=f->model.calls;unsigned char before[sizeof(f->control)];
    assert(pt_camd_control_state(&f->control)==PT_CAMD_HELD);memcpy(before,&f->control,sizeof(before));
    assert(pt_camd_control_activate(&f->control)==PT_CAMD_HOLD&&pt_camd_control_refresh(&f->control)==PT_CAMD_HOLD);
    assert(pt_camd_control_link(&f->control,0)==PT_CAMD_HOLD&&pt_camd_control_retire(&f->control,0)==PT_CAMD_HOLD&&pt_camd_control_close_step(&f->control)==PT_CAMD_HOLD);
    assert(calls==f->model.calls&&!memcmp(before,&f->control,sizeof(before))&&f->control.context==&f->model);quiet_registry(f);
}
static void uncertain_consumption(void)
{
    struct rig *f=&rigs[9];unsigned a;unsigned char before[sizeof(f->control.endpoints)];
    setup(f);clusters(f,1,1);assert(pt_camd_control_activate(&f->control)==PT_CAMD_OK);memcpy(before,registry(f),sizeof(before));
    f->model.fault=UNLOCK_UNKNOWN;assert(pt_camd_control_refresh(&f->control)==PT_CAMD_HOLD&&!f->model.locked&&f->control.lock==&f->model.lock_token);
    assert(!memcmp(before,registry(f),sizeof(before)));held(f);
    f=&rigs[10];setup(f);clusters(f,1,1);assert(pt_camd_control_activate(&f->control)==PT_CAMD_OK&&pt_camd_control_refresh(&f->control)==PT_CAMD_OK);a=index_of(f,"Port 0");
    assert(pt_camd_control_link(&f->control,a)==PT_CAMD_OK);f->model.fault=REMOVE_UNKNOWN;
    assert(pt_camd_control_retire(&f->control,a)==PT_CAMD_HOLD&&!f->model.node.links&&f->control.links[a]);held(f);
    f=&rigs[11];setup(f);assert(pt_camd_control_activate(&f->control)==PT_CAMD_OK);f->model.fault=DELETE_UNKNOWN;
    assert(pt_camd_control_close_step(&f->control)==PT_CAMD_HOLD&&!f->model.library.nodes&&f->control.node);held(f);
    f=&rigs[12];setup(f);assert(pt_camd_control_activate(&f->control)==PT_CAMD_OK);
    assert(pt_camd_control_close_step(&f->control)==PT_CAMD_MORE&&!f->control.node);
    f->model.fault=CLOSE_UNKNOWN;assert(pt_camd_control_close_step(&f->control)==PT_CAMD_HOLD&&!f->model.opened&&f->control.library);held(f);
}
static void callback_reentry(void)
{
    struct rig *f=&rigs[13];unsigned a,b;setup(f);clusters(f,2,3);f->model.reenter=1;
    assert(pt_camd_control_activate(&f->control)==PT_CAMD_OK&&pt_camd_control_refresh(&f->control)==PT_CAMD_OK);a=index_of(f,"Port 0");b=index_of(f,"Port 1");
    assert(pt_camd_control_link(&f->control,a)==PT_CAMD_OK&&pt_camd_control_link(&f->control,b)==PT_CAMD_OK);
    assert(pt_camd_control_retire(&f->control,a)==PT_CAMD_OK);close_all(f);
    assert(f->model.reentry_checks==f->model.calls&&f->model.reentry_checks>=12);
}
int main(void)
{
    assert(PT_MIDI_PORTS==16&&PT_MIDI_ENDPOINT==64);
    activation_and_guards();receiver_snapshot();snapshot_atomicity();acquisition_failures();retirement_and_reuse();known_nonconsumption();uncertain_consumption();callback_reentry();
    puts("CAMD CONTROL HOST PASS:8 groups;copied16-cluster snapshots;actual model node/link custody;receiver-only availability;4 retained UNKNOWN contexts;NO ABI/DELIVERY/CONNECT/TIMING AUTHORITY");
    return 0;
}
