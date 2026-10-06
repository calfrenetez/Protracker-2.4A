# Explicit quantized AmiGUS trigger levels

`amigus_trigger_levels` prepares a numeric seven-field AmiGUS voice plan with
explicit `uint16_t` left and right register levels. It is an additive pure helper;
the existing `pt_amigus_voice_request` and `pt_amigus_voice_plan_prepare` retain
their layout and volume/pan behavior.

The new request contains the existing geometry request plus exact left/right
levels. Both inactive geometry `volume` and `pan` must be zero. Every register
value is meaningful, including silence, asymmetry and independent maxima. The
helper applies no default, clamp, gain conversion or approximation. It delegates
the five non-level fields, cache geometry, loop/interpolation, rational rate and
numeric address bounds to the unchanged legacy helper, then publishes the
complete seven-field result once.

The caller supplies readable ordinary sample, format, request and output
descriptors. Complete declared PCM capacity, including spare values, and slice
storage must have valid aligned, overflow-safe, disjoint extents. Positive frames
must fit the declared PCM capacity. The helper guards those spans and its local
legacy request/plan before scratch initialization. A refusal preserves the full
external output bytes. It reads metadata but does not scan PCM or slice values,
allocate, call a backend or access a device. Unknown enclosing caller contexts
remain the caller's obligation. These are numeric bounds, not evidence of
physical sample-RAM capacity, resource ownership or voice-stop semantics.

The canonical mono renderer's centred levels are `32639/32895`; an explicit
request preserves both levels and all five geometry fields exactly. The existing
legacy `volume=64, pan=128` result remains `32768/32768`. The new helper does not
change the normalizer's legacy-domain search or install a fallback. It is not yet
connected to the genuine sampler factory, controller or normalizer. The separate
tagged factory extension remains a private source draft.

The distinct corrected host attempt `first-v3` passes its ASan/UBSan fixture on
the frozen D1 v3 sources: 940 committed source files plus four private overlays
and one same-commit generated font form 945 saved inputs; 17 translation units
produce six complete software lines, 912 bytes, with RC0 and empty stderr. This
qualification preceded the later exact four-path source adoption; no additional
test or target action accompanies that copy. The
record contains 16 preparation, two compiler/runtime and eight enclosing root
wrapper calls, all with positive driver reaping and owned-group quiescence.
Compilation took about 3.965 seconds and the product about 0.557 seconds in this
single host observation. These are host measurements, not musical timing bounds.
Independent saved-host review passes without findings. It checks saved past
driver records and byte custody, without replaying the qualifier or querying
current process liveness. Pinned compiler/Git wrappers do not bind the complete
compiler/linker/SDK/sanitizer runtime closure.

The first actual `first-v2` host attempt remains a separate compile failure:
fixture-local `REFUSE` redefined the included unchanged legacy macro under
`-Werror`; compiler RC1, 405 diagnostic bytes, no product or runtime. V3 changes
only fixture-local `RESET/REFUSE` names to `D1_RESET/D1_REFUSE`, retaining helper
H/C, qualifier, assertions and the exact expected stream. Nothing retries or
relabels that first failure. The six host lines cover legacy parity across 16705
volume/pan pairs, 144 source/cache/endian/channel/loop/interpolation geometries,
the real canonical mono renderer, all 65536 individual register values,
overflow/rate/relocation cases, complete aliases and exact refusal preservation.

The qualifier accepts observed completion only after required raw output and
custody reads. Its whole budget is 600 seconds, with 30-second Git reads,
120-second compilation, 180-second product execution and a five-second owned
cleanup reserve. One whole-plan assignment provides logical refusal atomicity,
not CPU/thread/device atomicity. Synchronous filesystem work is not preempted;
neither sampled completion nor successful process cleanup establishes hard
real-time behavior.
No timeout, signal-escalation or uncertain-cleanup failure path was exercised.

The [selected saved host packet](../evidence/enhanced-editor/amigus-trigger-levels-host/README.md)
contains the complete current product stream and original compile diagnostics.
Its pure offline verifier checks that selected projection only. Complete private
source, tools, archives, process logs, host product and custody remain separate.
This D1 result supplies no genuine master/cache owner, enqueue, activation,
whole-song production, native build/execution, Amiberry, physical memory,
completion/ordering/stop, exact live timing, audio or listening acceptance.
