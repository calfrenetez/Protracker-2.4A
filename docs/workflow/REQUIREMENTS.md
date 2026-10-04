# Requirement / implementation / test map

The supplied addendum is authoritative. The complete original package, including
its36-case acceptance specification and unexecuted planning matrix, is preserved
in [package-v1](package-v1/README_FIRST.md). The production/test map below remains
implementation coverage, not a claim that mandatory native acceptance has passed.

| Requirement | Production seam | Focused tests |
|---|---|---|
| ZT1 stable IDs, frames/formats/names/loop/usage, silent selection, labelled editor/audition | editor/workflow + manager view; existing sampler/audition | editor_workflow_test; planar editor regression; native controller candidate |
| ZT1 stored references including instrument-only/off-order/muted/track16 | core/sample_usage begin/step; fixed preview | sample_usage_test |
| ZT1 unknown/reserved/protected ownership, strict free-slot eligibility | sample_usage options/flags + canonical predicate | sample_usage_test; sampler_workflow_test |
| ZT1 version-tagged initially-empty selected cleanup, stale/cancel refusal | usage preview + workflow cleanup job + editor service | sample_usage_test; sampler_workflow_test; editor_workflow_test |
| ZT1 stopped/capture/preview gate, explicit Stop+Apply | workflow activity predicate; existing capture attachment and confirmed barrier; native action | editor_workflow_test; editor_capture regression; actual native coexistence gate open |
| ZT1 one atomic undo, no renumber/history purge, truthful retained bytes | sampler workflow resource apply/discard; shared pattern history | sampler_workflow_test; editor_workflow_test |
| ZT2 half-open validated loop/no-loop/corrupt/end boundary and viewport | editor/sample_range + shared loop adapter | sample_range_test; editor_workflow_test |
| ZT2 actual current range, strict free slot, exact six formats, contained loop/markers, independent PCM | sampler copy begin/step/commit + common usage | sampler_workflow_test; copy boundary test; editor_workflow_test |
| ZT2 preallocation/cancel/stale/budget/single undo, source/other slots preserved | sampler immutable versions + existing journal | sampler_workflow_test; sampler/slots/pin/cache regressions |
| ZT2 no repeated long PCM redraw, bounded summary work/cancel/version/view tags | editor/wave_summary; owner-task idle and view bins | wave_summary_test; editor_workflow_test; native latency/memory gate open |
| ZT3 explicit/empty/instrument-only/portamento identity, zero-instrument inheritance/controlflow | core/event_resource using actual flow/pitch | event_resource_test |
| ZT3 ambiguous/detached/unreachable/budget/no fallback and invalidation | structured resolver result + revision/generation/header tags | event_resource_test; editor_workflow_test |
| ZT3 silent select/open, MIDI route-only, captured event return/input consumption | workflow/controller hooks; existing sampler/channel pages | editor_workflow_test; actual native MIDI/input gate open |
| Shared storage/low-bit integrity, source immutability/retained leases/no backend uploads | sampler versions/bounded allocator; task-side-only helpers | sampler_workflow_test; existing sampler Paula/wavetable tests |
| Existing architecture/layout/import/export/baseline preserved | no baseline/vendor changes; unchanged format/routing engine | pre/post seven sanitizer groups, planar golden, native crossbuilds; real musical/coexistence gate open |

Native core/controller fixtures run in ordinary memory and cannot prove remote
input, GUI responsiveness, audio output, IRQ deadline compliance, device sample
RAM capacity/order/completion or listening acceptance. Full editor input/visual
exercises and real A1200 coexistence are separate mandatory gates. Host allocations
and step counters are not substituted for real 030 CPU/Chip/Fast measurements.
