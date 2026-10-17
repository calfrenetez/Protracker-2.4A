#include "camd_control.h"
#include <string.h>
static int span(const void *p,size_t n){return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int apart(const void *p,size_t n,const void *q,size_t m)
{uintptr_t a=(uintptr_t)p,b=(uintptr_t)q;return span(p,n)&&span(q,m)&&(!n||!m||a>=b+m||b>=a+n);}
static int zero(const void *p,size_t n)
{const unsigned char *b=p;size_t i;for(i=0;i<n;++i)if(b[i])return 0;return 1;}
static int reject_send(void *c,const char *s,const uint8_t *b,size_t n)
{(void)c;(void)s;(void)b;(void)n;return 0;}
static int valid(const struct pt_camd_control *c){return c&&c->self==c;}
static enum pt_camd_result admission(const struct pt_camd_control *c)
{return !valid(c)?PT_CAMD_INVALID:c->busy?PT_CAMD_BUSY:c->state==PT_CAMD_HELD?PT_CAMD_HOLD:PT_CAMD_OK;}
static enum pt_camd_result leave(struct pt_camd_control *c,enum pt_camd_result r)
{c->busy=0;return r;}
static enum pt_camd_result hold(struct pt_camd_control *c)
{c->state=PT_CAMD_HELD;return leave(c,PT_CAMD_HOLD);}
static int used(const struct pt_camd_control *c,unsigned p)
{
    unsigned t,b;const struct pt_midi_port *port=&c->endpoints.midi.ports[p];
    if(port->sustain||port->needs_flush)return 1;
    for(t=0;t<16;++t)for(b=0;b<16;++b)if(port->pending_off[t][b])return 1;
    for(t=0;t<PT_MIDI_TRACKS;++t)if((c->endpoints.midi.voices[t].bound||c->endpoints.midi.voices[t].active)&&c->endpoints.midi.voices[t].endpoint==p)return 1;
    return 0;
}
int pt_camd_control_init(struct pt_camd_control *c,const struct pt_camd_ops *ops,void *context,size_t bytes)
{
    struct pt_midi_endpoint_sink sink;
    if(!c||!ops||!span(c,sizeof(*c))||!apart(c,sizeof(*c),ops,sizeof(*ops))||
       !ops->open||!ops->create_node||!ops->add_sender||!ops->lock||!ops->next_copy||!ops->unlock||!ops->remove_link||!ops->delete_node||!ops->close||
       (context?(!bytes||!span(context,bytes)):bytes!=0)||!apart(c,sizeof(*c),context,bytes)||!zero(c,sizeof(*c)))return 0;
    sink.context=NULL;sink.context_bytes=0;sink.send=reject_send;
    if(!pt_midi_endpoints_init(&c->endpoints,&sink))return 0;
    c->ops=*ops;c->context=context;c->context_bytes=bytes;
    memcpy(c->node_name,"ProTracker 2.4G",sizeof("ProTracker 2.4G"));c->self=c;return 1;
}
enum pt_camd_result pt_camd_control_activate(struct pt_camd_control *c)
{
    enum pt_camd_result a=admission(c);if(a!=PT_CAMD_OK)return a;
    if(c->state!=PT_CAMD_EMPTY)return PT_CAMD_REFUSED;
    c->busy=1;c->library=c->ops.open(c->context);
    if(!c->library)return leave(c,PT_CAMD_REFUSED);
    c->state=PT_CAMD_PARTIAL;c->node=c->ops.create_node(c->context,c->library,c->node_name);
    if(!c->node)return leave(c,PT_CAMD_REFUSED);
    c->state=PT_CAMD_READY;return leave(c,PT_CAMD_OK);
}
enum pt_camd_result pt_camd_control_refresh(struct pt_camd_control *c)
{
    struct pt_midi_endpoint_name all[PT_MIDI_PORTS],selected[PT_MIDI_PORTS];
    const void *seen[PT_MIDI_PORTS],*last=NULL;struct pt_camd_view v;
    unsigned count=0,n=0,i;enum pt_camd_next next;enum pt_camd_done done;
    enum pt_camd_result r=admission(c);if(r!=PT_CAMD_OK)return r;
    if(c->state!=PT_CAMD_READY)return PT_CAMD_REFUSED;
    c->busy=1;c->lock=c->ops.lock(c->context,c->library);
    if(!c->lock)return leave(c,PT_CAMD_REFUSED);
    for(;;){
        memset(&v,0xa5,sizeof(v));next=c->ops.next_copy(c->context,c->library,last,&v);
        if(next==PT_CAMD_END){r=PT_CAMD_OK;break;}
        if(next!=PT_CAMD_ITEM){r=PT_CAMD_REFUSED;break;}
        if(count==PT_MIDI_PORTS){r=PT_CAMD_CAPACITY;break;}
        if(!v.cursor||v.receivers>1||!v.name[0]||!memchr(v.name,0,sizeof(v.name))){r=PT_CAMD_INVALID;break;}
        for(i=0;i<count;++i)if(seen[i]==v.cursor||!strcmp(all[i].identity,v.name))break;
        if(i!=count){r=PT_CAMD_INVALID;break;}
        seen[count]=v.cursor;memset(&all[count],0,sizeof(all[count]));
        memcpy(all[count].identity,v.name,strlen(v.name)+1);
        if(v.receivers)selected[n++]=all[count];
        ++count;last=v.cursor;
    }
    done=c->ops.unlock(c->context,c->library,c->lock);
    if(done!=PT_CAMD_DONE)return hold(c);
    c->lock=NULL;
    if(r==PT_CAMD_OK){
        enum pt_midi_endpoint_result observed=pt_midi_endpoints_observe(&c->endpoints,n?selected:NULL,n);
        if(observed==PT_ENDPOINT_CAPACITY)r=PT_CAMD_CAPACITY;
        else if(observed!=PT_ENDPOINT_OK)r=PT_CAMD_INVALID;
    }
    return leave(c,r);
}
enum pt_camd_result pt_camd_control_link(struct pt_camd_control *c,unsigned p)
{
    const struct pt_midi_endpoint_name *name;enum pt_camd_result a=admission(c);
    if(a!=PT_CAMD_OK)return a;
    if(c->state!=PT_CAMD_READY)return PT_CAMD_REFUSED;
    if(p>=PT_MIDI_PORTS||!(name=pt_midi_endpoints_name(&c->endpoints,p)))return PT_CAMD_INVALID;
    if(c->links[p])return PT_CAMD_IN_USE;
    if(used(c,p))return PT_CAMD_IN_USE;
    c->link_names[p]=*name;c->busy=1;
    c->links[p]=c->ops.add_sender(c->context,c->library,c->node,c->link_names[p].identity);
    return leave(c,c->links[p]?PT_CAMD_OK:PT_CAMD_REFUSED);
}
enum pt_camd_result pt_camd_control_retire(struct pt_camd_control *c,unsigned p)
{
    enum pt_camd_done done;enum pt_camd_result a=admission(c);
    if(a!=PT_CAMD_OK)return a;
    if(c->state!=PT_CAMD_READY)return PT_CAMD_REFUSED;
    if(p>=PT_MIDI_PORTS||!pt_midi_endpoints_name(&c->endpoints,p))return PT_CAMD_INVALID;
    if(used(c,p))return PT_CAMD_IN_USE;
    c->busy=1;
    if(c->links[p]){
        done=c->ops.remove_link(c->context,c->library,c->links[p]);
        if(done==PT_CAMD_NO_EFFECT)return leave(c,PT_CAMD_REFUSED);
        if(done!=PT_CAMD_DONE)return hold(c);
        c->links[p]=NULL;
    }
    if(pt_midi_endpoints_forget(&c->endpoints,p)!=PT_ENDPOINT_OK)return hold(c);
    memset(&c->link_names[p],0,sizeof(c->link_names[p]));return leave(c,PT_CAMD_OK);
}
enum pt_camd_result pt_camd_control_close_step(struct pt_camd_control *c)
{
    unsigned p;enum pt_camd_done done;enum pt_camd_result a=admission(c);
    if(a!=PT_CAMD_OK)return a;
    if(c->state==PT_CAMD_CLOSED)return PT_CAMD_FINISHED;
    if(c->state!=PT_CAMD_CLOSING){
        for(p=0;p<PT_MIDI_PORTS;++p)if(used(c,p))return PT_CAMD_IN_USE;
        c->state=PT_CAMD_CLOSING;
    }
    c->busy=1;
    for(p=0;p<PT_MIDI_PORTS;++p)if(c->links[p]){
        done=c->ops.remove_link(c->context,c->library,c->links[p]);
        if(done==PT_CAMD_NO_EFFECT)return leave(c,PT_CAMD_REFUSED);
        if(done!=PT_CAMD_DONE)return hold(c);
        c->links[p]=NULL;return leave(c,PT_CAMD_MORE);
    }
    if(c->node){
        done=c->ops.delete_node(c->context,c->library,c->node);
        if(done==PT_CAMD_NO_EFFECT)return leave(c,PT_CAMD_REFUSED);
        if(done!=PT_CAMD_DONE)return hold(c);
        c->node=NULL;return leave(c,PT_CAMD_MORE);
    }
    if(c->library){
        done=c->ops.close(c->context,c->library);
        if(done==PT_CAMD_NO_EFFECT)return leave(c,PT_CAMD_REFUSED);
        if(done!=PT_CAMD_DONE)return hold(c);
        c->library=NULL;return leave(c,PT_CAMD_MORE);
    }
    c->context=NULL;c->context_bytes=0;memset(&c->ops,0,sizeof(c->ops));
    c->state=PT_CAMD_CLOSED;return leave(c,PT_CAMD_FINISHED);
}
int pt_camd_control_state(const struct pt_camd_control *c)
{return !valid(c)||c->busy?-1:(int)c->state;}
const struct pt_midi_endpoints *pt_camd_control_registry(const struct pt_camd_control *c)
{return !valid(c)||c->busy?NULL:&c->endpoints;}
