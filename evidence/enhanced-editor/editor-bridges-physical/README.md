# Exact editor bridge fixture on real A1200 — 6 October 2026

The same 385144-byte `PTEditorBridgesTest` previously qualified in Amiberry
passed once on the real 68030 A1200. Source commit is
`5771d3ce1fcc3dee1967ec0c728e1fa654954bba`; product SHA256 is
`21c5efc7fe5d066f56e81ea8cbcea8bc7e95892e8af0a55608a1410d882d9b1e`.
Complete 624-byte stdout matched the three saved host markers, RC0 and COMPLETE
were positively observed in 230.127 seconds under the fixed 420-second bound.
The actual physical CPU, plipbox address and installed bridge CRC/size were checked
before upload. Stack65536 was configured; stack use was not measured.

The executed software covers legacy editor15, checked lifetime39/control alias18,
and bridge lifecycle9/admission75 across authoritative 8/16/24-bit masters. Clocks,
voices, quiet predicates and device callbacks were injected; the fake bus operated
on ordinary RAM. This qualifies portable software execution on the physical CPU.
It does not qualify allocation placement, actual Paula/AmiGUS output or card RAM,
capacity, upload ordering/completion, all-voice stop, interrupts, exact musical
activation, audio/listening or the complete native editor/PLAY/event loop.

Fresh recipient-specific AmiConnect, Scott and AmiGUS hands-off covered both
physical and original030-return phases. All319 ordinary inputs,909 frozen source
identities, the exact saved native promotion and protected16 paths were checked
before and through admitted actions. Actual Safari stayed authenticated View Only;
no browser control or input was acquired. Existing physical configuration, players
and the other task's prepared media were not changed.

Separate completed-only settlement checked all six guest CRCs before exact
nonrecursive deletion; independent physical task/directory absence and disconnect
passed. The physical-only release to EMPTY94 explicitly retained the overall
return obligation. A new030 return-only reservation then verified the original
68030/no-FPU/2MiB Chip/128MiB Fast/AGA guest, idle status and audio DMA off, saved
the OS screen, disconnected and performed the normal owned SIGTERM-only stop.
Separate host custody/absence/original endpoint verification and four-record final
release passed EMPTY100. All three peer chats received explicit full release.
There was no test retry, failed cleanup, force kill or automatic failure recovery.

The earlier inert protocol canary's first host shutdown TypeError remains FAILED
and preserved in the separate `editor-physical-protocol-canary` packet, followed
by its separately approved recovery release. It is not rewritten by this success.

Run `python3 -B verify_saved.py` to verify saved bytes and relationships only.
Private lease files and the executable are excluded. Operational scripts are
custody records referring to the original private window; do not replay them.
The saved EMPTY100 record establishes that window's release, not current target
availability. Later target use requires fresh coordination and live admission.
