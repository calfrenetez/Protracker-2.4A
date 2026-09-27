# Preserve Studio queue/session storage through adapter quiescence

The integrated reserved output previously freed its queue and detached its session
before asking the adapter to confirm callback removal. Reset confirms transfer
release, but does not itself prove that adapter callbacks no longer reference the
queue/session. Those objects now survive until the separate quiesce result is
exactly1 and the reservation has no retained interrupt guard. Pending, error or
unexpected status retains storage and blocks restart/detach. Access release follows
confirmed quiescence and queue cleanup. Each step makes at most one output or
quiescence operation. Unreserved output needs no additional quiescence callback.

The new regression fails against the prior release ordering. Host staged-source
editor/wavetable/Studio fixtures pass in28.076s, including queue inspection inside
a delayed callback, false success with an IRQ guard, startup/reset failures,
Stop/natural drain and direct24-bit byte parity. Existing display edits excluded.

Native candidate254748bytes SHA256
4d2b97edb8872f365eda024eca6e6f8786141f20686276951d3c00f7187691d7;
154 manifest inputs checked against index (font generated from pinned source).
Shared030 run render-files-1790550632920181000 passedRC0,141Fast allocations,
zeroowned bytes/noChip fallback, all4audioDMAoff. Independent elevated identity,
running, DMA and exact run/launcher absence checks passed without extra cleanup.
Window explicitly released. Injected port/library only, no real interrupt, card,
MMIO/audio output or physical acceptance. Native PLAY remains unwired.
