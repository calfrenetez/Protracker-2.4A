# Third native CIA timing gate failed; cleanup recovered separately

1 October 2026. Candidate tree650aa0c8e1ef9b6f7072ed18a68140bd22e4a838,
29820 bytes, SHA2560047c1b8bc06001e96032e4716eb2d139c6a81dc9beed790fd1629545d8583f2.
Exact source bytes match committed ce8019c; no dirty classic frontend inputs.
Earlier unused lock-refused attempt paths independently proved absent before run.
Fresh AmiConnect coordination and shared lock preceded one bounded execution.

RC20: twelve checks using the actual assembler count helper PASS before resource
acquisition. Sixteen IRQ observations all late44..47 EClock ticks, programming
span36..37 ticks at709379Hz. Unchanged logical-frame gate FAILED. These are not
pure IRQ latency bounds or universal physical timing claims. No empirical lead,
priority, tolerance, output, audio.device or production-path change.

Native log confirms exact vector removal, owned timer stopped, unchanged Task
priority and signal allocation, both clock readers closed, two Fast allocations
and zero owned bytes. Runner immediate cleanup claim is retained verbatim in
native-result.json but SUPERSEDED by independent-cleanup-failure.json: an empty
cia-timing directory reappeared. Overall qualification remains FAILED. A premature
RELEASE/PASS message was immediately retracted; coordinator ACK133 retained hold.

Read-only inspection found no files or links. Under existing testing/cleanup
scope and fresh no-conflict coordination ACK134, filesystem-only recovery used
non-recursive rmdir on exactly that empty child and parent. Same original PID79263,
shared030 profile, connected localhost bridge, running68030/all4DMAoff guarded it.
Nine absence observations over2s PASS, followed by a separate locked read-only
identity/audio/path-absence check PASS. No reset/resume/restart/test retry. Explicit
RELEASE reported only after inspecting both successful records. This clears this
cleanup hold; it does not retroactively pass the timing or initial cleanup gates.

Runner now observes absence over2s to catch delayed recreation, never repeats
removal, and explicitly still requires independent release verification. Five
host guard tests PASS, including delayed empty-directory and dangling-link
recreation while preserving unrelated files. This finite observation interval is
not a guarantee against later filesystem changes.

User now authorizes physical ProTracker/AmiGUS testing. No physical operation in
this CIA scope; fresh exact candidate qualification and ownership are required.
