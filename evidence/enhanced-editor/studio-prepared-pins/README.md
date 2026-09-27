# Prepared Studio pins — 27 September 2026

Ordinary sampler songs now acquire exact pre-pinned master versions through a
private provider. Bounded generation/header and slot/current-token/owner/descriptor
checks precede each additional retain. No whole-project PCM validation, allocation
or source copy occurs at acquisition. Each active/pending-repeat voice retains its
own reference; mixer close precedes release of the preparation pins. Metadata-only
versions preserve shared backing until the last independent reference is released.

The private helper requires genuine held pins and previously validated immutable
inputs. It does not promote samples or validate untrusted data. Public sampler_pin,
the general sampler_studio provider and mixer/voice validation are unchanged;
mutable EFx uses its existing separate provider. Unexpected source replacement or
metadata changes fail closed. In-place edits during playback remain forbidden.

Coverage:
- Real project-validator call instrumentation, with no production hooks: no calls
  during prepared playback, including repeat sources and pre-roll. Public provider
  still calls the validator and rejects an invalid unused source.
- Exact8/16/24 pin clones without additional budget, stale generation/current-token
  and descriptor/pointer/capacity refusal, output atomicity and independent release.
- Metadata-only shared backing survives history/sampler/document ownership release
  until all old/new voice pins are dropped; no leaks or precision loss.
- Ready session refuses changed metadata/source identity before first output;
  source preparation, exact stereo24/pingpong/slice PCM, queue/edit/undo/dispose
  guards and pending/active-repeat lifetimes remain passing.

Validation:
- Final instrumented sampler song ASan/UBSan PASS5.304s; public sampler provider
  PASS3.594s; Studio mixer PASS0.726s; sampler EFx regression PASS5.321s.
- Staged editor wavetable/legacy guard/Studio lifetime suite PASS27.456s.
- Native build/main syntax PASS;156 staged dependencies/font verified.
  Binary319120bytes SHA256
  fb09d79cb7ed3a2bc39eca0bd104c302b689fb3a56f08357674fe4f9dcff4795.
- Fresh AmiConnect confirmation/current idle other-task snapshots plus shared
  lock/live guards: render-files-1790484818829408000 RC0 within90s.513 Fast
  allocations,zero owned bytes/no Chip fallback. All4DMAoff and exact cleanup
  verified; shared030/DevBench explicitly released with no recovery hold.

Limits: initial/reset validation and allocation remain synchronous. The mixer
still scans the selected source for voice setup; no hard real-time guarantee.
No native PLAY/clock/card transport, audible or physical acceptance. Native editor
package and unrelated display/harness changes are unchanged.
