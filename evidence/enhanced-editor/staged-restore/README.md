# Prepared interval / separate range restore commit —27 September2026

next_prepare stages an interval and prepares any range-start cache leases with
bounded acquisition/upload calls; it never invokes a voice callback. Ready output
is repeatable without advancement/additional writes. next_commit rechecks exact
master/cache identities and restores fractional cursor positions once, without
allocation/upload. Early/double commit refuses. Legacy next_step composes both
operations and preserves output atomicity if restoration fails.

Host instrumented ASan/UBSan wavetable7.426s PASS, including final changed-PCM
commit refusal. Staged editor/guard/Studio28.275s PASS before that final test-only
case. Range oracle covers pre-roll, fractional cursor, final span, future trigger,
ready cancel, generation/descriptor stale refusal and uncertain restore ownership.
Editor tests cancel ready-but-unstarted restore through edit/undo, Stop and dispose
without invoking any restore/stop callback. Project save bytes remain identical;
project/PCM validation counters remain unchanged during prepared operations.

Native build/main syntax PASS,163staged inputs/generated font verified. Binary
360108bytes SHA256be8417d4e62c41d33b3426a3c8e9a8769aab000cd9fac2ed00b08bf57d9d6302.
Shared030 render-files-1790503162743616000 PASSED within90s RC0,1094Fast allocations,
zeroowned, noChipfallback; all4PaulaDMAoff. Runner exact run/launch cleanup and
independent absence checks passed. Explicit release sent to AmiConnect. No
lifecycle/physical operation. Native fixture includes final descriptor-refusal case.

This is preparation/commit separation, not a native scheduler. A later driver must
schedule commit, zero-frame commands and interval transitions against a clock.
Native PLAY/card output remains disabled; injected callbacks do not prove card
access, physical sound or real-time throughput. Unrelated classic display work is
preserved and excluded from staged tests/build/commit.
