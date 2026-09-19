#ifndef PT_PROJECT_H
#define PT_PROJECT_H
#include "channels.h"
#include "pcm.h"

#define PT_PROJECT_ROWS 64
#define PT_PROJECT_PATTERNS 256
#define PT_PROJECT_ORDERS 256
#define PT_PROJECT_SAMPLES 255
#define PT_PROJECT_SLICES 4096
#define PT_PROJECT_NAME 32
#define PT_MIDI_ENDPOINT 64

enum pt_note_kind { PT_NOTE_NONE, PT_NOTE_PERIOD, PT_NOTE_MIDI, PT_NOTE_OFF };
enum pt_loop_kind { PT_LOOP_NONE, PT_LOOP_FORWARD, PT_LOOP_PINGPONG, PT_LOOP_CROSSFADE };
enum pt_project_mode { PT_MODE_WAVETABLE, PT_MODE_STUDIO };
enum pt_project_capability {
    PT_CAP_CHANNELS = 1, PT_CAP_AMIGUS = 2, PT_CAP_MIDI = 4,
    PT_CAP_16BIT = 8, PT_CAP_24BIT = 16, PT_CAP_STEREO = 32,
    PT_CAP_SLICES = 64, PT_CAP_PINGPONG = 128, PT_CAP_CROSSFADE = 256,
    PT_CAP_OFF = 512, PT_CAP_STUDIO = 1024, PT_CAP_VELOCITY = 2048,
    PT_CAP_MIDI_NOTE = 4096
};
#define PT_CAP_KNOWN 8191UL

/* Exactly one classic effect column. PERIOD retains the raw MOD period;
 * MIDI uses note 0..127. OFF is independent of classic ECx note cut.
 * flags bit 0 means velocity is present. slice is 1-based, 0 means none. */
struct pt_event {
    uint16_t pitch, slice;
    uint8_t kind, instrument, effect, parameter, velocity, flags;
};
struct pt_sample {
    char name[PT_PROJECT_NAME];
    struct pt_pcm pcm;
    uint32_t loop_start, loop_end, crossfade;
    uint32_t *slices;
    uint16_t slice_count;
    uint8_t loop, volume, interpolation;
    int8_t finetune;
};
struct pt_extension {
    uint32_t id, length;
    uint16_t version;
    const uint8_t *data;
};
struct pt_project {
    char title[PT_PROJECT_NAME];
    struct pt_channels channels;
    uint16_t order_count, pattern_count, sample_count, bpm;
    uint8_t speed, mode;
    uint16_t *orders;
    struct pt_event *events; /* pattern, row, channel order */
    struct pt_sample *samples;
    char midi_input[PT_MIDI_ENDPOINT];
    char midi_output[PT_CHANNEL_LIMIT][PT_MIDI_ENDPOINT];
    uint32_t midi_flags; /* bit 0 send clock, bit 1 receive clock */
    struct pt_extension *extensions; /* preserved unknown optional chunks */
    uint16_t extension_count;
};
enum pt_project_result { PT_PROJECT_OK, PT_PROJECT_INVALID, PT_PROJECT_TRUNCATED,
                         PT_PROJECT_UNSUPPORTED, PT_PROJECT_CAPACITY,
                         PT_PROJECT_CHECKSUM, PT_PROJECT_ALIAS };
struct pt_project_requirements {
    size_t events, pcm_values, slices, extension_bytes;
    uint16_t orders, samples, extensions;
    uint32_t capabilities;
};
struct pt_project_storage {
    uint16_t *orders; size_t order_capacity;
    struct pt_event *events; size_t event_capacity;
    struct pt_sample *samples; size_t sample_capacity;
    int32_t *pcm; size_t pcm_capacity;
    uint32_t *slices; size_t slice_capacity;
    struct pt_extension *extensions; size_t extension_capacity;
    uint8_t *extension_data; size_t extension_bytes;
};

/* No allocation. Decode validates first, then writes only to disjoint staging
 * storage; failure leaves output and storage untouched. All capacities count
 * elements except extension_bytes. The decoded project owns no input pointers.
 * Commit the returned project only after the caller's file/UI transaction ends.
 */
int pt_project_event_valid(const struct pt_project *,const struct pt_event *);
enum pt_project_result pt_project_validate(const struct pt_project *, uint32_t *);
enum pt_project_result pt_project_size(const struct pt_project *, size_t *);
enum pt_project_result pt_project_encode(const struct pt_project *, uint8_t *, size_t, size_t *);
enum pt_project_result pt_project_probe(const uint8_t *, size_t, struct pt_project_requirements *);
enum pt_project_result pt_project_decode(const uint8_t *, size_t,
                                         const struct pt_project_storage *, struct pt_project *);
#endif
