# Native critical-arm candidate: host/build PASS; NOT RUN (busy lock)

1 October 2026. Shortened diagnostic-only final clock/count/start path in native
assembly. It reads actualEClock, checks same expectedfrequency, performs full64-bit
target-minus-actual including unsigned borrow refusal and16-bit timer capacity,
and writes only acquiredtimer low/high/control. No calibrated compensation,
predictedtimestamp, targetshift, tolerance, priority or production change.
C offset assertions and object/finalHUNK disassembly verified ABI/clock vector,
borrow rejection and byte stores. Twelve arithmetic boundary checks using SAME
assembler are in fixture BEFORE any resource acquisition; these remain NOT RUN.
C task/owner/clock checks and ownedstop/mask/pending setup precede the criticalpath.
Failure leaves ownedtimer stopped/masked and vector retained for safe closure.
Samefixed16targets/independent60000framecutoff+CtrlC everyWait andHOLD semantics.

Exactcandidate tree8e125629f4b28119277af306de43eff429005ae3,
29820bytes SHA2560047c1b8bc06001e96032e4716eb2d139c6a81dc9beed790fd1629545d8583f2.
PinnedcrossbuildPASS. Host8testsPASS1.631s:9CIAowner cases,clock/alarm failure,
400000clockoracle comparisons and runnerbusy/paused/cleanup guards. Native fixture
and new assembler tests NOT RUN: separately scheduled attempt1790881047026208000
refused FIRST nonblocking sharedlock, BEFORE Guestconstruction/commands/staging.
No lockbypass, retry, lifecycle, timeracquisition or filecleanup performed.
Hostreconstructed prelaunch-result explicitly distinguished from originalrunner.

Runner now records busylock refusal/no staging before Guestcreation, with real
competing-file-descriptor test preserving the originalholder. This does not prove
sharedavailability; new windows require freshcoordination/exactcandidate guards.
Previous two nativeRC20timing failures/cleanupPASS remain separate. PhysicalOFF/
deferred/unprobed, no output/frontend/playback/listening/physical acceptance.
