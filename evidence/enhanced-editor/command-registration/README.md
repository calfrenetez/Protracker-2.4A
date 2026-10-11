The controller now records which constructor issued each command slot and its
serial. Registration observations can therefore recognise genuine CONTROL and
STOP commands that acquire no new reader. A TRIGGER must still record new
readers. Unknown kinds, mismatched serials and malformed captured fields refuse
registration. The query follows no nested pointer and calls no backend.

The issued metadata remains live until actual NULL command disposal clears the
slot. It supplies no queue activation or retirement authority. The shared command
record layout is unchanged; the private controller owner grows, so all callers
must rebuild coherently.

The focused registration fixture passed all 19 genuine cases with assertions,
ASan and UBSan: two wide/uint8 lifetimes, sixteen captured-identity/metadata
refusals and one nested-pointer exclusion. Its observed dependency discovery
covered 79 translation units; strict compilation and the fixture both returned
zero, with the exact 324-byte oracle. Original masters, pins, caches, saves and
callback/write counts remain asserted.

After the five source/test paths were adopted, seven affected ordinary drivers
passed once against the same product: registration19, default43, STOP52,
lineage55, CONTROL25, joint31 and the unchanged pair62. Their original fixture
oracles assert 287 cases. Saved direct-child waits, both EOFs, stream closes,
timer/handler restoration and unchanged source/tool/protected inputs passed an
independent review. This ordinary route does not separately export nested
compiler/fixture counters, actual dependency discovery or process-group absence.
See [results.json](results.json) for compact evidence and source hashes.

Separate strict m68000/soft-float/nix20/Os/Werror dependency, object and assembly
checks passed for the exact changed controller, admitting 73 non-system paths.
No link or artifact execution occurred. Individual stack reports do not qualify
aggregate stack, ABI, interrupt residency, WCET or exact musical activation.

All sixteen unrelated display/editor paths and their modes were preserved. The
finite complete-song extension remains private and separately qualified; its
earlier failed candidates are preserved. This milestone supplies host-model and
compiler-only evidence. Amiberry, real A1200/AmiGUS runtime, device capacity,
upload ordering/completion, voice stop, audio and listening are not established
by these checks. Authoritative masters, selective caches, master-preserving save,
direct 24-bit Studio mixing and the accepted classic layout retain their existing
architecture. No deferred ZooperTracker feature is introduced.
