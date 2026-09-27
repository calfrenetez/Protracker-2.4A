# Owned wavetable preparation — 27 September 2026

Song begin claims an idle voice owner while publishing PREPARING. No playback
call advances until analysis succeeds and all selected masters are pinned.
Prepare guards generation/header/bridge revision, performs one analysis step,
then promotes at most one required source per call. Completed analysis transfers
its renderer sequence reset for identical playback without a second measurement.
Synchronous open wraps the same state machine. Failed/incomplete/transferred
preflights cannot transfer again; failed/private/incomplete sequences cannot rewind.

Cancel/failure before readiness frees workspaces/source pins and clears the song
claim. Idle backend/outer reservation remain bound for caller reuse or explicit
close. Existing confirmed-stop/cache-detach rules apply after readiness. Editor
begin/prepare participates in the change barrier: edit, Stop and disposal cancel
before modifying/freeing borrowed storage. Promoted unchanged sampler masters may
remain, with original precision and save content preserved.

Coverage:
- Exclusive pending owner; no early source promotion, uploads or voice callbacks.
- Cancellation before analysis, after compatibility and after master promotion.
- Stale sampler generation/project header, late unsupported source and promotion
  budget failure; all analysis/controller/pin allocation failures clean up.
- Prepared whole-song/range playback, exact range restore and existing uncertain
  stop/cache retention; safe one-time transfer and rewind refusal.
- Pending editor edit, Stop and dispose cancel; playback edit barriers retained.

Validation:
- Dispatch ASan/UBSan PASS6.540s before final transfer-refusal additions; final
  staged dispatch plus editor/legacy guard/Studio suite PASS26.588s.
- Renderer sequence ASan/UBSan PASS3.299s, including rewind state reset/refusal.
- Private EFx queued renderer ASan/UBSan PASS3.507s.
- Native build and index-export native editor syntax PASS;149 dependency hashes
  match staged inputs, generated font separately verified. Binary293688bytes,
  SHA256 `236eca8b82b03d21f4341ce88f76f04b70130d7591427fc0e757291a4ddd58d3`.
- Fresh AmiConnect-coordinated shared030 run
  `render-files-1790481783352925000`: RC0 within90s,273 Fast allocations, zero
  owned bytes/no Chip fallback. Shared lock/live guards, all4DMAoff and exact
  cleanup verified; explicit release delivered to AmiConnect, no recovery hold.

Limits: borrowed arrays must remain immutable until cancellation/close. Bridge
sync still validates project/PCM synchronously per call; initial scans, metadata
reset and whole-sample promotion copies also remain synchronous. No hard latency
or real-time guarantee, native PLAY/clock/driver, audible or physical AmiGUS
acceptance. Unrelated display changes and native editor package are unchanged.
