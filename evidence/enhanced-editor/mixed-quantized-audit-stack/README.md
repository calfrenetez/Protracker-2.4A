# Strict quantized audit: local compiler stack metadata

The separate first object-only stack attempt passed at committed admission
`d1d3844d1f736e2bcdc53eeae5aad47f30f34818`. It copied the same saved 955-input
HOST/compiler source pool and compiled the same 39 ordered units with the original
assertion-enabled portable flags plus `-fstack-usage -c`. No linking or executable
creation occurred.

Independent saved-only review confirms 39 objects, 39 `.su` files and all 604
reported local rows: 117 `static`, 487 `dynamic,bounded`. The actual raw `-M` union
has 124 dependencies, comprising 92 source and 32 selected SDK files. All 1,275
retained inner members, 108 owned drivers (90 compiler, eight inner Git, ten root)
and 216 raw streams match their recorded pins. Drivers returned zero with empty
stderr, were reaped and left their owned groups quiet. Current 1,171-source map,
47 controls, protected 16 paths and unchanged saved admission/index/status agree;
staging remained empty. Recorded inner/root times are 12.699/13.813 seconds.

Selected local rows, retaining compiler-generated names, are:

| Compiler row | Bytes | Qualifier |
| --- | ---: | --- |
| `pt_pcm_resample_filtered_progress.part.0` | 16,572 | `dynamic,bounded` |
| `pt_render_stream` | 5,536 | `dynamic,bounded` |
| `pt_mixed_quantized_audit_step.part.12` | 4,720 | `dynamic,bounded` |
| `pt_mixed_quantized_audit_close.part.17` | 4,684 | `dynamic,bounded` |
| `qa_same_sequence` | 4,572 | `dynamic,bounded` |

The largest row is associated with the filtered resampler's automatic
`int32_t weights[4098]`. Its presence in the compiled pool does not prove that the
strict fixture executes it. All source associations are annotations to original
compiler rows; no row is rebased or combined.

These are local compiler measurements. They supply no aggregate call-chain,
callback, library/assembly, IRQ or OS stack bound and qualify no Amiga `Stack`
setting. No HUNK/object equivalence is asserted. The prior 232,864-byte linked
HUNK remains unchanged and NEVER_EXECUTED. Native assertions, Amiberry/A1200,
Fast/Chip placement, launcher/device/timing/audio and listening remain unqualified.
The original SOURCE and HOST failures, including V3's unknown return/tuple, and
separate HOST/compiler successes remain preserved. This packet is metadata only;
full raw evidence remains in the separately pinned saved attempt.
