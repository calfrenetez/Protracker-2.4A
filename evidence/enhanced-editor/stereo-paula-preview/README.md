# Stereo Paula preview — 2026-09-23

Pinned native build PASS. Coordinated shared030 Paula/cache/PCM regression PASS.
A three-frame interleaved true24 stereo master produced two distinct four-byte
Chip copies. Voices0/1 observed period428, voices2/3 inactive. After reference
replay first-word clearing, left byte2=1 and right byte2=255; both pad bytes0.
Original master data, channels2, bits24 and frames3 unchanged. All3 return codes0,
all4 DMA channels off, owned files removed, shared lock released and AmiConnect
notified. Hashes and logs are retained alongside this report.

Host checks cover stereo same-rate conversion, stereo filtered resampling against
existing reference conversion, dual-channel padding, capacity and alias refusal.
Existing mono/rate/loop regression remains enabled.

No physical/analogue listening or AmiGUS acceptance. CIAA fallback execution is
skipped when Workbench owns its vector. Shared baseline unchanged. Full editor
build still contains unrelated dirty display work; no clean package claimed.

Full host suite:

```
Ran 105 tests in 172.006s

OK
```
