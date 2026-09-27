# Strict interval clock gate —27 September2026

Clock-arm owns one pending positive emitting interval without starting a voice.
Service uses injected absolute render-frame timestamps: <=256 elapsed frames plus
one bounded prefetch operation before deadline, no invented elapsed time on repeat
calls. Direct manual advancement cannot bypass the armed gate. Exact-deadline
completion requires previously ready prefetch and <=256 remaining phase frames;
otherwise stop with DEADLINE, never perform late upload/dispatch. Regression and
checked-deadline overflow fail with CLOCK. Unconfirmed stops retain active leases
and master pins until close, and the editor edit/dispose barrier cancels the gate.

Host evidence: ASan/UBSan instrumented wavetable7.354s PASS (before test-only
refinement splitting unready from excessive-debt cases); final staged editor,
guard and Studio28.073s PASS. Fixture covers exact ready/write-free trigger,
no early/double start, zero-interval/manual-bypass refusal, overflow/regression,
late service, unready batch with bounded debt, ready batch with excessive debt,
Stop/cancel and retained uncertain voice ownership. Value/project validation
counters remain unchanged during service. All new allocations use matched
malloc/free, including native macro wrappers.

Native build and committed-editor main syntax PASS;163 staged inputs/generated
font verified. Binary356472bytes SHA256
b5c53c0a85318a0696a3a71c3831c6cb498391b007b0d13668aeb21b7675b0ec.
Shared030 run render-files-1790502739884164000 passed within90s, RC0,
1069 Fast allocations, zero owned bytes, budget refusal without Chip fallback.
All4PaulaDMAoff; exact run/launch cleanup confirmed by runner and independent
absence check. AmiConnect coordinated and explicitly received resource release.
No lifecycle or physical operations.

This is a per-interval injected clock contract, not a complete scheduler or hardware
clock. Caller still owns zero-frame startup commands, silent pre-roll, restoration,
and transitions/rearming. Native PLAY/card output remains disabled. Synchronous
callback duration, physical clock accuracy/audio/performance remain unqualified.
No changes to classic layout, master precision or saving; unrelated display edits
were excluded from staged tests/build and commit.
