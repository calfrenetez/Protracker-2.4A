# Bounded PowerPacker PP20 MOD loading, dev19

The shared document loader validates a PP20 bitstream before decoding into separate
memory and running strict MOD preflight. It commits only a complete valid candidate.
The editor and converter use this path; no powerpacker.library install is needed.

## Evidence

- All 28 host groups pass. ASan/UBSan tests cover malformed bounds/headers, output
  capacity and aliasing, unchanged output on failure, mutations, all seven scratch/
  candidate allocation failures, peak-memory budget, repeated reloads and exact
  classic MOD round-trip. Independent Python test streams cover all skip counts
  0–32, short literal lengths, each match class, both long-match offset branches,
  overlapping references, invalid backreferences and output-length overruns.
- The existing genuine upstream `mod.loving_is_easy.pp` at the pinned libxmp
  revision decompresses to 49,798 bytes with expected MD5
  80ba11ca20f7ffef184a58c1fc619c18. Its full host atomic-document suite passes.
  The pinned upstream invalid-stream fixture is also refused in local inspection.
- Native PTPp20Test passes using our synthetic baseline fixture. The native
  converter loads the genuine packed file and emits the exact uncompressed MOD.
- Actual editor input opens a packed baseline, edits/undoes, refuses a malformed
  PP20 load without losing redo, redoes/saves the edit, loads the genuine packed
  module through ASL and exports exact ordinary MOD bytes. PTG CRC validates;
  reopen/resave is identical, both exits are normal and DMA is off.
- Core source/compiler/binary hashes are retained. Screenshots are actual native
  output. Third-party music and derived projects stay local; tracked evidence
  contains identities/results only, not musical data.

## Provenance and boundaries

`vendor/pp20-reference/` retains the pinned, unmodified libxmp algorithm with its
explicit Public Domain notice and attribution to Stuart Caie, Heikki Orsila and
Claudio Matsuoka. Our adaptation uses bounded indices and a validation-only pass.
The pinned native 2.3F path calls powerpacker.library version 35+; that separate
assembler path is unchanged. No GPL/decompiled original compressor is included.

PP20 has no checksum; structurally valid bit changes cannot all be detected.
PX20 encryption, nested containers, packed enhanced projects and PP20 saving are
unsupported. The format limits unpacked size to 24 bits. Caller budgets include
scratch memory. Physical hardware and subjective audio acceptance remain separate.
Amiberry is reserved through explicit AmiConnect handover for successive tests.
