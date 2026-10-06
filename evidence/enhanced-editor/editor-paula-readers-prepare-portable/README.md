# Queued-Paula controller portable build evidence

The corrected portable compiler/link attempt passed. Its original independently
read-back candidate is **469,568 bytes**, SHA-256
`971dda5d3c3670375e6f518f6728e6452cbbb51ad4131b937985052f679f875c`,
observed as Amiga HUNK. The executable is not bundled here. **The native entry
and constructor have not run; Amiberry and physical A1200 testing are NOT_RUN.**

This packet preserves two distinct attempts:

- [`first-failure/manifest.json`](first-failure/manifest.json): the original
  single attempt stopped at `compile-link` RC1 after 88 successful compiler
  calls. The [unaltered diagnostic](first-failure/native/compile-link.stderr)
  records four `-Werror=misleading-indentation` findings. No product was created.
- [`corrected-current/manifest.json`](corrected-current/manifest.json): the
  separately corrected fixture passed all 89 compiler calls with empty stderr.
  [Independent saved-byte verification](corrected-current/saved-byte-verification.json)
  confirmed the exact HUNK candidate and preserved all 1,111 prior-attempt files.

Each complete fixture uses **80 ordered C units, 919 frozen inputs and 230
compiler dependencies**, with the genuine included producer main and controller
constructor retained. The actual saved host compile argv supplies the ordered
units; all 89 compiler argv/stdout/stderr records per attempt and both complete
six-call host records (12 total) are included. The original source86 baseline is
`86cb65fdef27a19082a4d469acee8aa3bb39e0bb`, plus four exact uncommitted additive
overlays at build time and the generated committed font. Corrected source C/test
C are the only changed frozen inputs; header and Python runner are unchanged.
Later source publication is separate from these build records.

The flags retain `-m68000`, `-msoft-float`, nix20, `-UNDEBUG` and
`-Wall -Wextra -Werror`. Both attempts retain the **120-second per-call and
600-second overall bounds**; they completed in 14.47 and 15.08 seconds. All 16
unrelated protected paths, source86 HEAD and the empty index were preserved in
the original observations.

Both [expected outputs](corrected-current/expected-native.stdout) retain all
three markers, including the new controller marker and inherited producer
markers: **755 bytes**, SHA-256
`facfb3c2fe6d7c70cb89b496cb92bdb5830dfde8266ad227641671405c998e3d`.
These are expected host results, not observed native output.

Run `python3 -B verify_saved.py` from any directory. The pure-stdlib verifier
checks [all declared bytes](packet-manifest.json), complete compiler/host call
relationships, source corrections, expected markers, candidate metadata and
unchanged prior custody using only bundled records. It never imports a builder,
opens the excluded product or accesses targets, installed SDKs or source pools.
The original builder and observers are included as inert `.reference` files.

This evidence establishes host compilation and saved bytes. Native constructor
execution, stack/placement, live timing, DMA/device completion/voice stop, card
RAM, audio and listening remain unqualified. See the separate
[controller host packet](../editor-paula-readers-prepare/README.md) and
[integration documentation](../../../docs/EDITOR_PAULA_READERS_PREPARE.md).
