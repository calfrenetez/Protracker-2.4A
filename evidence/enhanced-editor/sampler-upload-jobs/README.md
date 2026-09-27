# Sampler-owned upload jobs — 27 September 2026

The sampler bridge owns an independent exact master pin and stable PCM descriptor
through begin/step/cancel. Public begin validates/promotes only the requested
sample; prepared-song begin retains an already validated exact pin. New uploads
begin without bus writes. Each step converts at most256bytes. Complete publication
transfers the device lease; failure/cancel frees device resources before the master.
Generation/header/bridge/backend/cache-version/exact descriptor guards reject stale
jobs. Selected channel navigation is allowed. Adapter guards preserve reservation
and live-ownership checks. Existing synchronous bridge/song calls share these jobs.

Validation:
- ASan/UBSan sampler-wavetable4.012s PASS;96 new cases across8/16/24-bit masters,
  public/private begin, success/hit, cancellation, master edit/history release,
  sampler release, document disposal then cancel, source capacity/data changes,
  bridge version/owner changes, detach-busy, lost/swapped reservation, transfer
  failure, generation invalidation, UI navigation and memory/allocation refusal.
- Instrumented wavetable6.870s PASS: no project/PCM validator calls during prepared
  playback; public validation still enforced. Existing range/stop ownership passes.
- Staged editor/legacy guard/Studio27.897s PASS.
- Native build/editor-main syntax PASS;160 staged inputs/font verified.
  Binary335328bytes SHA256
  0d8a383e663ce28b303173a77bc3d680adc6591125300984e658ea3d53e6334e.

- Coordinated shared030 run render-files-1790487488754777000 RC0 within90s;
  sampler-upload/editor fixtures PASS,945 Fast allocations, zero owned bytes and
  no Chip fallback. All4DMAoff and exact run cleanup verified. Explicitly released
  to AmiConnect, no recovery hold or physical operations.

Owner/context structs must outlive jobs; normal callers cancel before mutation or
project disposal. Job storage cannot move while active. Initial public validation,
promotion/allocation and callbacks remain synchronous. Song/editor dispatch still
drives jobs synchronously: yielding batch scheduling is unfinished. Injected bus
fixtures do not prove physical AmiGUS, native PLAY/clock, audible acceptance or
real-time performance. Unrelated display work and editor package preserved.
