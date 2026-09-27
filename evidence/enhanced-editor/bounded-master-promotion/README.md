# Bounded master promotion — 27 September 2026

Wavetable preparation now reserves one selected master version or copies at most
4096 PCM/marker bytes per call, only after full validation/capability success.
The internal promotion job charges the complete allocation against the sampler
budget immediately. No partial descriptor or PCM becomes visible in the project;
publication and the sampler-owned reference occur only after the complete copy.
The job reference transfers to the caller pin. Existing immutable versions reuse
storage without allocation. Cancellation discards unpublished bytes; successful
masters/pins keep their normal independent lifetimes and original precision.

Per-step guards check generation, table/count/current-version and exact source
descriptor identity without scanning values. The caller has already validated the
project and must serialize/cancel before any borrowed source edits or destruction.
The job is not an untrusted input validator. Public sampler_pin retains its full
validation and synchronous behavior. Allocation is synchronous;4KiB is a copy-work
bound, not a guaranteed duration or allocator latency.

Coverage:
- True8/16/24-bit stereo masters with1100 markers; partial-word and PCM/marker
  boundary copies; no partial publication; exact enhanced-project save bytes.
- Budget refusal, allocator failure through song open, cancellation before copy,
  during PCM and marker copies; stale generation/table/metadata poison and cleanup.
- Existing-version reuse without allocation and pins surviving sampler release.
- Large2100-frame24-bit song preparation takes three copy calls; cancellation and
  stale failure discard partial data, completion preserves exact master, no output.
- Existing sampler edits/imports/undo, Studio pins, song/range/stop and editor guards.

Validation:
- Final standalone promotion ASan/UBSan PASS3.221s.
- Sampler, Studio pin and instrumented wavetable dispatch ASan/UBSan PASS13.615s
  before the final save-byte assertions; those assertions pass in the final
  standalone and staged integrated suites.
- Final staged editor wavetable/legacy guard/Studio lifetime suite PASS26.843s.
- Native build/main syntax PASS;154 staged input hashes/font verified.
  Binary310260bytes SHA256
  483e0795fa5f0a35b9ce9fa770076cfeaa377f938d1c71d4688cba81dbf3abfb.
- Fresh AmiConnect coordination/current idle other-task snapshots and shared
  lock/live guards: render-files-1790483569621016000 RC0 within90s.384 Fast
  allocations, zero owned bytes/no Chip fallback. All4DMAoff, exact cleanup and
  explicit release verified; no recovery hold.

Limits: initial/reset validation and allocator calls remain synchronous. Studio
provider first-trigger pinning and backend conversions still use their validating
public paths. Native PLAY/clock/card transport, audible and physical acceptance
remain open. Existing editor package and unrelated display/harness work unchanged.
