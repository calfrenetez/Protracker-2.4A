# Native Paula reservation qualification

1 October 2026. Indexed tree `95e505f1657dadb0a2729e55dd95eaa13e87270e`;
88 source hashes checked against indexed export and Git tree, zero generated
sources. PTExecPaulaReservationTest 12188 bytes SHA256
`6bcc2d0d563125784fc1f99a8201df1481da127e0e0ffd9676cecd277d3575a7`.
The guard binary was built but not executed.

The production header owns one private port and two IOAudio requests. OpenDevice
opens without allocating channels; subsequent BeginIO NOWAIT allocation requests
exactly all four at minimum precedence, so it cannot steal existing channels.
Only exact successful allocation with its key permits raising precedence to127.
A pending LOCK observes theft; it is not the mechanism preventing theft.
Each advance polls/submits bounded work without waiting on unfinished IO.
Close is allowed only after future playback clients confirm all readers stopped.
FREE uses only the acquired mask/key; pending/free error retains resources.
CloseDevice/DeleteIORequest/DeleteMsgPort follow confirmed command and LOCK
completion. There are no WRITE, MMIO or DMA operations in this component.

Primary command semantics: [AmigaOS Audio Device documentation](https://wiki.amigaos.net/wiki/Audio_Device).
The pinned SDK supplies IOAudio/command declarations and BeginIO's alib prototype.

ASan/UBSan host12 cases PASS0.500s: partial resource setup failure, idle close,
exclusive readiness, repeated open refusal, pending command/close retention,
allocation refusal/partial returned mask, precedence failure, completed theft
observer and failed FREE retention without retry. Host Exec/device stubs execute
the real production header; they do not establish target IO semantics.

ONE coordinated shared030 run `1790841823999865000` PASS1.724s outer.
Native3 cases: idle open/close, actual exclusive reservation/pending LOCK and
second-client busy refusal preserving original readiness. Both owners' requests,
locks, channels, device opens and ports close; ownership pointers/masks/flags
clear. Native log confirms this separately from harness DMA-off checks.
Normal completion/RC0/exact cleanup pass. Independent subsequent shared-locked
process/profile/bridge/68030/running/all4DMAoff and run+launcher absence pass.
50 polls per operation with Delay1 tick;90s fixture/140s runner/200s outer within
240s reservation. No native retry/reset/resume. TAKE and RELEASE coordinated
with AmiConnect; peer idle/no-hold reports refreshed before TAKE.

This qualifies reservation/close and expected busy refusal only. The header is
not wired into pt_paula_voice_api, mixed frontend PLAY or the classic player.
Voice start/control/stop, hardware clock validation, pending start acknowledgement,
cache/read ownership under DMA, actual audio/timing/endurance and physical tests
remain requirements. Powered-off physical Amiga was neither probed nor touched.
Unrelated display work and classic player remain preserved.
