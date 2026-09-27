#include "amigus_interrupt_owner.h"
#include <string.h>
enum pt_amigus_interrupt_result pt_amigus_interrupt_owner_begin(
    struct pt_amigus_interrupt_owner *o,struct pt_amigus_reservation *r,
    const struct pt_amigus_interrupt_api *api,void *binding)
{
    if(!o || o->reservation || !r || !r->opened || !r->reserved || !r->access ||
       r->interrupt || !api || !api->install || !api->remove || !api->quiesce ||
       !binding)return PT_AMIGUS_INTERRUPT_INVALID;
    memset(o,0,sizeof(*o));o->api=*api;o->reservation=r;o->binding=binding;
    r->interrupt=1; /* Publish ownership BEFORE a potentially partial install. */
    o->install_code=o->api.install(o->api.context,r,binding);
    return o->install_code?PT_AMIGUS_INTERRUPT_FAILED:PT_AMIGUS_INTERRUPT_READY;
}
int pt_amigus_interrupt_owner_stop(struct pt_amigus_interrupt_owner *o)
{
    if(!o || !o->reservation)return 0;
    if(o->done)return 1;
    if(!o->remove_requested) {
        o->remove_requested=1;
        o->api.remove(o->api.context,o->reservation);
    }
    if(o->api.quiesce(o->api.context,o->reservation,o->binding)!=1)return 0;
    o->reservation->interrupt=0;o->done=1;
    return 1;
}
int pt_amigus_interrupt_owner_detach(struct pt_amigus_interrupt_owner *o)
{
    if(!o || (o->reservation && !o->done))return 0;
    memset(o,0,sizeof(*o));return 1;
}
