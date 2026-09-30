#ifndef PT_PAULA_INTERNAL_H
#define PT_PAULA_INTERNAL_H
#include "paula_dispatch.h"
/* Private exclusive-session entry points. Token must match song_owner exactly;
 * NULL is reserved for public direct calls when no song owns the controller. */
int pt_paula_stop_owned(struct pt_paula_voices *,unsigned,void *);
int pt_paula_close_owned(struct pt_paula_voices *,void *);
int pt_paula_dispatch_owned(struct pt_paula_voices *,uint64_t,unsigned,
    const struct pt_render_plan *,const struct pt_paula_render_caps *,struct pt_paula_batch *,void *);
int pt_paula_prepare_owned(struct pt_paula_prepared *,struct pt_paula_voices *,uint64_t,unsigned,
    const struct pt_render_plan *,const struct pt_paula_render_caps *,void *);
#endif
