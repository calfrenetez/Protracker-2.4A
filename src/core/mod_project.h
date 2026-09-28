#ifndef PT_MOD_PROJECT_H
#define PT_MOD_PROJECT_H
#include "project.h"
#include "mod_inspect.h"
#define PT_CLASSIC_RATE 8287UL /* Nominal PAL rate at period 428; finetune is separate. */
#define PT_CLASSIC_HEADER_TAG 0x434d4f44UL /* CMOD: original 1084-byte header */
enum pt_mod_export_issue {
    PT_EXPORT_CHANNELS=1, PT_EXPORT_ROUTING=2, PT_EXPORT_PRECISION=4,
    PT_EXPORT_STEREO=8, PT_EXPORT_RATE=16, PT_EXPORT_SLICES=32,
    PT_EXPORT_LOOPS=64, PT_EXPORT_MIDI_AUDIO=128, PT_EXPORT_OFF=256,
    PT_EXPORT_LIMITS=512, PT_EXPORT_PANNING=1024, PT_EXPORT_METADATA=2048,
    PT_EXPORT_VELOCITY=4096, PT_EXPORT_NOTES=8192, PT_EXPORT_TEMPO=16384,
    PT_EXPORT_PADDING=32768 /* Private playback only; strict export reports LIMITS. */
};
enum pt_conversion_class { PT_CONVERSION_LOSSLESS, PT_CONVERSION_CONVERTED,
                           PT_CONVERSION_BOUNCED, PT_CONVERSION_INCOMPLETE };
struct pt_mod_export_report {
    uint32_t issues;
    enum pt_conversion_class classification;
    size_t bytes; /* Nonzero only when representable under the selected policy. */
};
/* Strict import currently refuses MOD preflight warnings rather than silently
 * normalising them. Storage uses the same separate staging contract as PTG. */
/* Reader decoding targets unpublished disjoint staging: read failure may write
 * a prefix. Callback/source must remain stable throughout probe and decode. */
enum pt_project_result pt_mod_project_probe_reader(pt_mod_read,void *,size_t,struct pt_project_requirements *);
enum pt_project_result pt_mod_project_decode_reader(pt_mod_read,void *,size_t,const struct pt_project_storage *,struct pt_project *);
enum pt_project_result pt_mod_project_probe(const uint8_t *,size_t,struct pt_project_requirements *);
enum pt_project_result pt_mod_project_decode(const uint8_t *,size_t,
                                             const struct pt_project_storage *,struct pt_project *);
enum pt_project_result pt_mod_export_analyse(const struct pt_project *,struct pt_mod_export_report *);
/* Direct, lossless path only. Any reported issue leaves output untouched and
 * returns UNSUPPORTED. Future transformations require explicit caller policy. */
enum pt_project_result pt_mod_export_direct(const struct pt_project *,uint8_t *,size_t,size_t *);
/* Explicit precision-only conversion: signed 16/24-bit mono PCM is rounded to
 * nearest 8-bit, ties away from zero, then saturated. No dither/resampling.
 * All other classic constraints still apply. Source PCM is never modified.
 * The policy analysis retains issue bits but supplies bytes when precision is
 * the only issue. Classification stays CONVERTED whenever precision changes.
 * Export preflight/alias/capacity failure preserves output and written. */
enum pt_project_result pt_mod_export_analyse_round8(const struct pt_project *,struct pt_mod_export_report *);
enum pt_project_result pt_mod_export_round8(const struct pt_project *,uint8_t *,size_t,size_t *);
/* Optional deterministic TPDF dither before precision reduction. Difference of
 * two uniform draws spans just under +/- one output LSB; fixed xorshift32 seed
 * 0x243f6a88 per export, in sample/frame order. Only >8-bit samples draw noise.
 * Same analysis, refusal and immutable-source contract as round8. Existing
 * 8-bit samples stay byte-identical. No noise shaping or resampling. */
enum pt_project_result pt_mod_export_tpdf8(const struct pt_project *,uint8_t *,size_t,size_t *);
/* Bounded synchronous immutable-source export. Policy0=direct,1=round8,
 * 2=fixed TPDF; no other conversion is implied. Validates before any sink call.
 * Sink consumes each block completely and returns1; failure may emit a prefix.
 * No allocation, blocks <=1084 bytes. Caller stages/verifies before publication. */
enum pt_project_result pt_mod_export_stream(const struct pt_project *,unsigned,pt_project_sink,void *);
/* Private Paula playback representation: round8 plus one silent padding byte
 * after each odd-length master. Header lengths include padding; loop endpoints
 * retain strict even-word limits. Never changes source frames/PCM or save/export
 * policy. Analysis retains PRECISION/PADDING issues and CONVERTED classification.
 * Same bounded stream and preflight/alias/capacity guarantees as export above. */
enum pt_project_result pt_mod_playback_analyse(const struct pt_project *,struct pt_mod_export_report *);
enum pt_project_result pt_mod_playback_encode(const struct pt_project *,uint8_t *,size_t,size_t *);
enum pt_project_result pt_mod_playback_stream(const struct pt_project *,pt_project_sink,void *);
#endif
