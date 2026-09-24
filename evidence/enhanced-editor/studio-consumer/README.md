# Studio consumer ownership

2026-09-24. Eight Studio host tests pass with address/undefined sanitizers.
New consumer test covers submit refusal retry, natural draining, submit failure,
successful cancellation, pending/failed cancellation, polling failure and eventual
confirmed completion. Lease memory is never released on uncertain completion.

Pinned m68k-amigaos-gcc compiled PTStudioConsumerTest with C99, m68000,
msoft-float, nix20, Os and Wall/Wextra/Werror. File inspection confirms an Amiga
loadseg executable. Binary SHA256: 65f20a2434404d2d54d2e5e1b3a20136606ab2ba39efa3bddf03d901ca48609a.

This is host/compile evidence only. No emulator or physical run for this new
consumer; no native audio driver, DMA, hardware cancellation or deadline claim.
Queue and callback context must outlive successful consumer detach.
