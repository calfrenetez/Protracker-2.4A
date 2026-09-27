# Revision-bound wavetable song guards — 27 September 2026

Immutable song sessions now capture exact bridge/backend/reservation identities
alongside project header, sampler generation and bridge version. Every preparation
or playback entry verifies these identities and the currently owned WAVETABLE
resource lease. Ownership loss latches refusal. The guard no longer calls bridge
sync or scans every master PCM; public bridge/backend validation remains intact.

Coverage includes invalid initial 24-bit PCM, bridge/backend/reservation replacement
with identical contents, wrong sampler/project/table/count/revision, resource loss,
closing/faulted backend, lease loss and wrong resource, poisoning and cancellation.
Healthy unchanged master promotion and UI channel selection remain accepted.
The existing whole-song/range, uncertain-stop and editor mutation barriers pass.

Host instrumentation renames only the real project validator translation unit and
counts wrapper calls. Pending next/consume/complete, stale refusal, ready prepare,
ordinary next and silent consume do not invoke that validator. The initial attempt
to assert the same for an entire successful analysis step failed correctly:
measurement completion resets flow with its own full validation. The test now
isolates the owner guard and retains that synchronous setup/reset limitation.
No production test hooks or bypasses were added.

Validation:
- Final instrumented dispatch fixture ASan/UBSan PASS (6.457s).
- Staged editor wavetable/legacy edit guard/Studio lifetime suites ASan/UBSan PASS
  (26.787s), excluding unrelated working-tree display edits.
- Native build and staged native editor syntax PASS; all149 dependency hashes
  match staged input, generated font verified. Binary296992bytes, SHA256
  `25d5d8ac5048720e65df54cc3d93517bd24a4ea7bf0b94696ee9d453df0e71a3`.
- Fresh AmiConnect coordination and idle other-task snapshots, nonblocking shared
  lock/live identity guards: `render-files-1790482431610341000` RC0 within90s,
  334 Fast allocations, zero owned bytes/no Chip fallback. All4DMAoff and exact
  run/launch cleanup verified; explicit release delivered to AmiConnect.

Limits: borrowed arrays and callback contexts remain immutable/live until close;
identity checks do not detect unauthorized in-place writes. The ownership predicate
is synchronous/non-reentrant. Initial/static/reset validation, command-time PCM
checks and whole-source promotion remain synchronous. This is injected software
ownership evidence, not a hard latency/performance guarantee, native PLAY/clock/
card transport, audible playback or physical acceptance. Editor package unchanged.
