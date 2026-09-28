# Integrated recording build and regression

Source tree f303cefc84460adde81b0dacd08842352e7526ff (791771c product source).
All147 native executables build. All350 listed source dependencies, one generated
font header and147 binary hashes are verified. The139 previously existing
executables are byte-identical to the prior full Studio-prefill build; eight
recording fixture executables are new. All four new production Exec variants
have individual passing emulator evidence in capture-staging, capture-session,
amigus-capture and editor-capture. PT24GEdit and PTPaulaTest remain unchanged from
the qualified odd-sample/stopped-visual candidates.

Full host sweep:207 cases in172 modules pass,253.947s.170 modules run in the
isolated export; editor_wavetable and editor_invert_reference run their Git
isolation helpers from the repository with current791771c HEAD/index. Unrelated
working-tree display edits are excluded. No failures in this complete sweep.

Additional exact-current-binary shared030 checks fill saved runtime-evidence gaps:
- Sample dispatch:7 Fast allocations, unsupported input/donor refusal, exact24-bit
  master and retained redo.
- RAW import:160 Fast allocations,16 explicit formats, bounded multi-block input,
  allocation/fill/limit refusals and undo/redo.
- WAV import:60 Fast allocations, six formats, bounded exact masters and refusal.
- Project streaming:6 Fast allocations, exact layout/CRC, mixed masters/loops/slices/
  extensions and sink refusal with7164-byte workspace.
All four return0 and zero owned bytes in24.773s total. Each has normal cleanup and
an independent later locked guest/running/all4DMAoff/exactpath-absence check before
the next starts. Explicit AmiConnect RELEASE after the sequence.

verified.json includes hash matches to earlier passing result files as pointers
only. Unmatched binaries are NOT automatically untested or failed (different
instrumented fixture builds and historic record layouts exist), and a matching
hash is not blanket acceptance of every feature. The full build is not a claim
that all147 executables were run under identical conditions. Actual device input,
AmiGUS hardware/MMIO, physical sound/timing and native recording controls remain
unaccepted. Physical Amiga stayed off. No display/layout changes are included.
