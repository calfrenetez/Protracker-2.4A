# Exact editor bridge fixture — 6 October 2026

The exact `PTEditorBridgesTest` from source commit
`5771d3ce1fcc3dee1967ec0c728e1fa654954bba` passed once in the shared
68030/no-FPU/AGA emulator, with 2 MiB Chip and 128 MiB Fast RAM configured.
The 385144-byte product SHA256 is
`21c5efc7fe5d066f56e81ea8cbcea8bc7e95892e8af0a55608a1410d882d9b1e`.
Its complete 624-byte stdout exactly matched all three host fixture markers;
return code was zero and completion was observed within 213.214 seconds of the
single launch, below the fixed 420-second limit. Guest CRC and size checks cover
the product, launcher and complete logs.

The executed bodies cover 15 existing editor lifetime regressions, 39 checked
owner lifecycle cases, 18 whole-control alias/public-admission cases, nine bridge
lifecycle cases and 75 bridge alias/admission refusals across 8/16/24-bit masters.
The test uses injected clocks, ownership/quiet predicates, output callbacks and
an ordinary-RAM fake sample bus. It exercises actual portable editor/owner code
on the target CPU. It does not run the complete editor application or activate
real Paula/AmiGUS output, DMA, card registers, native interrupts or musical timing.
Configured stack size is 65536 bytes; total stack use and allocation placement
were not measured. Physical execution remains **NOT_RUN**.

Fresh Scott deferral, AmiGUS hands-off confirmation and AmiConnect sequencing
preceded the live EMPTY60 admission. One reservation and one owned startup were
used. The completed stage's six files were moved intact into private custody;
guest Status FULL showed the candidate absent, audio DMA was off, the original
CPU/RAM/AGA configuration was checked and the OS screenshot was saved. The
owned guest was disconnected and closed through the bounded shared wrapper.
The first independent release verifier then refused because it held the
transport lock that `LauncherGuard.release` itself acquires. That host-only
failure, original script and four records are preserved. A separately reviewed
changed verifier closes its third-lock probe before the release API acquires
the lock; it repeats only independent host readback and normal release. There
was no target, test or settlement replay and no shared-source/service change.

The changed host verifier confirmed process/listener absence, the healthy
disconnected original DevBench endpoint, missing-owner HTTP403, exact custody,
all 275 preparation/harness/dependency pins, 909 source identities and the 16
protected work paths. Normal four-record release returned EMPTY68, without
reservation, inflight operation, hold or possible resident. All peer chats were
explicitly notified. This receipt establishes that completed window's release,
not permanent availability. Root review was self-only.

The earlier failed progress-fixture startup and its unexecuted product remain
unchanged. The previous compiler manifest's NEVER_EXECUTED statement describes
its original compiler-only evidence tier; this separate packet records the
later exact editor bridge fixture execution. Later physical testing requires
fresh separate ownership/control/transport coordination and the exact candidate.
Real device capacity, upload ordering/completion, voice stop, exact scheduled
activation, audio/listening and native PLAY/event-loop wiring remain open.

Run `python3 -B verify_saved.py` to check this packet's saved bytes and result
relationships. It performs no producer replay, target query, reservation,
cleanup or release. The operational scripts are custody records only: they
refer to the original private window and must not be replayed from this packet.
The private lease envelope and executable are not published.
