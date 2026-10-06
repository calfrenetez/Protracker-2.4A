# Exact paired plan geometry

`mixed_readers_plan_normalize` translates a complete renderer boundary into the
numeric geometry supported by the paired sampler factory. It uses caller-owned
Fast workspace and performs no allocation, master promotion, cache preparation,
upload, enqueue or activation. Its READY result describes geometry; the genuine
factory must still obtain current master ownership and original ACTIVE keys.

The input holds all 64 renderer action slots and 16 logical origins. The result
has at most 16 records in global track order. A track may contain one TRIGGER,
CONTROL or STOP, or a TRIGGER followed by one final CONTROL whose voice fields
match except for step. The folded trigger uses the final step and gains. Other
duplicates, SEGMENT, REPEAT and unsupported routes refuse the entire boundary.
No action is dropped, split or moved in musical time.

Paula uses the factory's fresh four-slot map. AmiGUS slots follow global track
order, including four Paula plus twelve card tracks or sixteen card tracks.
TRIGGER source identity is resolved before a foreign PCM pointer can be read.
CONTROL and STOP require a logical predecessor at an earlier original frame.
These origins describe the timeline and never substitute for runtime ACTIVE
proof. An empty boundary creates no command; completion does not imply STOP.

Paula accepts only the factory's whole, even mono sample geometry: 2–131070
frames, offset zero, no loop or interpolation. Period and volume come from the
canonical converter. An even partial range that the converter can describe still
refuses because the current factory emits the entire cache.

AmiGUS compares all seven semantic register fields from the canonical renderer
and existing factory converter at numeric origin zero. That origin is a
comparison basis, not a card allocation. The adapter preserves the exact
quantized rate through a checked rational request. It searches the finite legacy
volume/pan domain for exact left/right register equality, charging one candidate
per work item. If no pair matches, it refuses. In particular, the renderer's
default centred mono gains cannot be represented exactly by the current legacy
trigger volume/pan interface. A separate explicit quantized-level extension is
being developed; this adapter does not approximate or install a fallback.

Calls are serialized. Each step charges at most 256 actions, origins, source
items, conversions or gain candidates. Complete input, PCM spare capacity,
slice, extension, context and workspace extents remain guarded separately from
that work bound. Fixed source tags and headers are checked before former source
tables. Results are published only as a complete successful batch. Cancellation
clears used workspace and consumes the exact original owner slot without walking
expired source tables or releasing external pins. Extra workspace capacity stays
unchanged.

The corrected v4 host ASan/UBSan fixture passes all six complete groups: 941 saved
inputs, 38 translation units, compiler and product RC0 with empty stderr and
positive reaping/group quiescence. Independent saved-outcome review also passes.
The first v3 attempt remains a separate failure: successful compilation,
then UBSan during a misaligned typed alias-pointer snapshot in the fixture. V4
changes only that fixture's byte access and alignment expectations, retaining
aligned overlap coverage. Production code and earlier evidence stay unchanged.
The source-only Paula oracle and original owner-slot findings were repaired
before the first execution. No normalizer native, Amiberry or physical execution
is claimed here.

The [saved host projection](../evidence/enhanced-editor/mixed-readers-plan-normalize-host/README.md)
preserves the complete current output and original failure diagnostics separately.
Its offline verifier checks that selected packet; complete private source, tool
and process custody remain separate.

This is one component of future whole-song playback. A complete late-row audit,
genuine retained masters, transfer of the SAME audited sequence, a composite
editor barrier and a producer that commits only admitted work remain separate
implementation gates. Native stack, memory placement, real scheduled output,
IRQ timing, card completion/ordering/voice stop, audio and listening require
their own evidence. The existing paired controller's tests do not qualify this
new adapter.
