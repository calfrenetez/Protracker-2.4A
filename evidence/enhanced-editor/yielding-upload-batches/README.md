# Yielding upload batches — 27 September 2026

Song/editor next_step and complete_step return UPLOADING while acquiring only the
current batch's selected sample caches. Repeat the same operation on later owner
turns. Each acquisition step begins one job or uploads <=256bytes; a separate final
step verifies every current master and actual cache address then applies callbacks.
No new voice starts/restores or software time consumption occurs during acquisition.
Prepared dispatch transfers already-held leases, including duplicate-source pins.
Existing synchronous song/editor calls drive the same steps; standalone public
mutable dispatch retains its validating path.

Stop/edit/dispose cancels in-flight jobs and unstarted cache leases before stopping
active voices. Failed/uncertain stops retain their independent playback leases and
session master pins. Later acquisition/descriptor failure starts no earlier voice.
Initial allocations/final callback batches remain synchronous. Existing hardware
voices can continue during acquisition; no native event loop, hardware clock,
lookahead/deadline policy, card transport or physical performance proof is supplied.
Classic display and existing packaged editor remain unchanged.

Validation:
- ASan/UBSan instrumented wavetable suite7.042s PASS, including new three-voice
  batches over two600-byte sources (third trigger hits a pinned cache). Exactly
  ten calls separate acquisition/write/hit/commit; each upload <=128 address/data
  register writes (64 words/256bytes), total600 register writes for1200 PCM bytes.
  No project/PCM value-validator calls during prepared playback.
- Cancellation after allocation/partial copy, stale generation, later source
  capacity/transfer failure, previously completed source descriptor change,
  duplicate pins, UI selection, uncertain second start, pending retrigger Stop
  with an unconfirmed old voice, and final cleanup all checked.
- All existing exact fractional range/oracle modes now use next_step while refusing
  consume/complete during restoration; output interval remains untouched pending.
- Staged editor/legacy guard/Studio suite27.826s PASS. Edit/undo, Stop and dispose
  cancel an unpublished cache allocation before writes/voice starts. Existing
  active-voice edit/undo/dispose barriers and Studio lifetimes pass.
- Native build/main syntax PASS;160 staged inputs/font verified. Binary343632bytes
  SHA256 f0933fddde0f979b09091f049d3c337187d17fd082977cad1e304363b80fe983.

- Coordinated shared030 run render-files-1790488129890475000 RC0 within90s;
  song/editor upload tests PASS,1005 Fast allocations,zero owned bytes/no Chip
  fallback. All4DMAoff/exact cleanup verified; explicitly released to AmiConnect
  with no recovery hold. No physical or lifecycle operations.

All device behavior is injected software, not physical AmiGUS acceptance.
