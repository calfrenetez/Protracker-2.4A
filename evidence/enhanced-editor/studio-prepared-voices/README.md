# Prepared Studio voices — 27 September 2026

The ordinary prepared sampler-song owner now calls a private pull/dispatch/mixer
path that reuses completed PCM validation for voice trigger, independent segment
and repeat-source setup. Its private provider still checks current generation,
header and exact version/descriptor identity and retains each active/pending pin.
Only repeated source-value scans are skipped. Existing voice shape/capacity,
format, geometry and alias checks and mixer rollback/release behavior remain.

No public opt-in flag or persistent trusted-mixer state is added. Public Studio
APIs always call the fully validating voice helpers, even after private calls on
the same mixer. Arbitrary providers and mutable EFx stay outside the call chain.
The private functions require validated immutable sources and the prepared owner;
they do not grant validation/ownership or accept untrusted PCM by contract.

Coverage:
- Test wrappers count real project and PCM validator calls: neither is called
  during prepared direct playback, including pre-roll, slices, offset segments
  and repeat handoffs. Exact reference output and ownership are preserved.
- Both public/private mixer paths pass existing control, independent segment,
  pending repeat, rollback, output alias, true24 and final-release cases.
- Private capacity/rate/channel refusal retains old pins; public trigger/segment/
  repeat still reject invalid sample values after private calls on that mixer.
- Public song rejects an arbitrary provider's invalid PCM; mutable sequence and
  sampler EFx regressions remain validating. Initial setup/reset is still checked.

Validation:
- Final instrumented sampler song ASan/UBSan PASS5.519s.
- Mixer public/private/negative suite PASS0.768s; public song PASS3.196s;
  instrumented renderer sequence PASS3.210s; sampler EFx PASS5.372s.
- Final staged editor wavetable/guard/Studio suite PASS27.231s.
- Initial offset-case run failed an unused-slot assertion because its new fixture
  also assigned an unintended second instrument. Fixture corrected and both
  affected host suites passed before the final native build/emulator run.
- Native build/main syntax PASS;157 staged dependencies/font verified.
  Binary320004bytes SHA256
  7e301e198ec9928d116827570302450cd742f3d8dba45518d29f1f3ce5d2f1be.
- Coordinated shared030/DevBench run render-files-1790485350284033000 passed
  within the90s deadline: return code0,523 Fast allocations, zero owned bytes
  and no Chip fallback. Guarded cleanup verified all4DMAoff and exact run files
  removed. Emulator/DevBench explicitly released to AmiConnect afterward.
  This is software ownership/memory evidence with injected bus/library only.

Limits: synchronous initial/reset validation, allocation and bounded queue block
checks remain. No native PLAY/transport, real-time deadline, audible or physical
acceptance. Existing native editor package and unrelated display work preserved.
