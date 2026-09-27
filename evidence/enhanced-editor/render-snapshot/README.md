# Exact interval-start voice snapshots — 27 September 2026

The audited renderer provides a bounded read-only copy of all16 software voices
and resolved per-side gains at the start of a pending interval. It preserves Q32
fractional phase, loop-relative position and pending repeat descriptors. It does
not allocate, read/edit source PCM, pin samples or advance the sequence. Returned
PCM pointers are borrowed; their existing owner must retain them. Private-bank,
mid-interval, completed and invalid protocol states refuse without changing output.

Host ASan/UBSan snapshot fixture PASS3.177s. Eighteen loop/mode/block combinations
compare resumed24-bit PCM byte-for-byte with uninterrupted Studio reads and the
offline renderer: forward/ping-pong, lead-in/song/row-range, blocks1/17/256, tempo
and pattern delay, fractional positions and repeated snapshots. Invalid/null,
pre-next, post-consume, post-complete and private-bank publication are refused.
Existing wavetable dispatch/song/cache/ownership regression PASS5.935s.

Native cross-build passed:31 dependency hashes match staged source bytes;
70992-byte PTExecRenderSequenceTest SHA256
`b89de1d8b65131042f854774226e32553533a3b8acd88f4e7f6cd7a2e2a8075a`.
Compiler/runtime digests and flags are in build.json. This is build evidence,
not a native pass. The fixture uses production Fast allocation for owned objects.

Shared030 run render-files-1790479010316536000 exceeded its90-second deadline.
The runner result remains passed=false; no return code/completion marker was
observed. Exact guest staging/launch files are preserved and an unresolved
recovery hold was reported to AmiConnect. No retry/reset/cleanup followed.
A bounded read-only assessment under the shared lock confirmed matching030
identity/bridge, responsive IPC and all4PaulaDMAoff; task-list output did not
identify the fixture by name and does not prove exit. See recovery-readonly.json.
The earlier sandbox refusal at process inspection occurred before guest launch.

Wavetable row-range sessions still refuse. The current initialized-trigger plan
cannot restore arbitrary fractional sample position; this snapshot API must not
be mistaken for hardware restore support. No native UI, real MMIO/card output,
audible acceptance or physical operations were performed. Full row-range wiring
requires phase-preserving restore capability and retained source/cache ownership.
