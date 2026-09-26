# Private EFx editor-owner integration

2026-09-26 UTC. New explicit budgeted direct/queued editor-owner entry points use
the sampler's private EFx producer and existing before-change guard. Pattern/sample
edit keys, undo, Stop and dispose close private ownership before mutation. Output
failure now requests bound output shutdown too. Held queue copies remain valid;
ordinary Studio retains direct full-precision master behavior.

Five clean-source host sanitizer cases pass in28.775s: ordinary/EFx editor owner,
ordinary/EFx sampler song and private queued producer. Two initial editor tests
also passed in the working tree. The clean source snapshot combines HEAD9e00b24
with only staged changes; unrelated editor/display work was excluded and snapshot
identity checked. Native m68000/nix20 build uses pinned compiler and -fbbb=-;
exact compiler/runtime/source/header/binary hashes are in native-build.json.

Shared030 run render-files-1790466063188243000 passes RC0 within120 seconds:
111 Fast/not-Chip allocations, zero final owned bytes, budget refusal without Chip
fallback. Fixture covers actual pattern/sample keys and undo, held leases across
Stop/dispose/edit, odd FIFO-tail output-stop callback, stale version failure,
invalid-size non-destructive refusal, normal output drain and format/budget refusal.
All4DMAoff; exact run/launch paths cleaned and absence verified. Explicit release
to AmiConnect and Scott. No native application window, lifecycle/config/device or
physical operations. This validates editor-owner integration, not an enabled
native Studio PLAY feature, actual AmiGUS output or real-time performance.
