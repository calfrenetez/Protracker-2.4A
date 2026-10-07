# Project CRC update: HOST qualification

The shared private project CRC uses one immutable 16-entry/64-byte nibble table
in the contiguous, streaming and positional-reader paths. Polynomial, initial
and final XOR, header bytes20..23, byte order, <=1024-byte reads, validation,
callback/refusal behavior, master PCM and emitted project bytes are preserved.
No allocation, initialization state or public API was added.

Five strict C99/O1/assertions/ASan+UBSan groups passed: focused independent
bitwise reference and fixed mixed/song goldens, existing project round trip,
existing stream, existing stream/read faults and existing positional reader.
The unchanged genuine mixed RAM fixture then passed all65 cases with its original
446-byte SOFTWARE_ONLY oracle. Master/save beforeimages and all20 original keys
remain asserted. Stream workspace7184 is a HOST observation only.

The first collector omitted the existing native/readers_ram include path.
Its first five groups succeeded, then mixed65 compile failedRC1 with the complete
200-byte missing-header diagnostic. [first-attempt.json](first-attempt.json)
remains FAILED; [first-audit.json](first-audit.json) verifies that record.
A separately SOURCE-reviewed command added only that include and a distinct
output executable, then compiled and ran only the remaining mixed65 group.
[remaining-mixed65.json](remaining-mixed65.json) records RC0, compile7.807s,
run0.602s, full oracle, empty errors and positive HOST group quiet/reaping.
The first receipt and all22 original streams remained byte-exact. No CRC group
was repeated. [combined-audit.json](combined-audit.json) qualifies this combined
HOST evidence and exact selected source/proposal/protected/compiler custody.
All26 full command streams are saved under first/ and remaining/.

The canonical standalone source recipe is tests/test_project_crc_nibble.py;
existing tests/test_native_mixed_ram_port.py supplies the unchanged65 fixture.
Archived collectors are fixed provenance with original private workspace paths,
not relocatable launchers. No candidate binaries or secret leases are published.

Updated native crossbuild and exact Amiberry/real-A1200 execution are NOT_RUN.
No runtime improvement or cause of the original90-second native failure is
inferred. Repeated validation remains unchanged. Native90/stack131072, diagnostic
case/phase grammar, target admission/cleanup/release and exact scheduling remain
separate gates. No DMA, device capacity/ordering/completion/voice-stop, IRQ/WCET,
whole stack, timing, audio or listening acceptance follows from these HOST tests.
