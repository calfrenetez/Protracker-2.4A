#ifndef PT_MIDI_H
#define PT_MIDI_H
#include <stddef.h>
#include <stdint.h>
#define PT_MIDI_TRACKS 16
#define PT_MIDI_PORTS 16
enum pt_midi_result {PT_MIDI_OK,PT_MIDI_INVALID,PT_MIDI_UNAVAILABLE,PT_MIDI_SEND_FAILED};
/* send must atomically accept the whole 1-3 byte message, or return zero with
   none accepted. Endpoint indexes are stable identities owned by the adapter.
   Calls are serialized with replay; this module never blocks or allocates. */
struct pt_midi_sink {void *context;int (*send)(void *,unsigned,const uint8_t *,size_t);};
struct pt_midi_voice {uint8_t bound,endpoint,channel,active,note;};
struct pt_midi_port {
    uint8_t connected,needs_flush;
    uint16_t sustain; /* channels where this instance sent sustain on */
    uint8_t pending_off[16][16];
};
struct pt_midi {
    struct pt_midi_sink sink;
    struct pt_midi_voice voices[PT_MIDI_TRACKS];
    struct pt_midi_port ports[PT_MIDI_PORTS];
};
int pt_midi_init(struct pt_midi *,const struct pt_midi_sink *);
/* Disconnect remembers owned note releases. Reconnect flushes those releases
   before new notes. Never reuse an endpoint index for a different destination
   while it has pending releases; enumerate stable endpoint identities. */
enum pt_midi_result pt_midi_connection(struct pt_midi *,unsigned,int);
/* One active note per tracker track. MIDI channel is 1..16. Binding a new route
   releases the previous route first. A failed release leaves binding intact. */
enum pt_midi_result pt_midi_bind(struct pt_midi *,unsigned,unsigned,unsigned);
enum pt_midi_result pt_midi_unbind(struct pt_midi *,unsigned);
enum pt_midi_result pt_midi_note_on(struct pt_midi *,unsigned,unsigned,unsigned);
enum pt_midi_result pt_midi_note_off(struct pt_midi *,unsigned);
enum pt_midi_result pt_midi_stop(struct pt_midi *);
enum pt_midi_result pt_midi_controller(struct pt_midi *,unsigned,unsigned,unsigned);
enum pt_midi_result pt_midi_program(struct pt_midi *,unsigned,unsigned);
enum pt_midi_result pt_midi_bend(struct pt_midi *,unsigned,unsigned);
/* Optional transport bytes only; the shared sequencer must schedule 24 PPQN. */
enum pt_midi_result pt_midi_realtime(struct pt_midi *,unsigned,unsigned);
#endif
