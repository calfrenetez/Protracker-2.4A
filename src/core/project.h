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
                         PT_PROJECT_CHECKSUM, PT_PROJECT_ALIAS,
                         PT_PROJECT_PENDING, PT_PROJECT_STALE };
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
/* Successful validation/size/encode output publication must be disjoint from
 * the project, tables, slices, extensions and full declared PCM capacities.
 * Encode's written scalar must also be disjoint from its actual encoded bytes.
 * ALIAS leaves outputs and source unchanged. Unrepresentable spans fail closed.
 * Existing validation remains synchronous; these guards scan metadata only and
 * allocate nothing. NULL validation caps retains the ordinary validation path. */
enum pt_project_result pt_project_validate(const struct pt_project *, uint32_t *);
/* Incremental task-side validation, not an IRQ/audio scheduling primitive.
 * Caller owns this small workspace; never modify its fields or use completion as
 * a general validation certificate. Storage is borrowed, never allocated/pinned.
 * Keep the project/tables/PCM/metadata alive and immutable through cancel or the
 * next begin. Every in-place edit must change revision or generation; an ordinary
 * valid channels.selected cursor change alone is permitted. Header/table/count
 * and tag changes refuse STALE before borrowed arrays are traversed. A resumed
 * sample descriptor is also checked before its PCM/slices are read.
 * Begin does finite metadata-only full-span preflight (<=255 samples and <=4090
 * extensions), no PCM values. Each step performs fixed header/current-sample
 * comparisons plus <=work items: one MIDI endpoint/order/sample descriptor/PCM
 * value/slice/event/extension descriptor. No spare PCM capacity is read.
 * Work items contain bounded fixed fields; this is not a wall-clock guarantee.
 * The workspace and published caps must be disjoint from the project, tables,
 * markers, extensions and full declared PCM capacity; wrapped spans fail closed.
 * Begin checks these publication spans before the incremental semantic body;
 * unrepresentable/missing positive declared storage therefore refuses ALIAS.
 * ALIAS/STALE/invalid work preserve caller workspace and published outputs.
 * Semantic validation failures are sticky until begin/cancel; partial caps stay
 * private. Existing pt_project_validate remains the synchronous oracle.
 * Begin OK means initialized only, never validated: get immediately returns
 * PENDING. Step/get OK means the complete current semantic pass has succeeded.
 */
#define PT_PROJECT_VALIDATION_WORK_MAX 4096U
struct pt_project_validation {
    const struct pt_project *project;
    struct pt_project snapshot;
    struct pt_sample sample;
    uint16_t slice_limits[PT_PROJECT_SAMPLES];
    size_t index, value, values;
    uint32_t revision, generation, capabilities, previous_slice;
    int32_t low, high;
    unsigned phase, sample_index, last_work;
    enum pt_project_result result;
};
enum pt_project_result pt_project_validation_begin(struct pt_project_validation *,
    const struct pt_project *, uint32_t revision, uint32_t generation);
enum pt_project_result pt_project_validation_step(struct pt_project_validation *,
    uint32_t revision, uint32_t generation, unsigned work);
/* NULL caps is a checked status/current query, with no transitive output-span
 * scan: PENDING while current unfinished, OK only complete, STALE on changed
 * identity. Non-NULL caps publishes only complete/current, otherwise untouched. */
enum pt_project_result pt_project_validation_get(const struct pt_project_validation *,
    uint32_t revision, uint32_t generation, uint32_t *caps);
/* Genuine caller-owned workspace only; reads no former source storage. NULL or
 * wrapped workspace spans are ignored without dereferencing them. */
void pt_project_validation_cancel(struct pt_project_validation *);

enum pt_project_result pt_project_size(const struct pt_project *, size_t *);
enum pt_project_result pt_project_encode(const struct pt_project *, uint8_t *, size_t, size_t *);
/* Bounded synchronous serialization, byte-identical to encode. Validates before
 * emitting; computes CRC in a first pass then emits in blocks <=1024 bytes.
 * Source must remain immutable/alive. Sink returns1 on complete consumption;
 * failure returns INVALID, may have emitted a prefix, leaves written unchanged.
 * Caller must stage/verify before publishing. No allocation. written uses the
 * same master/metadata/capacity guard and refuses ALIAS before sink callbacks.
 * The opaque sink/context must separately honor source immutability. */
typedef int (*pt_project_sink)(void *,const uint8_t *,size_t);
enum pt_project_result pt_project_stream(const struct pt_project *,pt_project_sink,void *,size_t *);
/* Stable, synchronous positional source; exact reads return 1. Preflight uses
 * at most 1092 bytes per callback and leaves requirements unchanged on failure.
 * This API validates input only; it does not load or allocate master samples. */
typedef int (*pt_project_read)(void *,size_t,uint8_t *,size_t);
enum pt_project_result pt_project_probe_reader(pt_project_read,void *,size_t,struct pt_project_requirements *);
/* Decode into unpublished staging only: read failure can modify storage, but
 * leaves the output descriptor unchanged. Source must remain stable/disjoint. */
enum pt_project_result pt_project_decode_reader(pt_project_read,void *,size_t,
    const struct pt_project_storage *,struct pt_project *);
enum pt_project_result pt_project_probe(const uint8_t *, size_t, struct pt_project_requirements *);
enum pt_project_result pt_project_decode(const uint8_t *, size_t,
                                         const struct pt_project_storage *, struct pt_project *);
#endif
