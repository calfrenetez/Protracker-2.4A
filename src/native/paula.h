#ifndef PT_NATIVE_PAULA_H
#define PT_NATIVE_PAULA_H
#include "project.h"
#include "playback.h"
struct IOAudio;
struct MsgPort;
struct pt_paula {
    struct MsgPort *port;
    struct IOAudio *audio,*lock;
    uint8_t *data,*staging;
    size_t bytes,pattern_bytes;
    unsigned opened,locked,started,mode,pattern;
};
/* Zero-initialize once. 0=song, 1=selected pattern. No channel stealing. */
const char *pt_paula_play(struct pt_paula *,const struct pt_project *,unsigned mode,unsigned position,unsigned pattern);
const char *pt_paula_audition(struct pt_paula *,const struct pt_project *,unsigned sample,unsigned period);
void pt_paula_stop(struct pt_paula *);
const char *pt_paula_sync(struct pt_paula *,const struct pt_project *);
void pt_paula_poll(struct pt_paula *,struct pt_playback *);
#endif
