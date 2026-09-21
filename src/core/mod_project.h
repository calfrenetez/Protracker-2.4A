#ifndef PT_MOD_PROJECT_H
#define PT_MOD_PROJECT_H
#include "project.h"
#define PT_CLASSIC_RATE 8287UL /* Nominal PAL rate at period 428; finetune is separate. */
#define PT_CLASSIC_HEADER_TAG 0x434d4f44UL /* CMOD: original 1084-byte header */
enum pt_mod_export_issue {
    PT_EXPORT_CHANNELS=1, PT_EXPORT_ROUTING=2, PT_EXPORT_PRECISION=4,
    PT_EXPORT_STEREO=8, PT_EXPORT_RATE=16, PT_EXPORT_SLICES=32,
    PT_EXPORT_LOOPS=64, PT_EXPORT_MIDI_AUDIO=128, PT_EXPORT_OFF=256,
    PT_EXPORT_LIMITS=512, PT_EXPORT_PANNING=1024, PT_EXPORT_METADATA=2048,
    PT_EXPORT_VELOCITY=4096, PT_EXPORT_NOTES=8192, PT_EXPORT_TEMPO=16384
};
enum pt_conversion_class { PT_CONVERSION_LOSSLESS, PT_CONVERSION_CONVERTED,
                           PT_CONVERSION_BOUNCED, PT_CONVERSION_INCOMPLETE };
struct pt_mod_export_report {
    uint32_t issues;
    enum pt_conversion_class classification;
    size_t bytes; /* Nonzero only when direct export is representable. */
};
/* Strict import currently refuses MOD preflight warnings rather than silently
 * normalising them. Storage uses the same separate staging contract as PTG. */
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
#endif
