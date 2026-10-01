# Bulk advance — host PASS, native FAILED/incomplete

1 October 2026. Changes remain uncommitted/staged; HEAD/origin40edfff is the last
qualified milestone. Bulk arithmetic advances valid voice state without PCM reads,
using the unchanged frame transition for end/loop/repeat handoff, then overflow-safe
modular doubling. Host4096random16voice partitions+256wide scalar trajectories and
40small frame-reader trajectories PASS1.162s.5related ASan/UBSan suites including
195mixed scenarios PASS14.844s. All source/reader/lease/deadline guards unchanged.

Isolated build163indexed sources/zero generated/4binaries PASS, exact tree
e99847bd0ea98a732b219adbe53e8a0ac994d48c; native236776bytes SHA256
ff73f741277b6ee2b4b834a608271c69c3ea5c3dd43cea950acc343977d7128d.
Planned native40frame-reader/32random16voice/256wide equivalence plus6running-cost
cases. ONE run1790811697827119000 FAILED at90s fixture deadline; runner exited and
preserved owned run/launcher. No done/test.rc/completion/zero-owned-allocation/
reader-pin-lease-counter-device-request cleanup proof. Native acceptance and cost
remeasurement NOT established. Raw log empty at one later guarded read-only check;
root cause is unknown (no assertion or successful equivalence marker observed).
Do not infer merely excessive test workload, correct native behavior or a crash.

Standard lock/live process/profile/68030/running/bridge guards passed before
staging. After failure, one separate locked read-only check showed running guest,
all4DMAoff, owned paths present; this is not fixture termination or cleanup proof.
AmiConnect HOLD, peers informed via coordinator; no retry/reset/resume/lifecycle/
physical operations. Original deadline failure remains failed even if later finishes.
Earlier host build approval-review timeout occurred before process creation; allowed
single retry completed the build. Separate from native failure.

Recovery proposal requiring user approval: refresh peer/AmiConnect hold, reserve
recovery-only window, acquire shared lock and verify exact owned030 process/profile/
bridge; preserve failed log and launcher, stop ONLY that owned emulator through
existing harness, prove all emulator processes absent before removing ONLY
Dev/Tests/render-files-1790811697827119000 and its exact launch file, start fresh
owned030 through harness, verify newPID/bridge/68030/running/DMAoff and exact path
absence, then independent subsequent locked verification and explicit RELEASE.
No physical/config/network/CIA/install changes, no guest test in recovery window.
Restart discards old address space; never normal cleanup or acceptance of failed
fixture. Subsequent smaller diagnostic/native tests need separately coordinated
windows and remain authorized software work after recovery approval.
