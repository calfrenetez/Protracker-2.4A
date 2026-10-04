# Cancellable initial readers preparation — 4 October 2026

The optional readers setup validates incrementally before allocating or publishing
a playback pool. Begin is metadata-only; step advances at most 4096 validator
items; cancellation owns/frees no resources and reads no former arrays. Completed,
current transfer uses one fixed pool allocation and a private checked bridge
initializer, avoiding both legacy synchronous PCM scans. Legacy APIs remain
unchanged. This is task-side software preparation, not editor PLAY integration.

| Evidence | Result |
| --- | --- |
| Final startup ASan/UBSan fixture | PASS: six formats/work1,7,4096; no semantic reads at begin/transfer; cancellation, stale/freed sources, aliases, allocation/callback refusals, empty projects and 20 zero-owner lifecycles |
| Unchanged genuine reader ownership fixture | PASS, original v1 proof preserved; legacy source prefix remains byte-exact |
| Three affected Paula/validator/scheduled-reader host groups | PASS on final v2 source |
| Two pinned 68000/software-float portability builds | PASS once each; assertions enabled, 32 RC0 records/21 full-M queries per build, 53 source/32 SDK dependencies per fixture, seven pinned runtimes |
| Amiberry / real A1200 execution of these exact fixtures | NOT RUN |

Exact products: `PTPaulaReadersTest` 168816 bytes,
SHA256 `9dcff2d4eec448ec09c2642ab2ecd43c2512227d0c75dc0d0c38d1a441e73407`;
`PTPaulaReadersStartupTest` 155792 bytes,
SHA256 `d32b442ad5e980c7acb91a243c4c5a009d88e3484b331ff4cf520e7a461319b1`.
Allocation callbacks use ordinary injected memory. There is no new Exec/Fast/Chip
placement, DMA, device transfer, voice-stop, activation, IRQ, timing, audio or
listening qualification.

One same-workload Mac CPU observation on an 8 MiB stereo24 master changed the
largest setup call from 20.276 ms to 0.085 ms, across 1 versus 515 calls. Total
setup CPU was 20.276 versus 15.210 ms. The 2384-byte host caller workspace is new;
requested heap payload remained 28472 bytes, with one pool allocation, no Chip
calls and zero final owned bytes. These observations are not 030 elapsed latency,
RSS, a worst-case bound or a hardware memory-class measurement.

`payload.tar.gz` holds byte-preserved actual reports, logs, executable products,
full dependency records, exact v1/v2 owned-source overlays, canonical source
closures, baseline reader bytes, compiler/Python/runtime/SDK pins, source reviews,
benchmark products and preparation histories. Full 8093-entry source inventories
refer to the pinned `c8550f0` baseline plus the six owned overlay paths. Original
paths inside proof records are retained; archived product/log paths map into their
corresponding `proof/<lane>` directory. `compiled-overlay` preserves the exact
pre-build documentation; later scoped qualification prose is the only doc delta.
Original preparation/parser/host-inspection refusals, passing v1 before the empty
guard, import-mode refusal and unpublished packet preparation revisions remain
separate. No product or target failure is hidden or retried.

Run `verify_payload.py` to check all member hashes, direct source bindings,
archived host products, native HUNK/assertion/marker bindings and recorded scope.
It neither extracts nor executes an artifact or contacts a target.

The shared emulator recovery hold and separately unresolved physical cleanup
remain external gates. Peer nonownership/last View Only reports are not root
target clearance. No Guest, lock, reservation, control, upload, reset, audio or
target test was created here. Fresh verified release, exact-candidate coordination
and live guards are required before any future native test; physical acceptance
follows its own exact emulator qualification and separate physical guards.

Next software work is a renderer-boundary adapter to persistent readers: fold
same-boundary trigger/control, require actual ACTIVE original keys for continuing
controls, and keep terminal STOP separate from traversal DONE. Native activation
and full song transport remain unfinished.
