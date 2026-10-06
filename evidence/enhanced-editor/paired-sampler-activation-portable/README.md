# Paired sampler/activation portable compiler evidence

The complete combined HOST fixture compiles and links with the pinned m68000,
soft-float, nix20 C99 toolchain. All41 calls pass RC0 with empty stderr: version,
four helper lookups, seven runtime lookups,28 dependency calls and one link.
The fixed600s overall bound and120s call budget remain; the driver wait reserves
5s for owned compiler cleanup. Actual successful elapsed time was8.718507416s.

Exact HUNK:266204 bytes, SHA256
`f4c5eeac8cfc6a5ebd4081eceff8ea33691bc2ce3fe5dccfbcb99984f9ca8342`.
Expected full software stdout is1760 bytes, SHA256
`84bebd4a722df63a1a49bbd27a4668b48cfa08f2831c69e7f74178d76989c8c9`.
The product has NEVER_EXECUTED. Native entry, constructor, emulator and physical
execution are NOT_RUN. No timing, stack/IRQ, real cache capacity/completion/
ordering/stop, DMA/audio or listening proof follows.

Complete saved inputs, dependency metadata, sanitized exact call logs, tool/
runtime byte pins and offline verification summary are retained. No source pool,
SDK body, executable, private freeze/authority/control or active launch metadata
is published. $PRIVATE and $SDK replace absolute roots; original stdout/stderr
pins describe unsanitized originals, files.json binds published bytes. Helper
queries establish driver lookup paths only, not empirical helper invocation.
Successful group quiescence was observed; timeout cleanup is not empirically
qualified by this positive build.

The actual V2 first failure occurred before GCC: Git RC0 emitted a macOS temp
directory warning, rejected by the stderr gate. Its zero compiler-call receipt
and actual readbacks remain separate. Earlier V1 preparation was withdrawn before
caller invocation following static review. These are not product execution
failures and are never relabelled PASS.
